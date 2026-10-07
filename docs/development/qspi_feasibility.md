# QSPI switching investigation

2026-10-06. Offline review only; no firmware changes or hardware access.
**Conclusion:** a narrow exact-image QSPI mode is plausible without replacing
the bootloader. This initial review established feasibility, not qualification.
RAM switching stays unchanged.

Follow-up: an opt-in `QSPI_HANDOFF=1` implementation has been built from source
`254f7ff239b9e613fe293dcbe4bae5a88b84c76e`. Default builds remain RAM-only.
The initial investigation below is static evidence; the implementation's
virtual results are recorded separately at the end.

The later [additional-image checkpoint](qspi_additional_images.md) supersedes
the candidate below and adds exact QSPI FataMorgana and RAM HP-filter entries.
Its physical tests remain open.

## What the installed updater does

The previously captured loader matches the pinned Daisy v5.4 binary:
SHA-256 `b442456ab1c7f9689b73e57fb09ca55331d02760b54be28c10a207afed95f06e`.
The [reference source](https://github.com/daisyaudio/DaisyBootloader/blob/7171ab579f7a90c60def9da0017eb17e9ce6bd58/bootloader/src/bootloader.cpp)
helps interpret its linked instructions; source-to-binary reproducibility is
not established. Source SHA-256:
`7d4ce3ecd9a27aec45fd731010496940d7df77c131350de85c145d662363976b`.

- `SearchBin` searches the media root, skips directories/hidden files, and
  selects the first filename containing `.bin` or `.BIN`. Multiple root BINs
  are not a selection mechanism. Files under `aurora/` are not candidates.
- `EnsureValidBinary` compares file bytes with flash at `0x90040000`.
  Matching bytes skip programming; it does **not** rewrite on every boot.
  The comparison covers file length, not the unused application tail.
- Changed files are installed at that same fixed address. There is no
  selectable destination or retained multi-image dispatch interface.
- RAM-linked reset vectors select a 480-KiB copy to `0x24000000`, followed
  by QSPI deinitialization. QSPI-linked vectors select execution at
  `0x90040000` without the RAM copy.
- USB readiness/search is checked before the timeout branch. The linked
  timeout is 2,000 ms; slow/missing media can lead to the already-installed
  application instead of reinstalling the selector.
- File-read, erase, and write results are not checked by the update loop.
  Its success log is not a readback proof; no atomic update or rollback exists.

Linked landmarks in this exact binary: byte comparison `0x080061d6..0x0800625c`,
matching-file skip `0x080062da..0x080062e2`, erase/write calls `0x08006330` /
`0x0800635a`, USB search `0x08005ece`, timeout compare `0x08005f86`,
RAM copy/deinit `0x080059c8..0x080059f2`, QSPI selection `0x080059fa`.

## Memory boundaries

Ranges below are half-open. Existing calibration/settings locations are not
spare application storage and must not be touched by a selector writer.

| Region | Role |
| --- | --- |
| Internal flash `0x08000000..0x08020000` | Captured recovery-loader region; never program it |
| QSPI `0x90000000..0x90800000` | Captured 8-MiB external NOR |
| QSPI below `0x90040000` | Reserved/persistent space; includes calibration sector at offset `0x1000` and settings at `0x2000` |
| QSPI `0x90040000` | Fixed updater application destination; currently stores the selector |
| AXI SRAM `0x24000000` | Running selector; independent of its installed QSPI copy |
| D2 SRAM `0x30008000` | Existing authenticated payload staging buffer |

Dirt Verb 1.1 is 95,196 bytes, SHA-256
`e1775fb6c46dd83e33abaf599eb6d6089b7ff56692b42ac48748d2d1555d2784`,
stack `0x20020000`, reset `0x90040959`. Its reset address is inside its
expected application extent `[0x90040000, 0x900573dc)`.

For healthy reads of this file, the old updater writes that extent and erases
`[0x90040000, 0x90061000)`: two 64-KiB requests with inclusive 4-KiB-sector
ends. A hypothetical tight writer needs only `[0x90040000, 0x90058000)`.
These are derived footprints, not measured hardware writes. The current pinned
libDaisy eraser uses an exclusive end, unlike the old updater's linked eraser;
do not mix their contracts. Files exactly divisible by 64 KiB cause the old
loop to perform another erase before its terminating zero-length read.

## Options

1. **Ordinary updater installation:** place Dirt Verb as the sole root BIN,
   then boot. Reuses all existing update machinery, but replacing it with the
   selector later is a manual drive operation. Simplest way to use Dirt Verb;
   not panel switching.
2. **Rename/copy the chosen file to the root, then reset:** delegates flashing
   to the existing updater but changes persistent media selection. Dirt Verb
   cannot restore the selector filename for us. Without another mechanism,
   subsequent boots keep Dirt Verb selected. Adds media-write failure cases.
3. **Narrow selector QSPI mode:** keep the selector as the sole root BIN.
   Authenticate Dirt Verb into existing staging, initialize the QSPI handle,
   erase/program only its reviewed application sectors, read back exact bytes,
   and perform a QSPI-specific handoff. The next boot can reinstall the selector
   from USB. No bootloader modification or USB filesystem writes are needed.

Option 3 best preserves the panel-menu goal. Reuse pinned libDaisy rather than
add a flash driver. The selector already runs from RAM, so erasing its installed
copy does not inherently erase its executing instructions. However, its minimal
initializer currently does not initialize QSPI, and the loader's RAM branch
deinitializes it. QSPI setup/memory mapping must therefore be explicit.
Current handoff copies to AXI SRAM and sets a RAM VTOR; it cannot launch this
image unchanged. A separate small terminal path must preserve executable QSPI
mapping and select its vectors, while retaining reviewed cleanup invariants.

Calling internal bootloader routines by address is not a simpler safe API:
they require private object state and use `0x24000000` as their file buffer,
overwriting the selector's executing memory. Installing at another QSPI offset
does not relocate Dirt Verb's embedded addresses. No binary relocation proposed.

## Approved development experiment

- Add an exact-image, application-bounded writer using the existing SDK.
  Check errors and complete readback; the current SDK bulk `Write` ignores
  individual `WritePage` results, so its final OK alone is insufficient.
- Virtually exercise Dirt programming and QSPI launch using existing NOR and
  handoff facilities. Check protected bytes, mapping, DMA cleanup, and rejection
  of corrupt staging/readback. Do not add a USB stack to prove reinstall.
- Physically test selector → Dirt → boot-time selector reinstall, missing/slow
  media, and ordinary stock restoration. Keep ST-Link disconnected. Interrupted
  programming recovery must be established before claiming reliable recovery.

Reset without ready selector media may boot Dirt or an incomplete application.
Reinstallation is conditional, not a guarantee. Opaque payload code has normal
hardware access; a bounded selector writer cannot establish that Dirt never
writes other persistent state. Reliability and recovery remain release gates.

## Checks performed

- Rehashed the pinned loader and supplied Dirt BIN; decoded and checked vectors.
- Fetched the pinned reference through `gh api`; checked its SHA-256.
- Reinspected the linked comparison, skip, USB/timeout, copy, and update ranges
  with existing GNU Arm 10 `objdump -D -b binary -m arm -M force-thumb
  --adjust-vma=0x08000000`.
- Asserted exact file identity, reset bounds, healthy-read erase footprint, and
  sector rounding with host Python. All passed.
- `make check`: 38 host tests and Python compilation passed; `git diff --check`
  passed. No emulator or physical campaign was run: this is static feasibility
  evidence, not new firmware execution evidence.

## Implementation checkpoint

The opt-in writer uses pinned libDaisy and checks each erase/page operation,
complete mapped readback, and launch-time staged bytes. Its fixed erase extent
is `[0x90040000, 0x90058000)`. After any erase attempt, failure stops rather than
returning to a menu whose installed application may have changed. A separate
88-byte terminal trampoline preserves QSPI mapping, clears the DMA arena, and
branches to Dirt's reset vector. The existing RAM trampoline is unchanged.

Fresh isolated build identities:

| Build | Manifest SHA-256 | BIN SHA-256 |
| --- | --- | --- |
| Synthetic-media companion | `2b00a6e3b32221a3109bacc1d4a81aa056622f5f41d7c79240c8c56631484d07` | `efa6aecfa4f2df23e851522376f1ee25a2708f7825314d4a5495dff0e1c29ca1` |
| Real-USB candidate, not installed | `ab5b8d4935b6f58eff3418c70a9b730970e42a031e79d401bb5483d80b47df82` | `ab0ad26dbf00b90fe747a69e8c1775b2e2c433d1eb352cce5758d9853867046a` |

`make check` passes 43 host tests and Python compilation. Hosted host-test CI
passes for source `254f7ff`. Default builds still exclude Dirt; no payload
binaries are distributed by this project.

### Virtual results

The existing development NOR/handoff lane executes the linked writer, then
stops at Dirt's genuine reset entry. An independent full 8-MiB comparison checks
the programmed payload, erased sector tail, and every byte outside the erase
extent. The linked mapping request, terminal branch, vectors, and all 32 KiB of
cleared DMA memory pass. Continuing that same processor reaches 85 finite audio
callbacks and captures 8,192 frames. A linked ADC control change changes PCM;
two changed-control runs produce identical PCM.
This short impulse capture has only two nonzero PCM words: it is a bounded
control/output check, not proof of Dirt's complete reverb tail or button modes.

The older synthetic I2C cadence stalls Dirt's first callback. Direct Dirt
startup stalls there too, so this is not evidence of a selector-only failure.
The opt-in virtual lane now uses an 11-us byte cadence only for the two pinned
SDK fast-I2C timing words; Dirt actually writes `0x1080091a`. Unknown words
fail closed. This is a timing approximation, not an electrical clock model.
Default lanes retain their old cadence; no guest IRQ priorities or completion
flags are patched.

New live RAM-image screens for official Aurora and FDN pass with the companion.
Earlier complete switching-cycle evidence is retained, not relabeled as a new
execution against this candidate. There are 93 passing applicable VCV tests;
two historical export tests fail because they pin the former standalone commit
and file count. Those stale assumptions remain documented, not silently skipped
as successful checks. Full payload-bearing NOR records stay local and ignored.

### Physical boundary

The real-USB candidate has not been copied to a drive or installed. Before
release, test selector → Dirt with responsive controls/audio, reset with ready
selector media and successful selector reinstall, and ordinary stock USB
restoration. Missing/slow media and interrupted-programming recovery remain
open. Keep ST-Link disconnected because it previously caused resets.

Virtual programming and handoff checks do not establish physical QSPI timing,
controller/cache behavior, USB enumeration, payload safety, or recovery. Stop
here until the user can participate in the exact-build physical campaign.
