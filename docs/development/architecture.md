# Source map

The installed layout and launch contract are in [How it works](../how_it_works.md).

| Source | Responsibility |
| --- | --- |
| `firmware/images.hpp` | Exact supported identities and shared presentation metadata |
| `support/flash_bundle.hpp` | Stateless catalog-slot lookup and vector-based menu presence |
| `firmware/selector.cpp` | Panel, staged authentication and storage adapter |
| `firmware/selector_controller.hpp` | Startup, remembered boot and manual selection |
| `support/choice_journal.hpp` | Exact saved identity, CRC and two-sector append/recycle persistence |
| `firmware/flash_driver.hpp` | Inherited mapping and choice-only hardware writes |
| `firmware/handoff_sequence.hpp` | Validation and cleanup ordering |
| `firmware/handoff.cpp` | Peripheral cleanup and final SRAM/QSPI entry |
| `firmware/copy_jump.s` | Stackless copy, DMA clearing, VTOR/MSP and terminal jump |
| `firmware/resident_boot.s` | Power-on copy and check of the selector |
| `scripts/sealed_bundle.py` | Immutable build authentication and source-bound layout |
| `scripts/prepare_bundle.py` | Offline preparer export and reference assembly |
| `scripts/bundle_preparer.html` | Local input authentication and single-file assembly |

The linker fragments reserve staging and separate SRAM4 jump code. The selector
has no filesystem, USB transport, synthetic-media build or runtime application
programmer. Linked tests execute the production build with modeled NOR.
