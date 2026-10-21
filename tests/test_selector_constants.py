"""Cross-language constants reject ambiguous or unevaluated definitions."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from selector_constants import read_constants


class SelectorConstantsTests(unittest.TestCase):
    def test_literal_integer_definitions(self):
        self.assertEqual(read_constants("#define SELECTOR_A 0xff\n#define SELECTOR_B 1024\n"),
                         {"A": 255, "B": 1024})

    def test_ambiguous_or_nonliteral_definitions_reject(self):
        for source in ("", "#define SELECTOR_A 012\n", "#define SELECTOR_A (1 + 2)\n", "#define SELECTOR_A 1.5\n",
                       "#define SELECTOR_A 1\n#define SELECTOR_A 2\n"):
            with self.subTest(source=source), self.assertRaises(ValueError):
                read_constants(source)
