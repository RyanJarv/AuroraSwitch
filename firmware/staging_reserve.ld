#include "../support/selector_constants.h"
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
ASSERT(selector_staging_start == SELECTOR_STAGING_BASE, "Unexpected selector staging base")
ASSERT(selector_staging_end <= SELECTOR_STAGING_END, "Selector staging exceeds SRAM1/2")

/* The pinned SRAM linker places initialized .data last in the installer.
 * This extent lets USB backup verification compare the complete installed BIN. */
selector_installer_size = LOADADDR(.data) + SIZEOF(.data) - ADDR(.isr_vector);
ASSERT(selector_installer_size > 0 && selector_installer_size <= LENGTH(SRAM),
       "Unexpected selector installer extent")
ASSERT(SIZEOF(.qspiflash_text) == 0 && SIZEOF(.qspiflash_data) == 0,
       "Selector installer must contain only SRAM-linked application data")

/* Installer verification reads the unchanged AXI load image. Startup must copy
 * mutable globals to DTCM rather than modifying bytes inside that image. */
ASSERT(ADDR(.data) >= SELECTOR_DTCM_BASE &&
       ADDR(.data) + SIZEOF(.data) <= SELECTOR_DTCM_END,
       "Selector initialized globals must run in DTCM")
ASSERT(ADDR(.bss) >= SELECTOR_DTCM_BASE &&
       ADDR(.bss) + SIZEOF(.bss) <= SELECTOR_DTCM_END,
       "Selector BSS must stay outside the AXI installer image")
