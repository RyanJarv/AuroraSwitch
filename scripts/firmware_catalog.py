"""Read the compiled catalog without SDK/network dependencies or C++ text parsing."""

from functools import lru_cache
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def validate_catalog(data: dict) -> dict:
    """Reject ambiguous identities and inconsistent family metadata before use."""
    if data.get("schema") != 1 or not data.get("images"):
        raise ValueError("unsupported or empty firmware catalog")
    colors = {row["name"]: row for row in data["colors"]}
    categories = {row["name"]: row["color"] for row in data["categories"]}
    if (
        len(colors) != len(data["colors"])
        or len(categories) != len(data["categories"])
        or any(color not in colors for color in categories.values())
    ):
        raise ValueError("ambiguous catalog colors/categories")
    for color in colors.values():
        if (
            not re.fullmatch(r"#[0-9a-fA-F]{3}(?:[0-9a-fA-F]{3})?", color["css"])
            or len(color["rgb"]) != 3
            or not all(0 <= channel <= 1 for channel in color["rgb"])
            or sum(color["rgb"]) <= 0
        ):
            raise ValueError("invalid catalog color")
    seen = {key: set() for key in ("id", "filename", "sha256", "name")}
    families = {}
    references = {}
    category_colors = {}
    for row in data["images"]:
        for key, values in seen.items():
            if row[key] in values:
                raise ValueError("duplicate firmware " + key)
            values.add(row[key])
        if (
            any(
                not isinstance(value, str) or not value or any(ord(c) < 32 for c in value)
                for key, value in row.items()
                if key not in ("bytes", "stack", "reset", "resident", "download", "rgb")
            )
            or not re.fullmatch(r"[a-z0-9][a-z0-9.-]*", row["id"])
            or not re.fullmatch(r"[a-z0-9][a-z0-9-]*", row["family"])
            or not re.fullmatch(r"[a-z0-9-]+", row["reference"])
            or any(c in row["filename"] for c in ("/", "\\"))
            or not row["filename"].endswith(".bin")
            or not re.fullmatch(r"[0-9a-f]{64}", row["sha256"])
            or not 16 <= row["bytes"] <= 480 * 1024
            or row["execution"] not in ("sram", "qspi")
            or row["availability"] not in ("Release", "Development")
            or row["category"] not in categories
            or row["color"] not in colors
            or row["rgb"] != colors[row["color"]]["rgb"]
            or type(row["resident"]) is not bool
            or (row["download"] is not None and not row["download"].startswith("https://"))
        ):
            raise ValueError("invalid firmware metadata")
        family = tuple(row[key] for key in ("family_name", "reference", "category", "color"))
        if row["family"] in families and families[row["family"]] != family:
            raise ValueError("inconsistent firmware family")
        families[row["family"]] = family
        for mapping, key in (
            (references, row["reference"]),
            (category_colors, (row["category"], row["color"])),
        ):
            if key in mapping and mapping[key] != row["family"]:
                raise ValueError("duplicate family reference or category color")
            mapping[key] = row["family"]
    resident = [row for row in data["images"] if row["resident"]]
    if len(resident) != 1 or resident[0]["execution"] != "qspi":
        raise ValueError("exactly one reviewed QSPI resident image is required")
    return data


@lru_cache(maxsize=1)
def read_catalog() -> dict:
    # One invocation per command; no generated catalog cache can become stale.
    with tempfile.TemporaryDirectory(prefix="aurora-catalog-") as directory:
        executable = Path(directory) / "catalog"
        subprocess.run(
            [
                os.environ.get("CXX", "c++"),
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                str(ROOT / "firmware/catalog_dump.cpp"),
                "-o",
                str(executable),
            ],
            check=True,
        )
        return validate_catalog(json.loads(subprocess.check_output([str(executable)], text=True)))


def resident_image() -> dict:
    return next(row for row in read_catalog()["images"] if row["resident"])
