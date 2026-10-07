"""Check SPA data, HTML fallbacks, pinned assets and links without a browser."""

import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import unittest
from urllib.parse import urlsplit
import xml.etree.ElementTree as ET

from scripts.render_reference import PANEL_CONTROLS, label_controls, read_reference, slug

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "site"


class ReferenceParser(HTMLParser):
    """Collect link targets, accessibility landmarks and freshness metadata."""

    def __init__(self):
        super().__init__()
        self.ids = []
        self.links = []
        self.metadata = {}
        self.swatches = 0
        self.current = []
        self.h1_count = 0
        self.nav_labels = []
        self.scripts = []
        self.images = []
        self.details = []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        classes = attrs.get("class", "").split()
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag == "a":
            self.links.append(attrs.get("href", ""))
            if attrs.get("aria-current") == "page":
                self.current.append(attrs["href"])
        if tag == "link" and attrs.get("rel") == "stylesheet":
            self.links.append(attrs["href"])
        if tag == "script":
            self.links.append(attrs["src"])
            self.scripts.append(attrs)
        if tag == "img":
            self.links.append(attrs["src"])
            self.images.append(attrs)
        if tag == "details":
            self.details.append(attrs)
        if tag == "meta":
            self.metadata[attrs.get("name")] = attrs.get("content")
        if tag == "h1":
            self.h1_count += 1
        if tag == "nav":
            self.nav_labels.append(attrs.get("aria-label"))
        if "swatch" in classes:
            self.swatches += 1


class ReferenceSiteTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "docs/firmware_reference.md").read_text()
        _, cls.sections, cls.entries = read_reference(cls.source)
        cls.pages = {}
        for file in SITE.glob("*.html"):
            parser = ReferenceParser()
            parser.feed(file.read_text())
            cls.pages[file.name] = parser

    def test_static_pages_match_source_and_renderer(self):
        expected = {section.filename for section in self.sections}
        self.assertEqual(set(self.pages), expected, "Regenerate pages with make reference-html")
        fingerprints = {
            "reference-sha256": hashlib.sha256((ROOT / "docs/firmware_reference.md").read_bytes()).hexdigest(),
            "renderer-sha256": hashlib.sha256((ROOT / "scripts/render_reference.py").read_bytes()).hexdigest(),
        }
        for filename, parser in self.pages.items():
            with self.subTest(page=filename):
                for key, digest in fingerprints.items():
                    self.assertEqual(parser.metadata.get(key), digest, "Run make reference-html")

    def test_links_and_fragments_resolve_on_every_page(self):
        for filename, parser in self.pages.items():
            with self.subTest(page=filename):
                self.assertEqual(len(parser.ids), len(set(parser.ids)))
                for link in parser.links:
                    target = urlsplit(link)
                    if target.scheme or target.netloc:
                        self.assertEqual(target.scheme, "https", link)
                        continue
                    destination = target.path or filename
                    self.assertEqual(Path(destination).name, destination, link)
                    self.assertTrue((SITE / destination).is_file(), link)
                    if target.fragment:
                        self.assertIn(destination, self.pages, link)
                        self.assertIn(target.fragment, self.pages[destination].ids, link)

    def test_index_has_summary_and_install_directions_not_a_second_directory(self):
        parser = self.pages["index.html"]
        self.assertEqual(parser.swatches, 0)
        self.assertEqual(len(self.entries), 12)
        page = (SITE / "index.html").read_text()
        body = re.search(r'<main id="main">\n(.*?)\n</main>', page, re.S)[1]
        self.assertIn('id="quick-install"', body)
        self.assertIn('id="select-firmware"', body)
        self.assertIn("git clone https://github.com/RyanJarv/AuroraSwitch.git", body)
        self.assertIn('make download-release USB_DIR="/Volumes/AURORA"', body)
        self.assertIn("wait for green", body)
        self.assertNotIn('class="firmware-entry"', page)
        self.assertNotIn('class="firmware-list"', page)
        self.assertNotIn("Reference sources and maintenance", page)
        self.assertNotIn('id="reference-maintenance"', page)
        for section in self.sections:
            if any(entry.anchor == section.anchor for entry in self.entries):
                self.assertEqual(page.count(f'href="{section.filename}"'), 2)
                self.assertNotIn(section.title, body)

    def test_firmware_pages_have_control_headings_and_version_labels(self):
        for section in self.sections:
            entries = [entry for entry in self.entries if entry.anchor == section.anchor]
            if not entries:
                continue
            parser = self.pages[section.filename]
            page = (SITE / section.filename).read_text()
            with self.subTest(firmware=section.title):
                self.assertIn(section.anchor, parser.ids)
                self.assertEqual(parser.current, [section.filename, section.filename])
                self.assertNotIn("On this page", parser.nav_labels)
                self.assertNotIn('class="page-links"', page)
                self.assertNotIn('class="back-link"', page)
                self.assertEqual(parser.swatches, 1)
                for heading in re.findall(r"^### (.+)$", section.markdown, re.M):
                    self.assertIn(slug(heading), parser.ids)
                self.assertIn(entries[0].color, page)
                self.assertNotIn('class="version-label"', page)
                for entry in entries:
                    if len(entries) > 1:
                        version = entry.name.removeprefix(section.title).strip()
                        self.assertIn(version + (" (development)" if entry.availability == "Development" else ""), page)

    def test_html_fallbacks_have_accessible_navigation_and_local_spa(self):
        for filename, parser in self.pages.items():
            page = (SITE / filename).read_text()
            with self.subTest(page=filename):
                self.assertEqual(parser.h1_count, 1)
                self.assertIn("main", parser.ids)
                self.assertIn("Firmware directory", parser.nav_labels)
                self.assertIn("Mobile firmware directory", parser.nav_labels)
                self.assertEqual(len(parser.scripts), 1)
                script = parser.scripts[0]
                self.assertEqual(script["type"], "module")
                self.assertEqual(urlsplit(script["src"]).path, "app.js")
                self.assertEqual(urlsplit(script["data-reference"]).path, "reference.json")
                reference = parser.metadata["reference-sha256"]
                renderer = parser.metadata["renderer-sha256"]
                self.assertEqual(urlsplit(script["data-reference"]).query, f"v={reference}-{renderer}")
                digest = hashlib.sha256((SITE / "app.js").read_bytes()).hexdigest()
                self.assertEqual(urlsplit(script["src"]).query, f"v={digest}")
                self.assertIn('name="viewport"', page)
                self.assertIn('lang="en"', page)
                self.assertIn('class="mobile-menu"', page)
                self.assertIn('class="skip-link"', page)

    def test_spa_data_matches_every_fallback(self):
        data = json.loads((SITE / "reference.json").read_text())
        self.assertEqual(set(data["pages"]), set(self.pages))
        for filename, parser in self.pages.items():
            page = (SITE / filename).read_text()
            with self.subTest(page=filename):
                for field in ("reference", "renderer"):
                    self.assertEqual(data[f"{field}_sha256"], parser.metadata[f"{field}-sha256"])
                body = re.search(r'<main id="main">\n(.*?)\n</main>', page, re.S)[1]
                self.assertEqual(data["pages"][filename]["body"], body)
                self.assertIn("AuroraSwitch", data["pages"][filename]["title"])
        menu = ReferenceParser()
        menu.feed(data["menu"])
        self.assertEqual(set(menu.links), set(data["pages"]))

    def test_preact_files_match_pinned_upstream_bytes(self):
        for file, digest in {
            "preact.module.js": "a1cefabf06ec626adcb92731537e1e04fd09a7908e22551bab50540106dc950d",
            "LICENSE": "1fe6958409c8c257a70c587a18b6f7f412b179b456630790d30b2ec9a8e4b7d4",
        }.items():
            with self.subTest(file=file):
                self.assertEqual(hashlib.sha256((SITE / "vendor" / file).read_bytes()).hexdigest(), digest)

    def test_control_descriptions_and_caveats_are_preserved(self):
        self.assertIn("Mix is not dry/wet.", (SITE / "tempest-100.html").read_text())
        self.assertIn("USB table loading remains", (SITE / "fatamorgana-ram-experiment.html").read_text())
        self.assertIn("not been independently verified", (SITE / "the-oscillator-is-a-lie-002.html").read_text())
        self.assertIn("step boundary or reset", (SITE / "morse.html").read_text())
        self.assertIn("Without tables, a generated cube", (SITE / "fatamorgana-ram-experiment.html").read_text())

    def test_control_views_show_panel_and_visible_details_section(self):
        for section in self.sections:
            if not any(entry.anchor == section.anchor for entry in self.entries):
                continue
            parser = self.pages[section.filename]
            page = (SITE / section.filename).read_text()
            with self.subTest(page=section.filename):
                self.assertEqual(len(parser.images), 1)
                self.assertEqual(parser.images[0]["src"], "aurora-panel.svg")
                for number, name in enumerate(PANEL_CONTROLS, 1):
                    self.assertIn(f"{number} {name}", parser.images[0]["alt"])
                details = [item for item in parser.details if item.get("id") == "details-and-sources"]
                self.assertEqual(details, [])
                self.assertIn('<h2 id="details-and-sources">Details &amp; sources</h2>', page)
                self.assertNotIn('class="reference-details"', page)
                self.assertNotIn('class="conventions"', page)
                self.assertNotIn('class="control-key"', page)
                self.assertNotIn('How to read the controls', page)
                self.assertNotIn('Names refer to the original Aurora panel', page)
                self.assertIn('<span class="control-number">', page)

    def test_control_numbers_only_label_table_controls(self):
        self.assertEqual(label_controls('<tr><td>Time + CV</td><td>Delay</td></tr>'),
                         '<tr><td><span class="control-number">2</span> Time + CV</td><td>Delay</td></tr>')
        cell = label_controls('<tr><td>Shift + Freeze</td><td>Time</td></tr>')
        self.assertIn('control-number">8</span>', cell)
        self.assertIn('control-number">9</span>', cell)
        self.assertNotIn('control-number">2</span>', cell)
        unrelated = '<tr><td>Clock ratio</td><td>Reverse</td></tr>'
        self.assertEqual(label_controls(unrelated), unrelated)

    def test_panel_drawing_preserves_physical_control_order(self):
        panel = ET.parse(SITE / "aurora-panel.svg").getroot()
        ns = {"svg": "http://www.w3.org/2000/svg"}
        knobs = [item for item in panel.findall('.//svg:use', ns) if item.get("href") == "#knob"]
        self.assertEqual(len(knobs), 6)
        self.assertEqual(len([item for item in panel.findall('.//svg:use', ns)
                              if item.get("href") == "#button"]), 3)
        # Warp/Blur/Mix alternate with the right-hand Time/Reflect/Atmosphere column.
        self.assertEqual({item.get("x") for item in knobs[::2]}, {knobs[0].get("x")})
        self.assertEqual({item.get("x") for item in knobs[1::2]}, {knobs[1].get("x")})
        self.assertLess(float(knobs[0].get("x")), float(knobs[1].get("x")))
        self.assertEqual([float(item.get("y")) for item in knobs],
                         sorted(float(item.get("y")) for item in knobs))
        for name in PANEL_CONTROLS:
            self.assertIn(name, " ".join(panel.itertext()))

    def test_navigation_rejects_unknown_colors_or_availability(self):
        for old, new in (("| Blue |", "| Invisible |"), ("| Release |", "| Available maybe |")):
            with self.subTest(replacement=new), self.assertRaises(ValueError):
                read_reference(self.source.replace(old, new, 1))

    def test_navigation_rejects_missing_link_targets(self):
        with self.assertRaisesRegex(ValueError, "Missing firmware section"):
            read_reference(self.source.replace(
                "[FDN 1.2.2](#fdn-122--blue)", "[FDN 1.2.2](#missing-firmware)", 1))

    def test_navigation_rejects_duplicate_selector_colors(self):
        with self.assertRaisesRegex(ValueError, "duplicate selector colors"):
            read_reference(self.source.replace("| Green |", "| Blue |", 1))

    def test_versions_share_stable_family_colors(self):
        for anchor, color, count in (("flux-capacitor--yellow", "Yellow", 3),
                                     ("morse--white", "White", 2)):
            versions = [entry for entry in self.entries if entry.anchor == anchor]
            self.assertEqual(len(versions), count)
            self.assertEqual({entry.color for entry in versions}, {color})
        with self.assertRaisesRegex(ValueError, "Inconsistent family"):
            read_reference(self.source.replace(
                "| Yellow | [Flux Capacitor 0.1.0]", "| Azure | [Flux Capacitor 0.1.0]"))

    def test_navigation_rejects_sections_without_a_color_entry(self):
        with self.assertRaisesRegex(ValueError, "Unlinked firmware sections"):
            read_reference(self.source + "\n## Unlisted firmware\n\nMissing catalog row.\n")
