# Community-image virtual checkpoint

Status: **paused at Cloudscape's unresolved control/audio check**, 2026-10-06.
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
- Cloudscape: repeated launch/audio activity and raw control-input consumption;
  **the changed-audio check does not pass**.

Each launch observes 85 balanced callbacks and all 8192 captured stereo frames,
with the complete ordered control/buffer observations. The new catalog changes
neither the staging capacity nor launch-time revalidation/handoff/DMA cleanup.
Native CPU checks verify the unchanged 104-byte trampoline and complete DMA
zeroing for the official payloads; the real-USB candidate's linked cleanup
seam repeats independently. All 28 host tests pass. Two representative linked
loading controls omit Freeze or corrupt the input and verify refusal before
retry; no substantial media model was added.

## Why Cloudscape is paused

Its callback sees the tested raw control change, but all captured audio bytes
match the unchanged-control run. A separate second-control trial gives the
same result. This could reflect control-service timing, what that parameter
does, or the limited capture window; the cause has not been localized.

The all-image campaign intentionally stops on that comparison. The preserved
observation report labels it `blocked-cloudscape-control-audio`, not PASS.
Continuing would mean investigating this firmware's control producer or effect
behavior, beyond straightforward image onboarding. Next decide whether to
defer Cloudscape or authorize that bounded investigation. Do not replace this
check with a weaker “ADC changed” claim or implement DSP behavior in the host.

## Frozen changed builds

Both builds come from source `25c393e794c5ef3002b261a6ee7a6aaa11f91809`.
Packaging rebuilds application and libDaisy from fresh isolated tracked
checkouts, records pinned dependencies/toolchain and checks ELF/BIN agreement.
The complete manifests, hashes and bounded result summaries are in
[the machine-readable record](evidence/community_virtual_20261006.json).

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
controls/audio, reset/re-entry, a small EchoGarden/Oscillator compatibility
screen, then ordinary stock USB restore. It requires user participation; keep
ST-Link disconnected because it has caused resets. Virtual results do not close
the [recovery release gate](recovery_release_gate.md). More modeling and whole
state equivalence are not prerequisites to that physical screen.
