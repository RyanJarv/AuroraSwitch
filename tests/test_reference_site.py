"""Keep the static Pages copy current and its in-page links usable, without network."""

import hashlib
from html.parser import HTMLParser
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ReferenceParser(HTMLParser):
    """Collect static link targets and freshness metadata."""

    def __init__(self):
        super().__init__()
        self.ids = []
        self.links = []
        self.digest = None
        self.swatches = 0

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag == "a":
            self.links.append(attrs.get("href", ""))
        if tag == "meta" and attrs.get("name") == "reference-sha256":
            self.digest = attrs.get("content")
        if attrs.get("class") == "swatch":
            self.swatches += 1


class ReferenceSiteTests(unittest.TestCase):
    def test_static_reference_matches_source(self):
        page = (ROOT / "site/index.html").read_text()
        parser = ReferenceParser()
        parser.feed(page)
        self.assertEqual(parser.digest, hashlib.sha256(
            (ROOT / "docs/firmware_reference.md").read_bytes()).hexdigest(),
            "Refresh site/index.html using make reference-html")
        self.assertEqual(len(parser.ids), len(set(parser.ids)))
        for link in parser.links:
            if link.startswith("#"):
                self.assertIn(link[1:], parser.ids)
            else:
                self.assertTrue(link.startswith("https://"), link)
        self.assertEqual(parser.swatches, 12)
        self.assertNotRegex(page, r"<script\b")
        self.assertIn('name="viewport"', page)
        self.assertIn('lang="en"', page)
        for heading in re.findall(r"^## (.+)$", (ROOT / "docs/firmware_reference.md").read_text(), re.M):
            self.assertIn(heading, page)
