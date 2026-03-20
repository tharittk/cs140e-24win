@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

#include "rpi-asm.h"
#include "armv6-coprocessor-asm.h"


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ many page numbers are in 
@    armv6-coprocessor-asm.h
@ and the other armv6-*.h files

// just read the cache type: use C code to figure out if it is unified
// b6-13, b6-14
MK_FN(get_cache_type)
    asm_not_implemented()
    bx lr

// void reset(void)
// clear the dcache, icache, tlbs
MK_FN(mmu_reset)
    mov r0, #0

    @ B4-45: Invalidate entire I-cache (with ARM Errata)
    mcr p15, 0, r0, c7, c5, 0   ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    .rept   11                  ; /* ARM Ltd recommends at least 11 nops */\
    nop                         ;                                   \
    .endr

    @ we don't "clean" the cache because we don't want to write it back (current data could be garbage)
    mcr p15, 0, r0, c7, c6, 0  @ invlidaate D-CACHE 

    @ B4-45: Invalidate both I-TLB and D-TLB cache
    @ otherwise, when you enable, you might have TLB hit from the previous mapping (which may not be valid)
    mcr p15, 0, r0, c8, c7, 0
    @ B2-22: Looks like TLB maintenance should be followed by a prefetch flush.
    @ B2-24: Since this is CP15 maintenance, we need to do prefect flush
    @ why prefetch flush makes sense ? since we are invalidating I-TLB, 
    @ the pipeline could have prefetched instructions based on stale TLB entries (the one invalidated)
    mcr p15, 0, r0, c7, c5, 4  

    @ B2-22: TLB maintenance and subsequent prefetch flush are guaranteed to be completed with DSB
    mcr p15, 0, r0, c7, c10, 4 @ DSB
    bx lr

// void cp15_domain_ctrl_wr(uint32_t dom_reg);
// NOTE:
//   - definitely need to do prefetch flush 
//   - I don't *think* we need the full cp15 sync: flush_btb, 
//     dsb, prefetch flush.
MK_FN(cp15_domain_ctrl_wr)
    @ b staff_cp15_domain_ctrl_wr
    @ asm_not_implemented()
    @ same as domain access control set

    mcr p15, 0, r0, c3, c0, 0 @ read
    mcr p15, 0, r0, c7, c5, 4 @ PREFETCH FLUSH
    bx lr

// void cp15_set_procid_ttbr0(uint32_t proc_and_asid, fld_t *pt);
// sequence from b2-25
MK_FN(cp15_set_procid_ttbr0)
    @ b staff_cp15_set_procid_ttbr0

    @ 1. 
    @ B4-41: The hardware has to be able to find the page table when there is a TLB miss. 
    @ You will write the address of the page table to the page table register ttbr0, 
    @ set both TTBR1 and TTBC to 0. (See below) Note the alignment restriction

    @ 2.
    @ B4-52: The ARM allows each TLB entry to be tagged with an address space identifier 
    @ so you don't have to flush when you switch address spaces. 
    @ Set the current address space identifier (pick a number between 1..63).
    @ B4-52 points to B2-24

    @ 3.
    @ Coherence requirements: B2-21, B2-22, B2-23, B2-24 rules for changing page table register. 
    @ And B2-25 the cookbook for changing an ASID.
    @ ------------------------------------------
    @ B4-52: DSB should be executed before changing reg13
    mcr p15, 0, r3, c7, c10, 4 @ DSB

    // Follows B2-24 sequence (pretty much pinned-vm.c: mmu-switch)
    // set ASID = 0 first to prevent alias (3-128)
    lsl r4, r0, #8
    mcr p15, 0, r4, c13, c0, 1
    // B2.7.5 Do BTB after change to ProcessID, followed by prefect flush
    mov r3, #0
    mcr p15, 0, r3, c7, c5, 6 @ FLUSH BTB
    mcr p15, 0, r3, c7, c5, 4 @ PREFETCH FLUSH

    // set TTBRs
    @ 3-61: ttbr0, ttbr1, ttbc
    @ upper 18-bit, pt is the second argument (r1), 14-bit = 16 KB alignment
    mcr p15, 0, r1, c2, c0, 0 @ Write Translation Table Base Register 0 (pt is arg1)
    mcr p15, 0, r3, c7, c5, 6 @ FLUSH BTB
    mcr p15, 0, r3, c7, c5, 4 @ PREFETCH FLUSH

    mcr p15, 0, r3, c2, c0, 1 @ Write Translation Table Base Register 1 (set to 0)
    mcr p15, 0, r3, c7, c5, 6 @ FLUSH BTB
    mcr p15, 0, r3, c7, c5, 4 @ PREFETCH FLUSH

    mcr p15, 0, r3, c2, c0, 2 @ Write Translation Table Base Control Register (set to 0)
    mcr p15, 0, r3, c7, c5, 6 @ FLUSH BTB
    mcr p15, 0, r3, c7, c5, 4 @ PREFETCH FLUSH

    // set asid to asid - original r0
    // B2.7.5 Do BTB after change to ttbr0, ttbr1, ttbcr, followed by prefect flush
    mcr p15, 0, r0, c13, c0, 1
    mcr p15, 0, r3, c7, c5, 6 @ FLUSH BTB
    mcr p15, 0, r3, c7, c5, 4 @ PREFETCH FLUSH

    bx lr
