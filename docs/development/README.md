# Development

Keep this a small exact-image selector. Build tools are needed by developers;
the distributable output is an offline browser preparer. It has not yet been
published for this development workflow.

```sh
make check
make test-sanitize
make build
make package
make preparer
```

Use GNU Arm 10-2020-q4-major. `make package` builds clean committed sources and
pinned dependencies in isolation, checks ELF/BIN identity and seals a development
bundle. `make preparer` additionally exports a standalone HTML file. Neither
command downloads vendor firmware, touches a USB drive or programs hardware.

[Architecture](../how_it_works.md), [saved choices](persistent_boot.md),
[testing](testing.md), [release workflow](releases.md) and the
[physical release gate](recovery_release_gate.md) describe the relevant boundaries.

[Failure modes and prevention plan](failure_modes.md) reviews worst outcomes,
existing safeguards, proposed checks and the limits of software containment.

## Add firmware

Measure a user-owned binary with `scripts/inspect_firmware.py`, then review its
startup, memory layout, USB assets and persistent writes. Add one catalog record
in `firmware/images.hpp`, reuse or add its family metadata, and document its
controls in `docs/firmware_reference.md`. The fixed flash slots, preparer and menu
are derived from this catalog. Support requires exact-image authentication and
affected execution checks, not just adding metadata.

Shared numeric definitions live in `support/selector_constants.h`. The selector,
wrapper, linkers and build tools consume those definitions. Independent test
expectations retain literal addresses and format bytes.

Keep external execution-model code and commands in their owning repository.
Historical checkpoints retain exact identities; do not transfer physical claims
to changed binaries. Never distribute locally assembled vendor-containing BINs.

## Current validation

[Testing](testing.md) describes the checks and their limits. Keep run reports,
raw captures and build identities in ignored local artifact storage rather than
committing per-run records. Superseded plans and checkpoint prose remain in Git
history.
