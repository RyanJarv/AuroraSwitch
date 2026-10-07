"""Host-only synthetic safety tests; no device or vendor files required."""
from pathlib import Path
import os
import shutil
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

    def test_qspi_catalog(self):
        self.compile_and_run("qspi_catalog_test")

    def test_qspi_bounded_writer_and_faults(self):
        self.compile_and_run("qspi_programming_test")

    def test_no_programming_targets(self):
        for target in ("program", "program-dfu", "program-boot", "debug", "openocd", "debug_client"):
            with self.subTest(target=target):
                result = subprocess.run(["make", "-n", "-C", str(ROOT / "firmware"), target],
                    capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("physical deployment is not authorized", result.stderr)

    def test_linked_inputs_invalidate_incremental_build(self):
        """Exercise Make dependencies without an Arm compiler or SDK checkout."""
        with tempfile.TemporaryDirectory(prefix="aurora-make-tests-") as directory:
            root = Path(directory)
            core = root / "sdk/libs/libDaisy/core"
            core.mkdir(parents=True)
            archive = core.parent / "build/libdaisy.a"
            archive.parent.mkdir()
            archive.write_bytes(b"synthetic archive")
            linker = core / "sram.lds"
            linker.write_text("/* synthetic linker input */\n")
            upstream = core / "Makefile"
            # Match the pinned upstream dependency structure, not its compiler.
            upstream.write_text(
                "OBJECTS = $(BUILD_DIR)/probe.o\n"
                "LDSCRIPT = $(SYSTEM_FILES_DIR)/sram.lds\n"
                "$(BUILD_DIR)/probe.o: probe.cpp Makefile\n"
                "\t@echo compile-probe\n"
                "$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile\n"
                "\t@echo link-selector -o $@\n")
            shutil.copyfile(ROOT / "firmware/Makefile", root / "Makefile")
            for name in ("probe.cpp", "staging_reserve.ld", "trampoline_reserve.ld"):
                (root / name).write_text("synthetic input\n")
            builds = ((0, "build-experimental-dma", "AuroraSwitch"),
                      (1, "build-virtual-experimental-dma", "AuroraSwitchVirtual"))
            for virtual, build, target in builds:
                folder = root / build
                folder.mkdir()
                (folder / "probe.o").write_bytes(b"synthetic object")
                (folder / f"{target}.elf").write_bytes(b"synthetic ELF")
            for path in root.rglob("*"):
                if path.is_file():
                    os.utime(path, (1000, 1000))
            for virtual, build, target in builds:
                elf = root / build / f"{target}.elf"
                os.utime(elf, (2000, 2000))
                command = ["make", "-n", f"{build}/{target}.elf",
                           f"SDK_ROOT={root / 'sdk'}", f"VIRTUAL_TRANSPORT={virtual}"]
                baseline = subprocess.run(command, cwd=root, capture_output=True, text=True)
                self.assertEqual(baseline.returncode, 0, baseline.stderr)
                self.assertNotIn("link-selector", baseline.stdout)
                for changed in (archive, linker, upstream):
                    with self.subTest(virtual=virtual, changed=changed.name):
                        result = subprocess.run(command + ["-W", str(changed)],
                                                cwd=root, capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertIn("link-selector", result.stdout)
                        if changed == upstream:
                            self.assertIn("compile-probe", result.stdout)

    def test_synthetic_objects_cannot_share_installable_directory(self):
        result = subprocess.run(["make", "-n", "-C", str(ROOT / "firmware"),
            "VIRTUAL_TRANSPORT=1", "BUILD_DIR=build-experimental-dma"],
            capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Use BUILD_DIR=build-virtual-experimental-dma", result.stderr)


if __name__ == "__main__":
    unittest.main()
