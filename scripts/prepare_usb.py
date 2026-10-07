"""Fetch stable public releases, authenticate them, then prepare a chosen drive."""
import argparse
import io
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import urllib.request
import urllib.parse
import zipfile

from prepare_payloads import prepare


# Exact-byte authority stays in the firmware catalog, not in this URL list.
DOWNLOADS = {
    "Aurora_v1_4_4.bin": "https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip",
    "AR_FDN_v1_2_2.bin": "https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin",
    "flux-capacitor-0.3.0.bin": "https://github.com/DaveParr/aurora-flux-capacitor/releases/download/v0.3.0/flux-capacitor-0.3.0.bin",
    "aurora-morse-0.2.0.bin": "https://github.com/DaveParr/Aurora-Morse/releases/download/v0.2.0/aurora-morse-0.2.0.bin",
}


def release_selector(tag: str, destination: Path) -> Path:
    """Check release bytes and tag identity before any drive writes (not a signature)."""
    repository = "RyanJarv/AuroraSwitch"
    if tag == "latest":
        tag = subprocess.check_output([
            "gh", "release", "view", "--repo", repository,
            "--json", "tagName", "--jq", ".tagName"], text=True).strip()
        if not tag:
            raise ValueError("no latest published release")
    subprocess.run(["gh", "release", "download", tag, "--repo", repository,
                    "--dir", str(destination)], check=True)
    manifest = json.loads((destination / "manifest.json").read_bytes())
    commit = subprocess.check_output([
        "gh", "api", f"repos/{repository}/commits/{urllib.parse.quote(tag, safe='')}",
        "--jq", ".sha"], text=True).strip()
    print(f"Checking release {tag} from {commit}", flush=True)
    if (manifest.get("schema") != "aurora-switch-development-bundle-v1"
            or manifest.get("source_commit") != commit
            or manifest.get("build_provenance", {}).get("configuration") != {
                "EXPERIMENTAL_HANDOFF": 1, "DMA_ARENA_CLEANUP": 1, "VIRTUAL_TRANSPORT": 0}):
        raise ValueError("release source identity or USB build configuration mismatch")
    files = manifest.get("files", {})
    if set(files) != {"AuroraSwitch.bin", "AuroraSwitch.elf", "AuroraSwitch.map"}:
        raise ValueError("unexpected release artifact inventory")
    for name, identity in files.items():
        data = (destination / name).read_bytes()
        if len(data) != identity["bytes"] or hashlib.sha256(data).hexdigest() != identity["sha256"]:
            raise ValueError(f"release byte mismatch: {name}")
    return destination / "AuroraSwitch.bin"


def check_destination(usb: Path) -> None:
    """Refuse missing drives, redirected outputs and competing updater files."""
    if not usb.is_absolute() or not usb.is_dir() or usb.resolve() == Path("/"):
        raise ValueError("USB path must be an existing absolute drive directory")
    for entry in usb.iterdir():
        if entry.suffix.lower() == ".bin" and entry.name != "AuroraSwitch.bin":
            raise ValueError(f"move root-level updater off the drive first: {entry.name}")
    for path in [usb / "aurora", usb / "AuroraSwitch.bin",
                 *(usb / "aurora" / name for name in DOWNLOADS)]:
        if path.is_symlink():
            raise ValueError(f"refusing symlink destination: {path}")


def download(url: str, filename: str) -> bytes:
    """Bound downloads and extract only the named BIN, never archive paths."""
    limit = 2 * 1024 * 1024
    with urllib.request.urlopen(url, timeout=30) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError(f"download too large: {filename}")
    if url.endswith(".zip"):
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            if archive.getinfo(filename).file_size > limit:
                raise ValueError(f"archive image too large: {filename}")
            data = archive.read(filename)
    return data


def stage(usb: Path, selector: Path) -> None:
    """Verify every payload before drive writes; copy the selector last."""
    check_destination(usb)
    selector_bytes = selector.read_bytes()
    with tempfile.TemporaryDirectory(prefix="aurora-usb-") as temporary:
        root = Path(temporary)
        files = []
        for name, url in DOWNLOADS.items():
            print(f"Fetching {name}", flush=True)
            path = root / name
            path.write_bytes(download(url, name))
            files.append(path)
        prepared = root / "prepared"
        prepare(files, prepared)
        check_destination(usb)
        (usb / "aurora").mkdir(exist_ok=True)
        for source in sorted((prepared / "aurora").iterdir()):
            destination = usb / "aurora" / source.name
            shutil.copyfile(source, destination)
            if destination.read_bytes() != source.read_bytes():
                raise ValueError(f"USB readback mismatch: {source.name}")
        destination = usb / "AuroraSwitch.bin"
        destination.write_bytes(selector_bytes)
        if destination.read_bytes() != selector_bytes:
            raise ValueError("USB readback mismatch: AuroraSwitch.bin")
    print("USB prepared. Safely eject it, insert into Aurora, and power cycle.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--usb", type=Path, required=True)
    parser.add_argument("--release-tag", help="download a checked GitHub release instead of building")
    args = parser.parse_args()
    selector = Path(__file__).resolve().parents[1] / "firmware/build-experimental-dma/AuroraSwitch.bin"
    try:
        check_destination(args.usb)
        if args.release_tag:
            with tempfile.TemporaryDirectory(prefix="aurora-release-") as temporary:
                stage(args.usb, release_selector(args.release_tag, Path(temporary)))
        else:
            stage(args.usb, selector)
    except (OSError, ValueError, KeyError, zipfile.BadZipFile,
            subprocess.CalledProcessError) as error:
        parser.exit(2, f"USB preparation failed: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
