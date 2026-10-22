# Failure modes and prevention plan

This review assumes a component is wrong and asks what it could damage. It is
an engineering plan, not a list of discovered defects or a qualification report.
The code boundaries described here were reviewed at `0628413`; unrelated local
build-script changes were excluded. No firmware behavior or hardware was changed.

The priorities are preserving calibration and ordinary USB recovery, preventing
unintended execution, and avoiding unnecessary flash wear. Correct audio and
controls, usable selection, and private handling of vendor files also matter.

## What we can promise

We cannot guarantee that nothing goes wrong after arbitrary future changes.
Future code can remove a check, bypass a driver, change the test oracle, or
program hardware directly. Hardware faults and the behavior of closed-source
firmware add assumptions that software tests cannot eliminate.

The useful promise is narrower: for a specific reviewed build, under stated
hardware and fault assumptions, define which operations are allowed and verify
that other operations are rejected before they have side effects. Keep a tested
recovery route for failures outside that boundary.

Today the selector, flash driver and launched firmware share privileged access.
The handoff leaves no independent monitor behind. Full-image SHA-256 establishes
that bytes match an accepted image; it does not prove that image cannot damage
state. CRC and FNV checks detect classes of corruption; neither establishes
trusted origin. Readback detects a bad write after it happened.

## Physical boundaries to preserve

These literal board limits are deliberately independent of the production
layout definitions in `support/selector_constants.h`.

| Asset | Boundary | Worst loss |
| --- | --- | --- |
| Internal loader and device configuration | Separate from external QSPI; no selector programming authority is intended | Loss of ordinary recovery, altered boot/debug access, potentially destructive protection changes |
| Calibration and vendor settings | QSPI offsets below `0x40000`, including `0x1000` and `0x2000` | Permanent loss of unit-specific calibration/settings without a backup |
| Resident application, wrapper and installed images | QSPI mapped at `0x90000000`; application starts at offset `0x40000` | No application/menu, unintended execution, corrupted installed firmware |
| Choice journal | Only offsets `[0x7e0000, 0x7e2000)` of the 8-MiB QSPI device | Lost/wrong saved choice; flash wear |
| Execution memory | AXI application RAM `0x24000000`, DMA `[0x30000000, 0x30008000)`, staging `[0x30008000, 0x30040000)`, 1-KiB trampoline at `0x38000000` | Corrupted code/stack, arbitrary execution, bad peripheral commands |
| Host files and published assets | User-selected originals, local output and release inventory | Lost files, disclosed vendor binaries, installation of the wrong artifact |

## Failure families

Each plan below separates prevention from detection and recovery. Existing
checks are useful only while their implementation, inputs and lower layers meet
their contracts. Proposed checks are not claimed to exist already.

### 1. A flash operation reaches the wrong place

**Assume:** an address is a mapped pointer instead of a chip offset; arithmetic
wraps; an erase end is interpreted differently; a page crosses a boundary; an
opcode, device geometry or status-register value is wrong; a dependency ignores
an error; or code bypasses the checked driver.

**Worst outcome:** erase calibration, installed code or the entire chip; damage
the internal loader/configuration through a separate programming path; change
protection bits so normal writes or recovery stop working. Counting writes alone
would miss a single destructive write to the wrong address.

**Current protection:** `ChoiceFlashDriver` accepts only the two journal sector
starts for erasure, and aligned 80-byte records in their range for programming.
The journal checks results and readback. These guards precede calls into the
pinned SDK, whose command construction and mode/status changes remain trusted.
They do not constrain arbitrary privileged code or vendor firmware.

**Plan:** retain the small driver and test its production adapter, then observe
the actual QSPI commands below the SDK. Independently check opcode, offset,
address width, transfer length, page/sector boundaries and status-register
effects. Invalid requests must emit no write-enable/program/erase commands.
Use overflow-safe half-open ranges and separate policies for installation and
normal selection. A runtime guard must run before the command it constrains;
a test-only observer establishes evidence but cannot prevent damage on a module.
If lower-layer command construction needs its own guard, keep it small and in
the owning layer rather than duplicating the whole flash implementation.

**Acceptance:** inject wrong offsets (including mapped addresses), lengths,
opcodes and success/error returns at each relevant boundary. Require zero
unauthorized commands and unchanged bytes outside the journal. Deliberately
remove the guard and verify that an independent observer fails the test. Include
initialization/status writes, not just array writes. Do not silently modify
protection bits as a generic recovery mechanism.

