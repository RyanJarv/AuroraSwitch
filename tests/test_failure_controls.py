"""Ensure consequential USB save safeguards have working negative controls."""

from pathlib import Path
import os
import resource
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FailureControls(unittest.TestCase):
    def test_choice_safeguards(self):
        cases = (
            (
                "repeat suppression",
                "firmware/usb_files.hpp",
                "if (ReadChoice() == static_cast<int>(index))",
                "if (false)",
                "writes == previous",
            ),
            (
                "CRC",
                "support/usb_choice.hpp",
                "stored != ChoiceCrc(record.data(), record.size() - 4)",
                "(false && stored != ChoiceCrc(record.data(), record.size() - 4))",
                "DecodeChoice(torn) == -1",
            ),
            (
                "readback",
                "firmware/usb_files.hpp",
                "saved && closed && ReadChoice() == static_cast<int>(index)",
                "saved && closed",
                "!WriteChoice(0)",
            ),
        )
        for name, filename, before, after, assertion in cases:
            with self.subTest(safeguard=name), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                for relative in (
                    "firmware/usb_files.hpp",
                    "firmware/usb_files_test.cpp",
                    "firmware/images.hpp",
                    "support/usb_choice.hpp",
                    "support/selector_constants.h",
                    "support/sram_image_vectors.hpp",
                    "tests/stubs/ff.h",
                ):
                    target = root / relative
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(ROOT / relative, target)
                target = root / filename
                source = target.read_text()
                self.assertEqual(source.count(before), 1)
                target.write_text(source.replace(before, after))
                executable = root / "negative"
                subprocess.run(
                    [
                        os.environ.get("CXX", "c++"),
                        "-std=c++17",
                        "-Wall",
                        "-Wextra",
                        "-Werror",
                        "-I" + str(root / "tests/stubs"),
                        str(root / "firmware/usb_files_test.cpp"),
                        "-o",
                        str(executable),
                    ],
                    check=True,
                    capture_output=True,
                )
                result = subprocess.run(
                    [str(executable)],
                    capture_output=True,
                    text=True,
                    preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_CORE, (0, 0)),
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Assertion", result.stderr)
                self.assertIn(assertion, result.stderr)
