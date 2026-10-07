# Development guide

Read [how it works](how_it_works.md) and the [source map](architecture.md) first.
Reuse the pinned SDK, libDaisy, FatFs, and hash implementation. Keep the loader
small; do not add a second emulator or general firmware manager.

## Commands

```sh
make -j2 build       # fetch dependencies and build USB firmware
make check           # host tests and Python syntax checks
make package         # isolated USB build from clean committed source
make build-virtual   # synthetic-media build; never install
make package-virtual # isolated synthetic-media package
```

The [user guide](user_guide.md) lists prerequisites and outputs.
Packaging records source and tool identities and checks ELF/BIN agreement.
It does not prove behavior or byte-identical ELF/MAP across build paths.

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

## Tag builds and releases

Pushing a tag tests and builds USB firmware with the pinned Arm toolchain.
The workflow creates a **draft prerelease** with
`AuroraSwitch.bin`, ELF, MAP, `manifest.json`, and `SHA256SUMS`. Third-party
payloads are excluded. Existing releases are not overwritten.

```sh
git tag -a v0.1.0-beta.1 -m "AuroraSwitch beta test build"
git push origin v0.1.0-beta.1
```

Keep drafts unpublished until the [recovery checks](recovery_release_gate.md)
pass. A successful build is not a hardware test. Do not move a release tag.

To test the workflow without a tag or release:

```sh
gh workflow run firmware-release.yml --ref main
```

Download that run's firmware artifact from GitHub Actions. Extract it and run
`sha256sum --check SHA256SUMS` in the bundle directory. Only the BIN is installed
on Aurora; keep the other files for verification and reports.
