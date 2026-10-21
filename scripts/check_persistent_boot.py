"""Bounded linked startup/launch regressions using the existing external runner.

Synthetic USB and choice files; starts at the resident wrapper with prepared
loader handoff state. Does not execute the loader's USB updater, real USB, audio,
physical reset, or recovery. Local records include private third-party bytes.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib

from prepare_resident import authenticate_bundle, compose, layout, sha
from package import toolchain_identity
from verify_images import authenticator, catalog


def run(bundle: Path, images: Path, runner_root: Path, output: Path) -> dict:
    sys.path.insert(0, str(runner_root.resolve()))
    from harness.lib import emulator
    from spike.aurora_selector.audio_routes import prepare_platform, RENODE, identities
    from spike.aurora_selector.loader_boundary import authenticate_usb_runtime
    from spike.aurora_renode_characterization import monitor_path, ANSI
    from spike.renode_snapshot_probe import _isolated_environment

    manifest, binary, boot = authenticate_bundle(bundle)
    if manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"] != 1:
        raise ValueError("a sealed synthetic-media companion is required")
    tools, _ = toolchain_identity()
    elf = bundle / "AuroraSwitchVirtual.elf"
    nm = subprocess.check_output([str(tools / "arm-none-eabi-nm"), "--defined-only", "--format=posix", str(elf)], text=True)
    def symbol(name):
        values = [int(line.split()[2], 16) for line in nm.splitlines() if line.split()[0] == name]
        if len(values) != 1: raise ValueError("missing/ambiguous symbol " + name)
        return values[0]
    main = symbol("main")
    # The authentication function is local C++; find its single mangled name.
    auth_names = [line.split()[0] for line in nm.splitlines() if "AuthenticateFile" in line.split()[0]]
    if len(auth_names) != 1: raise ValueError("ambiguous authentication seam")
    auth = symbol(auth_names[0])
    media = symbol("selector_virtual_media")
    store = symbol("selector_virtual_boot_store")
    diagnostic = symbol("selector_qspi_diagnostic")
    wrapper_invalid = (layout()["ENTRY"] & ~1) + symbol("resident_invalid") - symbol("selector_resident_boot_start")
    with authenticator() as checker:
        rows = catalog(checker)
        present = {}
        for index, image in enumerate(rows):
            path = images / image["filename"]
            if not path.exists(): continue
            data = path.read_bytes()
            if len(data) != image["bytes"] or sha(data) != image["sha256"]:
                raise ValueError("payload identity: " + image["filename"])
            subprocess.run([str(checker), image["filename"], str(path)], check=True, capture_output=True)
            present[index] = data
    dirt_index = next(i for i, row in enumerate(rows) if row["filename"] == "DirtVerb 1.1.bin")
    if not {0, dirt_index} <= present.keys(): raise ValueError("FDN and Dirt files are required")
    combined = compose(present[dirt_index], binary, boot)
    runtime = authenticate_usb_runtime(RENODE)
    inputs = identities("fdn")
    results = []
    output.mkdir(parents=True, exist_ok=False)
    for scenario in ("dirt", "repeat-dirt", "fdn", "menu", "bad-record", "bad-file", "bad-resident", "bad-wrapper"):
        directory = output / scenario
        directory.mkdir()
        with emulator._exclusive_emulator_lock(), tempfile.TemporaryDirectory(prefix="persistent-vm-") as temporary:
            temp = Path(temporary)
            platform, includes = prepare_platform(temp, "fdn", len(binary))
            choice = 0 if scenario == "fdn" else dirt_index
            record = b"AURBOOT1" + struct.pack("<I", 1) + rows[choice]["sha256"].encode()
            record += struct.pack("<I", zlib.crc32(record))
            if scenario == "repeat-dirt":
                record = (output / "dirt" / "boot-store.bin").read_bytes()[16:96]
            if scenario == "bad-record": record = record[:-1] + bytes([record[-1] ^ 1])
            initial = bytearray(bytes(range(256)) * (0x800000 // 256))
            initial[0x40000:0x40000 + len(combined)] = combined
            if scenario == "bad-resident": initial[0x40958] ^= 1
            if scenario == "bad-wrapper": initial[layout()["METADATA"] - 0x90000000 + 12] ^= 1
            nor = temp / "nor.bin"
            nor.write_bytes(initial)
            commands = includes + ["logLevel 3", 'mach create "persistent-boot"',
                "machine LoadPlatformDescription " + monitor_path(platform),
                "auroraQspi.auroraNor ArraySubset true",
                "sysbus LoadBinary " + monitor_path(nor) + " 0x90000000",
                "cpu VectorTableOffset 0x90040000", "sysbus WriteDoubleWord 0xe000ed88 0x00f00000"]
            for port, pin in (("C", 11), ("B", 5), ("B", 6)):
                commands.append(f"gpioPort{port} OnGPIO {pin} {'false' if scenario == 'menu' and port == 'B' else 'true'}")
            descriptors = {}
            for index, row in enumerate(rows):
                payload = present.get(index, b"")
                if scenario == "bad-file" and index == choice: payload = payload[:-1] + bytes([payload[-1] ^ 1])
                descriptors[index] = list(struct.pack("<4I64sI", 1, 1, index + 1, len(payload),
                    ("0:/aurora/" + row["filename"]).encode(), 0))
                if payload:
                    path = temp / f"payload-{index}.bin"
                    path.write_bytes(payload)
                    commands.append(f"sysbus LoadBinary {monitor_path(path)} {hex(0xc0100000 + index * 0x40000)}")
            init = temp / "init.py"
            init.write_text("from System import Array, Byte\nbus = machine.SystemBus\n"
                + f"bus.WriteBytes(Array[Byte]({list(struct.pack('<4I', 1, 1, 0, 0) + record + bytes(80))!r}), {store})\n"
                + f"bus.WriteBytes(Array[Byte]({list(struct.pack('<4I64sI', 1, 1, 1, 0, b'', 0))!r}), {media})\n"
                + "print('PERSISTENT_SELECTOR_MAIN')\n")
            hook = temp / "media.py"
            hook.write_text("from System import Array, Byte\nbus = machine.SystemBus\n"
                + "index = int(cpu.GetRegister(0).RawValue)\n"
                + f"descriptor = {descriptors!r}[index]\n"
                + "size = descriptor[12] | descriptor[13]<<8 | descriptor[14]<<16 | descriptor[15]<<24\n"
                + "if size: bus.WriteBytes(bus.ReadBytes(0xc0100000 + index * 0x40000, size), 0xc0000000)\n"
                + f"bus.WriteBytes(Array[Byte](descriptor), {media})\n"
                + "print('PERSISTENT_FILE index=' + str(index))\n")
            commands += [f'cpu AddHook {hex(main)} "exec(open(\'{init}\').read())"',
                f'cpu AddHook {hex(auth)} "exec(open(\'{hook}\').read())"',
                f'cpu AddHook {hex(wrapper_invalid)} "print(\'PERSISTENT_WRAPPER_REJECT\'); cpu.IsHalted = True"']
            reset = rows[choice]["reset"] & ~1
            halt = reset if choice == 0 else 0x900421b8
            commands += [f'cpu AddHook {hex(reset)} "print(\'PERSISTENT_PAYLOAD_RESET\')"',
                f'cpu AddHook {hex(halt)} "print(\'PERSISTENT_PAYLOAD_HALT\'); cpu.IsHalted = True"',
                'emulation RunFor "2.000000"', "pause", "auroraQspi.auroraNor ArrayDiagnostics",
                "auroraQspi.auroraNor ExportArray " + monitor_path(directory / "nor-after.bin"),
                f'dump_file {hex(store)} 176 "{directory / "boot-store.bin"}"',
                f'dump_file {hex(diagnostic)} 20 "{directory / "qspi.bin"}"',
                f'dump_file 0x24000000 {len(present[0])} "{directory / "ram.bin"}"',
                'echo "PERSISTENT_CFSR"', "sysbus ReadDoubleWord 0xe000ed28",
                'echo "PERSISTENT_HFSR"', "sysbus ReadDoubleWord 0xe000ed2c", "q"]
            command = [str(RENODE), "--disable-xwt", "--console"]
            for entry in commands: command += ["-e", entry]
            completed = subprocess.run(command, capture_output=True, text=True, timeout=120,
                                       env=_isolated_environment(temp / "environment"))
            raw = completed.stdout + completed.stderr
            (directory / "raw.log").write_text(raw)
            (directory / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
            clean = ANSI.sub("", raw).replace("\r", "")
            if completed.returncode or any(s in clean for s in ("There was an error", "No such command",
                    "Unhandled exception", "CPU abort", "Errors during compilation", "PythonException")):
                raise ValueError("persistent runner failed: " + str(directory / "raw.log"))
            positive = scenario in ("dirt", "repeat-dirt", "fdn")
            if clean.splitlines().count("PERSISTENT_PAYLOAD_HALT") != int(positive):
                raise ValueError("unexpected/missing payload entry: " + str(directory / "raw.log"))
            if clean.splitlines().count("PERSISTENT_SELECTOR_MAIN") != int(scenario != "bad-wrapper"):
                raise ValueError("wrapper did not enter expected selector startup")
            if scenario == "bad-wrapper" and clean.splitlines().count("PERSISTENT_WRAPPER_REJECT") != 1:
                raise ValueError("corrupt wrapper metadata did not reject")
            after = (directory / "nor-after.bin").read_bytes()
            if after != initial or clean.splitlines().count("IS25LP064_ARRAY enabled=1 erase_commands=0 program_bytes=0") != 1:
                raise ValueError("flash changed or writes occurred")
            state = (directory / "boot-store.bin").read_bytes()
            if scenario != "bad-wrapper" and struct.unpack_from("<I", state, 8)[0] != 0:
                raise ValueError("automatic/menu boot wrote the choice")
            if scenario == "fdn" and (directory / "ram.bin").read_bytes() != present[0]:
                raise ValueError("RAM handoff bytes changed")
            if re.findall(r"(?m)^PERSISTENT_(?:CFSR|HFSR)\s+0x([0-9a-fA-F]+)$", clean) != ["00000000", "00000000"]:
                raise ValueError("startup CPU fault")
            results.append({"scenario": scenario, "payload_entered": positive, "nor_sha256": sha(after),
                "erase_commands": 0, "program_bytes": 0, "choice_writes": 0,
                "raw_log_sha256": sha(raw.encode())})
            print("PASS " + scenario, flush=True)
    if authenticate_bundle(bundle) != (manifest, binary, boot) or inputs != identities("fdn"):
        raise ValueError("input identity changed during execution")
    report = {"schema": "aurora-persistent-boot-development-v1", "physical_qualified": False,
        "synthetic_transport": True, "bootloader_usb_update_executed": False,
        "source_commit": manifest["source_commit"], "selector_manifest_sha256": bundle.name,
        "runtime": runtime, "model_inputs": inputs, "results": results}
    (output / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--images", type=Path, required=True)
    parser.add_argument("--runner-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.bundle.resolve(), args.images.resolve(), args.runner_root.resolve(), args.output.resolve())
