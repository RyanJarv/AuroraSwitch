"""Host-only synthetic safety tests; no device or vendor files required."""
from pathlib import Path
import hashlib
import os
import re
import struct
import zlib
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]


class HelperTests(unittest.TestCase):
    def compile_and_run(self, source: str, *, usb: bool = False) -> bytes:
        """Use the same warnings and isolated output for every native probe."""
        with tempfile.TemporaryDirectory(prefix="aurora-host-tests-") as directory:
            executable = Path(directory) / source
            command = [os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror"]
            if os.environ.get("AURORA_HOST_SANITIZERS") == "1":
                command += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                            "-fno-omit-frame-pointer"]
            if usb:
                command += ["-I" + str(ROOT / "tests/fakes"), str(ROOT / "firmware/media_usb.cpp")]
            command += [str(ROOT / "firmware" / (source + ".cpp")), "-o", str(executable)]
            subprocess.run(command, check=True)
            return subprocess.run([str(executable)], check=True, stdout=subprocess.PIPE).stdout

    def test_sanitizer_gate_rejects_undefined_behavior(self):
        with tempfile.TemporaryDirectory(prefix="aurora-ubsan-negative-") as directory:
            root = Path(directory)
            (root / "firmware").mkdir()
            (root / "firmware/ub_probe.cpp").write_text(
                "#include <climits>\n"
                "int main() { volatile int value = INT_MAX; "
                "volatile int overflow = value + 1; (void)overflow; }\n")
            execute = subprocess.run

            def capture(*args, **kwargs):
                # Run the actual compiler and executable; retain their output so
                # neither a compilation error nor an unrelated crash can pass.
                kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                return execute(*args, **kwargs)

            with mock.patch(__name__ + ".ROOT", root), mock.patch.dict(os.environ, {
                "AURORA_HOST_SANITIZERS": "1", "UBSAN_OPTIONS": "halt_on_error=0",
            }), mock.patch.object(subprocess, "run", side_effect=capture):
                with self.assertRaises(subprocess.CalledProcessError) as failure:
                    self.compile_and_run("ub_probe")
            self.assertEqual(len(failure.exception.cmd), 1)  # Execution, not compilation.
            self.assertIn("runtime error: signed integer overflow", failure.exception.stderr)

    def test_production_usb_file_policy(self):
        self.assertEqual(hashlib.sha256((ROOT / "tests/fakes/file_media_before_refactor.hpp").read_bytes()).hexdigest(),
                         "dd02ce0a7a9aeeaa9bed6d2c734a51483a12bbd65d277bce164ade0bbbdd057d")
        self.compile_and_run("file_media_test", usb=True)

    def test_catalog_boot_records_match_independent_crc_and_generation(self):
        records = self.compile_and_run("boot_record_test")
        hashes = re.findall(r'\{"0:/aurora/[^"]+",\s*"([0-9a-f]{64})"',
                            (ROOT / "firmware/images.hpp").read_text())
        self.assertGreater(len(hashes), 1)
        self.assertEqual(len(records), len(hashes) * 4 * 80)
        for index, digest in enumerate(hashes):
            for case, generation in enumerate((1, 2, 0xffffffff, 0)):
                with self.subTest(image=index, generation=generation):
                    offset = (index * 4 + case) * 80
                    record = records[offset:offset + 80]
                    self.assertEqual(record[:8], b"AURBOOT1")
                    self.assertEqual(struct.unpack_from("<I", record, 8)[0], generation)
                    self.assertEqual(record[12:76], digest.encode())
                    self.assertEqual(struct.unpack_from("<I", record, 76)[0], zlib.crc32(record[:76]))

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

    def test_selector_with_replaced_media(self):
        self.compile_and_run("selector_controller_test")

    def test_persistent_boot_choice(self):
        self.compile_and_run("boot_choice_test")

    def test_resident_image_bounds_and_exact_patch(self):
        self.compile_and_run("resident_image_test")

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
            root = Path(directory) / "firmware"
            root.mkdir()
            constants = root.parent / "support/selector_constants.h"
            constants.parent.mkdir()
            shutil.copyfile(ROOT / "support/selector_constants.h", constants)
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
                "OBJECTS = $(addprefix $(BUILD_DIR)/,probe.o copy_jump.o resident_boot.o)\n"
                "LDSCRIPT = $(SYSTEM_FILES_DIR)/sram.lds\n"
                "$(BUILD_DIR)/probe.o: probe.cpp Makefile\n"
                "\t@echo compile-probe\n"
                "$(BUILD_DIR)/copy_jump.o: copy_jump.s Makefile\n"
                "\t@echo compile-assembly\n"
                "$(BUILD_DIR)/resident_boot.o: resident_boot.s Makefile\n"
                "\t@echo compile-assembly\n"
                "$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile\n"
                "\t@echo link-selector -o $@\n")
            shutil.copyfile(ROOT / "firmware/Makefile", root / "Makefile")
            for name in ("probe.cpp", "copy_jump.s", "resident_boot.s", "staging_reserve.ld", "trampoline_reserve.ld"):
                (root / name).write_text("synthetic input\n")
            builds = ((0, "build-experimental-dma", "AuroraSwitch"),
                      (1, "build-virtual-experimental-dma", "AuroraSwitchVirtual"))
            for virtual, build, target in builds:
                folder = root / build
                folder.mkdir()
                for name in ("probe.o", "copy_jump.o", "resident_boot.o", "staging_reserve.ld", "trampoline_reserve.ld"):
                    (folder / name).write_bytes(b"synthetic build output")
                (folder / f"{target}.elf").write_bytes(b"synthetic ELF")
            for path in root.parent.rglob("*"):
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
                for changed in (archive, linker, upstream, Path("../support/selector_constants.h"),
                                Path("staging_reserve.ld"), Path("trampoline_reserve.ld")):
                    with self.subTest(virtual=virtual, changed=changed.name):
                        result = subprocess.run(command + ["-W", str(changed)],
                                                cwd=root, capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertIn("link-selector", result.stdout)
                        if changed == upstream:
                            self.assertIn("compile-probe", result.stdout)
                        if changed == Path("../support/selector_constants.h"):
                            self.assertEqual(result.stdout.count("compile-assembly"), 2)
                            self.assertEqual(result.stdout.count("-E -P -x c"), 2)

    def test_synthetic_objects_cannot_share_installable_directory(self):
        result = subprocess.run(["make", "-n", "-C", str(ROOT / "firmware"),
            "VIRTUAL_TRANSPORT=1", "BUILD_DIR=build-experimental-dma"],
            capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Use BUILD_DIR=build-virtual-experimental-dma", result.stderr)


if __name__ == "__main__":
    unittest.main()
