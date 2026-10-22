"""Host-only synthetic safety tests; no device or vendor files required."""

from pathlib import Path
import os
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
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
            if source == "usb_files_test":
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

    def test_shared_helpers(self):
        for source in ("handoff_sequence_test", "usb_files_test", "menu_test"):
            with self.subTest(source=source):
                self.compile_and_run(source)

    def test_usb_selection(self):
        self.compile_and_run("selector_controller_test")

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
