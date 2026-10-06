"""Synthetic packaging controls, independent of firmware/hardware qualification."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
spec = importlib.util.spec_from_file_location("aurora_package", ROOT / "scripts/package.py")
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)


class PackageTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix="aurora-package-test-")
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.build = self.root / "firmware/build-experimental-dma"
        self.build.mkdir(parents=True)
        for suffix in ("elf", "bin", "map"):
            (self.build / f"AuroraSwitch.{suffix}").write_bytes(suffix.encode())
        for patcher in (mock.patch.object(bundle, "ROOT", self.root),
                        mock.patch.object(bundle, "setup"),
                        mock.patch.object(bundle.subprocess, "check_output", side_effect=["", "a" * 40] * 4),
                        mock.patch.object(bundle.subprocess, "run", side_effect=self.convert)):
            patcher.start()
            self.addCleanup(patcher.stop)

    def convert(self, command, **kwargs):
        Path(command[-1]).write_bytes((self.build / "AuroraSwitch.bin").read_bytes())

    def test_exact_reuse(self):
        first = bundle.package()
        self.assertEqual(first, bundle.package())
        self.assertEqual({path.name for path in first.iterdir()},
                         {"AuroraSwitch.elf", "AuroraSwitch.bin", "AuroraSwitch.map", "manifest.json"})

    def test_dirty_source_rejects(self):
        with mock.patch.object(bundle.subprocess, "check_output", return_value=" M firmware/selector.cpp"):
            with self.assertRaisesRegex(RuntimeError, "clean committed"):
                bundle.package()
        self.assertFalse((self.root / "dist").exists())

    def test_mismatched_elf_rejects(self):
        with mock.patch.object(bundle.subprocess, "run",
                               side_effect=lambda command, **kwargs: Path(command[-1]).write_bytes(b"different")):
            with self.assertRaisesRegex(RuntimeError, "ELF/BIN"):
                bundle.package()
        self.assertFalse((self.root / "dist").exists())

    def test_tampered_bundle_rejects(self):
        path = bundle.package()
        (path / "AuroraSwitch.bin").write_bytes(b"changed")
        with self.assertRaisesRegex(RuntimeError, "immutable bundle bytes"):
            bundle.package()

    def test_extra_bundle_file_rejects(self):
        path = bundle.package()
        (path / "unexpected.bin").write_bytes(b"extra")
        with self.assertRaisesRegex(RuntimeError, "inventory"):
            bundle.package()
