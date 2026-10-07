# Community-image virtual checkpoint

Historical five-image checkpoint. The newer seven-image candidate and bounded
Morse/Flux observations are in [the public-release checkpoint](public_release_checkpoint.md).
Preserve the results and identities below; they are not a live pass on that new build.

Status: **bounded virtual checks complete; stop before physical testing**, 2026-10-06.
No new emulator subsystem or loader feature was required for this intake.
Nothing has been installed on hardware in this tranche.

## What passed

The changed selector discovers and authenticates all five catalog identities.
Bounded virtual runs follow its real button, verification and RAM handoff path
into each payload's reset entry and audio callback. The tests inspect complete
callback buffers and captured audio, not just a “started” message.

- Official Aurora and FDN: repeated launch, control-input and changed-audio checks.
- EchoGarden: repeated launch, control-input and changed-audio checks.
- The Oscillator Is a Lie: repeated launch, control-input and changed-audio checks.
- Cloudscape: repeated launch, control-input and changed-audio checks after the
  audio stimulus was moved after the control change.

Each launch observes 85 balanced callbacks and all 8192 captured stereo frames,
with the complete ordered control/buffer observations. The new catalog changes
neither the staging capacity nor launch-time revalidation/handoff/DMA cleanup.
Native CPU checks verify the unchanged 104-byte trampoline and complete DMA
zeroing for the official payloads; the real-USB candidate's linked cleanup
seam repeats independently. All 28 host tests pass. Two representative linked
loading controls omit Freeze or corrupt the input and verify refusal before
retry; no substantial media model was added.

## Cloudscape's corrected stimulus

Initially, its callback saw the raw control change but all captured audio bytes
matched baseline. A second-control trial also gave no difference. The important
ordering issue was that the input impulse arrived **before** the control change.
This short dry-response comparison could not establish a control effect.

Using the existing late-impulse parameter (frame 7000 rather than 4096) puts the
input after the observed ADC change. One baseline and two changed-control runs
now produce different complete PCM hashes, with identical changed repetitions.
No firmware, callback binding, peripheral model or host DSP approximation was
changed. The original blocked report remains historical, not silently relabeled.
The all-image campaign still stops if an observable audio effect is absent.

## Frozen changed builds

Both builds come from source `25c393e794c5ef3002b261a6ee7a6aaa11f91809`.
Packaging rebuilds application and libDaisy from fresh isolated tracked
checkouts, records pinned dependencies/toolchain and checks ELF/BIN agreement.
The complete manifests, hashes and bounded result summaries are in
[the current machine-readable record](evidence/community_late_impulse_20261006.json).
The [earlier inconclusive comparison](evidence/community_virtual_20261006.json)
is preserved separately. Existing passing receipts for the other four images
were reused rather than rerunning their completed live campaigns.

| Build | Manifest / package identity | BIN SHA-256 |
| --- | --- | --- |
| Real USB, **not deployed or physically qualified** | `e1daea377e4067f2efe96e1f426b088deb160a80e6173fe5eafa897acadd3908` | `e79ed1ec20a5187f8e770a9cd77d41041f03d9deef79920f53fd5bf43782d6e7` |
| Synthetic-media virtual companion, **never install** | `245ccecf580f14220165084fe1e5fd05411d840294c1d96a8269f7624027c289` | `8d00bd882b45fd2777563c95f1d209dfd18a5a3d29710c6f60ad10244fe8eb06` |

With the required Arm toolchain on PATH:

```sh
make test
python3 scripts/package.py           # real-USB development candidate
python3 scripts/package.py --virtual # synthetic-media diagnostic companion only
```

Earlier full official-image switching-cycle evidence remains bound to its
original companion and was regression-replayed. It is not relabeled as a new
catalog reset/re-entry cycle. Changes to the catalog create a new candidate;
older physical observations do not qualify it.

## What this does not prove

Synthetic storage is not real USB enumeration or FatFs transport. These short
runs do not prove all controls, sound quality, long-run stability, real IRQ/DMA
timing, exact normal-boot state or target persistence safety. Third-party
firmware remains unsandboxed after launch. Payload bytes are not distributed.

The remaining physical screen is both official launch directions, responsive
controls/audio, reset/re-entry, a small compatibility screen of the three custom
images, then ordinary stock USB restore. It requires user participation; keep
ST-Link disconnected because it has caused resets. Virtual results do not close
the [recovery release gate](recovery_release_gate.md). More modeling and whole
state equivalence are not prerequisites to that physical screen.
