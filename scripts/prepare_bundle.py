"""Export an offline preparer containing only our prebuilt selector and wrapper.

Users supply their own exact vendor images in the browser. The resulting single
installer is private: it contains third-party firmware and is never a release asset.
"""

import base64
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from firmware_catalog import read_catalog
from sealed_bundle import authenticate_bundle, layout, checksum
from selector_constants import CONSTANTS

ROOT = Path(__file__).resolve().parents[1]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def make_template(bundle: Path) -> dict:
    manifest, selector, boot = authenticate_bundle(bundle)
    # Catalog/layout must belong to the sealed build, not another checkout.
    head = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(
        ["git", "-C", str(ROOT), "status", "--porcelain"], text=True
    ).strip()
    if head != manifest["source_commit"] or dirty:
        raise ValueError("export requires the clean sealed source checkout")
    if manifest["build_provenance"]["configuration"]["VIRTUAL_TRANSPORT"] != 0:
        raise ValueError("synthetic historical packages cannot become installers")
    return {
        "schema": 2,
        "source_commit": head,
        "manifest": bundle.name,
        "constants": CONSTANTS,
        "layout": layout(head),
        "images": read_catalog()["images"],
        "selector": base64.b64encode(selector).decode(),
        "selector_sha256": digest(selector),
        "boot": base64.b64encode(boot).decode(),
        "boot_sha256": digest(boot),
    }


def _admit_originals(images: list[dict], files: list[bytes]) -> tuple[dict[int, bytes], bytes]:
    """Identify exact vendor files; return their catalog indices and required Dirt."""
    resident = [i for i, row in enumerate(images) if row["resident"]]
    by_hash = {row["sha256"]: i for i, row in enumerate(images)}
    if len(resident) != 1 or len(by_hash) != len(images):
        raise ValueError("ambiguous catalog")
    provided = {}
    for data in files:
        index = by_hash.get(digest(data))
        if index is None or index in provided or len(data) != images[index]["bytes"]:
            raise ValueError("unsupported, changed or duplicate firmware")
        if struct.unpack_from("<II", data) != (images[index]["stack"], images[index]["reset"]):
            raise ValueError("firmware vectors differ")
        provided[index] = data
    if resident[0] not in provided:
        raise ValueError("the original Dirt Verb image is required")
    return provided, provided[resident[0]]


def _checked_bundle_end(template: dict, selector: bytes, boot: bytes, dirt: bytes) -> int:
    """Validate component ordering, all catalog slots and the updater erase margin."""
    constants, layout_data, images = template["constants"], template["layout"], template["images"]
    base, entry, metadata, image, capacity = (
        layout_data[k] for k in ("BASE", "ENTRY", "METADATA", "IMAGE", "CAPACITY")
    )
    data_start, limit = (constants[k] for k in ("BUNDLE_DATA", "BUNDLE_LIMIT"))
    wrapper = entry & ~1
    # Fixed-address Dirt must end before the wrapper and selector regions.
    if (
        base != constants["QSPI_APPLICATION_BASE"]
        or entry & 3 != 1
        or not base + len(dirt) <= wrapper < metadata < image
    ):
        raise ValueError("bundle layout bounds")
    # Check the prebuilt code and metadata fit their reserved regions.
    if (
        not 0 < len(boot) <= 256
        or wrapper + len(boot) > metadata
        or metadata + 16 > image
        or not 8 < len(selector) <= capacity
        or image + capacity > data_start
    ):
        raise ValueError("bundle layout bounds")
    # Keep all slots below the stock updater's extra erase range.
    if (
        not data_start < limit
        or limit + constants["UPDATER_BLOCK_SIZE"] + constants["CHOICE_SECTOR_SIZE"]
        > constants["QSPI_BASE"] + constants["CHOICE_OFFSET"]
    ):
        raise ValueError("bundle layout bounds")
    stack, reset = struct.unpack_from("<II", selector)
    if (
        stack != constants["DTCM_END"]
        or not reset & 1
        or not constants["AXI_SRAM_BASE"] + 8 <= reset < constants["AXI_SRAM_BASE"] + len(selector)
    ):
        raise ValueError("selector vectors")
    end = data_start
    for row in images:
        end = (end + 3) & ~3
        if row["slot_offset"] != end - constants["QSPI_BASE"]:
            raise ValueError("catalog slot differs")
        end += row["bytes"]
        if end > limit:
            raise ValueError("bundle exceeds reserved flash storage")
    return end


def assemble(template: dict, files: list[bytes]) -> bytes:
    """Reference implementation of the browser's bounded byte assembly; no tools."""
    if template["schema"] != 2:
        raise ValueError("unsupported preparer")
    constants, layout_data, images = template["constants"], template["layout"], template["images"]
    selector, boot = (
        base64.b64decode(template[name], validate=True) for name in ("selector", "boot")
    )
    if digest(selector) != template["selector_sha256"] or digest(boot) != template["boot_sha256"]:
        raise ValueError("damaged preparer")
    provided, dirt = _admit_originals(images, files)
    end = _checked_bundle_end(template, selector, boot, dirt)
    base, entry, metadata, image = (layout_data[k] for k in ("BASE", "ENTRY", "METADATA", "IMAGE"))
    wrapper = entry & ~1
    # Include absent slots too: a reinstall must clear stale optional images.
    output = bytearray(b"\xff" * (end - base))
    output[: len(dirt)] = dirt
    struct.pack_into("<I", output, 4, entry)
    output[wrapper - base : wrapper - base + len(boot)] = boot
    struct.pack_into(
        "<4I",
        output,
        metadata - base,
        constants["WRAPPER_MAGIC"],
        len(selector),
        len(selector) ^ constants["U32_MASK"],
        checksum(selector),
    )
    output[image - base : image - base + len(selector)] = selector
    for index, data in provided.items():
        offset = images[index]["slot_offset"] - (base - constants["QSPI_BASE"])
        output[offset : offset + len(data)] = data
    return bytes(output)


def export_preparer(bundle: Path, output: Path) -> None:
    template = make_template(bundle)
    page = (ROOT / "scripts/bundle_preparer.html").read_text()
    # Inline trusted release data so the page works offline, even from file://.
    page = page.replace(
        "/* RELEASE_TEMPLATE */ null",
        json.dumps(template, separators=(",", ":")).replace("<", "\\u003c"),
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x") as stream:
        stream.write(page)
    print("Offline preparer (contains no vendor firmware):", output)
