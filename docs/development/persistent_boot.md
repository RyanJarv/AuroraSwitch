# Persistent boot and Dirt application reuse

Current development firmware remembers an authenticated Freeze selection by its
catalog SHA-256. At the first media discovery of the next startup it reopens and
authenticates that exact image before handoff. Hold Shift + Reverse at power-on
to open the menu. There is one automatic attempt per startup; a missing image,
bad record, authentication failure, or reconnect does not launch another image.

Two 80-byte files, `aurora/boot-a.dat` and `aurora/boot-b.dat`, contain a version,
generation, exact image hash, and CRC32. Only a changed explicit launch writes
the inactive record; sync, close, and exact readback must succeed before launch.
Automatic boots and menu navigation do not write. A damaged record can leave
the previous valid choice usable. This is not a transactional FAT filesystem:
power loss or a failed USB device can still damage files. Neither record reserves
a QSPI settings sector or authenticates firmware; the catalog and fresh staged
hash/vector checks do that. USB media is required for the choice and image.

## Dirt resident installer

The ordinary selector and Dirt both occupy QSPI at `0x90040000`. Merely checking
before writing cannot avoid the stock updater reinstalling the ordinary selector
over Dirt on the next boot. The separate **private resident installer** preserves
a stable application image on both flash and the USB root:

| Address | Contents |
| --- | --- |
| `0x90040000..0x900573dc` | Exact Dirt 1.1, except its reset-vector word |
| `0x90058000` | Stackless resident startup wrapper, at most 256 bytes |
| `0x90058100` | Fixed bounded selector length, complement, and checksum |
| `0x90059000` onward | Embedded selector binary, at most 480 KiB |

The wrapper copies the selector into AXI SRAM, checks the bounded copy and its
vectors, sets SRAM VTOR/MSP, and enters its real reset handler. It inherits the
ordinary loader's clocks/MPU and disabled-cache handoff policy. Its FNV checksum
detects accidental copy corruption; it is not cryptographic authentication.
No bootloader/internal-flash patch is made. Addresses below the application,
including calibration and existing settings, are excluded from the installer.

Dirt selection still authenticates the original complete file in staging. The
resident path compares its installed bytes, permitting only the exact wrapper
reset word, verifies the wrapper and embedded-selector checksum, then repeats
validation through normal cleanup. The trampoline enters Dirt's **original**
authenticated reset PC; all its other vectors/code/data addresses stay fixed.
It performs no application erase/program. A detected resident mismatch refuses
launch rather than erasing the dispatcher. An ordinary nonresident Dirt launch
also skips erase/program when its complete authenticated image already matches.
The resident path retains the loader's live memory-mapped QSPI configuration
instead of calling SDK initialization, which would issue status-register writes.

The root installer never changes when a firmware is selected. With healthy
reads and identical root/flash bytes, the pinned stock updater's existing byte
comparison skips its application programming path. Never put an ordinary
selector BIN at the root alongside or instead of the resident installer when
testing this behavior: the updater installs changed root bytes.

## Prepare locally

Run from clean committed source with the pinned GNU Arm toolchain:

```sh
make check
make package
python3 scripts/prepare_resident.py \
  --bundle dist/SELECTOR_MANIFEST_SHA256 \
  --dirt 'local-firmware/DirtVerb 1.1.bin' \
  --output .deps/private-resident-install
```

The command creates a new local folder only. Its `AuroraSwitch.bin` is the
combined root installer, and `aurora/DirtVerb 1.1.bin` is the unchanged original
needed for discovery and fresh launch authentication. Other supported images
remain in `aurora/`. `resident.json` binds input package, source, toolchain via
the package, output identities, layout and exact four-byte patch.

**Never distribute this folder or its installer:** they contain third-party
firmware. The preparation command never copies to a drive or programs a module.
The separate `make package-virtual` companion produces a clearly named
`AuroraSwitchResidentVirtual.bin`; never install a synthetic-media image.

## Development verification and physical boundary

