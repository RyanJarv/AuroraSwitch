"""Synthetic USB staging tests; no device or firmware-execution claims."""
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import prepare_usb


class UsbTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.usb = self.root / "drive"
        self.usb.mkdir()
        self.selector = self.root / "selector.bin"
        self.selector.write_bytes(b"synthetic selector")
        (self.usb / "options.txt").write_bytes(b"keep settings")

    @staticmethod
    def prepare(files, output):
        """Stand in for the separately tested native authentication helper."""
        (output / "aurora").mkdir(parents=True)
        for source in files:
            (output / "aurora" / source.name).write_bytes(source.read_bytes())

    def test_stage_and_repeat_preserve_settings(self):
        with mock.patch.object(prepare_usb, "download", return_value=b"synthetic payload"), \
                mock.patch.object(prepare_usb, "prepare", side_effect=self.prepare) as verifier:
            prepare_usb.stage(self.usb, self.selector)
            prepare_usb.stage(self.usb, self.selector)
        self.assertEqual(verifier.call_count, 2)
        self.assertEqual((self.usb / "options.txt").read_bytes(), b"keep settings")
        self.assertEqual((self.usb / "AuroraSwitch.bin").read_bytes(), self.selector.read_bytes())
        self.assertEqual({p.name for p in (self.usb / "aurora").iterdir()}, set(prepare_usb.DOWNLOADS))

    def test_failed_download_or_authentication_does_not_write_drive(self):
        for failure in ("download", "prepare"):
            with self.subTest(failure=failure), \
                    mock.patch.object(prepare_usb, "download", return_value=b"bad") as download, \
                    mock.patch.object(prepare_usb, "prepare") as verifier:
                {"download": download, "prepare": verifier}[failure].side_effect = ValueError("reject")
                with self.assertRaisesRegex(ValueError, "reject"):
                    prepare_usb.stage(self.usb, self.selector)
                self.assertEqual([p.name for p in self.usb.iterdir()], ["options.txt"])

    def test_destination_guards(self):
        for destination in (Path("/"), Path("relative"), self.root / "missing"):
            with self.assertRaises(ValueError):
                prepare_usb.check_destination(destination)
        (self.usb / "Stock.BIN").write_bytes(b"recovery")
        with self.assertRaisesRegex(ValueError, "root-level updater"):
            prepare_usb.check_destination(self.usb)

    def test_output_symlink_is_rejected(self):
        (self.usb / "aurora").symlink_to(self.root, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "symlink"):
            prepare_usb.check_destination(self.usb)

    def test_zip_extracts_only_requested_member_and_bounds_download(self):
        stream = io.BytesIO()
        with zipfile.ZipFile(stream, "w") as archive:
            archive.writestr("Aurora_v1_4_4.bin", b"payload")
            archive.writestr("../ignored", b"never extracted")
        with mock.patch.object(prepare_usb.urllib.request, "urlopen", return_value=io.BytesIO(stream.getvalue())):
            self.assertEqual(prepare_usb.download("https://example.test/image.zip", "Aurora_v1_4_4.bin"), b"payload")
        with mock.patch.object(prepare_usb.urllib.request, "urlopen", return_value=io.BytesIO(b"x" * (2 * 1024 * 1024 + 1))):
            with self.assertRaisesRegex(ValueError, "too large"):
                prepare_usb.download("https://example.test/image.bin", "image.bin")
