"""Ensure the journal regression detects removal of its critical safeguards."""

from pathlib import Path
import os
import resource
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FailureControls(unittest.TestCase):
    def test_flash_driver_boundaries(self):
        driver = (ROOT / "firmware/flash_driver.hpp").read_text()
        start = driver.index("        if (offset < SELECTOR_CHOICE_OFFSET")
        end = driver.index("        // Use a checked single-page", start)
        cases = (
            (
                "program bounds",
                "firmware/flash_driver.hpp",
                driver[start:end],
                "",
                "!driver.WritePage(offset, record.size(), record.data())",
            ),
            (
                "erase bounds",
                "firmware/flash_driver.hpp",
                "        if (offset != SELECTOR_CHOICE_OFFSET &&\n"
                "            offset != SELECTOR_CHOICE_OFFSET + SELECTOR_CHOICE_SECTOR_SIZE)\n"
                "            return false;\n",
                "",
                "!driver.EraseSector(offset)",
            ),
            (
                "shifted production layout",
                "support/selector_constants.h",
                "#define SELECTOR_CHOICE_OFFSET 0x7e0000",
                "#define SELECTOR_CHOICE_OFFSET 0x7d0000",
                "driver.WritePage(offset, record.size(), record.data())",
            ),
        )
        for name, filename, before, after, assertion in cases:
            with (
                self.subTest(safeguard=name),
                tempfile.TemporaryDirectory(prefix="aurora-driver-negative-") as temporary,
            ):
                root = Path(temporary)
                for relative in (
                    "firmware/flash_driver.hpp",
                    "firmware/flash_driver_test.cpp",
                    "support/selector_constants.h",
                    "tests/stubs/daisy_seed.h",
                ):
                    target = root / relative
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(ROOT / relative, target)
                target = root / filename
                source = target.read_text()
                self.assertEqual(source.count(before), 1)
                target.write_text(source.replace(before, after))
                executable = root / "driver"
                subprocess.run(
                    [
                        os.environ.get("CXX", "c++"),
                        "-std=c++17",
                        "-Wall",
                        "-Wextra",
                        "-Werror",
                        "-I",
                        str(root / "tests/stubs"),
                        str(root / "firmware/flash_driver_test.cpp"),
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

    def test_journal_safeguards(self):
        source = (ROOT / "support/choice_journal.hpp").read_text()
        readback_start = source.index(
            "        data = Sector(sector);", source.index("if (!driver_.WritePage")
        )
        readback_end = source.index("\n    }", readback_start)
        cases = (
            (
                "CRC",
                "RecordWord(record, BootCrcOffset) == RecordCrc(record)",
                "true",
                "recovered == 0 || recovered == 1",
            ),
            (
                "repeat suppression",
                "if (previous.index == static_cast<int>(index))\n            return true;",
                "",
                "d.writes == writes && d.inits == inits && d.erases == erases",
            ),
            (
                "readback",
                source[readback_start:readback_end],
                "        return true;",
                "!damaged.Save(2)",
            ),
        )
        for name, before, after, assertion in cases:
            with (
                self.subTest(safeguard=name),
                tempfile.TemporaryDirectory(prefix="aurora-negative-") as temporary,
            ):
                root = Path(temporary)
                for directory in ("firmware", "support"):
                    (root / directory).mkdir()
                    for path in (ROOT / directory).iterdir():
                        if path.suffix in (".hpp", ".h", ".cpp"):
                            shutil.copyfile(path, root / directory / path.name)
                self.assertEqual(source.count(before), 1)
                (root / "support/choice_journal.hpp").write_text(source.replace(before, after))
                executable = root / "journal"
                subprocess.run(
                    [
                        os.environ.get("CXX", "c++"),
                        "-std=c++17",
                        "-Wall",
                        "-Wextra",
                        "-Werror",
                        str(root / "firmware/choice_journal_test.cpp"),
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
