"""Synthetic local-installer checks; no vendor binaries or hardware evidence."""
from contextlib import contextmanager
import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import prepare_resident as resident


class ResidentPreparationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.dirt = struct.pack("<II", 0x20020000, 0x90040959) + bytes(95196 - 8)
        self.selector = struct.pack("<II", 0x20020000, 0x24000009) + bytes(range(32))
        self.boot = b"synthetic-wrapper"
        self.manifest = {"source_commit": "a" * 40, "build_provenance": {"configuration": {"VIRTUAL_TRANSPORT": 0}}}
        self.input = self.root / "Dirt.bin"
        self.input.write_bytes(self.dirt)
        self.bundle = self.root / ("b" * 64)
        self.output = self.root / "private"
        @contextmanager
        def authenticator(): yield Path("synthetic-authenticator")
        for patcher in (
            mock.patch.object(resident, "authenticate_bundle", return_value=(self.manifest, self.selector, self.boot)),
            mock.patch.object(resident, "authenticator", authenticator),
            mock.patch.object(resident, "catalog", return_value=[{"filename": "DirtVerb 1.1.bin",
                "bytes": len(self.dirt), "sha256": hashlib.sha256(self.dirt).hexdigest()}]),
            mock.patch.object(resident.subprocess, "run", side_effect=self.authenticate),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def authenticate(self, command, **kwargs):
        self.assertEqual(Path(command[-1]).read_bytes(), self.dirt)
        self.assertNotEqual(Path(command[-1]), self.input)

    def test_exact_single_word_patch_and_private_inputs(self):
        receipt = resident.prepare(self.bundle, self.input, self.output)
        combined = (self.output / "AuroraSwitch.bin").read_bytes()
        layout = resident.layout()
        self.assertEqual(combined[:4], self.dirt[:4])
        self.assertEqual(combined[8:len(self.dirt)], self.dirt[8:])
        self.assertEqual(struct.unpack_from("<I", combined, 4)[0], layout["ENTRY"])
        self.assertEqual(combined[layout["IMAGE"] - layout["BASE"]:], self.selector)
        self.assertEqual((self.output / "aurora/DirtVerb 1.1.bin").read_bytes(), self.dirt)
        self.assertTrue(receipt["private_contains_third_party_firmware"])
        self.assertFalse(receipt["physical_qualified"])

    def test_virtual_image_cannot_have_installable_name(self):
        self.manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"] = 1
        resident.prepare(self.bundle, self.input, self.output)
        self.assertFalse((self.output / "AuroraSwitch.bin").exists())
        self.assertTrue((self.output / "AuroraSwitchResidentVirtual.bin").exists())

    def test_changed_input_and_authentication_failure_publish_nothing(self):
        self.input.write_bytes(self.dirt[:-1] + b"!")
        with self.assertRaises(ValueError): resident.prepare(self.bundle, self.input, self.output)
        self.assertFalse(self.output.exists())
        self.input.write_bytes(self.dirt)
        with mock.patch.object(resident.subprocess, "run", side_effect=ValueError("bad vectors")):
            with self.assertRaises(ValueError): resident.prepare(self.bundle, self.input, self.output)
        self.assertFalse(self.output.exists())

    def test_input_replacement_cannot_change_authenticated_private_bytes(self):
        def replace(command, **kwargs):
            self.authenticate(command, **kwargs)
            self.input.write_bytes(b"replacement")
        with mock.patch.object(resident.subprocess, "run", side_effect=replace):
            resident.prepare(self.bundle, self.input, self.output)
        self.assertEqual((self.output / "aurora/DirtVerb 1.1.bin").read_bytes(), self.dirt)

    def test_existing_output_preserved_and_bundle_drift_rejected(self):
        self.output.mkdir()
        original = self.output / "keep"
        original.write_bytes(b"original")
        with self.assertRaises(ValueError): resident.prepare(self.bundle, self.input, self.output)
        self.assertEqual(original.read_bytes(), b"original")
        with mock.patch.object(resident, "authenticate_bundle", side_effect=[
                (self.manifest, self.selector, self.boot), (self.manifest, b"changed", self.boot)]):
            with self.assertRaises(ValueError): resident.prepare(self.bundle, self.input, self.root / "new")
        self.assertFalse((self.root / "new").exists())

    def test_layout_rejects_bad_vectors_and_extents(self):
        for dirt, selector, boot in ((self.dirt[:-1], self.selector, self.boot),
                (self.dirt, self.selector[:8], self.boot), (self.dirt, self.selector, b"x" * 257),
                (self.dirt, bytes(len(self.selector)), self.boot)):
            with self.assertRaises(ValueError): resident.compose(dirt, selector, boot)


if __name__ == "__main__": unittest.main()
