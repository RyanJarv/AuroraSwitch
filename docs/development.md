# Development guide

Read [how it works](how_it_works.md) and the [source map](architecture.md) first.
Reuse the pinned SDK, libDaisy, FatFs, and hash implementation. Keep the loader
small; do not add a second emulator or general firmware manager.

## Commands

```sh
make dependencies -j2   # fetch pinned sources and build libDaisy
make -j2               # incremental USB build
make test              # portable host tests; no device access
make package           # clean committed source; isolated authenticated build
```

The [user guide](user_guide.md) lists prerequisites and outputs. Packaging records
source/tool/dependency identities and verifies ELF/BIN agreement. It does not
prove behavior or fully reproducible ELF/MAP bytes across build paths.

## Add firmware support

1. Pin the exact author file/version and record its size, SHA-256, and vectors.
   Review SRAM placement, startup, staging limits, inherited clocks/MPU, DMA,
   and persistent writes. QSPI-linked images are outside the current contract.
2. Add a reviewed entry to [images.hpp](../firmware/images.hpp), including the
   required path and a distinct menu color. Older versions are separate entries;
   do not replace existing identities or increase staging without a review.
3. Extend affected catalog/authentication tests. Run `make test` and authenticate
   local files with `make verify-images FIRMWARE_DIR=/path/to/all/catalog/files`.
   Use `prepare_payloads.py` for a subset. Do not commit payload BINs or manuals.
4. Commit source, package a fresh build, and run affected linked launch/control/
   audio and copy/DMA regressions. Use the image's own linked addresses. Relevant
   handoff changes also need complete switching-cycle tests.
5. Record exact artifact identities, results, and limits. Then test physical
   controls/audio, reset/re-entry, and ordinary stock recovery before release.

Host tests alone do not establish runtime compatibility. Virtual campaign results
are recorded externally; this repository does not yet provide a standalone
campaign runner. Stop onboarding if it needs substantial new modeling or delays
official-image reliability testing.

Documentation-only changes need link/diff checks, not new emulator campaigns.
Never transfer old evidence to a changed BIN. Use the
[reliability checklist](reliability_campaign.md#active-checklist) as the active plan.
