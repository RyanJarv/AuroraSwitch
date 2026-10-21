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
// Generic writer bounds: at most 224 KiB, ending at flash offset 0x78000.
#define SELECTOR_QSPI_MAXIMUM_END 0x78000
#define SELECTOR_QSPI_MAXIMUM_IMAGE_SIZE 0x38000
// Synthetic transport backing memory only; this is not physical SDRAM setup.
#define SELECTOR_VIRTUAL_FILE_BASE 0xc0000000

// Private composite installer: Dirt stays at the ordinary application base.
// Its power-on vector enters the wrapper (Thumb bit set), followed by a
// 16-byte metadata record and the saved selector image. Capacity is the
// reviewed maximum selector size, independent of the QSPI writer's bound.
#define SELECTOR_RESIDENT_BASE 0x90040000
#define SELECTOR_RESIDENT_ENTRY 0x90058001
#define SELECTOR_RESIDENT_METADATA 0x90058100
#define SELECTOR_RESIDENT_IMAGE 0x90059000
#define SELECTOR_RESIDENT_CAPACITY 0x78000
// Little-endian ASCII "ASR1" identifies the wrapper metadata format.
#define SELECTOR_WRAPPER_MAGIC 0x31525341
// Catch damage while copying the saved selector back into RAM (FNV-1a-32).
// This fast checksum is not image authentication; full-file SHA-256 identifies
// an approved firmware before launch.
#define SELECTOR_FNV_OFFSET_BASIS 2166136261
#define SELECTOR_FNV_PRIME 16777619
// CRC-32/ISO-HDLC reflected polynomial used for the two USB boot records.
#define SELECTOR_BOOT_CRC_POLYNOMIAL 0xedb88320
// Saved-choice generations wrap at 32 bits. A difference of half that range
// cannot tell us which record is newer, so it must be treated as ambiguous.
#define SELECTOR_GENERATION_HALF_RANGE 0x80000000
// Word arithmetic mask, unavailable diagnostic sentinel, and NVIC write-one
// masks have different meanings despite sharing the same numeric value.
#define SELECTOR_U32_MASK 0xffffffff
#define SELECTOR_DIAGNOSTIC_UNAVAILABLE 0xffffffff
#define SELECTOR_NVIC_ALL_INTERRUPTS 0xffffffff
// PCA9685 address-pin bits; libDaisy ORs these with its I2C base address 0x40.
#define SELECTOR_LED_DRIVER_A 0x00
#define SELECTOR_LED_DRIVER_B 0x01