**Limit:** this proves an exercised path, not containment of code that bypasses
it. Independent write restriction would require a separately enforced boundary;
see the final section.

### 2. Installation is incomplete or erases too much

**Assume:** the preparer emits a bad extent, the drive contains a different BIN,
the updater changes its erase convention, a read is short, or power fails during
installation.

**Worst outcome:** lose the menu and all installed applications, overwrite the
journal or reserved data, or report success with an incomplete installation.
Updating in place is not an atomic transaction.

**Current protection:** fixed slots, size limits and a reserved erase margin;
the reviewed stock loader normally writes from `0x90040000`. Its known EOF and
inclusive-sector behavior is documented in [stock updater assumptions](stock_updater.md).
It does not check every failure or provide rollback. A selector-side check runs
too late to prevent an updater mistake.

**Plan:** independently decode the final private installer and model the exact
qualified loader's erase/program commands. Exercise lengths around page,
sector, 64-KiB and EOF boundaries, including the extra erase. Cover full/subset
bundles, replacement with shorter images, old layouts, truncated reads and
interruption at each destructive phase. Treat a different loader identity as a
new qualification requirement. Keep one intended BIN on installation media.

**Acceptance:** every permitted installer leaves reserved bytes and journal
outside the updater's erase footprint. After interruption, either a fully
authenticated application starts or ordinary stock USB recovery can restore it.
The latter requires physical qualification; simulation cannot establish it.
No promise of uninterrupted availability or rollback is made.

### 3. The remembered choice is lost, ambiguous or repeatedly rewritten

**Assume:** a program/erase tears, flash reads or success statuses lie, generation
comparison is wrong, old records refer to removed firmware, or the controller
saves on every boot/button bounce.

**Worst outcome:** boot the wrong valid image, lose a choice, fail to reach the
menu, or wear flash prematurely. Automatic write retries could amplify a fault.

**Current protection:** two alternating sectors, page-aligned CRC records,
generation conflict handling, exact image hashes, readback and sticky save
failure. An ambiguous journal yields no saved choice. Authentication precedes
saving. Automatic boots and repeat selections do not save. A failed save warns
but allows the verified image to run for that session.

**Plan:** retain the meaningful existing cut/erase, wrap/conflict, serialization,
repeat-save and mutation tests. Add bounded state exploration against an
independent journal model for rollover, mixed valid/torn records, removed images
and dishonest driver results. Check boot choice as well as record bytes. Observe
commands over whole boot/menu cycles to catch controller-induced writes.

**Acceptance:** under the stated torn-write model, a reboot chooses the previous
or completely committed new image, or enters the menu when no unambiguous
supported choice exists; it never fabricates a choice. A changed successful
selection programs one 80-byte record, erasing only when recycling. Repeats and
automatic boots add zero selector array writes. Failed saves do not retry in a
loop. Measure SDK status writes separately from array writes and stock-loader
activity.

**Limit:** the sectors share one chip, controller and power supply. Prefix-cut
tests do not establish survival of arbitrary brownout corruption, endurance
failure or coherent corruption of both data and CRC. Physical power-cut tests
and conservative recovery behavior remain necessary.

### 4. Wrong bytes or wrong addresses are accepted for execution

**Assume:** the catalog, slot math, authentication, copy, linker layout or wrapper
metadata is wrong; staged bytes change; an optional image is absent; or an
attacker replaces a preparer together with its own hashes.

**Worst outcome:** execute arbitrary or malformed code with privileged hardware
access. Even a valid image can be the wrong user choice or wrong board version.
An intact selector cannot protect a corrupted wrapper that runs before it.

**Current protection:** exact vendor identity and vectors, bounds, resident
reset-word/wrapper checks, staged-image authentication and a second handoff-time
check; wrapper copy checksum and vector checks; linker assertions. Slot-vector
presence checks alone are not launch authorization.

**Plan:** keep the repeated authentication across the handoff boundary. Add an
independent final ELF/BIN/installer checker using reviewed physical limits rather
than importing all production constants. Check section load/run ranges, overlaps,
entry points, resident patch, metadata and each supplied/absent slot. Reject
unknown images and malformed templates in both preparers. Inject corruption
before and after staging and during the final copy, including destination bytes.
Model coherent wrong constants, not just a one-byte corrupt input.

