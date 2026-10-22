# Development

Build the USB-backed RAM selector in the base checkout. Development artifacts
are not physically qualified releases.

```sh
make setup
make check
make test-sanitize
make build
make package
```

Use GNU Arm 10-2020-q4-major on PATH. `make package` fresh-builds clean committed
source and pinned dependencies, seals only our ELF/BIN/MAP and manifest under
`dist/<manifest-sha256>/`, and checks source drift. It creates no Git worktree
and performs no installation. Generated reports stay ignored.

See [architecture](architecture.md), [tests](testing.md), [release packaging](releases.md),
[USB choice](persistent_boot.md), [failure priorities](failure_modes.md) and the
[physical gate](recovery_release_gate.md).

## Add firmware

Add exact identity, RAM vectors, filename, family/category/color and source link
in `firmware/images.hpp`; document controls in `docs/firmware_reference.md`.
Host tools and menu derive from this catalog. Do not add per-image loading modes.
Reject flash-running images. Inspect unknown files with
`python3 scripts/inspect_firmware.py --execution sram /path/to/file.bin`; inspection
measures identity but grants no support.

Review linked memory bounds, startup, persistent storage and preservation of the
selector's application area. Keep originals unchanged and obtain them separately
from authors. FataMorgana's reviewed RAM build is an explicit source-build image;
its USB tables remain separate. Run affected authentication/handoff checks and
regenerate the [reference site](reference_site.md).
