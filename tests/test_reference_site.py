"""Check SPA data, HTML fallbacks, pinned assets and links without a browser."""

import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import unittest
from urllib.parse import urlsplit

from scripts.render_reference import read_reference, slug

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
        self.cards = []
        self.current = []
        self.h1_count = 0
        self.nav_labels = []
        self.scripts = []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        classes = attrs.get("class", "").split()
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag == "a":
            self.links.append(attrs.get("href", ""))
            if "firmware-card" in classes:
                self.cards.append(attrs["href"])
            if attrs.get("aria-current") == "page":
                self.current.append(attrs["href"])
        if tag == "link" and attrs.get("rel") == "stylesheet":
            self.links.append(attrs["href"])
        if tag == "script":
            self.links.append(attrs["src"])
            self.scripts.append(attrs)
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

    def test_index_covers_every_version_and_color(self):
        parser = self.pages["index.html"]
        by_anchor = {section.anchor: section for section in self.sections}
        self.assertEqual(parser.cards, [by_anchor[entry.anchor].filename for entry in self.entries])
        self.assertEqual(parser.swatches, len(self.entries))
        self.assertEqual(len(self.entries), 12)
        self.assertIn("development-builds", parser.ids)
        page = (SITE / "index.html").read_text()
        for entry in self.entries:
            self.assertIn(entry.name, page)
            self.assertIn(entry.color, page)

    def test_legacy_firmware_fragments_still_find_the_index_card(self):
        for entry in self.entries:
            self.assertIn(entry.anchor, self.pages["index.html"].ids)

    def test_firmware_pages_have_local_control_links_and_version_labels(self):
        for section in self.sections:
            entries = [entry for entry in self.entries if entry.anchor == section.anchor]
            if not entries:
                continue
            parser = self.pages[section.filename]
            page = (SITE / section.filename).read_text()
            with self.subTest(firmware=section.title):
                self.assertIn(section.anchor, parser.ids)
                self.assertEqual(parser.current, [section.filename, section.filename])
                self.assertIn("On this page", parser.nav_labels)
                self.assertEqual(parser.swatches, len(entries))
                for heading in re.findall(r"^### (.+)$", section.markdown, re.M):
                    self.assertIn(slug(heading), parser.ids)
                    self.assertIn(f'#{slug(heading)}', parser.links)
                for entry in entries:
                    self.assertIn(f"{entry.name} · {entry.color}", page)
                    if entry.availability == "Development":
                        self.assertIn(f"{entry.name} · {entry.color} · development", page)

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

    def test_navigation_rejects_sections_without_a_color_entry(self):
        with self.assertRaisesRegex(ValueError, "Unlinked firmware sections"):
            read_reference(self.source + "\n## Unlisted firmware\n\nMissing catalog row.\n")
