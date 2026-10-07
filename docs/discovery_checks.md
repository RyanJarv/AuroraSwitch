# Supported-file discovery checks — 2026-10-06

Historical two-image checkpoint. Counts, status, and TODOs below belong to that
build, not the current catalog. See [verification status](verification.md) and
the [active checklist](reliability_campaign.md#active-checklist).

Scope: discover the existing two reviewed images on USB connection, not admit
new firmware or change the terminal handoff. The small menu helper is generic
over a bounded catalog. The firmware invokes its existing read-only staging,
Mbed TLS hash and vector authentication for each known filename. Only exact
matches enter the menu. This avoids a new filesystem enumerator, manifest
parser, payload format or alternate loader.

Discovery never authorizes launch. Freeze rereads/authenticates the selection;
Shift retains the existing same-staged-bytes recheck. Reverse (even with one
entry), disconnect and rediscovery invalidate any Verified state. Missing or
invalid files are omitted, unknown filenames ignored. Zero recognized entries
show red Freeze/dark Reverse; disconnected media show dim-blue Freeze.
Reconnect after editing the drive. Discovery temporarily uses staging and
adds full-file reads/hashes on connection; it is bounded by the compiled catalog
and the existing staging capacity. It is not a background hot-filesystem watch.

## Commands and results

From the repository root:

```sh
make test
make verify-images FIRMWARE_DIR=/home/me
make -j2 GCC_PATH=/home/me/opt/arm/gcc-arm-none-eabi-10-2020-q4-major/bin
git diff --check
```

- Eight host tests pass, including native menu empty/one/multiple-entry,
  skip/wrap/removal/preserved-selection and mid/final-probe disconnect checks.
  The menu tests use synthetic authentication results, not actual USB files.
- Both user-provided vendor files authenticate with the existing native
  Mbed TLS/vector verifier. Existing staging failures and package admission
  controls pass. Programming targets still reject.
- Actual USB-transport ARM build passes. Moving the first fresh output aside
  and rebuilding at the same path reproduces ELF, BIN and MAP byte-for-byte
  (`cmp` for all three). Existing pinned dependency binaries are reused;
  this is not a fresh rebuild of the dependencies.
- BIN is 94392 bytes. Staging remains 181888 bytes at `0x30008000`;
  DMA objects use 3728 bytes of the 32-KiB arena. SRAM4 reservation remains 1 KiB.
- `git diff --check` passes. No device access or USB-drive update was performed.

Exact fresh-build SHA-256 identities:

| Artifact | SHA-256 |
| --- | --- |
| BIN | `a8d4663c5f0cd2b484006d3b026a09c820c2ef8f4f5693542be5a2a803190cef` |
| ELF | `fd5dba3314e66b53b4c5eb06243d1270d8cdaf895296234d67551054bd314d55` |
| MAP | `bf88bf5657c98b18974580913498b386178af26a1b0c69519ab8447b04b4ec52` |

## Limits and follow-up

No actual USB/MSC/FatFs execution, linked discovery lifecycle, vendor launch,
real reset or hardware pass is claimed for this new BIN. The earlier selector's
virtual cycle evidence remains unchanged and image-specific. The synthetic
single-file backed reader can exercise one discovered image but does not prove
real multi-file media behavior. Current selection colors cover only FDN and
original Aurora; a larger reviewed catalog needs a distinguishable UI.

TODO: test this exact package with empty media, each single supported file,
both files, corruption, wrong length, renamed and unknown files, disconnect
during discovery/load, and reconnect; confirm Shift cannot launch solely after
discovery or a failed reload. Then repeat both launches and A→B→A reset cycles.
These are physical/linked integration obligations, not satisfied by the host
menu tests or by the older image's receipts.
