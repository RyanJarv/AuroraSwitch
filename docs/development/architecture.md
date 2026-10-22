# Source map

See [How it works](../how_it_works.md) for the design and memory contract.

| Source | Responsibility |
| --- | --- |
| `firmware/images.hpp` | Exact RAM image identities and shared presentation metadata |
| `firmware/selector.cpp` | Minimal hardware initialization, USB host, panel and authentication |
| `firmware/selector_controller.hpp` | Startup, one-shot automatic launch and explicit retries |
| `firmware/usb_files.hpp` | Concrete FatFs discovery, bounded reads and checked choice-file saves |
| `support/usb_choice.hpp` | Version/hash/CRC choice format, independent of menu positions |
| `firmware/handoff_sequence.hpp` | Validation and cleanup ordering |
| `firmware/handoff.cpp` | Peripheral cleanup, staged-byte rechecks and trampoline validation |
| `firmware/copy_jump.s` | Stackless RAM copy, DMA clearing, VTOR/MSP and terminal jump |
| `support/selector_constants.h` | Shared hardware bounds and binary-format constants |
| `scripts/package.py`, `scripts/sealed_bundle.py` | Reproducible selector-only build and authentication |

The linker reserves staging and SRAM4. USB/FatFs transfer buffers use the DMA
arena because USB DMA cannot reach DTCM. No transport macros or alternate
firmware modes exist for tests; native tests supply the FatFs boundary and
virtualization lives in its owning repository.
