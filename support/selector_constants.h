#pragma once

// Selector hardware and binary-format constants, shared by C++, assembly,
// preprocessed linker scripts and the Python build tools. Keep definitions as
// integer literals so all four consumers read the same values.

// Fast CPU-only RAM holds the firmware's stack (128 KiB DTCM). The startup
// vector uses its upper end as the initial main stack pointer (MSP). The stack
// grows downward, so that initial pointer can equal the end address.
#define SELECTOR_DTCM_BASE 0x20000000
#define SELECTOR_DTCM_END 0x20020000
// Main program RAM starts here. Both the selector and RAM-loaded firmware
// are linked to run at this AXI SRAM address (Daisy BOOT_SRAM layout).
#define SELECTOR_AXI_SRAM_BASE 0x24000000
// Keep loaded firmware separate from peripheral transfer buffers. The SDK uses
// the first 32 KiB of D2 SRAM for DMA, where peripherals read/write memory
// directly. The rest of SRAM1/2 stages the image; the final jump clears DMA only.
#define SELECTOR_DMA_BASE 0x30000000
#define SELECTOR_DMA_END 0x30008000
#define SELECTOR_STAGING_BASE 0x30008000
#define SELECTOR_STAGING_END 0x30040000
// SRAM4 survives replacement of AXI code; reserve 1 KiB for the jump routine.
#define SELECTOR_TRAMPOLINE_BASE 0x38000000
#define SELECTOR_TRAMPOLINE_SIZE 1024
// Tell the CPU where to find the new firmware's interrupt vectors: the
// Cortex-M7 System Control Block vector-table offset register (VTOR).
#define SELECTOR_VTOR_REGISTER 0xe000ed08
// IS25LP064A memory mapping (8 MiB). The Daisy loader's application starts
// 256 KiB into flash, preserving the loader-owned settings/calibration area.
#define SELECTOR_QSPI_BASE 0x90000000
#define SELECTOR_QSPI_END 0x90800000
#define SELECTOR_QSPI_APPLICATION_BASE 0x90040000
#define SELECTOR_QSPI_APPLICATION_OFFSET 0x40000
// CRC-32/ISO-HDLC reflected polynomial used for saved-choice records.
#define SELECTOR_BOOT_CRC_POLYNOMIAL 0xedb88320
// Word arithmetic and NVIC write-one masks have different meanings despite
// sharing the same numeric value.
#define SELECTOR_U32_MASK 0xffffffff
#define SELECTOR_NVIC_ALL_INTERRUPTS 0xffffffff
// PCA9685 address-pin bits; libDaisy ORs these with its I2C base address 0x40.
#define SELECTOR_LED_DRIVER_A 0x00
#define SELECTOR_LED_DRIVER_B 0x01

// Pinned stock updater reads/erases application files in 64-KiB blocks.
#define SELECTOR_UPDATER_BLOCK_SIZE 65536
// USB choice format: eight version bytes, catalog SHA-256 text, CRC-32.
#define SELECTOR_CHOICE_FILE_SIZE 76
