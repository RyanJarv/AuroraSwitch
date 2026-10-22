"""Measure a local binary for a proposed catalog entry; never authorize or install it."""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from firmware_catalog import read_catalog
from selector_constants import CONSTANTS


def inspect(path: Path, execution: str) -> dict:
    if execution != "sram":
        raise ValueError("only RAM execution is supported")
    capacity = CONSTANTS["STAGING_END"] - CONSTANTS["STAGING_BASE"]
    with path.open("rb") as stream:
        data = stream.read(capacity + 1)
    if not 16 <= len(data) <= capacity:
        raise ValueError("image is too short or exceeds staging SRAM")
    stack, reset = struct.unpack_from("<II", data)
    base = CONSTANTS["AXI_SRAM_BASE"]
    entry = reset & ~1
    if (
        not CONSTANTS["DTCM_BASE"] < stack <= CONSTANTS["DTCM_END"]
        or stack & 7
        or not reset & 1
        or entry < base
        or entry + 2 > base + len(data)
    ):
        raise ValueError("image vectors do not match the chosen execution type")
    digest = hashlib.sha256(data).hexdigest()
    return {
        "filename": path.name,
        "bytes": len(data),
        "sha256": digest,
        "stack": stack,
        "reset": reset,
        "execution": execution,
        "catalog_matches": [
            row["id"]
            for row in read_catalog()["images"]
            if (row["bytes"], row["sha256"], row["stack"], row["reset"], row["execution"])
            == (len(data), digest, stack, reset, execution)
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execution", required=True, choices=("sram",))
    parser.add_argument("file", type=Path)
    args = parser.parse_args()
    print(json.dumps(inspect(args.file, args.execution), indent=2))