The [stock-loader checkpoint](evidence/persistent_boot_loader_20261021.json)
binds source `5fe19cd`, isolated real/synthetic packages and the private installer.
All 74 host tests, both isolated Arm packages and 13 linked virtual cases pass.
One case executes the pinned stock loader from its reset vectors through the
resident wrapper and Dirt's real reset/constructors to `main`. Its captured QSPI
mapping is reused for the prepared-startup cases. The unmodified loader makes
one status-register write; the resident path adds none before payload `main`.
Every case preserves the complete 8-MiB NOR array with zero erase/program
commands. Explicit FDN selection writes one USB record; subsequent automatic
startup consumes that saved record without writing. Entry checks the complete
32-KiB DMA arena, installed image identity, stack, VTOR and terminal routine.

A negative control exposed the earlier wrapper hashing source bytes instead of
the SRAM copy. The corrected wrapper completes the copy, executes DSB and hashes
the SRAM bytes before jumping. Injecting the same SRAM corruption now stops it
before selector entry. Other cases cover repeated Dirt startup, held-button
override, and damaged/unknown records and payloads. The
[earlier checkpoint](evidence/persistent_boot_20261021.json) retains the original
11-case and browser results; this follow-up changes no site files. Raw records
remain private under `.deps/`.

`make check` covers choice corruption/torn records, sequence wrap, changed-choice
write suppression, launch authentication, menu override, reconnect behavior,
exact resident matching, bounded layout, and private-output guards.

The affected linked startup/launch lane reuses the existing external virtual
runner. Supply a directory with exact FDN and Dirt originals:

```sh
python3 scripts/check_persistent_boot.py \
  --bundle dist/VIRTUAL_MANIFEST_SHA256 \
  --images .deps/private-resident-install/aurora \
  --runner-root ../vcv-qubit \
  --output .deps/persistent-boot-check
```

The stock-loader case uses a no-media USB host model, then supplies synthetic
files to the selector. It does not execute the updater's USB comparison or
establish physical power-on behavior. The other cases use a prepared handoff.
The lane checks the actual linked wrapper, selector, authentication and terminal
launch, unchanged NOR, write suppression and negative controls. Private dumps
remain ignored. It does not qualify physical USB writes, flash timing/cache
gates, payload settings behavior/audio, or ordinary stock recovery.

## Physical screen and flash-write measurement

The [physical screen](evidence/persistent_boot_physical_20261021.json) binds the
installed 209,548-byte private image to SHA-256 `e6356fa7e929...`. Readback matched
the candidate and pinned stock loader. The user confirmed selector/Dirt operation
and an ordinary automatic Dirt power-on with USB connected and the debug ribbon
disconnected. That ordinary power-on had no command capture.

A temporary external ST-Link watchpoint observed CPU writes to QUADSPI_CCR
through two 15-second debugger-reset windows. Each recorded the same 13 command
setups, all from the stock loader, with no array erase/program setups. Two
identical status-register setup stores must not be counted as two completed
flash transactions. One reset reached Dirt and the installer readback remained
exact. The repeat remained in the selector waiting for USB; its SRAM image was
exact and CPU fault registers clear. It is not a successful automatic-boot pass.
The cause of that USB wait is unresolved.

Keep the debugger probe temporary: it changes execution timing, observes CPU
command setup rather than bus transactions, and cannot establish uninterrupted
cold-boot behavior. The existing linked virtual write checks remain permanent.
For nonintrusive physical measurement, capture the QSPI bus from power-on and
separate application erase/program, status-register operations and payload
settings writes. Identical readback alone cannot exclude rewriting identical
bytes. Private probe scripts and raw records remain under
`.deps/flash-write-probe-20261021/`; no firmware instrumentation was added.

Full qualification still requires repeated ordinary Dirt boots, RAM/Dirt changes,
held-button menu entry, missing/slow/corrupt/read-only USB, interrupted choice
writes, persistence-sector ownership during payload execution, and ordinary
stock USB restoration. Other firmware may write its own settings; zero selector
application writes does not promise a module with no flash writes. No hardware
operation is authorized by building or preparing these files.
