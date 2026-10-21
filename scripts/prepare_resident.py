"""Build a PRIVATE resident installer from a sealed selector and user-owned Dirt.

No downloads, mounted-drive changes, or programming. Output contains third-party
firmware and must not be distributed. Stock loader and Dirt code addresses stay
unchanged; the sole Dirt patch redirects its reset-vector word to our wrapper.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from package import toolchain_identity
from verify_images import authenticator, catalog

ROOT = Path(__file__).resolve().parents[1]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def layout() -> dict[str, int]:
    text = (ROOT / "support/resident_layout.h").read_text()
    return {name: int(value, 16) for name, value in re.findall(
        r"^#define SELECTOR_RESIDENT_(\w+) (0x[0-9a-fA-F]+)$", text, re.M)}


def checksum(data: bytes) -> int:
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def authenticate_bundle(bundle: Path) -> tuple[dict, bytes, bytes]:
    """Require immutable package identity, ELF/BIN agreement and tool identity."""
    raw = (bundle / "manifest.json").read_bytes()
    manifest = json.loads(raw)
    virtual = manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"]
    target = "AuroraSwitchVirtual" if virtual == 1 else "AuroraSwitch"
    names = {target + "." + suffix for suffix in ("elf", "bin", "map")}
    if (bundle.name != sha(raw) or manifest.get("schema") != "aurora-switch-development-bundle-v1"
            or manifest.get("development_only") is not True or manifest.get("physical_qualified") is not False
            or type(virtual) is not int or virtual not in (0, 1)
            or manifest["build_provenance"]["method"] != "fresh-isolated-tracked-checkouts"
            or set(manifest["files"]) != names
            or {p.name for p in bundle.iterdir()} != names | {"manifest.json"}):
        raise ValueError("unsupported selector bundle/provenance")
    data = {}
    for name, identity in manifest["files"].items():
        path = bundle / name
        data[name] = path.read_bytes()
        if path.is_symlink() or len(data[name]) != identity["bytes"] or sha(data[name]) != identity["sha256"]:
            raise ValueError("selector bundle bytes changed")
    tools, identities = toolchain_identity()
    if identities != manifest["build_provenance"]["toolchain"]:
        raise ValueError("selector toolchain changed")
    with tempfile.TemporaryDirectory(prefix="resident-elf-check-") as temporary:
        converted = Path(temporary) / "selector.bin"
        subprocess.run([str(tools / "arm-none-eabi-objcopy"), "-O", "binary",
                        str(bundle / (target + ".elf")), str(converted)], check=True)
        if converted.read_bytes() != data[target + ".bin"]:
            raise ValueError("selector ELF/BIN mismatch")
    symbols = subprocess.check_output([str(tools / "arm-none-eabi-nm"), "--defined-only", "--format=posix",
                                       str(bundle / (target + ".elf"))], text=True)
    def address(name):
        values = [int(row.split()[2], 16) for row in symbols.splitlines() if row.split()[0] == name]
        if len(values) != 1: raise ValueError("missing resident wrapper symbol: " + name)
        return values[0] - 0x24000000
    start, end = address("selector_resident_boot_start"), address("selector_resident_boot_end")
    binary = data[target + ".bin"]
    if not 0 <= start < end <= len(binary) or end - start > 256:
        raise ValueError("resident wrapper extent")
    return manifest, binary, binary[start:end]


def compose(dirt: bytes, selector: bytes, boot: bytes) -> bytes:
    """Deterministic bounded layout; original Dirt authentication is the caller's job."""
    addresses = layout()
    base, entry, metadata, image, capacity = (addresses[key] for key in
        ("BASE", "ENTRY", "METADATA", "IMAGE", "CAPACITY"))
    if (len(dirt) != 95196 or struct.unpack_from("<II", dirt) != (0x20020000, 0x90040959)
            or not 8 < len(selector) <= capacity or not 0 < len(boot) <= 256
            or not base + len(dirt) <= (entry & ~1) < metadata < image
            or metadata + 16 > image or image + capacity > 0x90800000):
        raise ValueError("resident input/layout bounds")
    stack, reset = struct.unpack_from("<II", selector)
    if stack != 0x20020000 or not reset & 1 or not 0x24000008 <= reset < 0x24000000 + len(selector):
        raise ValueError("selector reset/stack vectors")
    combined = bytearray(b"\xff" * (image - base + len(selector)))
    combined[:len(dirt)] = dirt
    struct.pack_into("<I", combined, 4, entry)
    offset = (entry & ~1) - base
    combined[offset:offset + len(boot)] = boot
    struct.pack_into("<4I", combined, metadata - base,
                     0x31525341, len(selector), len(selector) ^ 0xffffffff, checksum(selector))
    combined[image - base:] = selector
    assert combined[:4] == dirt[:4] and combined[8:len(dirt)] == dirt[8:]
    return bytes(combined)


def prepare(bundle: Path, dirt_path: Path, output: Path) -> dict:
    if output.exists() or output.is_symlink():
        raise ValueError("output already exists; choose a new local folder")
    manifest, selector, boot = authenticate_bundle(bundle)
    with dirt_path.open("rb") as source:
        dirt = source.read(95197)
    with authenticator() as executable, tempfile.TemporaryDirectory(prefix="resident-dirt-check-") as temporary:
        matches = [row for row in catalog(executable) if row["filename"] == "DirtVerb 1.1.bin"
                   and row["bytes"] == len(dirt) and row["sha256"] == sha(dirt)]
        if len(matches) != 1: raise ValueError("unsupported or changed Dirt image")
        private = Path(temporary) / "DirtVerb 1.1.bin"
        private.write_bytes(dirt)
        subprocess.run([str(executable), matches[0]["filename"], str(private)], check=True, capture_output=True)
    combined = compose(dirt, selector, boot)
    virtual = manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"] == 1
    filename = "AuroraSwitchResidentVirtual.bin" if virtual else "AuroraSwitch.bin"
    receipt = {"schema": "aurora-switch-private-resident-v1", "private_contains_third_party_firmware": True,
               "development_only": True, "physical_qualified": False, "synthetic_transport": virtual,
               "source_commit": manifest["source_commit"], "selector_manifest_sha256": bundle.name,
               "selector_sha256": sha(selector), "dirt_original_sha256": sha(dirt),
               "wrapper_sha256": sha(boot), "layout": layout(),
               "patch": {"offset": 4, "before": 0x90040959, "after": layout()["ENTRY"]},
               "files": {filename: {"bytes": len(combined), "sha256": sha(combined)},
                         "aurora/DirtVerb 1.1.bin": {"bytes": len(dirt), "sha256": sha(dirt)}}}
    # Reauthenticate inputs before publishing; all output is exclusive creation.
    if authenticate_bundle(bundle) != (manifest, selector, boot):
        raise ValueError("selector bundle changed during preparation")
    output.mkdir(parents=True, exist_ok=False)
    (output / "aurora").mkdir()
    for name, data in ((filename, combined), ("aurora/DirtVerb 1.1.bin", dirt)):
        with (output / name).open("xb") as stream: stream.write(data)
    with (output / "resident.json").open("x") as stream:
        json.dump(receipt, stream, sort_keys=True, indent=2)
        stream.write("\n")
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--dirt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.bundle, args.dirt, args.output)
    print(f"PRIVATE development image prepared in {args.output}; no firmware installed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
