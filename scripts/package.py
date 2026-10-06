"""Seal a clean development ELF/BIN/MAP bundle; never package vendor firmware."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import tempfile

from setup_dependencies import setup

ROOT = Path(__file__).resolve().parents[1]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def package() -> Path:
    setup()
    if subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain"], text=True).strip():
        raise RuntimeError("development packaging requires a clean committed source tree")
    commit = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()
    build = ROOT / "firmware/build-experimental-dma"
    payload = {f"AuroraSwitch.{suffix}": (build / f"AuroraSwitch.{suffix}").read_bytes()
               for suffix in ("elf", "bin", "map")}
    with tempfile.TemporaryDirectory(prefix="aurora-package-check-") as directory:
        converted = Path(directory) / "from-elf.bin"
        subprocess.run(["arm-none-eabi-objcopy", "-O", "binary",
            str(build / "AuroraSwitch.elf"), str(converted)], check=True)
        if converted.read_bytes() != payload["AuroraSwitch.bin"]:
            raise RuntimeError("ELF/BIN identity mismatch")
    manifest = {"schema": "aurora-switch-development-bundle-v1", "development_only": True,
        "physical_qualified": False, "source_commit": commit,
        "files": {name: {"bytes": len(data), "sha256": digest(data)}
                  for name, data in payload.items()},
        "dependency_revisions": {"Aurora-SDK": "69b74a88b25e2fb4d722fc269bfd9395dd28edb5",
            "libDaisy": "63fcabd38a20e14bc744499f0460e47925ea753e",
            "mbedtls": "2fc8413bfcb51354c8e679141b17b3f1a5942561"}}
    manifest_bytes = (json.dumps(manifest, sort_keys=True, indent=2) + "\n").encode()
    identity = digest(manifest_bytes)
    payload["manifest.json"] = manifest_bytes
    destination = ROOT / "dist" / identity
    destination.parent.mkdir(exist_ok=True)
    if destination.exists():
        if set(path.name for path in destination.iterdir()) != set(payload):
            raise RuntimeError("existing immutable bundle inventory changed")
        for name, data in payload.items():
            if (destination / name).read_bytes() != data:
                raise RuntimeError("existing immutable bundle bytes changed")
    else:
        with tempfile.TemporaryDirectory(prefix=".bundle-", dir=destination.parent) as directory:
            staging = Path(directory) / "sealed"
            staging.mkdir()
            for name, data in payload.items():
                (staging / name).write_bytes(data)
            os.rename(staging, destination)
    # Mutable build output is checked again after sealing.
    for suffix in ("elf", "bin", "map"):
        if (build / f"AuroraSwitch.{suffix}").read_bytes() != payload[f"AuroraSwitch.{suffix}"]:
            raise RuntimeError("build changed during packaging")
    print(f"DEVELOPMENT ONLY: {destination}")
    return destination


if __name__ == "__main__":
    package()
