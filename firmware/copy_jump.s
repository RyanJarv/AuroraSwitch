#include "../support/selector_constants.h"
.syntax unified
.cpu cortex-m7
.thumb

// Start the chosen firmware without returning to the selector. RAM firmware
// overwrites the selector itself, so this routine first runs from separate SRAM4.
// It uses no stack or external calls that could refer to overwritten memory.
// Before calling: verify staged bytes, stop DMA/peripherals that can write RAM,
// clean/disable caches, mask interrupts and copy this routine into SRAM4.
// R0 = staged source, R1 = AXI destination, R2 = nonzero exact image length.
.section .selector_copy_jump,"ax",%progbits
.balign 4
.global selector_copy_jump_blob_start
.global selector_copy_jump_start
.global selector_copy_jump_end
// Even data label for copying; the function symbol itself has Thumb bit set.
selector_copy_jump_blob_start:
.thumb_func
selector_copy_jump_start:
    cpsid i
    cbz r2, invalid_length
    mov r4, r1
copy_byte:
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    subs r2, r2, #1
    bne copy_byte
    // Clear the RAM that the next firmware expects to start empty.
    // Pinned Daisy SRAM linker: RAM_D2_DMA = [0x30000000, 0x30008000).
    // Staging begins at 0x30008000, outside this arena. Teardown has already
    // stopped peripherals that can write RAM and cleaned/disabled caches. Do not erase live
    // selector data earlier; this stackless terminal phase never returns.
    ldr r0, =SELECTOR_DMA_BASE
    ldr r1, =SELECTOR_DMA_END
    movs r3, #0
clear_dma_word:
    str r3, [r0], #4
    cmp r0, r1
    bne clear_dma_word
    dsb sy
    isb sy
    ldr r3, =SELECTOR_VTOR_REGISTER
    str r4, [r3]
    ldr r5, [r4]
    ldr r6, [r4, #4]
    movs r0, #0
    msr control, r0
    msr basepri, r0
    msr faultmask, r0
    msr psp, r0
    msr msp, r5
    dsb sy
    isb sy
    // If an unexpected target reset-handler return occurs, stay in SRAM4.
    adr r7, invalid_length
    adds r7, r7, #1
    mov lr, r7
    // NVIC/SysTick must already be disabled by the caller.
    cpsie i
    bx r6
invalid_length:
    b invalid_length
    .ltorg
selector_copy_jump_end:
