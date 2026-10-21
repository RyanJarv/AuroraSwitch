#include "../support/selector_constants.h"
/* Reserve SRAM4 for code that must survive overwriting the selector in AXI SRAM. */
SECTIONS
{
    .selector_trampoline (NOLOAD) : ALIGN(32)
    {
        selector_trampoline_ram = .;
        . += SELECTOR_TRAMPOLINE_SIZE;
        selector_trampoline_limit = .;
    } > RAM_D3
}
ASSERT(ADDR(.selector_trampoline) == SELECTOR_TRAMPOLINE_BASE,
       "Selector SRAM4 trampoline placement changed")
ASSERT(SIZEOF(.selector_trampoline) == SELECTOR_TRAMPOLINE_SIZE,
       "Selector SRAM4 reservation changed")
