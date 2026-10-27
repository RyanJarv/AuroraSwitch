# Selector packaging

`make package` builds tracked clean source with pinned SDK, libDaisy, mbedTLS and
GNU Arm 10-2020-q4-major. It rebuilds dependencies in an isolated temporary clone,
checks ELF/BIN agreement and source stability, and seals ELF/BIN/MAP plus manifest
in `dist/<manifest-sha256>/`. No new Git worktree is created.

The release workflow exports only `AuroraSwitch.bin`, provenance and SHA256SUMS.
The BIN contains our selector, catalog and RAM handoff routine. It contains no
third-party firmware, resident wrapper or private composite installer. Firmware
originals are obtained separately and kept unchanged in `aurora/` on USB.

Build provenance and virtual passes do not qualify physical installation,
power interruption, USB interoperability or stock restoration. Artifacts remain
`development_only: true`, `physical_qualified: false` until a separate supported
qualification decision. Nothing is published by local packaging.
