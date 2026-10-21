"""Bounded linked startup/launch regressions using the existing external runner.

One lane starts in the stock loader with no physical USB; other lanes start at
its prepared resident handoff. USB and choice files are synthetic. This does not
execute the loader's USB updater, real USB, audio, physical reset, or recovery.
Local records include private third-party bytes.
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
    observer_path = Path(__file__).with_name("PersistentBootObserver.cs")
    runner_identity, observer_identity = sha(Path(__file__).read_bytes()), sha(observer_path.read_bytes())
    sys.path.insert(0, str(runner_root.resolve()))
    from harness.lib import emulator
    from spike.aurora_selector.audio_routes import prepare_platform, RENODE, identities, LOADER
    from spike.aurora_selector.loader_boundary import authenticate_usb_runtime, LOADER_SHA256
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
    readback_symbols = [line for line in nm.splitlines() if line.split()[0] == "resident_readback_start"]
    if readback_symbols:
        copy_complete = symbol("resident_readback_start") - symbol("selector_resident_boot_start")
    else:
        # Historical negative control: this is the old source-hash comparison,
        # after the last SRAM store, not a verifier for that old implementation.
        comparisons = [i for i in range(0, len(boot) - 1, 2) if boot[i:i + 2] == b"\xbd\x42"]
        if len(comparisons) != 1: raise ValueError("ambiguous copy comparison")
        copy_complete = comparisons[0]
    trampolines = []
    for first, last in (("selector_copy_jump_blob_start", "selector_copy_jump_end"),
                        ("selector_qspi_jump_blob_start", "selector_qspi_jump_end")):
        blob = binary[symbol(first) - 0x24000000:symbol(last) - 0x24000000]
        branch = [i for i in range(0, len(blob) - 1, 2) if blob[i:i + 2] == b"\x30\x47"]
        if len(branch) != 1: raise ValueError("ambiguous terminal branch")
        trampolines.append((0x38000000 + branch[0], len(blob), sha(blob)))
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
    loader_bytes = LOADER.read_bytes()
    if sha(loader_bytes) != LOADER_SHA256:
        raise ValueError("stock bootloader identity changed")
    inputs = identities("fdn")
    handoff = None
    # Exact values independently observed at the pinned loader's terminal BLX.
    # After the real-loader lane, reuse its newly captured register values.
    prepared_mapping = {0x52005000: 0x400001, 0x52005014: 0x0f18edeb, 0xe000ed88: 0x00f00000}
    results = []
    output.mkdir(parents=True, exist_ok=False)
    for scenario in ("bad-copy", "loader-dirt", "dirt", "repeat-dirt", "fdn", "menu", "bad-record", "unknown-record", "bad-file", "bad-resident", "bad-wrapper",
                     "choose-fdn", "repeat-fdn"):
        directory = output / scenario
        directory.mkdir()
        with emulator._exclusive_emulator_lock(), tempfile.TemporaryDirectory(prefix="persistent-vm-") as temporary:
            temp = Path(temporary)
            platform, includes = prepare_platform(temp, "fdn", len(binary))
            choice = 0 if scenario in ("fdn", "choose-fdn", "repeat-fdn") else dirt_index
            image_base = 0x24000000 if choice == 0 else 0x90040000
            installed = present[0] if choice == 0 else combined[:len(present[dirt_index])]
            with platform.open("a") as stream:
                stream.write('persistentObserver: VCVQuBit.Development.PersistentBootObserver @ sysbus 0x10012000\n'
                    f'    imageBase: {hex(image_base)}\n    imageSize: {len(installed)}\n'
                    f'    imageHash: "{sha(installed)}"\n    resetVector: {hex(rows[choice]["reset"])}\n'
                    f'    finalCheckpoint: {hex((rows[choice]["reset"] & ~1) if choice == 0 else 0x900421b8)}\n')
            includes.append("include " + monitor_path(observer_path))
            record = b"AURBOOT1" + struct.pack("<I", 1) + rows[choice]["sha256"].encode()
            record += struct.pack("<I", zlib.crc32(record))
            if scenario == "repeat-dirt":
                record = (output / "dirt" / "boot-store.bin").read_bytes()[16:96]
            if scenario == "repeat-fdn":
                record = (output / "choose-fdn" / "boot-store.bin").read_bytes()[16:96]
            if scenario == "bad-record": record = record[:-1] + bytes([record[-1] ^ 1])
            second_record = bytes(80)
            if scenario == "unknown-record":
                record = b"AURBOOT1" + struct.pack("<I", 1) + rows[0]["sha256"].encode()
                record += struct.pack("<I", zlib.crc32(record))
                second_record = b"AURBOOT1" + struct.pack("<I", 2) + b"0" * 64
                second_record += struct.pack("<I", zlib.crc32(second_record))
            initial = bytearray(bytes(range(256)) * (0x800000 // 256))
            initial[0x40000:0x40000 + len(combined)] = combined
            if scenario == "bad-resident": initial[0x40958] ^= 1
            if scenario == "bad-wrapper": initial[layout()["METADATA"] - 0x90000000 + 12] ^= 1
            nor = temp / "nor.bin"
            nor.write_bytes(initial)
            commands = includes + ["logLevel 3", 'mach create "persistent-boot"',
                "machine LoadPlatformDescription " + monitor_path(platform),
                "auroraI2c1.auroraLed0 PwmRegisters true", "auroraI2c1.auroraLed1 PwmRegisters true",
                "auroraQspi.auroraNor ArraySubset true",
                "sysbus LoadBinary " + monitor_path(nor) + " 0x90000000"]
            for site, size, identity in trampolines:
                commands.append(f'persistentObserver ObserveBranch {hex(site)} {size} "{identity}"')
            if scenario == "loader-dirt":
                commands += ["sysbus LoadBinary " + monitor_path(LOADER) + " 0x08000000",
                    "cpu VectorTableOffset 0x08000000", "logLevel 3 nvic"]
            else:
                # Prepared loader handoff: inherited memory-mapped QSPI, not
                # model evidence that controller gating works electrically.
                commands += [f"sysbus WriteDoubleWord {hex(address)} {hex(value)}"
                             for address, value in prepared_mapping.items()]
                commands.append("cpu VectorTableOffset 0x90040000")
            dma = temp / "dirty-dma.bin"
            dma.write_bytes(b"\xa5" * 32768)
            commands.append("sysbus LoadBinary " + monitor_path(dma) + " 0x30000000")
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
                + f"bus.WriteBytes(Array[Byte]({list(struct.pack('<4I', 1, 3 if scenario == 'unknown-record' else int(scenario != 'choose-fdn'), 0, 0) + record + second_record)!r}), {store})\n"
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
            if scenario == "bad-copy":
                fault_pc = (layout()["ENTRY"] & ~1) + copy_complete
                fault_address = 0x24000000 + len(binary) - 1
                commands.append(f'cpu AddHook {hex(fault_pc)} "bus = machine.SystemBus; '
                    f'bus.WriteByte({fault_address}, bus.ReadByte({fault_address}) ^ 1); '
                    'print(\'PERSISTENT_COPY_FAULT\')"')
            if scenario == "loader-dirt":
                commands += ["powerBoundary ObserveInstructionCheckpoint 0x08005ab6 "
                    + monitor_path(directory / "loader-entry"),
                    f"powerBoundary AddCheckpointRegion 0x90040000 {(len(combined) + 4095) & ~4095}",
                    "powerBoundary AddCheckpointRegister 0xe000ed08",
                    "powerBoundary AddCheckpointRegister 0x52005000",
                    "powerBoundary AddCheckpointRegister 0x52005014",
                    "powerBoundary AddCheckpointRegister 0xe000ed88",
                    'emulation RunFor "18.000000"', "pause",
                    "auroraQspi.auroraNor Diagnostics", "powerBoundary ResumeInstructionCheckpoint",
                    "powerBoundary RetireObservations"]
            if scenario == "choose-fdn":
                commands += ['emulation RunFor "3.000000"', "pause", "gpioPortC OnGPIO 11 false",
                    'emulation RunFor "0.040000"', "pause", "gpioPortC OnGPIO 11 true"]
            # Discovery authenticates every present catalog image before the
            # saved selection is reloaded. Allow the complete populated drive,
            # not just the two-file fixture, to finish this bounded window.
            commands += ['emulation RunFor "4.000000"', "pause", "auroraQspi.auroraNor ArrayDiagnostics",
                "persistentObserver Diagnostics",
                "auroraQspi.auroraNor Diagnostics",
                "auroraQspi.auroraNor ExportArray " + monitor_path(directory / "nor-after.bin"),
                f'dump_file {hex(store)} 176 "{directory / "boot-store.bin"}"',
                f'dump_file {hex(diagnostic)} 20 "{directory / "qspi.bin"}"',
                'echo "PERSISTENT_FINAL_PC"', "cpu PC",
                'echo "PERSISTENT_FINAL_MEDIA"', f"sysbus ReadDoubleWord {media + 4}",
                f'dump_file 0x30008000 {len(present[choice])} "{directory / "staged.bin"}"',
                f'dump_file 0x38000000 1024 "{directory / "trampoline.bin"}"',
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
            positive = scenario in ("loader-dirt", "dirt", "repeat-dirt", "fdn", "choose-fdn", "repeat-fdn")
            if (clean.splitlines().count("PERSISTENT_PAYLOAD_HALT") != int(positive)
                    or clean.splitlines().count("PERSISTENT_PAYLOAD_RESET") != int(positive)
                    or clean.splitlines().count("PERSISTENT_DMA_ZERO") != int(positive)
                    or clean.splitlines().count("PERSISTENT_HANDOFF_BRANCH") != int(positive)
                    or clean.splitlines().count(f"PERSISTENT_HANDOFF branches={int(positive)} entries={int(positive)}") != 1):
                raise ValueError("unexpected/missing payload entry: " + str(directory / "raw.log"))
            if clean.splitlines().count("PERSISTENT_SELECTOR_MAIN") != int(scenario not in ("bad-wrapper", "bad-copy")):
                raise ValueError("wrapper did not enter expected selector startup")
            if scenario in ("bad-wrapper", "bad-copy") and clean.splitlines().count("PERSISTENT_WRAPPER_REJECT") != 1:
                raise ValueError("corrupt wrapper metadata did not reject")
            if scenario == "bad-copy" and clean.splitlines().count("PERSISTENT_COPY_FAULT") != 1:
                raise ValueError("missing SRAM copy fault injection")
            after = (directory / "nor-after.bin").read_bytes()
            if after != initial or clean.splitlines().count("IS25LP064_ARRAY enabled=1 erase_commands=0 program_bytes=0") != 1:
                raise ValueError("flash changed or writes occurred")
            status = re.findall(r"(?m)^IS25LP064_STARTUP .*?status_writes=(\d+) .*?rejected_status_writes=(\d+)$", clean)
            if scenario == "loader-dirt":
                # The stock loader initializes QE itself. The resident path
                # must add no status writes to that measured baseline.
                if (len(status) != 2 or status[0] != status[1]
                        or status[0][1] != "0" or int(status[0][0]) != 1
                        or "DAISY_INSTRUCTION_CHECKPOINT site=0x08005ab6 halted=1" not in clean):
                    raise ValueError("stock-loader handoff/status baseline missing or changed")
                frame = (directory / "loader-entry/registers.bin").read_bytes()
                if len(frame) != 200: raise ValueError("loader register checkpoint extent")
                registers = struct.unpack("<50I", frame)
                mmio = dict(struct.iter_unpack("<II", (directory / "loader-entry/mmio.bin").read_bytes()))
                if (registers[3] != layout()["ENTRY"] or registers[13] != 0x20020000
                        or registers[15] != 0x08005ab6 or mmio.get(0xe000ed08) != 0x90040000
                        or mmio.keys() != {0xe000ed08, *prepared_mapping}
                        or not mmio[0x52005000] & 1
                        or mmio[0x52005014] & 0x0c000000 != 0x0c000000
                        or (directory / "loader-entry/memory-90040000.bin").read_bytes()[:len(combined)] != combined):
                    raise ValueError("stock-loader entry/context differs")
                prepared_mapping = {address: mmio[address] for address in prepared_mapping}
                handoff = {"bootloader_sha256": sha(loader_bytes), "entry": registers[3],
                           "stack": registers[13], "vtor": mmio[0xe000ed08],
                           "registers": {hex(address): value for address, value in prepared_mapping.items()}}
            elif status != [("0", "0")]:
                raise ValueError("unexpected flash status-register write")
            state = (directory / "boot-store.bin").read_bytes()
            expected_writes = int(scenario == "choose-fdn")
            if scenario not in ("bad-wrapper", "bad-copy") and struct.unpack_from("<I", state, 8)[0] != expected_writes:
                raise ValueError("choice write count differs")
            if scenario == "choose-fdn":
                saved = state[16:96]
                if saved[12:76] != rows[0]["sha256"].encode() or zlib.crc32(saved[:76]) != struct.unpack_from("<I", saved, 76)[0]:
                    raise ValueError("explicit choice did not persist a valid exact image identity")
            if scenario in ("fdn", "choose-fdn", "repeat-fdn") and (directory / "ram.bin").read_bytes() != present[0]:
                raise ValueError("RAM handoff bytes changed")
            if re.findall(r"(?m)^PERSISTENT_(?:CFSR|HFSR)\s+0x([0-9a-fA-F]+)$", clean) != ["00000000", "00000000"]:
                raise ValueError("startup CPU fault")
            results.append({"scenario": scenario, "payload_entered": positive, "nor_sha256": sha(after),
                "erase_commands": 0, "program_bytes": 0, "choice_writes": expected_writes,
                "stock_bootloader_executed": scenario == "loader-dirt",
                "stock_status_writes": int(status[0][0]) if scenario == "loader-dirt" else 0,
                "raw_log_sha256": sha(raw.encode())})
            print("PASS " + scenario, flush=True)
    if (authenticate_bundle(bundle) != (manifest, binary, boot) or inputs != identities("fdn")
            or runner_identity != sha(Path(__file__).read_bytes()) or observer_identity != sha(observer_path.read_bytes())):
        raise ValueError("input identity changed during execution")
    report = {"schema": "aurora-persistent-boot-development-v1", "physical_qualified": False,
        "synthetic_transport": True, "bootloader_usb_update_executed": False,
        "source_commit": manifest["source_commit"], "selector_manifest_sha256": bundle.name,
        "runtime": runtime, "model_inputs": inputs, "runner_sha256": runner_identity,
        "observer_sha256": observer_identity, "stock_loader_handoff": handoff,
        "payloads": {rows[i]["filename"]: sha(data) for i, data in present.items()}, "results": results}
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
