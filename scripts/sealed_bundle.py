"""Authenticate sealed selector builds and their source-bound resident layout."""

import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from selector_constants import CONSTANTS
from package import toolchain_identity

ROOT = Path(__file__).resolve().parents[1]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def layout(source_commit: str) -> dict[str, int]:
    """Read the bundle's immutable source layout, never the working-tree header."""
    if not isinstance(source_commit, str) or not re.fullmatch(r"[0-9a-f]{40}", source_commit):
        raise ValueError("invalid selector source commit")
    # Older sealed candidates predate the shared constants header. Read either
    # path from that exact commit; a malformed current header never falls back.
    for path in ("support/selector_constants.h", "support/resident_layout.h"):
        try:
            text = subprocess.check_output(
                [
                    "git",
                    "--no-replace-objects",
                    "-C",
                    str(ROOT),
                    "show",
                    source_commit + ":" + path,
                ],
                text=True,
                stderr=subprocess.PIPE,
            )
            break
        except subprocess.CalledProcessError:
            continue
    else:
        raise ValueError("bundle source layout unavailable; fetch its source commit first")
    definitions = re.findall(r"^#define SELECTOR_RESIDENT_(\w+) (0x[0-9a-fA-F]+)$", text, re.M)
    required = {"BASE", "ENTRY", "METADATA", "IMAGE", "CAPACITY"}
    if len(definitions) != len(required) or {name for name, _ in definitions} != required:
        raise ValueError("unsupported resident layout definitions")
    return {name: int(value, 16) for name, value in definitions}


def checksum(data: bytes) -> int:
    value = CONSTANTS["FNV_OFFSET_BASIS"]
    for byte in data:
        value = ((value ^ byte) * CONSTANTS["FNV_PRIME"]) & CONSTANTS["U32_MASK"]
    return value


def authenticate_bundle(bundle: Path) -> tuple[dict, bytes, bytes]:
    """Require immutable package identity, ELF/BIN agreement and tool identity."""
    if (
        bundle.is_symlink()
        or not bundle.is_dir()
        or (bundle / "manifest.json").is_symlink()
        or not (bundle / "manifest.json").is_file()
    ):
        raise ValueError("selector bundle must contain regular files")
    raw = (bundle / "manifest.json").read_bytes()
    manifest = json.loads(raw)
    try:
        virtual = manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"]
        target = "AuroraSwitchVirtual" if virtual == 1 else "AuroraSwitch"
        names = {target + "." + suffix for suffix in ("elf", "bin", "map")}
        valid = (
            bundle.name == sha(raw)
            and manifest.get("schema") == "aurora-switch-development-bundle-v1"
            and manifest.get("development_only") is True
            and manifest.get("physical_qualified") is False
            and type(virtual) is int
            and virtual in (0, 1)
            and isinstance(manifest["source_commit"], str)
            and re.fullmatch(r"[0-9a-f]{40}", manifest["source_commit"])
            and manifest["build_provenance"]["method"] == "fresh-isolated-tracked-checkouts"
            and type(manifest["files"]) is dict
            and set(manifest["files"]) == names
            and {p.name for p in bundle.iterdir()} == names | {"manifest.json"}
        )
    except (KeyError, TypeError, AttributeError) as error:
        raise ValueError("unsupported selector bundle/provenance") from error
    if not valid:
        raise ValueError("unsupported selector bundle/provenance")
    data = {}
    for name, identity in manifest["files"].items():
        if (
            type(identity) is not dict
            or set(identity) != {"bytes", "sha256"}
            or type(identity["bytes"]) is not int
            or identity["bytes"] <= 0
            or not isinstance(identity["sha256"], str)
            or not re.fullmatch(r"[0-9a-f]{64}", identity["sha256"])
        ):
            raise ValueError("invalid selector file identity")
        path = bundle / name
        if path.is_symlink() or not path.is_file():
            raise ValueError("selector bundle must contain regular files")
        data[name] = path.read_bytes()
        if len(data[name]) != identity["bytes"] or sha(data[name]) != identity["sha256"]:
            raise ValueError("selector bundle bytes changed")
    tools, identities = toolchain_identity()
    if identities != manifest["build_provenance"].get("toolchain"):
        raise ValueError("selector toolchain changed")
    with tempfile.TemporaryDirectory(prefix="resident-elf-check-") as temporary:
        converted = Path(temporary) / "selector.bin"
        subprocess.run(
            [
                str(tools / "arm-none-eabi-objcopy"),
                "-O",
                "binary",
                str(bundle / (target + ".elf")),
                str(converted),
            ],
            check=True,
        )
        if converted.read_bytes() != data[target + ".bin"]:
            raise ValueError("selector ELF/BIN mismatch")
    symbols = subprocess.check_output(
        [
            str(tools / "arm-none-eabi-nm"),
            "--defined-only",
            "--format=posix",
            str(bundle / (target + ".elf")),
        ],
        text=True,
    )

    def address(name):
        matches = [
            row.split() for row in symbols.splitlines() if row.split() and row.split()[0] == name
        ]
        if len(matches) != 1 or len(matches[0]) < 3:
            raise ValueError("missing/ambiguous resident wrapper symbol: " + name)
        try:
            return int(matches[0][2], 16) - CONSTANTS["AXI_SRAM_BASE"]
        except ValueError as error:
            raise ValueError("invalid resident wrapper symbol: " + name) from error

    start, end = address("selector_resident_boot_start"), address("selector_resident_boot_end")
    binary = data[target + ".bin"]
    if not 0 <= start < end <= len(binary) or end - start > 256:
        raise ValueError("resident wrapper extent")
    return manifest, binary, binary[start:end]
