# Testing

## Local checks

```sh
make check
make test-sanitize
make build
make verify-images FIRMWARE_DIR=/path/to/private/originals
make package
```

Use GNU Arm 10-2020-q4-major for firmware builds. `make package` builds clean
committed source and pinned dependencies in isolation, checks ELF/BIN agreement
and records tool/artifact hashes. It never installs firmware. Native tests use
the production slot lookup, menu/controller, journal and handoff ordering.

Important cases cover slot bounds and omitted images, exact image admission,
interrupted journal writes/erases, repeat-choice write suppression, save-failure
launches, missing saved images and rejection after staged-byte corruption.
Three mutation controls ensure journal integrity, repeat suppression and
readback assertions actually fail when their safeguards are removed.

## Browser checks

With Firefox and geckodriver installed:

```sh
geckodriver --port 4444
# In a second terminal:
python3 scripts/check_reference_browser.py --webdriver http://127.0.0.1:4444
python3 scripts/check_preparer_browser.py --webdriver http://127.0.0.1:4444
```

The site smoke includes narrow layouts, dark mode, keyboard navigation and
no-JavaScript pages. The preparer checks local file input, rejection and
byte-for-byte Python assembly parity. If Firefox is not found automatically,
both checkers accept `--firefox /path/to/firefox`.

## Execution and qualification

The [fixed-slot validation record](evidence/fixed_slot_validation_20261021.json)
binds the production execution scenarios to exact firmware bytes. External
execution-model code and commands belong in their owning repository.

Observations stop at the checked handoff; later vendor settings writes are
outside those flash counts. Modeled power loss, ignored programming and matching
bytes do not establish electrical timing, physical USB updating, audio or stock
recovery. See the [physical release gate](recovery_release_gate.md).

## Formatting

Use clang-format 19.1.7 with `.clang-format` for tracked C++ headers/sources in
`firmware/` and `support/`: LLVM style, four-space indentation, 100 columns and
preserved include order. Python uses Black 25.1.0 with `pyproject.toml` over
`scripts/` and `tests/`. These are optional development tools, not build
requirements. Explain purpose first, then the hardware details that constrain it.
