"""Fresh-build a pinned RAM experiment; this does not admit or qualify FataMorgana."""
from pathlib import Path
import json
import os
import struct
import subprocess
import tempfile

from package import digest, toolchain_identity
from setup_dependencies import ROOT, PINS, DAISY, git, setup

SOURCE_URL = "https://github.com/jfriess/Aurora-Firmwares.git"
SOURCE_COMMIT = "d5504d76370c370fdb40adcf755d8a4b9c07ee6b"
DAISYSP = "4263388e7fa8dfd34fa85c6a1c697362dc6981c7"


def check_cache(path: Path, revision: str) -> None:
    """Reject changed tracked inputs; ignored archives are never build inputs."""
    if git(path, "rev-parse", "HEAD") != revision or git(
            path, "status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError(f"changed source cache: {path}")


def check_vectors(data: bytes) -> None:
    """Require the existing staging/RAM contract, without claiming startup safety."""
    if not 8 <= len(data) <= 181888:
        raise ValueError("RAM probe exceeds current staging or has no vectors")
    stack, reset = struct.unpack_from("<II", data)
    if stack != 0x20020000 or not reset & 1 or not 0x24000000 <= reset < 0x24000000 + len(data):
        raise ValueError("RAM probe vector contract mismatch")


def build() -> Path:
    """Rebuild all objects in isolated tracked checkouts and retain exact identities."""
    setup()
    tool_directory, tools = toolchain_identity()
    source = ROOT / ".deps/Aurora-Firmwares"
    if not source.exists():
        subprocess.run(["git", "clone", "--no-checkout", SOURCE_URL, str(source)], check=True)
        subprocess.run(["git", "-C", str(source), "checkout", "--detach", SOURCE_COMMIT], check=True)
    if git(source, "remote", "get-url", "origin") != SOURCE_URL:
        raise RuntimeError("unexpected FataMorgana source origin")
    sdk = ROOT / ".deps/Aurora-SDK"
    subprocess.run(["git", "-C", str(sdk), "submodule", "update", "--init", "libs/DaisySP"], check=True)
    inputs = [(source, SOURCE_COMMIT, "source"), (sdk, PINS["Aurora-SDK"][1], "sdk"),
              (sdk / "libs/libDaisy", DAISY, "sdk/libs/libDaisy"),
              (sdk / "libs/DaisySP", DAISYSP, "sdk/libs/DaisySP")]
    for path, revision, _ in inputs:
        check_cache(path, revision)
    environment = {"PATH": str(tool_directory) + os.pathsep + os.defpath, "LC_ALL": "C"}
    with tempfile.TemporaryDirectory(prefix="fata-fresh-", dir=ROOT / ".deps") as temporary:
        root = Path(temporary)
        for cache, revision, relative in inputs:
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(["git", "clone", "--no-local", "--no-checkout", str(cache), str(target)], check=True)
            subprocess.run(["git", "-C", str(target), "checkout", "--detach", revision], check=True)
        for relative in ("sdk/libs/libDaisy", "sdk/libs/DaisySP"):
            subprocess.run(["make", "-j2", "-C", str(root / relative),
                            f"GCC_PATH={tool_directory}"], check=True, env=environment)
        application = root / "source/FataMorgana"
        subprocess.run(["make", "-j2", "-C", str(application), "all", "APP_TYPE=BOOT_SRAM",
                        f"AURORA_SDK_PATH={root / 'sdk'}", f"GCC_PATH={tool_directory}"],
                       check=True, env=environment)
        artifacts = {f"FataMorgana.{suffix}": (application / "build" / f"FataMorgana.{suffix}").read_bytes()
                     for suffix in ("elf", "bin", "map")}
        converted = root / "from-elf.bin"
        subprocess.run([str(tool_directory / "arm-none-eabi-objcopy"), "-O", "binary",
                        str(application / "build/FataMorgana.elf"), str(converted)], check=True)
        if converted.read_bytes() != artifacts["FataMorgana.bin"]:
            raise ValueError("FataMorgana ELF/BIN mismatch")
        check_vectors(artifacts["FataMorgana.bin"])
    for path, revision, _ in inputs:
        check_cache(path, revision)
    manifest = {"schema": "aurora-switch-source-payload-v1", "development_only": True,
        "method": "fresh-isolated-tracked-checkouts", "source_url": SOURCE_URL,
        "source_commit": SOURCE_COMMIT, "dependencies": {relative: revision for _, revision, relative in inputs[1:]},
        "configuration": {"APP_TYPE": "BOOT_SRAM"}, "toolchain": tools,
        "usb_behavior_verified": False, "selector_admitted": False,
        "files": {name: {"bytes": len(data), "sha256": digest(data)} for name, data in artifacts.items()}}
    encoded = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    destination = ROOT / ".deps/fatamorgana-builds" / digest(encoded)
    if destination.exists():
        if {p.name: p.read_bytes() for p in destination.iterdir()} != {**artifacts, "manifest.json": encoded}:
            raise RuntimeError("existing FataMorgana bundle changed")
    else:
        destination.mkdir(parents=True)
        for name, data in {**artifacts, "manifest.json": encoded}.items():
            (destination / name).write_bytes(data)
    return destination


if __name__ == "__main__":
    print("DEVELOPMENT ONLY, NOT ADMITTED:", build())
