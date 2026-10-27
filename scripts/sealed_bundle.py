"""Authenticate immutable selector-only packages before local validation."""

import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from package import toolchain_identity

ROOT = Path(__file__).resolve().parents[1]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def authenticate_bundle(bundle: Path) -> tuple[dict, bytes]:
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
        target = "AuroraSwitch"
        names = {target + "." + suffix for suffix in ("elf", "bin", "map")}
        valid = (
            bundle.name == sha(raw)
            and manifest.get("schema") == "aurora-switch-selector-build-v2"
            and manifest.get("development_only") is True
            and manifest.get("physical_qualified") is False
            and manifest["build_provenance"]["configuration"]
            == {
                "application": "BOOT_SRAM",
                "storage": "usb",
                "dma_arena_cleanup": True,
            }
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
    with tempfile.TemporaryDirectory(prefix="selector-elf-check-") as temporary:
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
    return manifest, data[target + ".bin"]
