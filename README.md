# AuroraSwitch

A small development firmware selector for Qu-Bit Aurora on Daisy Seed.
Choose a supported firmware, authenticate it from USB storage, then launch it
from SRAM without replacing the recovery bootloader or writing the selected
image into flash. A reset returns to the installed selector through the
existing bootloader.

**Development status:** bounded virtual switching and CPU safety checks pass.
The current cleanup build still requires real-module USB, launch, audio, panel
and recovery testing. Do not treat those software checks as physical approval.

## Build

Requirements: Git, GNU Make, Python 3, a host C/C++ compiler, and the GNU Arm
Embedded **10-2020-q4-major** toolchain. Put its `bin` directory first on PATH.
Dependency setup fetches pinned public source; it does not install packages,
download vendor firmware or access a device.

```sh
make dependencies -j2
make -j2
make test
```

Output: `firmware/build-experimental-dma/AuroraSwitch.bin`, with matching ELF
and MAP. The default is the actual USB/FatFs transport, SRAM launch and DMA
arena cleanup. There are deliberately no programming targets. See
[verification boundaries](docs/verification.md) before using the output.

After committing changes, `make package` checks the clean source tree and
ELF/BIN agreement, then seals an exact development ELF/BIN/MAP/manifest bundle
under `dist/<manifest-sha256>/`. Identical inputs reuse the same bundle;
modified existing bundles reject. This includes no vendor binaries, is not an
installer, and does not qualify hardware behavior.

## Supported images

Supply your own legally obtained files. No vendor firmware or manuals are
included. Exact accepted hashes, lengths, vectors and paths are listed in
[firmware/images.hpp](firmware/images.hpp).

- Original Aurora `Aurora_v1_4_4.bin` (181868 bytes).
- FDN `AR_FDN_v1_2_2.bin` (104196 bytes).

```sh
make verify-images FIRMWARE_DIR=/path/to/your/files
```

Files must match the exact supported bytes, not just their filenames. Other
Aurora builds, QSPI-linked images and oversized images are not supported.
Adding an image requires reviewing its memory/link/runtime contract and
extending the tests; changing a hash alone is insufficient.

## Controls

At startup or drive reconnect, the selector reads and authenticates the two
catalog filenames under `aurora/`. Only files with the exact supported size,
SHA-256 and vectors enter the menu. Unknown filenames are ignored; missing or
corrupt files are omitted. No selector rebuild is needed to add/remove these
supported files. Keep their catalog filenames; renamed copies are not discovered.
After changing the drive contents, safely eject and reconnect it to rescan.
Discovery briefly shows amber and does not itself authorize launch.

With the selector running:

1. Reverse cycles through available entries: FDN (blue) or original Aurora
   (green). With only one present it stays on that entry. No available image
   leaves Reverse dark and Freeze red; no media leaves Freeze dim blue.
2. Freeze reads and authenticates the selected file. Freeze status is dim blue
   without media, white when selectable, amber while scanning/loading, green when
   verified, or red on failure. Reverse presses and media disconnect invalidate
   the loaded selection. Freeze reloads and authenticates the file again, so
   replacing a file after discovery cannot bypass the launch checks.
3. Shift launches only a verified image; it rechecks the same staged bytes
   before irreversible teardown. It does not reopen an unchecked file.

Put the two files under `aurora/` on a compatible FAT USB drive. The selector
uses `0:/aurora/...` internally. Loading is read-only; failures do not launch.
After launch the buttons/knobs belong to the selected firmware. Reset returns
through the existing bootloader; there is no automatic in-application return
gesture in the supplied vendor images.

## Recovery and safety

Keep an independently known-working original Aurora firmware available. Use
Aurora's documented recovery/updater process to restore it, with exactly the
intended updater BIN at the USB root. Payload files for the selector belong in
the `aurora/` subdirectory, not alongside updater BINs at the root.

The firmware does not program internal flash, replace the bootloader, or write
QSPI. Installing the selector initially is a separate, state-changing operation
which must preserve a recovery backup. No installer or raw programming command
is supplied here. See [architecture](docs/architecture.md) and
[verification](docs/verification.md) for the inherited clock/MPU and DMA/cache
constraints and the required first physical test.
