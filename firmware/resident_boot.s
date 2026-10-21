// This position-independent blob is copied into the local composite installer,
// not executed at its linked SRAM address. Dirt's reset vector points here.
#include "../support/resident_layout.h"
.syntax unified
.cpu cortex-m7
.thumb
.section .selector_resident_boot,"a",%progbits
.balign 4
.global selector_resident_boot_start
.global selector_resident_boot_end
selector_resident_boot_start:
    cpsid i
    ldr r0, =SELECTOR_RESIDENT_METADATA
    ldr r2, [r0]
    ldr r3, =0x31525341 // ASR1
    cmp r2, r3
    bne resident_invalid
    ldr r2, [r0, #4]
    cmp r2, #8
    bls resident_invalid
    ldr r3, =SELECTOR_RESIDENT_CAPACITY
    cmp r2, r3
    bhi resident_invalid
    ldr r3, [r0, #8]
    mvns r3, r3
    cmp r2, r3
    bne resident_invalid
    ldr r7, [r0, #12]
    ldr r0, =SELECTOR_RESIDENT_IMAGE
    ldr r1, =0x24000000
    ldr r5, =2166136261
    ldr r6, =16777619
    mov r8, r2
resident_copy:
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    subs r2, r2, #1
    bne resident_copy
    dsb sy
// Validate the SRAM bytes that will execute, not the flash bytes just read.
resident_readback_start:
    ldr r0, =0x24000000
    mov r2, r8
resident_readback:
    ldrb r3, [r0], #1
    eors r5, r5, r3
    mul r5, r5, r6
    subs r2, r2, #1
    bne resident_readback
    cmp r5, r7
    bne resident_invalid
    ldr r4, =0x24000000
    ldr r5, [r4]
    ldr r3, =0x20020000
    cmp r5, r3
    bne resident_invalid
    ldr r6, [r4, #4]
    tst r6, #1
    beq resident_invalid
    cmp r6, r4
    bls resident_invalid
    cmp r6, r1
    bhs resident_invalid
    dsb sy
    isb sy
    ldr r3, =0xe000ed08
    str r4, [r3]
    movs r0, #0
    msr control, r0
    msr basepri, r0
    msr faultmask, r0
    msr psp, r0
    msr msp, r5
    dsb sy
    isb sy
    adr r7, resident_invalid
    adds r7, r7, #1
    mov lr, r7
    cpsie i
    bx r6
resident_invalid:
    b resident_invalid
    .ltorg
selector_resident_boot_end:
