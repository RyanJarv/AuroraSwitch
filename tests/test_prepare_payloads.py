"""Synthetic preparation tests; not target execution or compatibility evidence."""
from contextlib import contextmanager
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import prepare_payloads as payloads
import verify_images


class PrepareTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix="payload-preparation-test-")
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.data = [b"synthetic-first-image", b"synthetic-second-image"]
        self.images = [{"filename": f"image-{i}.bin", "bytes": len(data),
                        "sha256": hashlib.sha256(data).hexdigest(),
                        "stack": 0x20020000, "reset": 0x24000011}
                       for i, data in enumerate(self.data)]
        self.files = []
        for i, data in enumerate(self.data):
            path = self.root / f"user-name-{i}.bin"
            path.write_bytes(data)
            self.files.append(path)
        self.output = self.root / "prepared"
        self.calls = []
        self.modes = []

        @contextmanager
        def authenticator(*, qspi=False):
            self.modes.append(qspi)
            yield Path("synthetic-authenticator")

        for patcher in (mock.patch.object(payloads, "authenticator", authenticator),
                        mock.patch.object(payloads, "catalog", return_value=self.images),
                        mock.patch.object(payloads.subprocess, "run", side_effect=self.check)):
            patcher.start()
            self.addCleanup(patcher.stop)

    def check(self, command, **kwargs):
        """Verify the mock authenticator receives private copies, not source files."""
        image = next(row for row in self.images if row["filename"] == command[1])
        data = Path(command[2]).read_bytes()
        self.assertEqual(hashlib.sha256(data).hexdigest(), image["sha256"])
        self.assertNotIn(Path(command[2]), self.files)
        self.calls.append(command[1])

    def test_exact_files_and_receipt(self):
        receipt = payloads.prepare(self.files, self.output)
        self.assertEqual(receipt["qualification"], "byte-authentication-only")
        self.assertEqual(json.loads((self.output / "payloads.json").read_text()), receipt)
        self.assertEqual(self.calls, [row["filename"] for row in self.images])
        self.assertEqual({p.name for p in self.output.iterdir()}, {"aurora", "payloads.json"})
        for image, data in zip(self.images, self.data):
            self.assertEqual((self.output / "aurora" / image["filename"]).read_bytes(), data)

    def test_subset_is_allowed(self):
        receipt = payloads.prepare(self.files[1:], self.output)
        self.assertEqual(receipt["images"], self.images[1:])

    def test_qspi_catalog_is_explicit_and_receipted(self):
        receipt = payloads.prepare(self.files, self.output, qspi=True)
        self.assertEqual(self.modes, [True])
        self.assertIs(receipt["development_qspi_catalog"], True)

    def test_non_boolean_catalog_mode_rejected(self):
        for mode in (1, "yes", None):
            with self.assertRaises(ValueError):
                payloads.prepare(self.files, self.output, qspi=mode)
        self.assertEqual(self.modes, [])
        self.assertFalse(self.output.exists())

    def test_missing_input_leaves_no_output(self):
        with self.assertRaises(FileNotFoundError):
            payloads.prepare([self.root / "missing"], self.output)
        self.assertFalse(self.output.exists())

    def test_changed_unknown_and_oversized_reject(self):
        for data in (b"unsupported", self.data[0][:-1] + b"!", b"x" * 100):
            with self.subTest(data=data):
                self.files[0].write_bytes(data)
                with self.assertRaisesRegex(ValueError, "unsupported or changed"):
                    payloads.prepare(self.files, self.output)
                self.assertFalse(self.output.exists())

    def test_duplicate_and_empty_reject(self):
        for files in ([], [self.files[0], self.files[0]]):
            with self.assertRaises(ValueError):
                payloads.prepare(files, self.output)
            self.assertFalse(self.output.exists())

    def test_all_inputs_pass_before_output(self):
        self.files[1].write_bytes(b"bad-second")
        with self.assertRaises(ValueError):
            payloads.prepare(self.files, self.output)
        self.assertFalse(self.output.exists())

    def test_cpp_authentication_failure_rejects(self):
        with mock.patch.object(payloads.subprocess, "run",
                               side_effect=subprocess.CalledProcessError(2, ["synthetic"])):
            with self.assertRaises(subprocess.CalledProcessError):
                payloads.prepare(self.files, self.output)
        self.assertFalse(self.output.exists())

    def test_source_change_cannot_change_prepared_bytes(self):
        def change_source(command, **kwargs):
            self.check(command, **kwargs)
            self.files[0].write_bytes(b"replaced-after-validation")
        with mock.patch.object(payloads.subprocess, "run", side_effect=change_source):
            payloads.prepare(self.files[:1], self.output)
        self.assertEqual((self.output / "aurora/image-0.bin").read_bytes(), self.data[0])

    def test_existing_output_is_never_overwritten(self):
        self.output.mkdir()
        recovery = self.output / "original.bin"
        recovery.write_bytes(b"recovery")
        with self.assertRaisesRegex(ValueError, "already exists"):
            payloads.prepare(self.files, self.output)
        self.assertEqual(recovery.read_bytes(), b"recovery")
        self.assertEqual(list(self.output.iterdir()), [recovery])

    def test_output_symlinks_reject(self):
        for target in (self.root / "missing", self.root):
            with self.subTest(target=target):
                self.output.symlink_to(target, target_is_directory=True)
                with self.assertRaisesRegex(ValueError, "already exists"):
                    payloads.prepare(self.files, self.output)
                self.output.unlink()

    def test_catalog_is_read_from_compiled_authority(self):
        row = self.images[0]
        line = f"0:/aurora/{row['filename']}\t{row['bytes']}\t{row['sha256']}\t{row['stack']}\t{row['reset']}\n"
        with mock.patch.object(verify_images.subprocess, "check_output", return_value=line):
            self.assertEqual(verify_images.catalog(Path("synthetic")), [row])
        for invalid in ("", line + line, line.replace("image-0.bin", "../escape.bin"),
                        line.replace(row["sha256"], "not-a-hash"),
                        line.replace(str(row["bytes"]), "999999")):
            with mock.patch.object(verify_images.subprocess, "check_output", return_value=invalid):
                with self.assertRaises(ValueError):
                    verify_images.catalog(Path("synthetic"))


if __name__ == "__main__":
    unittest.main()
