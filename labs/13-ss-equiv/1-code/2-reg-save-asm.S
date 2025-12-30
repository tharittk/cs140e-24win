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
    @ - doing mrs r0, cpsr_svc, and push r0

    asm_todo("implement this")

    @ sp should point to interrupt stack


    @ save 17 registers on that with stm base_address = sp (decrease before write)


    @ the base address regs[0] moves to r0

    @ call do_syscall

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
