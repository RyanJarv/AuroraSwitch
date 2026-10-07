# QSPI branch review — 2026-10-07

Reviewed baseline: `c8b4477`, branch `codex/qspi-dirt-verb`.
Scope: selector, handoff, writer bounds, build/package/payload tools, tests,
reference site, and active docs. No hardware or sibling-project changes.

## Fixed

- `make usb QSPI_HANDOFF=1` built in the QSPI directory but copied the default
  RAM BIN. A dry run confirmed the mismatch. It now rejects before building or
  copying; use the sealed opt-in package and manual copy instructions.
- The Fata build probe accepted images of 8–15 bytes; the selector requires
  at least 16. Both build modes now use that minimum. This did not bypass the
  selector's exact-image authentication.
- Docs now distinguish release/source catalogs and RAM/QSPI launch behavior.
  Removed retired-branch guidance and duplicate unsupported-image wording;
  marked frozen checkpoints as historical. QSPI programming, conditional
  menu re-entry, memory ranges, and fatal-after-erase behavior are explicit.

Both tooling defects had failing regression checks before the fixes. Firmware,
catalog identities, trampoline, staged revalidation, and cleanup are unchanged.
No new candidate or emulator campaign is needed for these host/docs changes.

## Checks

- `python3 -m unittest tests.test_prepare_usb tests.test_build_fatamorgana -v`
- `make check`: 65 tests and Python compilation pass.
- `git diff --check`: pass.
- Relative Markdown links: pass.

## Still open

Current firmware predates this review but changed during catalog retirement
and the family-color merge. Earlier virtual evidence does not qualify those
bytes. Seal and test that candidate before physical use. Physical QSPI handoff,
real USB behavior, interrupted-programming recovery, and ordinary stock restore
remain unverified. Do not add cleanup or emulator features without a concrete
failure; keep the next campaign bounded.
