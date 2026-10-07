"""Synthetic build/admission guards; no compiler or firmware execution."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import build_fatamorgana as builder


class FataBuildTests(unittest.TestCase):
    def test_vector_extent_and_runtime_guards(self):
        data = struct.pack("<II", 0x20020000, 0x24000009) + bytes(248)
        builder.check_vectors(data)
        for broken in (data[:7], data + bytes(181888),
                       struct.pack("<II", 0x2001ffff, 0x24000009) + data[8:],
                       struct.pack("<II", 0x20020000, 0x90040009) + data[8:],
                       struct.pack("<II", 0x20020000, 0x24000008) + data[8:]):
            with self.assertRaises(ValueError):
                builder.check_vectors(broken)

    def test_source_cache_drift_rejects(self):
        for revision, dirty in (("wrong", ""), (builder.SOURCE_COMMIT, " M source.cpp")):
            with patch.object(builder, "git", side_effect=[revision, dirty]):
                with self.assertRaisesRegex(RuntimeError, "changed source cache"):
                    builder.check_cache(Path("/unused"), builder.SOURCE_COMMIT)

    def test_fresh_build_and_sealed_bundle_guards(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / ".deps/Aurora-Firmwares").mkdir(parents=True)
            data = struct.pack("<II", 0x20020000, 0x24000009) + bytes(248)
            commands = []

            def execute(command, **kwargs):
                commands.append((command, kwargs))
                if command[:2] == ["git", "clone"]:
                    self.assertIn("--no-local", command)
                    Path(command[-1]).mkdir(parents=True, exist_ok=True)
                elif command[0] == "make" and "APP_TYPE=BOOT_SRAM" in command:
                    build = Path(command[command.index("-C") + 1]) / "build"
                    build.mkdir(parents=True)
                    for suffix, contents in (("bin", data), ("elf", b"synthetic-elf"), ("map", b"synthetic-map")):
                        (build / ("FataMorgana." + suffix)).write_bytes(contents)
                elif command[0].endswith("objcopy"):
                    Path(command[-1]).write_bytes(data)

            with patch.object(builder, "ROOT", root), patch.object(builder, "setup"), \
                    patch.object(builder, "toolchain_identity", return_value=(Path("/pinned-tools"), {"gcc": "synthetic"})), \
                    patch.object(builder, "check_cache"), patch.object(builder, "git", return_value=builder.SOURCE_URL), \
                    patch.object(builder.subprocess, "run", side_effect=execute):
                destination = builder.build()
                manifest = json.loads((destination / "manifest.json").read_bytes())
                self.assertIs(manifest["selector_admitted"], False)
                self.assertIs(manifest["usb_behavior_verified"], False)
                self.assertEqual(manifest["method"], "fresh-isolated-tracked-checkouts")
                self.assertEqual(manifest["source_commit"], builder.SOURCE_COMMIT)
                makes = [(cmd, kw) for cmd, kw in commands if cmd[0] == "make"]
                self.assertEqual(len(makes), 3)
                for cmd, kwargs in makes:
                    self.assertNotIn("MAKEFLAGS", kwargs["env"])
                    self.assertNotIn(str(root / ".deps/Aurora-SDK"), cmd)
                self.assertEqual(builder.build(), destination)
                (destination / "FataMorgana.bin").write_bytes(b"changed")
                with self.assertRaisesRegex(RuntimeError, "bundle changed"):
                    builder.build()


if __name__ == "__main__":
    unittest.main()
