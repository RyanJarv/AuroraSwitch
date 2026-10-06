/* Additional script after the upstream BOOT_SRAM layout: real reservation. */
SECTIONS
{
    .selector_trampoline (NOLOAD) : ALIGN(32)
    {
        selector_trampoline_ram = .;
        . += 1024;
        selector_trampoline_limit = .;
    } > RAM_D3
}
ASSERT(ADDR(.selector_trampoline) == 0x38000000,
       "Selector SRAM4 trampoline placement changed")
ASSERT(SIZEOF(.selector_trampoline) == 1024,
       "Selector SRAM4 reservation changed")