**Acceptance:** no unauthenticated image reaches the execution boundary in the
tested fault model. Corruption after the last existing check is explicitly
detected by a new check or recorded as an uncovered interval. A destination
readback is detection, not protection against an already incorrect copy address.
If critical wrapper/copy instructions change, review their emitted instructions
and affected execution paths, not just the C++ tests.

**Limit:** checks derived from the same mistaken constants can agree with each
other. Neither local checksums nor self-contained hash manifests establish
publisher trust or prevent corruption of the checking code itself.

### 5. Handoff leaves live hardware or invalid processor state

**Assume:** a DMA engine keeps writing, interrupts fire at the wrong vector table,
code/cache/stack state is stale, clocks are incompatible, or cleanup fails after
the selector is already dismantled.

**Worst outcome:** overwrite verified code, jump through invalid vectors, hang,
or produce incorrect/loud audio and unintended pin activity. Correct bytes alone
do not imply correct execution.

**Current protection:** validate before teardown, mask interrupts, reset known
bus masters, deinitialize runtime, recheck staged bytes, install/read back the
separate stackless trampoline, set execution state and clear the DMA arena.
Failures after teardown halt rather than return to an unusable UI.

**Plan:** inject active DMA, pending interrupts, poisoned RAM and inherited
peripheral/cache states. Observe bus writes through the entry boundary and assert
the final VTOR, MSP, Thumb entry, masks and permitted memory writes. Cover both
resident and copied applications. Keep per-firmware inherited-state requirements
explicit; indiscriminately resetting everything can break closed-source startup.

**Acceptance:** no old bus master writes into execution/staging memory after
cleanup; terminal failure never returns into the torn-down menu; both launch
routes satisfy the measured entry contract. Compare stock and selector behavior
for affected images, then separately qualify controls/audio and timing on hardware.
Audio comparison remains paused; this document claims no new audio result.

### 6. Startup or user interaction fails

**Assume:** a button sticks/bounces, startup release is mistaken for a press,
selection mapping changes, a timer/peripheral wait never completes, or the saved
application always crashes.

**Worst outcome:** unintended selection/save, unusable menu, permanent boot loop,
silence or uncontrolled output. An error LED is not useful if initialization
hangs before it can run.

**Current protection:** stable held Shift + Reverse enters the menu; menu actions
use press edges; invalid selections are rejected. Errors before handoff can stay
in the menu; some low-level failures intentionally halt.

**Plan:** test held/released/stuck inputs, missing slots, simultaneous inputs and
timebase/wait failures at startup. Keep a deterministic menu escape before
automatic launch. Bound waits where the underlying hardware contract permits it.
Consider a watchdog only with an explicit recovery policy: never retry writes
forever or silently launch a different image. A persistent crash counter would
itself introduce writes and another failure mode; do not add it without need.

**Acceptance:** the intended combination reliably reaches the menu on the exact
hardware, releasing it does not navigate/save, and navigation cancels automatic
boot. Tests distinguish a safe halt from a usable menu. Recovery remains possible
when the application cannot run; neither outcome guarantees continuous audio.

### 7. A launched vendor application behaves badly

**Assume:** an exact accepted vendor image writes flash, changes clock/power/GPIO
state, hangs, or drives invalid audio. Its own bug can execute arbitrary writes.

**Worst outcome:** any loss available to privileged application code, including
calibration or recovery damage. A selector write counter cannot attribute all
module writes to the selector.

**Current protection:** admission by exact image identity and per-image review.
There is no sandbox after handoff and no source-level proof of vendor behavior.

**Plan:** qualify each accepted image's observed writes and peripheral behavior
against stock operation, separately attributing loader, selector and application
commands. Requalify changed identities. Document required firmware-owned assets
and settings. Keep recovery and backups independent of the installed image.

**Acceptance:** no unexplained writes or output differences in the defined
workloads; preserve raw differences instead of relaxing comparisons to pass.
This is bounded evidence, not a proof for every possible application input.
Preventing arbitrary payload writes requires independent containment.

### 8. Preparation, builds or releases produce the wrong artifact

**Assume:** Python and browser assembly disagree, tooling/dependencies are wrong,
a stale binary is packaged, a release mixes identities, host scripts overwrite
files, or a private vendor-containing installer is published.

**Worst outcome:** distribute a broken or untrusted installer, disclose vendor
code, lose local files, or transfer a validation claim to untested bytes.
Replacing the HTML also replaces its local checks; its own hashes cannot prove
who supplied it.

**Current protection:** exact original hashes, pinned clean isolated builds,
ELF/BIN agreement, sealed inventories, browser/Python equivalence tests and a
release inventory containing the preparer rather than vendor originals.

