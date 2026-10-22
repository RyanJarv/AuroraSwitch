# Firmware downloads

Obtain the exact versions listed in the [user guide](user_guide.md#supported-firmware)
from their authors. No third-party firmware or manuals are included here.
Files shared only on Discord must be supplied manually.

## Obtain files

The [README quick start](../README.md#quick-start) covers installing the selector.
Obtain supported originals from the [reference](firmware_reference.md). Dirt Verb
and flash-running firmware are unsupported.

For Flux Capacitor and Morse's public GitHub releases, use GitHub CLI:

```sh
mkdir -p local-firmware
gh release download v0.3.0 --repo DaveParr/aurora-flux-capacitor \
  --pattern flux-capacitor-0.3.0.bin --dir local-firmware
gh release download v0.2.0 --repo DaveParr/Aurora-Morse \
  --pattern aurora-morse-0.2.0.bin --dir local-firmware
```

Put original supported BINs in `aurora/` on USB. The selector verifies each file
after reading it into RAM; it never changes the originals. A matching filename
is not enough: the bytes must match the supported version. Other versions, including newer releases, need separate support.

See the [user guide](user_guide.md#supported-firmware) for unsupported images
and their blockers.
