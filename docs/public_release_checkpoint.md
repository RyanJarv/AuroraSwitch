# Seven-image software checkpoint

2026-10-06: software checks pass. Physical testing and stock recovery are pending.

Flux v0.3.0 (yellow) and Morse v0.2.0 (white) join the catalog without changes
to staging or handoff. Staging remains 181888 bytes. Payloads are not distributed.

## Build identities

Both packages were built from source
`e9838144657bd4fa73048f4bc2452479359f0816` with pinned dependencies and
GNU Arm 10-2020-q4-major. Packaging checks ELF/BIN agreement.
The [machine-readable record](evidence/public_releases_20261006.json) contains
the full artifact hashes and result summaries.

| Build | Manifest SHA-256 | BIN SHA-256 |
| --- | --- | --- |
| Real USB; installation unconfirmed | `5a5a5c96814d526d0d3c7c249a71ebfd72d2c4eb62dc33b76999e62adc9b7fcf` | `7eeeba55132482037a3dc7aefe67cb625605fbdf30575607c8750d7e2d625155` |
| Synthetic media; never install | `5febca4aef3b4ba6699eede4b531c54ad7ce1db17b38394d8eeea6bfb504d608` | `feee7f0cb6e01795967cb39f454d8af16d73d2cfa6686fc7875e6769af61e975` |

## Results

- 28 host tests pass; the compiled authenticator accepts all seven local files.
- Aurora, FDN, Flux, and Morse each pass a baseline and two changed-control
  virtual runs: 12 executions, 85 callbacks, and 8192 stereo frames per run.
- Changed repetitions match. Complete payload readback, DMA clearing, control/
  audio observations, and the real-USB build's 20-store cleanup check pass.
- EchoGarden, Cloudscape, and Oscillator retain their
  [five-image results](community_virtual_checkpoint.md), not a fresh pass here.
- Earlier full official switching cycles were replayed, not rerun on this build.

Flux callback: `0x24002250`; Morse: `0x2400107c`. Each uses its own linked
DSP. Morse receives eight Freeze-gate pulses on PD11 and a Mix change;
Flux also changes Mix. A late impulse follows the control change.
Partial ADC scans are checked in order.

Earlier Morse trials used the wrong clock pin, zero output level, or a Blur
change after envelope latching. These were inconclusive setups, not passing
control tests or firmware failures.

## Limits and next step

Synthetic media and peripheral models do not prove real USB/FatFs, hardware
timing, every control, persistent-write safety, long-run DSP, or physical recovery.
Results apply to the recorded images only.

The real-USB build and seven payloads were staged and read back on a test drive.
Module installation remains unconfirmed. Use the [user guide](user_guide.md)
for setup and the [reliability checklist](reliability_campaign.md#active-checklist)
for the remaining physical tests.
