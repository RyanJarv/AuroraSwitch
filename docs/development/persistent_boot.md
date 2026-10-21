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

Its prepared loader-handoff entry is not a physical power-on or an execution of
the updater's USB comparison. It checks the actual linked wrapper, selector,
authentication and terminal launch, complete unchanged 8-MiB NOR and zero array
erase/program commands, zero choice writes on automatic boots, and negative
controls. Private dumps remain ignored. It does not qualify USB writes, flash
timing/cache gates, payload settings behavior/audio, or ordinary stock recovery.

Before physical use, qualify the exact installer on hardware: initial stock
installation, repeated Dirt boots, RAM/Dirt changes, held-button menu entry,
missing/slow/corrupt/read-only USB, interrupted choice writes, persistence-sector
ownership during payload execution, and ordinary stock USB restoration. Other
firmware may write its own settings; zero selector application writes does not
promise a module with no flash writes. No hardware operation is authorized by
building or preparing these files.
