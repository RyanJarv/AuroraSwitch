"""Host-only synthetic safety tests; no device or vendor files required."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class HelperTests(unittest.TestCase):
    def compile_and_run(self, source: str) -> None:
        """Use the same warnings and isolated output for every native probe."""
        with tempfile.TemporaryDirectory(prefix="aurora-host-tests-") as directory:
            executable = Path(directory) / source
            subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                str(ROOT / "firmware" / (source + ".cpp")), "-o",
                str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

    def test_shared_helpers(self):
        for source in ("staging_test", "handoff_sequence_test", "backed_reader_test"):
            with self.subTest(source=source):
                self.compile_and_run(source)

    def test_media_initialization_fail_closed(self):
        self.compile_and_run("media_initialization_test")

    def test_operation_timing(self):
        self.compile_and_run("operation_timing_test")

    def test_supported_image_discovery(self):
        self.compile_and_run("menu_test")

    def test_catalog_colors_and_capacity(self):
        self.compile_and_run("catalog_test")

    def test_no_programming_targets(self):
        for target in ("program", "program-dfu", "program-boot"):
            result = subprocess.run(["make", "-n", "-C", str(ROOT / "firmware"), target],
                capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("physical deployment is not authorized", result.stderr)


if __name__ == "__main__":
    unittest.main()
