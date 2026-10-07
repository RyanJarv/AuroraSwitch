# Additional-image checkpoint

Work stays on `codex/qspi-dirt-verb`; `main` remains RAM-only.

## Active checklist

- [x] Fresh-build FataMorgana's pinned upstream `BOOT_QSPI` configuration.
- [x] Review HP-filter Aurora vectors, size, and staging-region capacity.
- [x] Add opt-in exact catalog entries and application-bounded writer extents.
- [x] Run host admission, bounds, fault and build-isolation tests (54 pass).
- [ ] Seal a new selector and synthetic-media companion.
- [ ] Execute linked handoff/control/audio screens; rerun affected Dirt checks.
- [ ] Record results and stop before physical testing.

These entries are provisional, not compatibility or recovery claims. No new
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
