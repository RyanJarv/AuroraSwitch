# Testing

## Local checks

```sh
make check
make test-sanitize
make build
make verify-images FIRMWARE_DIR=/path/to/private/originals
python3 scripts/check_fatfs_install.py
make package
```

Use the [build setup](README.md) and [packaging contract](releases.md) for pinned
inputs and selector-only artifacts.

Native tests exercise the production controller, USB files, choice codec, menu
and handoff sequence. They cover unavailable USB, corrupt records, bounded reads,
changed/repeat saves, failures without retry loops, and cleanup-time staged-byte
rejection. Dirt coverage includes confirmation/cancellation, conflicts and aliases,
exact backups, copy/text failures, destination authentication and reset ordering.
The in-memory FatFs boundary injects read/write/sync/close/disconnect failures;
it does not emulate a USB controller.

The real-FatFs test checks backup/copy, remount/readback and restoration on an
ordinary disposable file, never mounted media. It compiles the pinned filesystem
with ARM-compatible integer widths and needs the SDK, native C/C++ compilers and
`mkfs.fat` (`dosfstools`). CI runs it alongside host tests and sanitizers.

Three mutation controls remove CRC checking, repeat suppression or save readback;
the corresponding native assertions must fail. The exact-image authenticator
also checks user-supplied originals locally.

Virtual integration code and commands belong in their owning repository.
Production-binary execution with modeled filesystem calls must be distinguished
from USB enumeration/mass-storage interoperability. Require no selector flash
program/erase/status/protection writes and unchanged on-module contents before
vendor entry or the Dirt reset request; subsequent stock installation activity
is separate from selector behavior.

## Browser checks

With Firefox and geckodriver installed:

```sh
geckodriver --port 4444
python3 scripts/check_reference_browser.py --webdriver http://127.0.0.1:4444
```

The smoke checks both dialogs, narrow layouts, dark mode, keyboard navigation
and static no-JavaScript pages. Add `--firefox /path/to/firefox` if needed.

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
