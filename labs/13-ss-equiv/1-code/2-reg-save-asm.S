#include "rpi-asm.h"

@ save all the general registers and the spsr in 
@ ascending order onto a block of memory on the
@ stack.  the pointer to the base of this to 
@ do_syscall (defined in `2-reg-save.c` and 
@ `2-reg-save-full.c`).
@
swi_trampoline:
    @ calling from swi, the mode is SUPERVISOR
    @ mov_ident wrote to the register in USER_MODE
    @ and we want to pass USER's register value to the handler

    @ - r13 (sp) is now SUPERVISOR (set that to interrupt then)

    @ - r0-r12 are shared (you don't really need caret) push them to stack
    @ - r13(sp), r14(lr) are banked in USER, use caret ^
    @ - r15 is the pc of swi (currently saved as lr when swi is called), push r14_svc
    @ - r16 is cpsr (now SUPERVISOR), we need to pass the spsr_svc (USER)
    @ - doing mrs r0, spsr_svc, and push r0
    @ - do this in reverse for stack semantic (at the end sp points to reg[0])

    @ sp should point to interrupt stack
    mov sp, #INT_STACK_ADDR

    @ make room for 17 regs
    @ must do {r0-r12} first because these will get clobbered

    @ make room for r16, r15, r14, r13
    sub sp, sp, #16

    stmfd sp!, {r0-r12} @ share general-purpose registers

    @ go back to original sp
    mov sp, #INT_STACK_ADDR

    mrs r0, spsr
    stmfd sp!, {r0}     @ USER cpsr
    
    stmfd sp!, {r14}    @ pc saved at lr

    stmfd sp, {r13, lr}^  @ USER sp and link reg `!` does not work with `^`
    sub sp, sp, #8

    @ point sp at regs[0], down 4x13 = 52 bytes (full descending)
    sub sp, sp, #52

    @ the base address regs[0] moves to r0
    mov r0, sp

    @ call do_syscall
    bl do_syscall

    @ note: this trampoline does not resume.
    asm_not_reached();

@ only handle single step.
.align 5; 
.globl swi_test_handlers
swi_test_handlers:
    b unhandled_reset
    b unhandled_undefined_instruction
    b swi_trampoline
    b unhandled_prefetch_abort 
    b unhandled_data_abort
    b unhandled_interrupt
    b unhandled_fiq


.weak do_syscall
do_syscall:
    asm_not_reached();
