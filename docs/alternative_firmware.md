# Firmware downloads

Obtain the exact versions listed in the [user guide](user_guide.md#supported-firmware)
from their authors. No third-party firmware or manuals are included here.
Files shared only on Discord must be supplied manually.

## Obtain files

The [README quick start](../README.md#quick-start) builds the selector, downloads
and checks Aurora 1.4.4, FDN 1.2.2, Flux Capacitor 0.3.0, and Morse 0.2.0,
then copies them to your USB drive. No Discord login is needed.

For Flux Capacitor and Morse's public GitHub releases, use GitHub CLI:

```sh
mkdir -p local-firmware
gh release download v0.3.0 --repo DaveParr/aurora-flux-capacitor \
  --pattern flux-capacitor-0.3.0.bin --dir local-firmware
gh release download v0.2.0 --repo DaveParr/Aurora-Morse \
  --pattern aurora-morse-0.2.0.bin --dir local-firmware
```

Use `scripts/prepare_payloads.py` as shown in the guide to check and prepare files.
A matching filename is not enough: their bytes must match the supported version.
Other versions, including newer releases, need separate support.

See the [user guide](user_guide.md#not-supported-yet) for unsupported images
and their blockers.
