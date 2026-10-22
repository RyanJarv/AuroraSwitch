# How it works

The stock updater installs one private bundle at its ordinary QSPI application
address. Dirt remains at that fixed address with only its four-byte reset entry
redirected to our small startup wrapper. Our selector and unchanged copies of
all included firmware follow in reserved flash ranges.

Physical installation and recovery remain unqualified for this development
build. See the [release gate](development/recovery_release_gate.md).

At power-on the wrapper copies the selector into RAM and checks that copy before
starting it. The selector checks the catalog-defined image slots, reads the last selection
from flash, and either verifies and boots that image or shows the menu.
Menu presence means installed, not authenticated: a damaged image is rejected
when selected. An unavailable or damaged saved image never triggers an
automatic fallback to different firmware. USB is used by the
stock updater and any payload-specific assets, not by the selector.

## Installed layout

| Region | Role |
| --- | --- |
| Below `0x90040000` | Existing loader/calibration/vendor settings; preserved |
| `0x90040000` | Dirt, with the reviewed reset-word redirect |
| `0x90058000` | Startup wrapper |
| `0x90058100` | Selector length/complement/checksum |
| `0x90059000` | Selector image |
| `0x90101000` onward | Fixed catalog-derived slots containing unchanged originals or erased bytes |
| `0x907e0000`, `0x907e1000` | Independent selection-journal sectors |

Every installer covers the complete slot area, including erased slots for omitted
images. This clears previous optional images on reinstall. It ends before the journal. The updater's byte comparison therefore
ignores changing selections, even if the installer stays on USB. Neither normal
boot nor firmware switching rewrites the installed bundle.

## Selection and launch

The saved record identifies an exact image hash, not its menu index. Two sectors
hold alternating generations. Each changed explicit selection appends one
80-byte record on an erased page and verifies it. An existing valid choice survives sector recycling. If neither sector contains
a valid record, the first explicit save initializes one. Automatic boot and repeat selections
perform no write or QSPI initialization.

A failed choice save shows the Freeze LED red for half a second, then attempts
the ordinary verified launch. The choice may not be remembered next time.
If flash mapping is unavailable, the Dirt handoff still refuses to proceed.

Before launch, the chosen original is copied from flash into separate staging
RAM and authenticated by full SHA-256, size and vectors. RAM firmware replaces
the selector at `0x24000000`. Dirt's installed bytes must match the authenticated
original except for the reviewed reset word; its original startup address is
used for explicit launch.

The selector stops interrupts, resets memory-writing peripherals, cleans cache
state, rechecks staged bytes, and executes a checked jump routine from separate
SRAM4. The final routine clears the DMA arena, sets VTOR/MSP and enters the
firmware's reset handler. This jump is not a hardware reset or a sandbox.

## Boundaries

Only exact catalog images are supported. Payloads retain full hardware privilege
and may write their own settings. The layout excludes reviewed calibration and
settings locations, but virtual startup checks do not qualify all later payload
writes, physical USB installation, audio, power interruption or stock recovery.
FataMorgana tables and other firmware-owned USB assets remain separate.
