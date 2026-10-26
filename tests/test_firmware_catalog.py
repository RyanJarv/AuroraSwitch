"""Catalog onboarding and independent preservation of existing launch contracts."""

import copy
import re
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import firmware_catalog
import inspect_firmware
from scripts import render_reference

ROOT = Path(__file__).resolve().parents[1]


class FirmwareCatalogTests(unittest.TestCase):
    def test_existing_launch_contracts_match_frozen_baseline(self):
        catalog = firmware_catalog.read_catalog()
        expected = json.loads((ROOT / "tests/fixtures/catalog_launch_v1.json").read_text())
        for index, frozen in enumerate(expected):
            row = catalog["images"][index]
            actual = {key: row[key] for key in frozen if key != "category"}
            actual["category"] = [c["name"] for c in catalog["categories"]].index(row["category"])
            self.assertEqual(actual, frozen)

    def test_invalid_metadata_rejects(self):
        original = firmware_catalog.read_catalog()
        mutations = (
            lambda d: d["images"].append(copy.deepcopy(d["images"][0])),
            lambda d: d["images"][0].update(filename="../escape.bin"),
            lambda d: d["images"][0].update(sha256="invalid"),
            lambda d: d["images"][0].update(download="http://unsafe.example/image.bin"),
            lambda d: d["images"][7].update(color="Blue", rgb=[0, 0, 0.4]),
            lambda d: d["images"][0].update(execution="unknown"),
        )
        for mutate in mutations:
            data = copy.deepcopy(original)
            mutate(data)
            with self.subTest(mutation=mutate), self.assertRaises(ValueError):
                firmware_catalog.validate_catalog(data)

    def test_new_family_needs_only_catalog_and_documentation(self):
        # Compile a modified COPY of the real catalog; no alternate test catalog
        # or launch implementation. The real exporter and constexpr views run.
        with tempfile.TemporaryDirectory(prefix="aurora-onboarding-") as temporary:
            root = Path(temporary)
            for directory in ("firmware", "support"):
                (root / directory).mkdir()
            for name in ("images.hpp", "catalog_dump.cpp", "catalog_test.cpp"):
                shutil.copyfile(ROOT / "firmware" / name, root / "firmware" / name)
            for name in ("selector_constants.h", "sram_image_vectors.hpp"):
                shutil.copyfile(ROOT / "support" / name, root / "support" / name)
            header = root / "firmware/images.hpp"
            source, count = re.subn(
                r"HpFilter,\s*Count", "HpFilter, Synthetic, Count", header.read_text()
            )
            self.assertEqual(count, 1)
            # Insert before each array's closing brace; indentation and line
            # wrapping are formatter choices, not part of the onboarding API.
            for name, entry in (
                (
                    "Families",
                    '{"synthetic", "Synthetic", Category::Delays, Color::Blue, "synthetic--blue"},',
                ),
                (
                    "FirmwareCatalog",
                    '{Family::Synthetic, "synthetic-v1", "Synthetic v1", "Test firmware", false, nullptr, '
                    '{"synthetic.bin", "' + "a" * 64 + '", 200000U, {0x20020000U, 0x24000009U}}},',
                ),
            ):
                start = source.index(name + "[]")
                closing = re.search(r"^\s*};", source[start:], re.M)
                self.assertIsNotNone(closing)
                offset = start + closing.start()
                source = source[:offset] + "\n    " + entry + "\n" + source[offset:]
            header.write_text(source)
            firmware_catalog.read_catalog.cache_clear()
            try:
                with mock.patch.object(firmware_catalog, "ROOT", root):
                    catalog = firmware_catalog.read_catalog()
            finally:
                firmware_catalog.read_catalog.cache_clear()
            self.assertEqual(len(catalog["images"]), 15)
            self.assertEqual(catalog["staging_capacity"], 200000)
            self.assertEqual(catalog["images"][-1]["category"], "Delays")
            self.assertEqual(catalog["images"][-1]["rgb"], [0, 0, 0.4])
            # The normal native catalog invariants also accept the addition.
            executable = root / "native-test"
            subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    str(root / "firmware/catalog_test.cpp"),
                    "-o",
                    str(executable),
                ],
                check=True,
            )
            subprocess.run([str(executable)], check=True)
            source = (
                ROOT / "docs/firmware_reference.md"
            ).read_text() + "\n## Synthetic — Blue\n\nTest controls.\n"
            with mock.patch.object(render_reference, "read_catalog", return_value=catalog):
                updated = render_reference.update_catalog_table(source)
                sections, entries = render_reference.read_reference(updated)
                self.assertEqual(entries[-1].name, "Synthetic v1")
                self.assertIn(
                    "synthetic.html",
                    render_reference.navigation(sections, entries, "index.html"),
                )
                with self.assertRaisesRegex(ValueError, "differs from compiled catalog"):
                    render_reference.read_reference(
                        updated.replace("| Test firmware |", "| Wrong description |")
                    )

    def test_inspection_measures_unknown_binary_without_admitting_it(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "unknown.bin"
            data = bytearray(128)
            struct.pack_into("<II", data, 0, 0x20020000, 0x24000009)
            path.write_bytes(data)
            measured = inspect_firmware.inspect(path, "sram")
            self.assertEqual(measured["catalog_matches"], [])
            self.assertEqual(measured["bytes"], 128)
            self.assertEqual(path.read_bytes(), data)
            for stack, reset in (
                (0x20020004, 0x24000009),
                (0x20020000, 0x24000008),
                (0x20020000, 0x24000081),
                (0x20020000, 0x90040009),
            ):
                struct.pack_into("<II", data, 0, stack, reset)
                path.write_bytes(data)
                with self.subTest(stack=stack, reset=reset), self.assertRaises(ValueError):
                    inspect_firmware.inspect(path, "sram")
