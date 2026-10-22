"""Exercise resident bundle admission; only external Arm tools are substituted."""

import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import sealed_bundle as resident


class ResidentAuthenticationTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="resident-admission-")
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.binary = struct.pack("<II", 0x20020000, 0x24000009) + bytes(range(64))
        self.converted = self.binary
        self.symbols = (
            "selector_resident_boot_start R 24000010\n" "selector_resident_boot_end R 24000020\n"
        )
        self.tools = {"gcc": {"version": "synthetic", "sha256": "f" * 64}}
        for patcher in (
            mock.patch.object(
                resident, "toolchain_identity", return_value=(Path("/tools"), self.tools)
            ),
            mock.patch.object(resident.subprocess, "run", side_effect=self.convert),
            mock.patch.object(
                resident.subprocess, "check_output", side_effect=lambda *a, **k: self.symbols
            ),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def convert(self, command, **kwargs):
        self.assertEqual(Path(command[0]).name, "arm-none-eabi-objcopy")
        Path(command[-1]).write_bytes(self.converted)

    def fixture(self, *, virtual=0, change=None, raw=None):
        target = "AuroraSwitchVirtual" if virtual == 1 else "AuroraSwitch"
        files = {
            target + ".bin": self.binary,
            target + ".elf": b"synthetic-ELF",
            target + ".map": b"synthetic-map",
        }
        manifest = {
            "schema": "aurora-switch-development-bundle-v1",
            "source_commit": "a" * 40,
            "development_only": True,
            "physical_qualified": False,
            "build_provenance": {
                "method": "fresh-isolated-tracked-checkouts",
                "configuration": {"VIRTUAL_TRANSPORT": virtual},
                "toolchain": copy.deepcopy(self.tools),
            },
            "files": {
                name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                for name, data in files.items()
            },
        }
        if change:
            change(manifest)
        if raw is None:
            raw = json.dumps(manifest).encode()
        path = Path(tempfile.mkdtemp(dir=self.root)) / hashlib.sha256(raw).hexdigest()
        path.mkdir()
        (path / "manifest.json").write_bytes(raw)
        for name, data in files.items():
            (path / name).write_bytes(data)
        return path

    def test_both_authenticated_transports_extract_exact_wrapper(self):
        for virtual in (0, 1):
            with self.subTest(virtual=virtual):
                manifest, binary, boot = resident.authenticate_bundle(self.fixture(virtual=virtual))
                self.assertEqual(
                    manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"], virtual
                )
                self.assertEqual(binary, self.binary)
                self.assertEqual(boot, self.binary[16:32])

    def test_changed_size_hash_inventory_and_bundle_identity_reject(self):
        for kind in ("bytes", "short", "extra", "missing", "name", "directory"):
            with self.subTest(kind=kind):
                path = self.fixture()
                if kind == "bytes":
                    (path / "AuroraSwitch.bin").write_bytes(b"!" + self.binary[1:])
                if kind == "short":
                    (path / "AuroraSwitch.bin").write_bytes(self.binary[:-1])
                if kind == "directory":
                    (path / "AuroraSwitch.elf").unlink()
                    (path / "AuroraSwitch.elf").mkdir()
                if kind == "extra":
                    (path / "extra.bin").write_bytes(b"extra")
                if kind == "missing":
                    (path / "AuroraSwitch.map").unlink()
                if kind == "name":
                    path = path.rename(path.with_name("0" * 64))
                with self.assertRaises(ValueError):
                    resident.authenticate_bundle(path)

    def test_symlink_manifest_artifact_and_bundle_reject(self):
        for kind in ("manifest.json", "AuroraSwitch.elf", "bundle"):
            with self.subTest(kind=kind):
                path = self.fixture()
                if kind == "bundle":
                    target = path.rename(path.with_name("actual"))
                    path.symlink_to(target, target_is_directory=True)
                else:
                    target = path.parent / "outside"
                    (path / kind).rename(target)
                    (path / kind).symlink_to(target)
                with self.assertRaises(ValueError):
                    resident.authenticate_bundle(path)

    def test_malformed_manifest_and_contract_reject(self):
        for raw in (b"{", b"[]", b"null", b"{}"):
            with self.subTest(raw=raw):
                with self.assertRaises(ValueError):
                    resident.authenticate_bundle(self.fixture(raw=raw))
        changes = (
            lambda m: m.update(schema="other"),
            lambda m: m.update(source_commit="--bad-revision"),
            lambda m: m.update(development_only=False),
            lambda m: m.update(physical_qualified=True),
            lambda m: m["build_provenance"].update(method="ambient"),
            lambda m: m["build_provenance"].update(toolchain={}),
            lambda m: m["build_provenance"].update(configuration=[]),
            lambda m: m["files"]["AuroraSwitch.bin"].update(bytes=float(len(self.binary))),
            lambda m: m["files"]["AuroraSwitch.bin"].pop("sha256"),
        )
        for index, change in enumerate(changes):
            with self.subTest(contract=index):
                with self.assertRaises(ValueError):
                    resident.authenticate_bundle(self.fixture(change=change))
        for virtual in (True, False, 1.0, 2, "1"):
            with self.subTest(virtual=virtual):
                with self.assertRaises(ValueError):
                    resident.authenticate_bundle(self.fixture(virtual=virtual))

    def test_toolchain_drift_and_elf_bin_disagreement_reject(self):
        path = self.fixture()
        with mock.patch.object(resident, "toolchain_identity", return_value=(Path("/tools"), {})):
            with self.assertRaisesRegex(ValueError, "toolchain"):
                resident.authenticate_bundle(path)
        self.converted = b"mismatched conversion"
        with self.assertRaisesRegex(ValueError, "ELF/BIN"):
            resident.authenticate_bundle(path)

    def test_wrapper_over_256_bytes_rejects(self):
        self.binary = bytes(512)
        self.converted = self.binary
        self.symbols = (
            "selector_resident_boot_start R 24000000\nselector_resident_boot_end R 24000101\n"
        )
        with self.assertRaisesRegex(ValueError, "extent"):
            resident.authenticate_bundle(self.fixture())

    def test_missing_duplicate_reversed_and_out_of_range_symbols_reject(self):
        path = self.fixture()
        for start, end in (
            (0x24000020, 0x24000010),
            (0x23FFFFFC, 0x24000010),
            (0x24000010, 0x24000200),
            (0x24000010, 0x24000010),
        ):
            self.symbols = (
                f"selector_resident_boot_start R {start:x}\nselector_resident_boot_end R {end:x}\n"
            )
            with self.subTest(start=start, end=end), self.assertRaisesRegex(ValueError, "extent"):
                resident.authenticate_bundle(path)
        for symbols in (
            "",
            "selector_resident_boot_start R 24000010\n" * 2,
            "selector_resident_boot_start R bad-address\nselector_resident_boot_end R 24000020\n",
        ):
            self.symbols = symbols
            with self.subTest(symbols=symbols), self.assertRaises(ValueError):
                resident.authenticate_bundle(path)


if __name__ == "__main__":
    unittest.main()
