/*
 * implement these.  ordered in difficulty.
 */
#include "rpi-asm.h"

@ return the current stack pointer.
MK_FN(rpi_get_sp)
    asm_todo("implement this")
    bx lr

MK_FN(rpi_cswitch)
    @ arg: **old saved sp, *new sp
    @ save current register set to old saved area
    mov r2, sp          @ save system sp
    ldr sp, [r0]        @ sp of old thread
    push {r4-r11, lr}   @ save registers to the old thread block
    mov sp, r1          @ sp of new thread
    pop {r4-r11, lr}    @ load registers of the new thread block

    blx lr               @ go to rpi_init_trampoline

    mov sp, r2          @ restore system sp

@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <rpi_init_trampoline> into LR
@	- <code> into r1, 
@	- <arg> into r0
@ 
MK_FN(rpi_init_trampoline)
    mov r0, r4          @ arg (moved in rpi_fork)
    mov r1, r5          @ r1 (moved in rpi_fork)
    blx r1
    @bx rpi_exit
