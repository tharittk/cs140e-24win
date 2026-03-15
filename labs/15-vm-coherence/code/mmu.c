// hardware mmu code: mostly they do error checking and call into
// assembly (your-mmu-asm.S).
#include "rpi.h"
#include "rpi-constants.h"
#include "rpi-interrupts.h"
#include "libc/helper-macros.h"
#include "mmu-internal.h"

// given.

int mmu_is_enabled(void)
{
    return cp15_ctrl_reg1_rd().MMU_enabled != 0;
}

// disable the mmu by setting control register 1
// to <c:32>.
//
// we use a C veneer over the assembly (mmu_disable_set_asm)
// so we can easily do assertions: the real work is
// done by the asm code (you'll write this next time).
void mmu_disable_set(cp15_ctrl_reg1_t c)
{
    assert(!c.MMU_enabled);

    // record if dcache on.
    uint32_t cache_on_p = c.C_unified_enable;

    staff_mmu_disable_set_asm(c);

    // re-enable if it was on.
    if (cache_on_p)
    {
        c.C_unified_enable = 1;
        cp15_ctrl_reg1_wr(c);
    }
}

// disable the MMU by flipping the enable bit.   we
// use a C vener so we can do assertions and then call
// out to assembly to do the real work (you'll write this
// next time).
void mmu_disable(void)
{
    cp15_ctrl_reg1_t c = cp15_ctrl_reg1_rd();
    assert(c.MMU_enabled);
    c.MMU_enabled = 0;
    mmu_disable_set(c);
}

// enable the mmu by setting control reg 1 to
// <c>.   we start in C so we can do assertions
// and then call out to the assembly for the
// real work (you'll write this code next time).
void mmu_enable_set(cp15_ctrl_reg1_t c)
{
    assert(c.MMU_enabled);
    staff_mmu_enable_set_asm(c);
}

// enable mmu by flipping enable bit.
void mmu_enable(void)
{
    cp15_ctrl_reg1_t c = cp15_ctrl_reg1_rd();
    assert(!c.MMU_enabled);
    c.MMU_enabled = 1;
    mmu_enable_set(c);
}

// probably should merge with <set_procid_ttbr0>
void mmu_set_ctx(uint32_t pid, uint32_t asid, void *pt)
{
    assert(asid != 0);
    assert(asid < 64);
    // set_procid_ttbr0(pid, asid, pt);
    staff_cp15_set_procid_ttbr0(pid << 8 | asid, pt);
}

// set so that we use armv6 memory.
// this should be wrapped up neater.  broken down so can replace
// one by one.
//  1. the fields are defined in vm.h.
//  2. specify armv6 (no subpages).
//  3. check that the coprocessor write succeeded.
void mmu_init(void)
{
    staff_mmu_init();
    return;

    // initialize the MMU hardware state
    mmu_reset();

    // trivial: RMW the xp bit in control reg 1.
    // leave mmu disabled.
    unimplemented();

    // make sure write succeeded.
    struct control_reg1 c1 = cp15_ctrl_reg1_rd();
    assert(c1.XP_pt);
    assert(!c1.MMU_enabled);
}

// read and return the domain access control register
uint32_t domain_access_ctrl_get(void)
{
    return staff_domain_access_ctrl_get();
}

// NOTE: to stop duplicate symbol errors for
// <domain_access_ctrl_set> routine please delete
// it from <pinned-vm.c.>

// b4-42
// set domain access control register to <r>
void cp15_domain_ctrl_wr(uint32_t r);
__attribute__((weak)) void domain_access_ctrl_set(uint32_t r)
{
    // staff_domain_access_ctrl_set(r);

    // from pin_mmu_init()
    asm volatile("mcr p15, 0, %0, c3, c0, 0" ::"r"(r));
    uint32_t zero = 0;

    // asm volatile("mcr p15, 0, %0, c7, c10, 4" ::"r"(zero)); // DSB

    // // B2-24: CP15 changes only takes effect after PREFECTH_FLUSH. Not sure I need BTB Flush as well
    // asm volatile("mcr p15, 0, %0, c7, c5, 6" ::"r"(dummy) : "memory"); // BTB FLUSH
    asm volatile("mcr p15, 0, %0, c7, c5, 4" ::"r"(zero) : "memory"); // PREFETCH FLUSH

    // NOTE: I don't think we need DSB or BTB flush.
    // We don't change any translation mapping so BTB is currently (and will be) valid address
    // The documentation mention that write to cp15 completes only after prefect flush.
    // There is a risk that prefect flush instruction is re-order and finish before MCR ?
    // But remember: MCR is the register write (not memory access) so DSB or DMB won't even help
    // if you put it there. So, I think armv6 enforces that prefetch flush will be serialized after mcr

    // I put it in your-mmu.asm as well
    // cp15_domain_ctrl_wr(r);
    assert(domain_access_ctrl_get() == r);
}
#if 0
#endif

cp15_ctrl_reg1_t cp15_ctrl_reg1_rd(void)
{

    uint32_t raw = 0;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(raw));

    // C does not allow scalar casts to struct
    cp15_ctrl_reg1_t c1;
    memcpy(&c1, &raw, sizeof c1);

    return c1;
    // return staff_cp15_ctrl_reg1_rd();
}
