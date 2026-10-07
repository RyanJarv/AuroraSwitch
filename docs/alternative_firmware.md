# Firmware support

The compiled [catalog](../firmware/images.hpp) is the sole launch allowlist.
A filename or version label is not enough: size, SHA-256, stack, and reset vector
must match. Other versions need separate review and catalog entries.

## Obtain files

No third-party BINs or manuals are distributed in this repository or selector
packages. Obtain the exact supported versions from their authors, then use the
[user guide](user_guide.md#prepare-the-firmware-files) to validate and prepare them.
Discord-only files remain user-supplied.

Public assets can be downloaded with GitHub CLI:

```sh
mkdir -p local-firmware
gh release download v0.3.0 --repo DaveParr/aurora-flux-capacitor \
  --pattern flux-capacitor-0.3.0.bin --dir local-firmware
gh release download v0.2.0 --repo DaveParr/Aurora-Morse \
  --pattern aurora-morse-0.2.0.bin --dir local-firmware
```

Pass those files to `scripts/prepare_payloads.py` as described in the guide.
Downloading an author asset does not bypass catalog verification.
Do not substitute “latest” or mirror binaries here.

## Testing status

| Images | Virtual evidence | Physical evidence |
| --- | --- | --- |
| Official Aurora, FDN, Flux, Morse | Current seven-image companion: repeated bounded launch/control/audio screen | Open |
| EchoGarden, CloudscapeX, Oscillator | Earlier five-image companion: repeated bounded screen | Open |
| Official switching cycles | Both complete directions on an earlier companion | Open |

See the [current checkpoint](public_release_checkpoint.md) and
[earlier community checkpoint](community_virtual_checkpoint.md) for exact
identities and limits. Catalog membership is not physical qualification.

Morse is an externally clocked VCA: use the Freeze-gate input and turn Mix above
zero. Silence without these is expected.

## Public source identities

Flux v0.3.0 source commit:
`eed85107f9cc1df3c9ef93d478fc45d5d9f9b882`.
Its annotated tag object is
`4e346a8a54120bd58ddbe0a35f7af0ab0d67f5ba`, not the source commit.

Morse v0.2.0 source commit:
`7e702fdd92c7963047f6857a122c35b8db11d22a`.

Both pin Aurora SDK `69b74a88b25e2fb4d722fc269bfd9395dd28edb5`.
Released BINs are the catalog inputs; source-to-release reproducibility is not
claimed. Public source helps review, but does not certify a binary's safety.

## Deferred support

- **Tempest v1.0.0:** fits staging but initializes and saves QSPI settings at
  offset `8192`. Review its linked persistence path and stock recovery before
  admission; do not skip initialization or fabricate a successful save.
  Source `1d2d9d41961bf12cbbc86637e76d4df732cd376e`, tag `Tempest-v1.0.0`,
  BIN 109872 bytes, stack/reset `0x20020000` / `0x240033f5`,
  SHA-256 `6fdb962135b2784813523a70e73bd1644b1d5b1e6637bb042c714dd7830931eb`.
- **Dirt Verb:** QSPI-linked; outside the RAM-launch contract.
- **HP-filter Aurora:** exceeds the current staging capacity.
- **Older versions:** separate exact entries after review; no automatic fallback.

See the [development guide](development.md) for the short onboarding process.
Historical intake checks and preparation results are retained in the
[onboarding record](history/alternative_firmware_20261006.md).
