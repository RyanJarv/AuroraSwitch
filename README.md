# AuroraSwitch

A small development firmware selector for Qu-Bit Aurora on Daisy Seed.
Choose a supported firmware, authenticate it from USB storage, then launch it
from SRAM without replacing the recovery bootloader or writing the selected
image into flash. A reset returns to the installed selector through the
existing bootloader.

**Development status:** official Aurora, FDN, EchoGarden and The Oscillator Is
a Lie pass bounded virtual launch/control/audio checks. Cloudscape launches,
but a control-to-audio response is still unproven. All images in the new catalog
still need physical testing and ordinary stock recovery remains unproven.
See [the current checkpoint](docs/community_virtual_checkpoint.md).

AuroraSwitch is beta software with limited testing. Switching restarts the
selected firmware. Keep the original Aurora firmware available for restoration
using the normal USB updater.

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

After committing changes, `make package` checks the clean source tree, rebuilds
the application **and libDaisy** in isolated tracked-source checkouts using the
required toolchain, checks ELF/BIN agreement, then seals a development bundle
under `dist/<manifest-sha256>/`. Identical inputs reuse the same bundle;
modified existing bundles reject. This includes no vendor binaries, is not an
installer, and does not qualify hardware behavior.
The manifest records compiler/tool hashes and fixed build configuration.
Existing local objects/archives are never packaging inputs. Full ELF/MAP
reproducibility across temporary paths is not yet guaranteed; reuse requires
identical artifact and manifest bytes, not merely the same source commit.

## Image catalog (experimental)

Supply your own legally obtained files. No vendor firmware or manuals are
included. Exact accepted hashes, lengths, vectors and paths are listed in
[firmware/images.hpp](firmware/images.hpp).

- Original Aurora `Aurora_v1_4_4.bin` (181868 bytes).
- FDN `AR_FDN_v1_2_2.bin` (104196 bytes).
- EchoGarden `AuroraEchoGarden_v0_3_1_STABLE.bin` (94704 bytes).
- CloudscapeX `AuroraCloudscapeX.bin` (98968 bytes).
- The Oscillator Is a Lie `TheOscillatorIsALie_v0_0_2.bin` (103604 bytes).

Catalog membership identifies exact accepted bytes; it is not a qualification
claim. Cloudscape's outstanding control/audio check is documented above.

```sh
make verify-images FIRMWARE_DIR=/path/to/your/files
```

This command checks the complete catalog. To prepare only the images you own,
use the subset-friendly helper below.

Files must match the exact supported bytes, not just their filenames. Other
Aurora builds, QSPI-linked images and oversized images are not supported.
Adding an image requires reviewing its memory/link/runtime contract and
extending the tests; changing a hash alone is insufficient.

### Prepare user-supplied firmware

The repository and selector packages contain no third-party firmware. Download
supported versions from their authors, keep them locally (for example in ignored
`local-firmware/`), then prepare a new folder:

```sh
make setup
python3 scripts/prepare_payloads.py --output prepared-payloads \
  local-firmware/AR_FDN_v1_2_2.bin local-firmware/Aurora_v1_4_4.bin
```

Any supported image may be omitted. The helper authenticates private copies using the
selector's own catalog and verification predicate, restores the required
filenames and writes `prepared-payloads/aurora/` plus `payloads.json`. It refuses
unknown/changed bytes and existing output directories. Use a new output folder
for another run. Copy the verified `aurora/` folder onto the USB yourself; this
command does not install a selector, access hardware or overwrite recovery files.
The receipt proves bytes, not working controls/audio or physical recovery.

Community-image onboarding is tracked in
[alternative firmware](docs/alternative_firmware.md). The three supplied SRAM
images have exact catalog entries; see that document for their testing status.
Older versions will be admitted as
separate exact images after review; never replace a supported hash with “latest.”
Optional fetching from pinned public releases is a follow-up, not required for
the offline/manual workflow. Discord-only files remain user-supplied.

## Controls

At startup or drive reconnect, the selector reads and authenticates the five
catalog filenames under `aurora/`. Only files with the exact supported size,
SHA-256 and vectors enter the menu. Unknown filenames are ignored; missing or
corrupt files are omitted. No selector rebuild is needed to add/remove these
supported files. Keep their catalog filenames; renamed copies are not discovered.
After changing the drive contents, safely eject and reconnect it to rescan.
Discovery briefly shows amber and does not itself authorize launch.
USB/filesystem initialization failure stays red until reset and disables
loading/launch; reconnecting the drive does not clear an initialization fault.

With the selector running:

1. Reverse cycles through available entries: FDN (blue), original Aurora
   (green), EchoGarden (cyan), CloudscapeX (magenta), or The Oscillator Is a Lie
   (amber). With only one present it stays on that entry. No available image
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

**Hard release gate — currently unproven:** the worst permitted outcome of
supported selector installation/use is a malfunction recoverable by Aurora's
normal USB updater using the known-good original firmware. Recovery may require
the documented updater-entry controls, reset or power cycling, but must not
require ST-Link, ROM DFU, opening the module, recalibration or restoring internal
backups. Any violation blocks release; an experimental label does not waive it.
This is a release requirement, **not an established “always recoverable” claim**.
See [the recovery release gate](docs/recovery_release_gate.md).

Keep an independently known-working original Aurora firmware available. Use
Aurora's documented recovery/updater process to restore it, with exactly the
intended updater BIN at the USB root. Payload files for the selector belong in
the `aurora/` subdirectory, not alongside updater BINs at the root.

The selector's loading/handoff code does not program internal flash, replace
the bootloader, or write QSPI. **This is not a sandbox or a guarantee about the
launched firmware:** it has normal hardware access and may write settings,
calibration, USB files or flash according to its own behavior. Hashes identify
expected bytes; they are not vendor signatures or a firmware-safety audit.
Installing the selector initially is a separate, state-changing operation
which must preserve a recovery backup. No installer or raw programming command
is supplied here. See [architecture](docs/architecture.md) and
[verification](docs/verification.md) for the inherited clock/MPU and DMA/cache
constraints and the required first physical test.

For a source-guided walkthrough and risk explanation, read
[how it works](docs/how_it_works.md). The
[validation and boot-equivalence plan](docs/boot_equivalence_plan.md) preserves
historical comparison research, not a whole-state completion requirement.
The [reliability campaign](docs/reliability_campaign.md#active-checklist) is the
single active checklist: reliable official-image handoff and ordinary stock
recovery first; alternatives only when small and non-delaying.
The [load-timing recipe](docs/load_timing.md) describes diagnostic measurements;
no load timeout or watchdog is currently imposed.
