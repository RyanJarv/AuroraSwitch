# Development

Keep the loader small. Reuse the pinned SDK, libDaisy, FatFs, and SHA-256 code.
Comment non-obvious files, types, and functions briefly: purpose, key invariants,
and their role in the loader. Skip obvious names; explain why, not each statement.

## Build and test

```sh
make -j2 build
make check
make package
```

See `make help` for virtual build targets. Never install virtual firmware.
Packaging requires clean committed source; it records inputs and checks ELF/BIN
agreement. It does not prove runtime behavior.

## Add firmware

1. Pin the file/version, size, hash, and vectors. Review RAM layout, startup,
   DMA use, inherited clocks/MPU, and persistent writes.
2. Add its path and menu color to [images.hpp](../../firmware/images.hpp).
   Reuse its family's `menu_colors` constant for every version; new families
   need a distinct color. Keep existing identities and review staging changes.
3. Extend catalog/authentication tests. Run `make check` and validate local
   payloads with `prepare_payloads.py` or `make verify-images`.
4. Commit and package. Test linked launch, controls/audio, copy, and DMA cleanup.
   Handoff changes also need complete switching-cycle tests.
5. Record exact builds and limits, then test physical launch and stock recovery.

Do not commit payloads. Host tests alone do not prove compatibility.
Do not create a new emulator for a difficult image; defer it.

## Working notes

- [Firmware onboarding record](firmware_onboarding.md): older versions, Tempest,
  and the FataMorgana RAM experiment. Intake is not supported-image evidence.
- [Architecture](architecture.md): source map, memory, and handoff.
- [QSPI branch review](branch_review_20261007.md): tooling fixes and remaining test boundaries.
- [Builds and releases](releases.md): tag workflow and build-only CI.
- [Verification status](verification.md) and [active checklist](reliability_campaign.md#active-checklist).
- [Recovery checks](recovery_release_gate.md) and [optional timing diagnostics](load_timing.md).
- [QSPI feasibility](qspi_feasibility.md): updater boundaries and Dirt Verb implementation.
- [Additional-image checkpoint](qspi_additional_images.md): current opt-in
  Dirt/HP-filter scope and historical QSPI Fata evidence.
- [Seven-image checkpoint](checkpoints/public_releases_20261006.md) and [machine-readable evidence](evidence/public_releases_20261006.json) (history).

Historical records stay in `history/`. Do not transfer their results to changed
binaries. Documentation-only edits need link checks, not new emulator campaigns.