**Plan:** test the final downloadable HTML with supported, changed, missing,
duplicate and oversized inputs. Independently decode its output and compare
assembly byte-for-byte with the Python tool for full and subset catalogs. Keep
host outputs in explicit artifact directories and test refusal to overwrite
unrelated files where applicable. Check final uploaded inventory and identity,
not only pre-upload files. For stronger origin assurance, use an independently
trusted release verification channel/key; a checksum beside an artifact alone
is insufficient. Keep generated reports and vendor installers ignored locally.

**Acceptance:** public assets contain no vendor firmware; exact final artifacts
are bound to the reviewed source, tools and affected checks; unsupported inputs
produce no installer. No upload or hardware action is an implicit preparer step.
Browser resource exhaustion should fail preparation without damaging originals.

### 9. Future changes invalidate the safeguards themselves

**Assume:** a new firmware changes geometry, a library changes erase semantics,
a new initialization path writes calibration, a test uses the same wrong helper,
or CI skips a required check.

**Worst outcome:** every preceding failure passes a green pipeline, potentially
with convincing but irrelevant evidence.

**Plan:** require independent physical-limit expectations and a few deliberately
broken negative controls at the dangerous boundaries: wrong write range/opcode,
authentication bypass, bad handoff ordering and repeated save. Fail a control
only when its intended assertion fires, not when compilation or setup fails.
Check the final production build as well as host helpers. Changes to layout,
SDK, wrapper, handoff, catalog or installation require affected regressions and
a fresh authenticated candidate. Review changes to the gates themselves as
changes to the safety policy. Protect required checks/review in repository
settings where practical, recognizing that an administrator can change them.

**Acceptance:** each negative control defeats a relevant safeguard and is caught
by an independent oracle; skipped/failed checks cannot masquerade as passes.
Hardware/recovery claims stay attached to the tested artifact and board.
No gate proves a different future build safe.

## Implementation order

1. **Constrain and observe writes.** Extend existing driver tests and command
   observation to prove addresses and opcodes as well as counts. Keep normal
   operation separate from stock installation and vendor activity. This targets
   the most irreversible losses with the smallest change.
2. **Independently check final artifacts and updater extents.** Reuse existing
   package tests; add a small physical-layout oracle rather than another build
   framework. Include common-mode wrong constants and EOF erase cases.
3. **Extend existing fault tests at uncovered transitions.** Prioritize journal
   commit/recycle, post-check copy corruption, DMA cleanup and menu escape.
   Avoid duplicating tests of harmless implementation details.
4. **Qualify physical failure/recovery paths under separate authorization.** Use
   the [physical release gate](recovery_release_gate.md): exact larger-bundle
   installation, no-drive boot, controls/audio, interrupted saves/installations,
   calibration preservation and stock restoration. Virtual passes cannot replace
   these checks. A backup/readback can aid recovery; it does not prevent damage.

For each functional change, first preserve a failing local reproducer, then
build a fresh authenticated candidate and run affected regressions. Store raw
reports in ignored local artifact storage. This plan does not authorize hardware
access and does not resume audio testing.

## If independent containment is required

Software checks are the simplest practical improvement for the current design.
They do not meet a requirement that arbitrary privileged future code must be
unable to destroy protected state.

That stronger requirement needs an authority the changed code cannot override:
for example, a hardware write restriction or a separately protected supervisor
that owns programming access. Establish feasibility on the exact MCU, flash,
wiring, loader and vendor execution model before proposing implementation. Flash
protection geometry, persistence, quad-mode pins, status writes and ordinary USB
restoration all need review. Do not set readout protection, option bytes or flash
locks speculatively; they can impair recovery or cause data loss.

An MPU configuration alone is insufficient for this architecture: payloads run
privileged, and DMA also needs containment. ST explicitly states that MPU
attributes apply to CPU accesses rather than other bus masters such as DMA
([ST AN5156](https://www.st.com/content/st_com/en/technical-documents/an5156-introduction-to-security-for-stm32-mcus-stmicroelectronics_asset/chunk-008-topic-an5156-sec-feat-firewall.html)).
Running unchanged vendor applications unprivileged would be a substantial
compatibility redesign, not a small guard around the existing handoff.

Even independent containment would protect specified assets under specified
fault assumptions, not guarantee uninterrupted operation, correct audio, or
survival of every physical failure. Choose those guarantees explicitly before
adding that complexity.
