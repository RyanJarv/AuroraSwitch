# Testing

## Local checks

```sh
make check
make test-sanitize
make build
make verify-images FIRMWARE_DIR=/path/to/private/originals
make package
```

Use GNU Arm 10-2020-q4-major for firmware. Packaging fresh-builds clean committed
source and pinned dependencies, checks ELF/BIN identity and records tool/file
hashes. It never installs firmware or packages vendor originals.

Native tests use the production controller, USB file operations, choice codec,
menu and handoff sequence. The FatFs test boundary supplies in-memory files and
read/write/sync/close/disconnect failures; it does not emulate a USB controller.
Tests cover bounded reads, corrupt records, changed/repeat saves, no automatic
save/retry loops, missing firmware and cleanup-time staged-byte rejection.
Three mutation controls remove CRC checking, repeat suppression or save readback;
the corresponding native assertions must fail. The exact-image authenticator
also checks user-supplied originals locally.

Virtual integration code and commands belong in their owning repository.
Production-binary execution with modeled filesystem calls must be distinguished
from USB enumeration/mass-storage interoperability. Require no selector flash
program/erase/status/protection writes and unchanged on-module contents before
vendor entry; stock loader activity is a separate baseline.

## Browser checks

With Firefox and geckodriver installed:

```sh
geckodriver --port 4444
python3 scripts/check_reference_browser.py --webdriver http://127.0.0.1:4444
```

The smoke checks narrow layouts, dark mode, keyboard navigation and static
no-JavaScript pages. Add `--firefox /path/to/firefox` if needed.

## Evidence limits

Keep generated reports and failures in ignored storage. Bind results to exact
source/build/model/input hashes. A virtual handoff or unchanged array does not
qualify physical USB installation/recovery, power interruption, flash electrical
behavior or audio. Audio comparisons remain paused. See the
[physical gate](recovery_release_gate.md) and [failure plan](failure_modes.md).

## Formatting

Use clang-format 19.1.7 with `.clang-format` (LLVM, four spaces, 100 columns,
preserved include order) and Black 25.1.0 with `pyproject.toml`. These optional
tools are not build requirements. Explain purpose alongside necessary hardware details.
