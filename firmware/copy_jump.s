.syntax unified
.cpu cortex-m7
.thumb

// Isolated terminal handoff primitive, NOT peripheral teardown.
// Caller must first authenticate staged bytes, quiesce every bus master,
// disable/clean caches and interrupts, and copy this blob to executable SRAM4.
// R0 = staged source, R1 = AXI destination, R2 = nonzero exact image length.
// No stack, external calls or references to overwritten selector storage.
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
#ifdef SELECTOR_DMA_ARENA_CLEANUP
    // Pinned Daisy SRAM linker: RAM_D2_DMA = [0x30000000, 0x30008000).
    // Staging begins at 0x30008000, outside this arena. Teardown has already
    // reset bus masters and cleaned/disabled caches. Do not erase live
    // selector data earlier; this stackless terminal phase never returns.
    ldr r0, =0x30000000
    ldr r1, =0x30008000
    movs r3, #0
clear_dma_word:
    str r3, [r0], #4
    cmp r0, r1
    bne clear_dma_word
#endif
    dsb sy
    isb sy
    ldr r3, =0xe000ed08
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
