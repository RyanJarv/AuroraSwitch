"""Negative controls: the native probes must catch named broken safeguards."""
from pathlib import Path
import os
import resource
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FailureControls(unittest.TestCase):
    def test_named_safeguards_detect_broken_implementations(self):
        cases = (
            ("close", "firmware/file_media.hpp",
             "return media.Close() && valid && media.Ready();", "return valid && media.Ready();",
             "file_media_test", "RecordOperation(current, write) == expected"),
            ("sync", "firmware/file_media.hpp",
             "&& count == record.size() && media.Sync();", "&& count == record.size();",
             "file_media_test", "RecordOperation(current, write) == expected"),
            ("record-crc", "support/boot_choice.hpp",
             "&& RecordWord(record, 76) == RecordCrc(record)", "&& true",
             "boot_choice_test", "ReadBootChoice(store).index == 12"),
            ("unmount", "firmware/handoff_sequence.hpp",
             "!platform.Unmount()", "false",
             "handoff_sequence_test", "good.steps"),
        )
        for name, header, before, after, probe, assertion in cases:
            with self.subTest(safeguard=name), tempfile.TemporaryDirectory(prefix="aurora-negative-") as temporary:
                root = Path(temporary)
                for directory in ("firmware", "support"):
                    (root / directory).mkdir()
                    for path in (ROOT / directory).iterdir():
                        if path.suffix in (".hpp", ".h", ".cpp"):
                            shutil.copyfile(path, root / directory / path.name)
                shutil.copytree(ROOT / "tests/fakes", root / "tests/fakes")
                path = root / header
                text = path.read_text()
                self.assertEqual(text.count(before), 1)
                path.write_text(text.replace(before, after))
                command = [os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror"]
                if probe == "file_media_test":
                    command += ["-I" + str(root / "tests/fakes"), str(root / "firmware/media_usb.cpp")]
                executable = root / probe
                command += [str(root / "firmware" / (probe + ".cpp")), "-o", str(executable)]
                subprocess.run(command, check=True, capture_output=True)
                result = subprocess.run([str(executable)], capture_output=True, text=True,
                    preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_CORE, (0, 0)))
                self.assertNotEqual(result.returncode, 0, "broken safeguard passed: " + name)
                self.assertIn("Assertion", result.stderr)
                self.assertIn(assertion, result.stderr)


if __name__ == "__main__":
    unittest.main()
