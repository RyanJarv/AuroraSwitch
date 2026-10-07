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

Memory layout, launch invariants, and cleanup ordering are documented once in
[how it works](../how_it_works.md#memory-layout).
