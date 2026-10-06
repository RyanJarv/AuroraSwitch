"""Synthetic packaging controls, independent of firmware/hardware qualification."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
from contextlib import contextmanager

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
                        mock.patch.object(bundle, "fresh_build", self.fresh),
                        mock.patch.object(bundle.subprocess, "check_output", side_effect=["", "a" * 40, "a" * 40, ""] * 4),
                        mock.patch.object(bundle.subprocess, "run", side_effect=self.convert)):
            patcher.start()
            self.addCleanup(patcher.stop)

    @contextmanager
    def fresh(self, commit):
        yield self.build, {"gcc": {"version": "synthetic", "sha256": "b" * 64}}, Path("/synthetic-tools")

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

    def test_ambient_outputs_not_used(self):
        ambient = self.root / "firmware/build-experimental-dma"
        isolated = self.root / "fresh"
        isolated.mkdir()
        for suffix in ("elf", "bin", "map"):
            (isolated / f"AuroraSwitch.{suffix}").write_bytes(b"fresh-" + suffix.encode())
        self.build = isolated
        path = bundle.package()
        self.assertEqual((path / "AuroraSwitch.bin").read_bytes(), b"fresh-bin")
        self.assertEqual((ambient / "AuroraSwitch.bin").read_bytes(), b"bin")

    def test_source_drift_rejects(self):
        with mock.patch.object(bundle.subprocess, "check_output", side_effect=["", "a" * 40, "c" * 40]):
            with self.assertRaisesRegex(RuntimeError, "HEAD changed"):
                bundle.package()
        self.assertFalse((self.root / "dist").exists())

    def test_build_failure_rejects(self):
        with mock.patch.object(bundle, "fresh_build", side_effect=RuntimeError("build failed")):
            with self.assertRaisesRegex(RuntimeError, "build failed"):
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
