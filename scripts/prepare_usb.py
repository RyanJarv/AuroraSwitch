"""Fetch stable public releases, authenticate them, then prepare a chosen drive."""
import argparse
import io
from pathlib import Path
import shutil
import subprocess
import tempfile
import urllib.request
import zipfile

from prepare_payloads import prepare


# Exact-byte authority stays in the firmware catalog, not in this URL list.
DOWNLOADS = {
    "Aurora_v1_4_4.bin": "https://www.qubitelectronix.com/s/Aurora_v1_4_4.zip",
    "AR_FDN_v1_2_2.bin": "https://www.qubitelectronix.com/s/AR_FDN_v1_2_2.bin",
    "flux-capacitor-0.3.0.bin": "https://github.com/DaveParr/aurora-flux-capacitor/releases/download/v0.3.0/flux-capacitor-0.3.0.bin",
    "aurora-morse-0.2.0.bin": "https://github.com/DaveParr/Aurora-Morse/releases/download/v0.2.0/aurora-morse-0.2.0.bin",
}


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
    args = parser.parse_args()
    selector = Path(__file__).resolve().parents[1] / "firmware/build-experimental-dma/AuroraSwitch.bin"
    try:
        stage(args.usb, selector)
    except (OSError, ValueError, KeyError, zipfile.BadZipFile,
            subprocess.CalledProcessError) as error:
        parser.exit(2, f"USB preparation failed: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
