# Firmware downloads

Obtain the exact versions listed in the [user guide](user_guide.md#supported-firmware)
from their authors. No third-party firmware or manuals are included here.
Files shared only on Discord must be supplied manually.

## Obtain files

The [README quick start](../README.md#quick-start) covers preparing the development installer and links
to Aurora 1.4.4 and FDN 1.2.2. The development preparer also requires
the original Dirt Verb file; see the supported-version links in the user guide.

For Flux Capacitor and Morse's public GitHub releases, use GitHub CLI:

```sh
mkdir -p local-firmware
gh release download v0.3.0 --repo DaveParr/aurora-flux-capacitor \
  --pattern flux-capacitor-0.3.0.bin --dir local-firmware
gh release download v0.2.0 --repo DaveParr/Aurora-Morse \
  --pattern aurora-morse-0.2.0.bin --dir local-firmware
```

Add the original supported BINs to the offline preparer. It verifies each file
and creates one private `AuroraSwitch.bin` for USB installation; it never changes
the originals. A matching filename is not enough: the bytes must match the
supported version. Other versions, including newer releases, need separate support.

See the [user guide](user_guide.md#unsupported-firmware) for unsupported images
and their blockers.
