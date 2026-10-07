"""Prepare user-owned exact images offline; no downloads, flashing or USB access."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from verify_images import authenticator, catalog


def prepare(files: list[Path], output: Path, *, qspi: bool = False) -> dict:
    """Authenticate private copies, then publish a non-overwriting folder."""
    if type(qspi) is not bool:
        raise ValueError("qspi must be a boolean")
    if not files:
        raise ValueError("supply at least one supported firmware file")
    # New output only: never overwrite a drive, earlier bundle or recovery file.
    # A partial directory after an I/O failure must be inspected, not reused.
    if output.exists() or output.is_symlink():
        raise ValueError("output already exists; choose a new empty destination")
    with authenticator(qspi=qspi) as executable, tempfile.TemporaryDirectory(
            prefix="aurora-payloads-") as temporary:
        images = catalog(executable)
        staged = []
        maximum = max(image["bytes"] for image in images)
        for source in files:
            with source.open("rb") as stream:
                data = stream.read(maximum + 1)
            digest = hashlib.sha256(data).hexdigest()
            matches = [image for image in images
                       if image["bytes"] == len(data) and image["sha256"] == digest]
            if len(matches) != 1:
                raise ValueError(f"unsupported or changed image: {source.name}")
            image = matches[0]
            if any(row[0]["filename"] == image["filename"] for row in staged):
                raise ValueError("duplicate selected image")
            copy = Path(temporary) / image["filename"]
            copy.write_bytes(data)
            # Reuse staging, vectors, SHA and corruption controls from C++.
            subprocess.run([str(executable), image["filename"], str(copy)],
                           check=True, capture_output=True)
            staged.append((image, data))
        receipt = {"schema": "aurora-switch-payloads-v1",
                   "images": [image for image, _ in staged],
                   "qualification": "byte-authentication-only"}
        if qspi:
            receipt["development_qspi_catalog"] = True
        # All inputs pass before creating any public output. Exclusive creation
        # also refuses a destination created concurrently after the first check.
        output.mkdir(parents=True, exist_ok=False)
        folder = output / "aurora"
        folder.mkdir()
        for image, data in staged:
            with (folder / image["filename"]).open("xb") as stream:
                stream.write(data)
        with (output / "payloads.json").open("x") as stream:
            json.dump(receipt, stream, sort_keys=True, indent=2)
            stream.write("\n")
        return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="new local folder; never an existing drive root")
    parser.add_argument("files", type=Path, nargs="+")
    parser.add_argument("--qspi", action="store_true", help="use the opt-in development catalog")
    args = parser.parse_args()
    try:
        receipt = prepare(args.files, args.output, qspi=args.qspi)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(2, f"Payload preparation failed: {error}\n")
    print(f"Prepared {len(receipt['images'])} authenticated image(s) in {args.output}/aurora")
    print("No firmware installed; byte authentication is not compatibility qualification.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
