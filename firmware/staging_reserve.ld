/* Reuse pinned libDaisy RAM_D2; keep first 32 KiB for SDK DMA buffers.
 * Only SRAM1/2 are used by staging. No SDRAM initialization is required. */
SECTIONS
{
    .selector_staging (NOLOAD) :
    {
        . = ALIGN(32);
        selector_staging_start = .;
        KEEP(*(.selector_staging))
        . = ALIGN(32);
        selector_staging_end = .;
    } > RAM_D2
}
ASSERT(selector_staging_start == 0x30008000, "Unexpected selector staging base")
ASSERT(selector_staging_end <= 0x30040000, "Selector staging exceeds SRAM1/2")
