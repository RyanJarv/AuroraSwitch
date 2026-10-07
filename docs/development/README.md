# Development

Keep the loader small. Reuse the pinned SDK, libDaisy, FatFs, and SHA-256 code.
Comment non-obvious files, types, and functions briefly: purpose, key invariants,
and their role in the loader. Skip obvious names; explain why, not each statement.

## Build and test

Requires Git, GNU Make, Python 3, C/C++ compilers and GNU Arm Embedded
**10-2020-q4-major** on PATH. Published releases need none of these tools.

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git && cd AuroraSwitch
make -j2 build
make check
make package
```

See `make help` for virtual build targets. Never install virtual firmware.
Packaging requires clean committed source; it records inputs and checks ELF/BIN
agreement. It does not prove runtime behavior.

From a checkout, `make download-release USB_DIR="/Volumes/AURORA"` automates
checked release/public-firmware downloads and copying; it needs `gh` and host
compilers, but no Arm compiler. `scripts/prepare_payloads.py --output prepared
local-firmware/AR_FDN_v1_2_2.bin` optionally verifies a user-supplied file before
copying. Use a new output folder each time.

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

- [Next firmware batch](firmware_onboarding.md): older versions, Tempest, and
  the FataMorgana blocker. Intake is not supported-image evidence.
- [Architecture](architecture.md): source map, memory, and handoff.
- [Builds and releases](releases.md): tag workflow and build-only CI.
- [Verification status](verification.md) and [active checklist](reliability_campaign.md#active-checklist).
- [Recovery checks](recovery_release_gate.md) and [optional timing diagnostics](load_timing.md).
- [QSPI feasibility](qspi_feasibility.md): Dirt Verb investigation; no implemented support.
- [Current checkpoint](checkpoints/public_releases_20261006.md) and [machine-readable evidence](evidence/public_releases_20261006.json).

Historical records stay in `history/`. Do not transfer their results to changed
binaries. Documentation-only edits need link checks, not new emulator campaigns.
