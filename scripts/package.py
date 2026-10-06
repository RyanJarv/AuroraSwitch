"""Seal a clean development ELF/BIN/MAP bundle; never package vendor firmware."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import tempfile
import shutil
from contextlib import contextmanager

from setup_dependencies import setup, PINS, DAISY

ROOT = Path(__file__).resolve().parents[1]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


@contextmanager
def fresh_build(commit: str, *, virtual: bool = False):
    """Build tracked source and dependency checkouts, never ambient objects."""
    gcc = shutil.which("arm-none-eabi-gcc")
    if gcc is None:
        raise RuntimeError("GNU Arm 10-2020-q4-major toolchain required on PATH")
    tool_directory = Path(gcc).resolve().parent
    tools = {}
    for name in ("gcc", "g++", "as", "ar", "ld", "objcopy"):
        path = tool_directory / ("arm-none-eabi-" + name)
        tools[name] = {"sha256": digest(path.read_bytes()), "version":
            subprocess.check_output([str(path), "--version"], text=True).splitlines()[0]}
    if "10-2020-q4-major" not in tools["gcc"]["version"]:
        raise RuntimeError("release packaging requires GNU Arm 10-2020-q4-major")
    scratch = ROOT / ".deps"
    with tempfile.TemporaryDirectory(prefix="release-build-", dir=scratch) as directory:
        checkout = Path(directory) / "source"
        subprocess.run(["git", "clone", "--no-local", "--no-checkout", str(ROOT), str(checkout)], check=True)
        subprocess.run(["git", "-C", str(checkout), "checkout", "--detach", commit], check=True)
        dependencies = [("Aurora-SDK", PINS["Aurora-SDK"][1]),
                        ("mbedtls", PINS["mbedtls"][1]),
                        ("Aurora-SDK/libs/libDaisy", DAISY)]
        for relative, revision in dependencies:
            destination = checkout / ".deps" / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(["git", "clone", "--no-local", "--no-checkout",
                str(ROOT / ".deps" / relative), str(destination)], check=True)
            subprocess.run(["git", "-C", str(destination), "checkout", "--detach", revision], check=True)
        # No dependency archive or application object is copied into this tree.
        library = checkout / ".deps/Aurora-SDK/libs/libDaisy"
        # Do not inherit MAKEFLAGS/-e, CFLAGS or configuration overrides.
        build_environment = {"PATH": str(tool_directory) + os.pathsep + os.defpath,
                             "LC_ALL": "C"}
        subprocess.run(["make", "-j2", "-C", str(library), f"GCC_PATH={tool_directory}"],
                       check=True, env=build_environment)
        build_directory = "build-virtual-experimental-dma" if virtual else "build-experimental-dma"
        subprocess.run(["make", "-j2", "-C", str(checkout / "firmware"),
            f"GCC_PATH={tool_directory}", "EXPERIMENTAL_HANDOFF=1",
            "DMA_ARENA_CLEANUP=1", f"VIRTUAL_TRANSPORT={int(virtual)}",
            f"BUILD_DIR={build_directory}"], check=True, env=build_environment)
        yield checkout / "firmware" / build_directory, tools, tool_directory


def package(*, virtual: bool = False) -> Path:
    if type(virtual) is not bool:
        raise ValueError("virtual must be a boolean")
    setup()
    if subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain"], text=True).strip():
        raise RuntimeError("development packaging requires a clean committed source tree")
    commit = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()
    with fresh_build(commit, virtual=virtual) as (build, tools, tool_directory):
        return seal(build, commit, tools, tool_directory, virtual=virtual)


def seal(build: Path, commit: str, tools: dict, tool_directory: Path, *, virtual: bool = False) -> Path:
    target = "AuroraSwitchVirtual" if virtual else "AuroraSwitch"
    payload = {f"{target}.{suffix}": (build / f"{target}.{suffix}").read_bytes()
               for suffix in ("elf", "bin", "map")}
    with tempfile.TemporaryDirectory(prefix="aurora-package-check-") as directory:
        converted = Path(directory) / "from-elf.bin"
        subprocess.run([str(tool_directory / "arm-none-eabi-objcopy"), "-O", "binary",
            str(build / f"{target}.elf"), str(converted)], check=True)
        if converted.read_bytes() != payload[f"{target}.bin"]:
            raise RuntimeError("ELF/BIN identity mismatch")
    manifest = {"schema": "aurora-switch-development-bundle-v1", "development_only": True,
        "physical_qualified": False, "source_commit": commit,
        "build_provenance": {"method": "fresh-isolated-tracked-checkouts",
            "toolchain": tools, "configuration": {"EXPERIMENTAL_HANDOFF": 1,
                "DMA_ARENA_CLEANUP": 1, "VIRTUAL_TRANSPORT": int(virtual)}},
        "files": {name: {"bytes": len(data), "sha256": digest(data)}
                  for name, data in payload.items()},
        # Record the same pins used to authenticate and clone build inputs.
        "dependency_revisions": {**{name: revision for name, (_, revision) in PINS.items()},
            "libDaisy": DAISY}}
    manifest_bytes = (json.dumps(manifest, sort_keys=True, indent=2) + "\n").encode()
    if subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip() != commit:
        raise RuntimeError("source HEAD changed during packaging")
    if subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain"], text=True).strip():
        raise RuntimeError("source changed during packaging")
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
        if (build / f"{target}.{suffix}").read_bytes() != payload[f"{target}.{suffix}"]:
            raise RuntimeError("build changed during packaging")
    print(f"DEVELOPMENT ONLY: {destination}")
    return destination


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--virtual", action="store_true",
                        help="seal a synthetic-media test build, never install this on hardware")
    package(virtual=parser.parse_args().virtual)
