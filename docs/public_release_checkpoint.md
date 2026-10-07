# Morse and Flux: bounded compatibility checkpoint

2026-10-06 — **virtual screen passes; physical testing and stock recovery open**.

The catalog adds Flux Capacitor v0.3.0 (yellow) and Morse v0.2.0 (white).
No new loader mechanism, staging expansion, USB stack or handoff cleanup was
needed. The seven-image catalog still reserves 181888 bytes at `0x30008000`.
Exact staged-byte verification and the 104-byte DMA-cleaning trampoline remain
unchanged. Third-party BINs are user-supplied and are not redistributed.

## What was checked

Fresh isolated builds from source `e9838144657bd4fa73048f4bc2452479359f0816`
rebuild the selector and pinned libDaisy with GNU Arm 10-2020-q4-major.
Packaging checks ELF/BIN agreement and seals source/tool/dependency identities.
The current machine-readable [record](evidence/public_releases_20261006.json)
contains exact manifests, artifact hashes, the cleanup census and result summaries.

| Build | Manifest/package SHA-256 | BIN SHA-256 |
| --- | --- | --- |
| Real USB candidate, **installation unconfirmed; not physically qualified** | `5a5a5c96814d526d0d3c7c249a71ebfd72d2c4eb62dc33b76999e62adc9b7fcf` | `7eeeba55132482037a3dc7aefe67cb625605fbdf30575607c8750d7e2d625155` |
| Synthetic-media companion, **never install** | `5febca4aef3b4ba6699eede4b531c54ad7ce1db17b38394d8eeea6bfb504d608` | `feee7f0cb6e01795967cb39f454d8af16d73d2cfa6686fc7875e6769af61e975` |

The existing virtual launch runner executes discovery, Freeze verification,
Shift launch, payload replacement, reset entry and the payload's own audio
callback. Official Aurora/FDN and the two additions each pass one baseline and
two changed-control repetitions: **12 passing executions**, each with 85
balanced callbacks, 255 call/entry/return events and 8192 stereo frames.
Changed-control repetitions have identical full handoff/continuation projections.
Complete input, ADC, planar-buffer and PCM observations are checked; CFSR/HFSR
are clear. Full payload readback and the zero 32-KiB DMA arena are checked at
branch and reset entry. The real-USB ELF's 20-store cleanup seam also passes.

Flux's own callback is `0x24002250`; Morse's is `0x2400107c`. These bindings
come from each exact BIN's main/StartAudio registration, not stock addresses.
Both execute their own DSP. The observer supplies no musical implementation.

Morse needs a Freeze-gate clock and a nonzero Mix knob (output level). The
test supplies eight active-low pulses on PD11, through the existing GPIO model,
and changes Mix from raw ADC `32768` to `49152`. Flux changes its Mix control
from `0` to `49152`. An impulse at frame 7000 occurs after the control change.
Partial ADC scans at startup are retained and checked in order, not filtered out.

Earlier Morse trials were inconclusive test setups: wrong clock pin, zero
output level, and a Blur change after the active envelope had latched its
shape. They are not firmware failures or passing controls tests. Mix provides
the smaller unambiguous control/audio screen; Blur and all other controls
remain outside this bounded claim.

The three earlier community images retain their
[five-image checkpoint](community_virtual_checkpoint.md), not a transferred
live pass on this new selector. Earlier complete official switching cycles
remain regression evidence for the unchanged handoff mechanism, not a new
seven-image reset/re-entry pass. No whole-memory equivalence requirement is added.

## Obtain and prepare payloads

Use the [user guide](user_guide.md#prepare-the-firmware-files) for preparation
and the [firmware support page](alternative_firmware.md#obtain-files) for pinned
public downloads. These commands validate bytes; they do not install firmware.

## Checks and limits

`make test` passes all 28 host tests. The compiled authenticator passes all seven
real local files, including backing-file replacement and staged-byte mutation
controls. `python3 -m compileall -q scripts tests` and `git diff --check` pass.
Both packages are generated with `python3 scripts/package.py` (real USB) and
`python3 scripts/package.py --virtual` (diagnostic only) after committing source.

Virtual regression results and fail-closed observation controls are recorded
with the development archive. Synthetic storage/ADC/audio clocks do not prove
real USB/FatFs, hardware IRQ/DMA timing, sound quality, every control, persistent
settings safety, long-run reliability, physical recovery or normal-boot equivalence.
Payloads are unsandboxed after launch.

Next is a small physical screen on this exact real-USB candidate, then ordinary
stock USB restoration. **Keep ST-Link disconnected** given the reported resets.
The [recovery release gate](recovery_release_gate.md) remains open. After this
software tranche, the candidate and seven authenticated payloads were staged
to a test drive and read back. Module installation remains unconfirmed; see
the [reliability checklist](reliability_campaign.md#current-build).
