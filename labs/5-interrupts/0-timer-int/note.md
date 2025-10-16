### Interrupt Handling Summary

An interrupt requires both the **sender** and **receiver** sides to be enabled.

- **Sender (device):**  
  The device (e.g., timer) must be configured to generate interrupts and assert an interrupt signal `arm_timer_Load` and `arm_timer_Control` -- see `timer-interrupt.c`.

- **Receiver (CPU):**  
  The CPU must allow interrupts by:
  1. Enabling the interrupt source `Enable_Basic_IRQ` in the interrupt controller.
  2. Clearing the IRQ disable bit (I-bit) in the CPSR i.e., globally enabled.
  3. Handle through `IRQ_Basic_Pending` register and go to interrupt vector
Don' forget to clear the arm_timer_IRQClear (reg at timer) so that we won't repeatedly handle this

interrupts-asm.S: we call the interrupt_vector (C code) through this wrapper
```asm
interrupt_asm:
  mov sp, #INT_STACK_ADDR   @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4

  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	        @ uncomment if want to save caller-saved fp regs

  mov   r0, lr              @ Pass old pc
  bl    interrupt_vector    @ C function: expects C calling conventions.

  @ vpop {s0-s15}           @ pop caller saved fp regs
  pop   {r0-r12,lr} 	    @ pop integer registers

  @ return from interrupt handler: will re-enable general ints.
  @ Q: what happens if you do "mov" instead?
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.
```

The last line means that the original PSR has re-enabled IRQ-enabled bit (this global is disabled while handling - done by the hardware - you're not going to see the asm for that)
