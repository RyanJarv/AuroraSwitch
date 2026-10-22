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

The production flash-driver header also runs natively against an SDK-call
recorder. Independent chip offsets check every journal page, invalid addresses
and lengths, error propagation and indirect-mode initialization. Three driver
mutation controls remove program/erase bounds or shift the production journal
address; the independent expectations must reject each change. This recorder
checks calls into the SDK, not the SPI commands emitted by its implementation.

Controller/journal integration tests cover 100 changed choices, 300 automatic
boots, repeat choices, sector recycling and initialization/erase/program
failures. Idle updates and subsequent automatic boots must not retry saves.
An independent stock-updater model applies prepared files around 64-KiB EOF
boundaries to non-erased flash and checks that reserved regions remain intact.
The existing maximum-length erase-margin test bounds the supported file range.
Neither model executes physical USB installation or qualifies stock recovery.

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

Retain exact build identities, inputs and raw results with each local run.
Generated reports belong in ignored artifact storage, not in Git. External
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
