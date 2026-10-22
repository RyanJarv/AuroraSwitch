# Failure modes and prevention priorities

Prioritize lasting harm caused by AuroraSwitch: lost calibration/settings,
flash wear, altered protection/boot configuration, damaged hardware or loss of
ordinary stock USB recovery. USB-recoverable crashes, failed launches and audio
faults remain correctness issues but do not justify rollback systems, watchdogs
or a sandbox on their own.

## Responsibility and baseline

Our scope includes the selector, catalog, package, USB loading/settings, RAM
handoff and consequences of our dependency calls. Compare the same exact vendor
image under equivalent stock execution, hardware, inputs and settings. Vendor
behavior that would also happen stock is outside prevention scope. Additional
writes or harmful behavior caused by our loading, layout or inherited state
remain in scope even when the vendor issues the instruction.

We do not own unused-looking flash outside a vendor executable. The former
bundle layout, resident Dirt wrapper and journal depended on that unsupported
assumption. The current design installs only the selector at the stock
application entry and keeps originals/settings on USB. Compression or shorter
images would not establish ownership or resolve Dirt's fixed flash address.

## Consequential failures

| Failure | Required behavior and evidence |
| --- | --- |
| Our initialization or dependencies program/erase flash or change protection | Avoid DaisySeed/Hardware initialization and persistent storage; initialize only System, panel and USB. Observe actual production commands before handoff, separating the stock loader baseline. No selector array/status/protection writes are allowed. Check internal-flash controller activity too. |
| Changed, truncated, unsupported or flash-running file | Exact catalog hash, size and RAM vectors are required. Bound reads before staging. No filename-only admission and no patched vendor binaries. Reject without saving or launching. |
| Staged bytes change during USB/settings/peripheral cleanup | Hash staged bytes again at launch and after teardown. Reset memory-writing peripherals; verify the SRAM4 routine before replacing the selector. Post-teardown failure is terminal. |
| USB absent, disconnected, unreadable or empty | Keep menu/error state; process USB insertion and allow Freeze retry. Never automatically substitute an image, spin on filesystem operations or retry saves. |
| Choice file damaged, truncated or unsupported | Require exact version/size/hash/CRC; missing or invalid records open the menu. This file alone never authenticates firmware. |
| Choice save fails or power is removed | One explicit attempt; check write/sync/close/readback, warn and allow verified session launch. A torn save may lose the remembered choice. No on-module write and no automatic retry loop. FAT corruption can affect USB assets, so retain backups. |
| Automatic/repeat selection wears USB unnecessarily | Automatic boots never save; repeat choices validate the existing file without writing it. Count opens-for-write as well as data writes in tests. |
| Handoff changes vendor persistent behavior | Preserve required clock/MPU/startup contract; reset DMA/USB and clear DMA RAM before entry. Compare stock-relative persistent commands and bytes in affected startup paths. Matching startup does not cover all later workloads. |
| Vendor overwrites selector application area | Still review supported images for preservation of that area. RAM execution does not prove flash ownership; test startup and relevant settings/workloads against stock. Do not claim permanent selector retention from binary size alone. |
| Installation interrupted or selector unusable | Stock USB restoration must work independently of selector/menu access. Preserve bootloader, calibration and configuration. Physical verification remains required; local updater models are only bounded evidence. |
| Release accidentally includes originals | Package only our ELF/BIN/MAP and provenance/checksums. Release inventory must exclude vendor originals/composite installers. |

## Validation plan

1. Exercise production controller and FatFs operations: absent/reinserted USB,
   corrupt settings, exact-size reads, save failures, repeats and idle periods.
   Use negative controls for important admission and write-suppression checks.
2. Build clean authenticated selector-only artifacts. Launch exact originals
   through the production RAM path in virtual tests; reject modified/short/
   unsupported/flash images and post-load mutations. Observe flash commands,
   complete NOR readback and handoff vectors/stack/DMA. Filesystem boundary
   simulation must be labeled; it is not physical USB enumeration evidence.
3. Compare vendor startup/persistent behavior with stock where the handoff may
   matter. Preserve raw differences. Vendor writes also observed stock are not
   selector faults; additional effects need investigation before acceptance.
4. When separately authorized, perform the [physical recovery gate](recovery_release_gate.md).
   Local/virtual tests do not qualify electrical behavior, power interruption,
   USB interoperability, audio or stock restoration. Audio comparisons are paused.

The selector and vendor run privileged. Software checks bound our supported
paths, not arbitrary vendor code or a compromised device. Do not claim universal
containment or add broad infrastructure solely for hypothetical vendor faults.
