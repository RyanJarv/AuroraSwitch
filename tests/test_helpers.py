"""Host-only synthetic safety tests; no device or vendor files required."""

from pathlib import Path
import os
import struct
import zlib
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from firmware_catalog import read_catalog
import subprocess
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]


class HelperTests(unittest.TestCase):
    def compile_and_run(self, source: str) -> bytes:
        """Use the same warnings and isolated output for every native probe."""
        with tempfile.TemporaryDirectory(prefix="aurora-host-tests-") as directory:
            executable = Path(directory) / source
            command = [os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror"]
            if os.environ.get("AURORA_HOST_SANITIZERS") == "1":
                command += [
                    "-g",
                    "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=all",
                    "-fno-omit-frame-pointer",
                ]
            command += [str(ROOT / "firmware" / (source + ".cpp")), "-o", str(executable)]
            if source == "flash_driver_test":
                command += ["-I", str(ROOT / "tests/stubs")]
            subprocess.run(command, check=True)
            return subprocess.run([str(executable)], check=True, stdout=subprocess.PIPE).stdout

    def test_sanitizer_gate_rejects_undefined_behavior(self):
        with tempfile.TemporaryDirectory(prefix="aurora-ubsan-negative-") as directory:
            root = Path(directory)
            (root / "firmware").mkdir()
            (root / "firmware/ub_probe.cpp").write_text(
                "#include <climits>\n"
                "int main() { volatile int value = INT_MAX; "
                "volatile int overflow = value + 1; (void)overflow; }\n"
            )
            execute = subprocess.run

            def capture(*args, **kwargs):
                # Run the actual compiler and executable; retain their output so
                # neither a compilation error nor an unrelated crash can pass.
                kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                return execute(*args, **kwargs)

            with (
                mock.patch(__name__ + ".ROOT", root),
                mock.patch.dict(
                    os.environ,
                    {
                        "AURORA_HOST_SANITIZERS": "1",
                        "UBSAN_OPTIONS": "halt_on_error=0",
                    },
                ),
                mock.patch.object(subprocess, "run", side_effect=capture),
            ):
                with self.assertRaises(subprocess.CalledProcessError) as failure:
                    self.compile_and_run("ub_probe")
            self.assertEqual(len(failure.exception.cmd), 1)  # Execution, not compilation.
            self.assertIn("runtime error: signed integer overflow", failure.exception.stderr)

    def test_persisted_journal_contract(self):
        records = self.compile_and_run("choice_journal_test")
        images = read_catalog()["images"]
        self.assertEqual(len(records), 80 * len(images))
        for index, image in enumerate(images):
            record = records[index * 80 : (index + 1) * 80]
            self.assertEqual(record[:8], b"AURBOOT1")
            self.assertEqual(struct.unpack_from("<I", record, 8)[0], index + 1)
            self.assertEqual(record[12:76], image["sha256"].encode())
            self.assertEqual(struct.unpack_from("<I", record, 76)[0], zlib.crc32(record[:76]))

    def test_shared_helpers(self):
        for source in ("handoff_sequence_test", "flash_bundle_test", "menu_test"):
            with self.subTest(source=source):
                self.compile_and_run(source)

    def test_installed_selection(self):
        self.compile_and_run("selector_controller_test")

    def test_production_flash_driver_bounds(self):
        self.compile_and_run("flash_driver_test")

    def test_resident_image_bounds_and_exact_patch(self):
        self.compile_and_run("resident_image_test")

    def test_catalog_colors_and_capacity(self):
        self.compile_and_run("catalog_test")

    def test_no_programming_targets(self):
        for target in (
            "program",
            "program-dfu",
            "program-boot",
            "debug",
            "openocd",
            "debug_client",
        ):
            with self.subTest(target=target):
                result = subprocess.run(
                    ["make", "-n", "-C", str(ROOT / "firmware"), target],
                    capture_output=True,
                    text=True,
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("physical deployment is not authorized", result.stderr)


if __name__ == "__main__":
    unittest.main()
