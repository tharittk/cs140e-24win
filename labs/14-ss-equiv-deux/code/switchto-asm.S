#include "rpi-asm.h"

@ all of these are called from <switchto.h>: look there for more
@ help in understanding if need to.

@ drop in your code from last lab to get sp and lr.
@
@ void priv_get_sp_lr_asm(uint32_t mode, uint32_t *sp, uint32_t *lr);
@   - assumes caller is at privileged mode [we could check this]
@   - mode in r0
@   - sp pointer in r1
@   - lr pointer in r2
@
MK_FN(priv_get_sp_lr_asm)
    @ b staff_priv_get_sp_lr_asm

    @ from mode_get_lr_sp_asm
    mrs r3, cpsr        @ save current mode

    bic r12, r3, #0x1F  @ clear mode bit
    and r0, r0, #0x1F   @ extract intended mode bit
    orr r12, r12, r0    
    msr cpsr, r12       @ write, now mode change

    str sp, [r1]        @ write sp to memory addr at r1 (&sp)
    str lr, [r2]        @ write lr to memory addr at t2 (&lr)

    msr cpsr_c, r3      @ change to original mode
    bx lr
    asm_not_reached();

    @ Test passed

@ drop in your code from lab 13 [in 1-rfe-asm.S]
@
@ void switchto_user_asm(regs_t *r)  __attribute__((noreturn));
@  - <r> is a 17-entry register entry in r0.
@
@ switch to user mode loading r0-r15, and cpsr from <r>
MK_FN(switchto_user_asm)
    @ b staff_switchto_user_asm

    mov sp, r0          @ currently we are at non-user mode
    ldm sp, {r0-r15}^   @ load the regs[i] value to USER's registers i
    add sp, sp, #60     @ rfe expects {pc, cpsr}
    rfe sp
    asm_not_reached();

    @ Test passed

@ drop in your code from lab 13
@ 
@ void switchto_priv_asm(regs_t *r)  __attribute__((noreturn));
MK_FN(switchto_priv_asm)
    @ b staff_switchto_priv_asm

    @ NOTE: I can't really figure out how to do it with sp style
    @ there seems to be some coupling of mode change and the `sp` changes
    ldr r1, [r0, #64]       @ intended cpsr at r1
    msr cpsr, r1            @ change mode, 
    ldm r0, {r0-r15}        @ load other regs[i] to MODE's registers i
    asm_not_reached();

    @ Test passed

@ you'll have to write this, but it should mirror your other save
@ routines.    
@
@ void cswitchto_priv_asm(regs_t *old, regs_t *next);
@ 
@ voluntary context switch: save the current registers in <old>
@ and call the right switchto on <next>
MK_FN(cswitchto_priv_asm)
    @ initially just forward to ours.
    b staff_cswitchto_priv_asm

    @ to implement:
    @ 1. save all the callee regs (just like in your threads library)
    @    into the 17-entry register array we have been using.  note
    @    these have to be at the right offsets.
    @    e.g.:
    @       str r4,  [r0, #(4*4)]  @ r4
    @       str r5,  [r0, #(5*4)]  @ r5
    @       ... etc ...
    @   don't forget the cpsr!


    @ 2. just call the right switch routine: note, we should
    @    *not* reach this location when switching back!
    bl switchto_priv_asm
    asm_not_reached();

@ you'll have to write this, but it should mirror your other save
@ routines.    
@
@ void cswitchto_user_asm(regs_t *old, regs_t *next);
@ 
@ voluntary context switch: save the current registers in <old>
@ and call the right switchto on <next>
MK_FN(cswitchto_user_asm)
    @ initially just forward to ours.
    b staff_cswitchto_priv_asm

    asm_todo("save the registers: identical to <switchto_priv_asm>")
    bl switchto_user_asm
    asm_not_reached();
