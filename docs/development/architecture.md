# Source map

For the overview and rationale, read [how it works](../how_it_works.md).

| File / directory | Responsibility |
| --- | --- |
| [firmware/selector.cpp](../../firmware/selector.cpp) | USB setup, discovery, buttons, LED states, staged loading |
| [firmware/images.hpp](../../firmware/images.hpp) | Exact-image catalog and authentication predicate |
| [firmware/handoff_sequence.hpp](../../firmware/handoff_sequence.hpp) | Validation and irreversible cleanup ordering |
| [firmware/handoff.cpp](../../firmware/handoff.cpp) | Hardware cleanup, final verification, trampoline installation |
| [firmware/copy_jump.s](../../firmware/copy_jump.s) | Stackless payload copy, DMA clearing, target entry |
| [support/](../../support/) | Small staging, menu, vector, initialization, and timing helpers |
| [scripts/](../../scripts/) | Pinned dependency setup, payload preparation, isolated packaging |
| [tests/](../../tests/) | Portable host tests; compiled C++ probes live in `firmware/` |

The `.ld` fragments assert staging and trampoline placement. USB/FatFs objects
use the SDK's DMA-accessible arena; ordinary globals and the stack use DTCM.

Default builds use real USB/FatFs. The backed reader and `--virtual` package
are synthetic development facilities, not installable firmware.
No emulator or payload binaries are distributed here.

See the [development guide](README.md) for commands and onboarding.

## Memory layout

| Region | Purpose |
| --- | --- |
| AXI SRAM, `0x24000000` | Selector execution; overwritten by the payload |
| DTCM, `0x20000000` | Globals and stack; inaccessible to USB DMA |
| D2, `[0x30000000,0x30008000)` | SDK DMA arena; cleared at terminal handoff |
| D2, `[0x30008000,0x30034680)` | 181888-byte staging buffer; outside the cleared arena |
| SRAM4, `[0x38000000,0x38000400)` | Reserved trampoline space |
| SDRAM/QSPI | Bootloader mappings retained; not globally wiped or reconfigured |

## Handoff invariants

- Initialization starts USB, buttons, and LEDs, not ADC, audio, or persistence.
  It avoids the full SDK hardware initializer because that can save calibration
  defaults and reconfigure external memory.
- Discovery overwrites staging but cannot authorize launch. Freeze must load the
  entire selected file, close it successfully, and verify its hash/vectors.
- Shift revalidates those same bytes before teardown and again afterward.
  It never launches a separately reopened, unchecked file.
- Cleanup unmounts/stops USB, masks interrupts, resets scoped DMA/USB/I2C/SAI
  peripherals, and clears SysTick/NVIC enabled and pending state.
- MPU, clocks, and FMC/QSPI configuration remain for `BOOT_SRAM` startup.
  This is not a hardware reset; targets must initialize memory they read.
- Data caching is disabled for USB-DMA coherence. Runtime teardown disables
  caches before the copy; physical cache/DMA timing still needs testing.
- The trampoline has no stack or calls into the code it overwrites. It copies
  the exact payload, clears the DMA arena, sets vectors/stack/control registers,
  enables interrupts, and branches to the reset handler.
- Failure before teardown refuses launch. Failure afterward halts for reset;
  it cannot safely return to the partially dismantled selector.

There is no overall storage deadline. Upstream USB/FatFs calls can stall.
Launched firmware is not sandboxed and may write its own persistent settings.