// void mmu_disable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_disable_set_asm)
    @ b staff_mmu_disable_set_asm

    @ note: this piece of code is mapped into the same place in both
    @ MMU-enabled and MMU-disabled so BTB and Prefetch entries work identically
    @ to both MMU-enable and disable.   Without this requirement I believe is
    @ very hard to get right (perhaps impossible?) since we cannot flush
    @ the BTB and prefetch and switch MMU on/off atomically.  thus, would
    @ flush, but instructions could get fetched before we could disable,
    @ defeating the purpose.

    @ 3-74 Clean the D-Cache and write it back to memory
    @ ... and invalidadate - because now the address that may cause
    @ cache hit with mmu enable should not cause hit when disable.
    mov r1, #0
    mcr p15, 0, r1, c7, c14, 0  

    @ Test case required
    @ B4-45: Invalidate entire I-cache (with ARM Errata)
    mcr p15, 0, r0, c7, c5, 0   ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    .rept   11                  ; /* ARM Ltd recommends at least 11 nops */\
    nop                         ;                                   \
    .endr

    @ Test case required - but already handle in Clean & Invalidate D-Cache above
    @ Invalidate D-Cache
    @ mcr p15, 0, r0, c7, c6, 0  @ invlidaate D-CACHE 

    @ Disble MMU. Control is pre-programmed in r0
    mcr p15, 0, r0, c1, c0, 0

    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4 @ DSB

    mcr p15, 0, r0, c7, c5, 4 @ PREFETCH FLUSH

    mcr p15, 0, r0, c7, c5, 6 @ FLUSH BTB
    bx lr

// void mmu_enable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_enable_set_asm)
    @ b staff_mmu_enable_set_asm

    @ Assume mmu_reset() is called earlier

    @ 6-9
    @ Program all relevant CP15 registers of the corresponding world.
    @ [done in mmu_init] Program first-level and second-level descriptor page tables as required.
    @ [done in mmu_reset] Disable and invalidate the Instruction Cache for the corresponding world.

    @ Enable the MMU by setting bit 0 in the CP15 Control Register in the corresponding world
    @ r0 holds the pre-programmed register
    mcr p15, 0, r0, c1, c0, 0

    @ Why DSB -> Prefect Flush -> BTB ?
    @ DSB -> PF: must wait until the CP15 maintenance takes effect so that the newly fetched instruction
    @ is the valid one. If you PF first, the incoming fetched intruction may still refer the old mapping 
    @ , physical address in this case because MMU was not active.

    @ PF -> BTB: If you clean the BTB, the just-became stale instructions in the pipeline may pollute the BTB (calculation logic)
    @ (I think it might not be so bad since you will mispredict that anyway? - not so. You execute the bad address. In bare-metal, you might halt the system)
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4 @ DSB

    mcr p15, 0, r0, c7, c5, 4 @ PREFETCH FLUSH

    mcr p15, 0, r0, c7, c5, 6 @ FLUSH BTB

    bx lr

// Called after pte modification
// B2-23: Follow the sequence
MK_FN(mmu_sync_pte_mods)
    @ b staff_mmu_sync_pte_mods
    @ STR rx, [Page table entry] ;

    @ Clean line [Page table entry]
    @ B6-21: the code does not give specific MVA, so I flush the whole D-cache line (writeback)
    mov r0, #0
    mcr p15, 0, r0, c7, c14, 0 @ Clean entire D-Cache, Invalidate as well (looks like test wants cache misses)
    @ Invalidate all I-Cache (looks like test wants it - being conservative)
    mcr p15, 0, r0, c7, c5, 0   ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, r0, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    .rept   11                  ; /* ARM Ltd recommends at least 11 nops */\
    nop                         ;                                   \
    .endr

    @ DSB ; ensures visibility of the data cleaned from the D Cache
    mcr p15, 0, r0, c7, c10, 4 @ DSB

    @ Invalidate TLB entry by MVA [page address] 
    @ mcr p15, 0, Rd, c8, c5, 0  @ Invalidate TLB-I Cache
    @ B4-45
    mcr p15, 0, r0, c8, c7, 0 @ Invalidate both TLB D-Cache and TLB I-Cache

    @ Invalidate BTB
    mcr p15, 0, r0, c7, c5, 6 @ FLUSH BTB

    @ DSB            ; ensure completion of the Invalidate TLB
    mcr p15, 0, r0, c7, c10, 4 @ DSB

    @ PrefetchFlush
    mcr p15, 0, r0, c7, c5, 4 @ PREFETCH FLUSH
    bx lr

