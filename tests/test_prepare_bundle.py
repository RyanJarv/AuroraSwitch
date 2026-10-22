"""Local bundle assembly: independent layout, exact identities and rejection."""

import base64
import copy
import hashlib
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from prepare_bundle import assemble
from selector_constants import CONSTANTS


def identity(data):
    return hashlib.sha256(data).hexdigest()


def fixture():
    c = CONSTANTS
    selector = struct.pack("<II", 0x20020000, 0x24000009) + bytes(56)
    dirt = struct.pack("<II", 0x20020000, 0x90040009) + bytes(range(56))
    ram = struct.pack("<II", 0x20020000, 0x24000009) + bytes(range(56))
    rows = []
    for name, data, resident in [("dirt", dirt, True), ("ram", ram, False)]:
        stack, reset = struct.unpack_from("<II", data)
        rows.append(
            dict(
                id=name,
                name=name,
                bytes=len(data),
                slot_offset=c["BUNDLE_DATA"] - c["QSPI_BASE"] + len(rows) * len(data),
                sha256=identity(data),
                stack=stack,
                reset=reset,
                resident=resident,
            )
        )
    t = dict(
        schema=2,
        constants=c,
        layout={k: c["RESIDENT_" + k] for k in ("BASE", "ENTRY", "METADATA", "IMAGE", "CAPACITY")},
        images=rows,
        selector=base64.b64encode(selector).decode(),
        selector_sha256=identity(selector),
        boot=base64.b64encode(b"BOOT").decode(),
        boot_sha256=identity(b"BOOT"),
    )
    return t, [dirt, ram]


class PrepareBundleTests(unittest.TestCase):
    def test_prepared_installer_preserves_reserved_regions_at_eof_boundaries(self):
        # Independent model of the pinned loader's inclusive 4-KiB eraser and
        # erase-before-read loop. This checks our file extents, not USB hardware
        # behavior or the correctness of an arbitrary replacement bootloader.
        for size in (0xCFFFF, 0xD0000, 0xD0001):
            with self.subTest(installer_bytes=size):
                template, files = fixture()
                row = template["images"][1]
                length = size - (row["slot_offset"] - 0x40000)
                files[1] = files[1][:8] + bytes(length - 8)
                row.update(bytes=length, sha256=identity(files[1]))
                installer = assemble(template, files)
                self.assertEqual(len(installer), size)
                # Non-erased sentinels make even an unintended erase visible.
                flash = bytearray(b"\x5a" * 0x800000)
                for file_offset in range(0, len(installer) + 1, 65536):
                    destination = 0x40000 + file_offset
                    erase_end = destination + 65536 + 4096
                    self.assertLessEqual(erase_end, 0x7E0000)
                    flash[destination:erase_end] = b"\xff" * (65536 + 4096)
                    block = installer[file_offset : file_offset + 65536]
                    flash[destination : destination + len(block)] = block
                self.assertEqual(flash[:0x40000], b"\x5a" * 0x40000)
                self.assertEqual(flash[0x7E0000:], b"\x5a" * 0x20000)
                self.assertEqual(flash[0x40000 : 0x40000 + size], installer)

    def test_updater_eof_erase_cannot_reach_selection_sectors(self):
        # Pinned stock updater reads/erases in 64-KiB chunks, erasing before
        # fread. An exactly aligned file therefore gets one extra EOF erase.
        maximum = CONSTANTS["BUNDLE_LIMIT"] - 0x90000000
        size = maximum - 0x40000
        # The linked old eraser includes its end sector, adding 4 KiB.
        last_erased_end = 0x40000 + (size // 65536 + 1) * 65536 + 4096
        self.assertLessEqual(last_erased_end, 0x7E0000)
        template, files = fixture()
        template["constants"] = dict(CONSTANTS, BUNDLE_LIMIT=0x907D0000)
        with self.assertRaisesRegex(ValueError, "layout bounds"):
            assemble(template, files)

    def test_fixed_slots_preserve_originals_and_clear_absent_images(self):
        template, files = fixture()
        full = assemble(template, files)
        partial = assemble(template, [files[0]])
        self.assertEqual(len(partial), len(full))
        base = CONSTANTS["QSPI_APPLICATION_OFFSET"]
        for index, data in enumerate(files):
            offset = template["images"][index]["slot_offset"] - base
            self.assertEqual(full[offset : offset + len(data)], data)
            if index:
                self.assertEqual(partial[offset : offset + len(data)], bytes([255]) * len(data))
        self.assertEqual(full, assemble(template, list(reversed(files))))
        self.assertEqual(struct.unpack_from("<I", full, 4)[0], CONSTANTS["RESIDENT_ENTRY"])
        self.assertEqual(full[8 : len(files[0])], files[0][8:])

    def test_unaligned_slots_and_omitted_middle_image(self):
        template, files = fixture()
        # Odd image lengths exercise padding, and a third image leaves an
        # optional hole between two supplied images rather than only at EOF.
        files = [files[0] + b"a", files[1] + b"bcd", files[1] + b"efghi"]
        middle = copy.deepcopy(template["images"][1])
        template["images"].append(middle)
        offsets = [0x101000, 0x101044, 0x101088]
        for row, data, offset in zip(template["images"], files, offsets):
            row.update(bytes=len(data), sha256=identity(data), slot_offset=offset)
        full = assemble(template, files)
        subset = assemble(template, [files[2], files[0]])
        self.assertEqual(len(full), len(subset))
        base = CONSTANTS["QSPI_APPLICATION_OFFSET"]
        for index in (0, 2):
            offset = offsets[index] - base
            self.assertEqual(subset[offset : offset + len(files[index])], files[index])
        hole = subset[offsets[1] - base : offsets[2] - base]
        self.assertEqual(hole, bytes([255]) * len(hole))
        self.assertEqual(subset[offsets[0] - base + len(files[0]) : offsets[1] - base], b"\xff" * 3)

    def test_reject_changed_unknown_duplicate_and_missing_dirt(self):
        t, files = fixture()
        for inputs in (
            [files[1]],
            [files[0], files[0]],
            [files[0][:-1]],
            [],
            [files[0], b"unknown"],
        ):
            with self.subTest(inputs=inputs), self.assertRaises(ValueError):
                assemble(t, inputs)
        for name in ("selector", "boot"):
            bad = copy.deepcopy(t)
            bad[name] = base64.b64encode(b"changed").decode()
            with self.assertRaisesRegex(ValueError, "damaged preparer"):
                assemble(bad, files)
        bad = copy.deepcopy(t)
        bad["constants"] = dict(CONSTANTS, BUNDLE_LIMIT=0x90101004)
        with self.assertRaisesRegex(ValueError, "exceeds"):
            assemble(bad, files)
