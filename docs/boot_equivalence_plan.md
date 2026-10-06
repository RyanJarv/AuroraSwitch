# Validation and normal-boot equivalence plan

Status: plan, not completed qualification. No hardware inspection, installation,
reset or measurements are authorized by this document alone. Existing virtual
results apply only to their exact images and bounded models; see verification.md.

## Define the question before comparing

“Exact same state” is not a realistic whole-machine assertion for this loader.
Normal boot retains the selected payload in QSPI; selector boot retains the
selector there. The trampoline/staged file leave different RAM contents, and
timers, USB history and analog state can differ. Even two normal boots can
differ in timestamps, uninitialized memory or random seeds.

We instead want two separately reported results:

1. **Launch-contract equivalence:** every CPU, memory and peripheral condition
   the target startup relies on is equivalent or deliberately established.
2. **Observable behavior equivalence:** the same controlled input history gives
   equivalent control, display, audio and persistence behavior at defined
   checkpoints. Exact equality is required where deterministic; tolerances must
   be justified for measurements, never used to conceal unexplained differences.

Do not call a scoped match whole-device equality. A different state is acceptable
only after showing it is unused or overwritten before any relevant read, or
recording a narrow explained divergence. Unknown differences remain open.

## Reuse existing tools first

Portable host tests exercise staging, menu and handoff ordering. Existing
linked-firmware virtualization tooling already exercises the actual trampoline
and same-guest switching. Reuse it for baseline/selector runs and raw memory,
register, control and audio capture; do not build a second emulator here.
Virtual media is explicitly synthetic and cannot qualify real USB/FatFs.

The standalone repository must eventually include a reproducible evidence
recipe (or a documented optional tool dependency), not just an assertion that
external virtual tests passed. TODO: publish minimal recipes/receipts without
embedding vendor payloads or making unrelated projects required for host tests.

## Stage 1 — deterministic software comparison

Pin selector ELF/BIN/MAP/manifest, target bytes, normal-boot baseline, model and
tool identities. Keep inputs, sample rate, initial calibration/settings, clock
and random-state assumptions explicit. Never host-clear memory to make a
selector run look like a normal boot.

Capture corresponding checkpoints:

- Immediately before the target reset-handler entry: PC, MSP, PSP, CONTROL,
  PRIMASK/BASEPRI/FAULTMASK, VTOR, FPU state, cache/MPU configuration, interrupt
  masks/pending state, clocks and relevant peripheral registers.
- After C runtime/startup and hardware initialization: initialized data/BSS,
  stack/heap boundaries, audio/DMA/ADC/UI timers, USB state and output buffers.
- Before first audio/control work and over a bounded run: ordered reads/writes,
  button/knob processing, display state, audio buffers, errors and persistence.

Compare code bytes exactly. Compare all initialized regions exactly where the
contract permits it. Report retained-memory differences separately and analyze
read-before-write dependencies, especially DMA buffers, SDRAM and backup SRAM.
Do not zero or normalize unexplained differences out of the comparison.

Exercise normal→normal repeatability first, then selector→A, selector→B, A→reset
→selector→B→reset→selector→A, and the reverse cycle in the same guest instance.
Use silence, impulse, fixed tone and nonzero/reused buffer patterns plus
deterministic knob/button changes. A bounded audio prefix is not full DSP proof.

Fail-closed controls must detect a missed cleanup store, altered target vector,
stale staging, unexpected relevant write and retained buffer contamination.
State whether each control is a separate execution or a verifier sabotage.

## Stage 2 — physical transport and recovery

Before executing, confirm recoverable exact backups and a demonstrated normal
updater route. Pin the exact selector and target files. Installation/readback,
normal boot baselines and halted debugger checkpoints need separately scoped
authorization and account for halt-induced timing changes.

Test empty/one/two-image media, corrupt/renamed/wrong-size files, reconnect,
disconnect while scanning/loading, failed reload and launch without approval.
Include more than one supported drive and characterize read/error timeouts.
Check that USB/file/persistent state is not unexpectedly changed by the selector.

Repeat both launches and both switching orders, reset and cold power cycles.
Observe every accessible control, LEDs and audio response; passing audio with
frozen controls fails. Restore original firmware via the documented single-drive
procedure. Record hardware/bootloader revisions and exact failures, not anecdotes.

## Stage 3 — behavior and analog measurements

Use reproducible external audio/control stimuli and the same output recording
chain for normally installed and selector-launched copies of each target.
Compare latency, startup transients, idle noise, gain, clipping, freeze/reverse/
shift behavior, sample continuity, long-running stability and settings reload.
Begin with attenuated monitoring. Measurement tolerances must derive from
normal→normal variability and equipment limits, not the desired outcome.

Test power interruption during selector loading separately from installation
or target persistence writes: the risks are different. Do not claim power-loss
safety of third-party firmware based on a read-only selector.

## Release evidence and stopping rules

Publish a compact matrix: exact build, target, hardware, scenario, repetitions,
pass/fail, known divergence, raw receipt and untested boundary. A release is
experimental until transport, functional launch, both reset cycles and known-good
restore are demonstrated for that build. Wider hardware claims need coverage.

Stop on a frozen panel, unexpected persistent write, invalid target entry,
indeterminate installation or failed recovery. Preserve evidence before reset
when safe; never automatically chain a guessed recovery write. No current result
supports full hardware-state, electrical, timing or all-firmware equivalence.
