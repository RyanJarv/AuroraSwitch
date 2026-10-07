# Additional-image checkpoint

Work stays on `codex/qspi-dirt-verb`; `main` remains RAM-only.

## Active checklist

The checkpoint below is historical. FataMorgana QSPI was retired from the
catalog on 2026-10-07 in favor of the RAM build from the same pinned source.
No musical-feature difference was identified; physical equivalence is not
claimed. RAM avoids application-flash programming during launch. Preserve
the earlier QSPI evidence, but do not use it to qualify the changed selector.
The QSPI build helper remains available only to reproduce that experiment.

Current catalog: 12 RAM entries plus opt-in Dirt Verb and HP-filter Aurora
(14 total). Dirt remains the only QSPI payload. This catalog change requires
a new authenticated selector candidate and subsequent physical testing.

`make check` passes all 56 host tests and Python compilation after retirement.
The opt-in catalog test checks that the retired QSPI entry is rejected, RAM
Fata stays at index 11, and opt-in colors/paths are unique. HP-filter
moves from index 14 to 13; historical traces keep their original indices.
No live emulator campaign has been run against the changed selector yet.

The subsequent merge from `main` retains its fixed per-family colors (older
Flux/Morse versions now share their family color) and compact Pages directory.
Dirt Verb (red-pink) and HP-filter (lime) have development-only control pages;
Fata has only its RAM page. `make check` passes 63 tests after the merge.
Earlier frozen selector packages do not cover the merged firmware.

- [x] Fresh-build FataMorgana's pinned upstream `BOOT_QSPI` configuration.
- [x] Review HP-filter Aurora vectors, size, and staging-region capacity.
- [x] Add opt-in exact catalog entries and application-bounded writer extents.
- [x] Run host admission, bounds, fault and build-isolation tests (56 pass).
- [x] Seal a new selector and synthetic-media companion.
- [x] Execute linked handoff/control/audio screens; rerun affected Dirt checks.
- [x] Record results and stop before physical testing.

These entries pass bounded virtual screening, not physical compatibility or recovery. No new
firmware is distributed, copied to USB, or installed by this work.

## Exact payloads

| Image | Bytes | SHA-256 | Execution |
| --- | ---: | --- | --- |
| FataMorgana QSPI | 151420 | `d97311056ac1562b09e0afb518aeb3587d3df9bbb4a031d68da22be52bee8da1` | `0x90040000`, reset `0x90040a49` |
| HP-filter Aurora | 182212 | `94f4200efdf47cfb0c055fa51896da4d8c8d0f8b6a6d0a9bc9ec31553d85ebc7` | RAM, reset `0x2400070d` |

Fata comes from source `d5504d76370c370fdb40adcf755d8a4b9c07ee6b`, with
the same pinned SDK, dependencies, and GNU Arm 10 toolchain as the RAM build.
Its fresh-build manifest is
`c0a990a5d0458cf2e8bdd3823e0e7f33e97132429fb206029d4434684c29c64a`.
The USB filename is `FataMorgana-QSPI.bin`, avoiding collision with the exact
RAM experiment. Real USB/wavetable behavior still requires hardware testing.

Fata erases only application offsets `[0x40000, 0x65000)` (37 sectors).
Dirt retains `[0x40000, 0x58000)` (24 sectors). The writer derives the extent
from an actual authenticated catalog entry and checks rounded sector bounds;
it never accepts a selectable flash destination.

HP-filter is a supplied binary, without source provenance. Increasing the
opt-in catalog's staging capacity from 181888 to 182240 bytes accommodates it
within the existing `[0x30008000, 0x30040000)` region. No new memory bank or
streaming loader is needed. The default catalog's capacity remains unchanged.

No further versions are admitted automatically. Unknown filenames, changed
hashes, and cross-target vectors continue to fail closed. Physical flash,
handoff, USB, and ordinary stock restoration remain release requirements.

## Fail-first finding

The first Fata virtual launch (`fata-qspi-handoff-01`) performed zero erases,
zero page writes, and no terminal branch. The pointer-membership check rejected
a real entry because the header-local catalog has separate storage in each
translation unit. It was replaced with complete catalog-value comparison;
host tests require copied exact entries to pass and altered entries to fail.
This supersedes provisional selector source `50e54f8`; its bundle must not
be presented as a working launch candidate.

## Verified candidate

Frozen firmware source: `633ccd9fca00d8aafeac6b07c5362a8c8f1e4d79`.
Later preparation/Makefile/docs changes do not change its firmware bytes.

| Build | Manifest SHA-256 | BIN SHA-256 |
| --- | --- | --- |
| Virtual companion | `b1afa95917b5e145f24140f3e28aa00ad0a29152de69ff4ac91f4c03b0e94cee` | `db14b2046d55fdf2eacd87c6ee5598647a7c0a0aea1c73086693af6086f75232` |
| Real-USB candidate, not installed | `2afd97434f64ea70ba203c0ae5c4cbd8e0ffa63b85e2254affae72881db3c0cc` | `e74107e9562597f345f0ef57710cf0f3a9b9c7623d15f55a9b3be58c7c5994cf` |

Fata QSPI and HP-filter each pass 85 linked callback returns and 8192-frame
audio captures. A real ADC control change affects PCM; two changed runs agree
for each image. Fata's full 8-MiB flash census checks protected bytes and the
erased tail independently of the writer's own readback. Dirt's new-build
baseline/changed outputs match its previous frozen hashes. The RAM trampoline
and QSPI terminal trampoline are unchanged.

The Fata control edge at 0.150 s preceded audio startup and was rejected. The
verified 1.300-s edge requires both pre/post ADC states; no early rows are
discarded. Fast I2C remains the previously reviewed synthetic, opt-in cadence.

Checks: `make check` (56 tests, Python compilation), exact compiled payload
authentication, offline `prepare_payloads.py --qspi` for all three images,
94 applicable virtualizer tests, and independent replay of eight live records.
Full flash arrays stay ignored locally; evidence records contain hashes,
addresses, counters, and bounded results only.

## Next: physical testing

No drive or module was accessed. Keep ST-Link disconnected. Test the exact
candidate's launches with responsive panel/audio, reset/re-entry with ready
selector media, Fata USB/wavetable import, and ordinary stock USB restoration.
Missing/slow media and interrupted-write recovery remain open. Stop here until
the user is available; do not treat virtual results as a recovery guarantee.

### VM recovery-test gap (2026-10-07)

Skipped these end-to-end cases because the current VM does not execute the
installed updater's complete USB MSC/FatFs restore path:

- Interrupted programming, reboot, then stock USB restore.
- Damaged calibration/settings, then stock USB restore.
- Damaged bootloader, then stock USB restore.

Existing host fault tests check rejection, not reboot/recovery. Virtual NOR
censuses check write boundaries, not updater transport. Replacing file/flash
calls with successful host stubs would not close this gap, so no such results
are claimed. Calibration repair is not established by replacing application
bytes; normal USB recovery also depends on an intact, reachable updater.
