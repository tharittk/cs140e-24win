// put your code here.
//
#include "rpi.h"
#include "libc/bit-support.h"

// has useful enums and helpers.
#include "vector-base.h"
#include "pinned-vm.h"
#include "mmu.h"
#include "procmap.h"

// generate the _get and _set methods.
// (see asm-helpers.h for the cp_asm macro 
// definition)
// arm1176.pdf: 3-149

// define the following routines.
#if 1
// arm1176.pdf: 3-149
void lockdown_index_set(uint32_t x) {
    x &= 0b111;
    asm volatile ("mcr p15, 5, %0, c15, c4, 2"::"r"(x):"memory");
}

uint32_t lockdown_index_get(void) {
    uint32_t ret;
    asm volatile ("mrc p15, 5, %0, c15, c4, 2":"=r"(ret)::"memory");
    return ret;
}

void lockdown_va_set(uint32_t x) {
    // assume x has proper bits set
    asm volatile ("mcr p15, 5, %0, c15, c5, 2"::"r"(x): "memory");
}

uint32_t lockdown_va_get(void) {
    uint32_t va;
    asm volatile ("mrc p15, 5, %0, c15, c5, 2":"=r"(va):: "memory");
    return va;
}

void lockdown_pa_set(uint32_t x) {
    asm volatile ("mcr p15, 5, %0, c15, c6, 2"::"r"(x): "memory");
}

uint32_t lockdown_pa_get(void) {
    uint32_t pa;
    asm volatile ("mrc p15, 5, %0, c15, c6, 2":"=r"(pa):: "memory");
    return pa;
}

void lockdown_attr_set(uint32_t x) {
    asm volatile ("mcr p15, 5, %0, c15, c7, 2"::"r"(x): "memory");
}

uint32_t lockdown_attr_get(void) {
    uint32_t attr;
    asm volatile ("mrc p15, 5, %0, c15, c7, 2":"=r"(attr):: "memory");
    return attr;
}

// void xlate_pa_set(uint32_t x);

// routines to manually check that a translation
// can succeed.  we use these to check that 
// pinned translations are in the TLB.
// see:
//    p 3-80---3-82 in arm1176.pdf

// translate for a privileged read access
void xlate_kern_rd_set(uint32_t x);

// translate for a priviledged write access
void xlate_kern_wr_set(uint32_t x);

// get physical address after manual translation
uint32_t xlate_pa_get(void);

#endif

// do a manual translation in tlb:
//   1. store result in <result>
//   2. return 1 if entry exists, 0 otherwise.
int tlb_contains_va(uint32_t *result, uint32_t va) {
    // 3-79
    assert(bits_get(va, 0,2) == 0);
    // return staff_tlb_contains_va(result, va);

    // Translate va->pa, privilege write permission (3-82)
    // not sure about the privileddge / user
    asm volatile("mcr p15, 0, %0, c7, c8, 1"::"r"(va): "memory");
    // read the pa value see if sucessful
        uint32_t par;
    asm volatile("mrc p15, 0, %0, c7, c4, 0":"=r"(par):: "memory");
    // trace("contains ? pa result: %x (%b) \n", par, par);
    // translation sucessful or not is in bit 0 (3-81) - 1 is abort
    if(bit_get(par, 0))
        return 0;
    
    // Reconstruct the PA: PAR has the section base, VA has the offset.
    *result = (par & 0xfff00000) | (va & 0x000fffff);
    return 1;
}


// map <va>-><pa> at TLB index <idx> with attributes <e>
void pin_mmu_sec(unsigned idx,  
                uint32_t va, 
                uint32_t pa,
                pin_t e) {

    // staff_pin_mmu_sec(idx, va, pa, e);
    // return;

    demand(idx < 8, lockdown index too large);
    // lower 20 bits should be 0.
    demand(bits_get(va, 0, 19) == 0, only handling 1MB sections);
    demand(bits_get(pa, 0, 19) == 0, only handling 1MB sections);

    // if(va != pa)
    //     panic("for today's lab, va (%x) should equal pa (%x)\n",
    //             va,pa);

    debug("about to map %x->%x\n", va,pa);
    // these will hold the values you assign for the tlb entries.
    uint32_t x=0, va_ent=0, pa_ent=0, attr=0;

    // disable interrupt
    asm volatile("cpsid aif":::);
    // write an intended index to TLB lockdown index register
    x = idx & 0b111;
    asm volatile ("mcr p15, 5, %0, c15, c4, 2"::"r"(x): "memory");
    // write va to  TLB lockdown VA
    // ASID is bits [7:0]
    va_ent = (va & 0xfff00000) | (e.G << 9) | (e.asid);
    // trace("va_ent %x \n", va_ent);
    asm volatile("mcr p15, 5, %0, c15, c5, 2"::"r"(va_ent): "memory");
    // write attr to TLB lockdown attribute
    attr = (e.dom << 7) | (e.mem_attr << 1);
    asm volatile ("mcr p15, 5, %0, c15, c7, 2"::"r"(attr): "memory");
    // write pa to TLB lockdown PA
    pa_ent = (pa & 0xfff00000) | (e.pagesize << 6) | (e.AP_perm << 1) | 1;
    asm volatile ("mcr p15, 5, %0, c15, c6, 2"::"r"(pa_ent): "memory");
    // barrier
    asm volatile ("mcr p15, 0, %0, c7, c10, 4" :: "r" (0) : "memory"); // DSB
    asm volatile ("mcr p15, 0, %0, c7, c5, 4"  :: "r" (0) : "memory"); // ISB/PrefetchFlush

    // re-enable interrupt
    asm volatile("cpsie aif":::);

#if 0
    // put this back in when defined.
    if((x = lockdown_va_get()) != va_ent)
        panic("lockdown va: expected %x, have %x\n", va_ent,x);
    if((x = lockdown_pa_get()) != pa_ent)
        panic("lockdown pa: expected %x, have %x\n", pa_ent,x);
    if((x = lockdown_attr_get()) != attr)
        panic("lockdown attr: expected %x, have %x\n", attr,x);
#endif
}

// check that <va> is pinned.  
void pin_check_exists(uint32_t va) {
    if(!mmu_is_enabled())
        panic("XXX: i think we can only check existence w/ mmu enabled\n");

    uint32_t r;
    if(tlb_contains_va(&r, va)) {
        pin_debug("success: TLB contains %x, returned %x\n", va, r);
        assert(va == r);
    } else
        panic("TLB should have %x: returned %x [reason=%b]\n", 
            va, r, bits_get(r,1,6));
}

void domain_access_ctrl_set(uint32_t d) {
    staff_domain_access_ctrl_set(d);
}

static void *null_pt = 0;

// turn the pinned MMU system on.
//    1. initialize the MMU (maybe not actually needed): clear TLB, caches
//       etc.  if you're obsessed with low line count this might not actually
//       be needed, but we don't risk it.
//    2. allocate a 2^14 aligned, 0-filled 4k page table so that any nonTLB
//       access gets a fault.
//    3. set the domain privileges (to DOM_client)
//    4. set the exception handler up using <vector_base_set>
//    5. turn the MMU on --- this can be much simpler than the normal
//       mmu procedure since it's never been on yet and we do not turn 
//       it off.
//    6. profit!

// fill this in based on the test code.
void pin_mmu_init(uint32_t domain_reg) {
    // staff_pin_mmu_init(domain_reg);

    // Steps from (6-9)
    uint32_t sbz = 0;
    // invalidate unitfied TLB both I, D - (B4-45)
    asm volatile ("mcr p15, 0, %0, c8, c7, 0"::"r"(sbz): "memory");

    // set first-level (and second-level) deoscriptor page table (B4-41)
    asm volatile ("mcr p15, 0 , %0, c2, c0, 2"::"r"(sbz): "memory"); // TLB control reg 16 KB, do walk on miss - (3-61)
    uint32_t tlb_base0 = 0x4000;
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"::"r"(tlb_base0): "memory"); // base at 0x4000, other bits are default on reset

    // disable and invaliate I, D cache (3-74)
    asm volatile ("mcr p15, 0, %0, c7, c14, 0"::"r"(sbz): "memory"); // clean and invalidate D-cache
    asm volatile ("mcr p15, 0, %0, c7, c5, 0"::"r"(sbz): "memory"); // invalidate I-cache nad flush branch target cache

    // set access control for 16 domains via 32-bit domain_reg
    asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(domain_reg): "memory");

    // turn on MMU via RMW table 3-39 (3-47) or B4-40
    uint32_t control_reg1;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(control_reg1):: "memory");
    bit_set(control_reg1, 0);
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(control_reg1): "memory");

    // may turn on I-D cache
    return;
}

void pin_mmu_switch(uint32_t pid, uint32_t asid) {
    assert(null_pt);
    // staff function to replace
    // staff_mmu_set_ctx(pid, asid, null_pt);

    //  B2-25, do the asid<-0, prefetch flush, change TTBR,, prefetch flush, change asid to new val
    uint32_t sbz = 0;

    // set ASID = 0 first to prevent alias (3-128)
    uint32_t tmp = pid << 8; // ASID = 0, PRODIC = pid
    asm volatile ("mcr p15, 0, %0, c13, c0, 1"::"r"(tmp): "memory");

    // prefetct flush
    asm volatile ("mcr p15, 0, %0, c7, c5, 4"::"r"(sbz): "memory");

    // change TTBR (set to null_pt)
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"::"r"(null_pt): "memory");

    // prefetch flush
    asm volatile ("mcr p15, 0, %0, c7, c5, 4"::"r"(sbz): "memory");

    // set asid to asid
    uint32_t proc_asid = (pid << 8) | (asid & 0xff);
    asm volatile ("mcr p15, 0, %0, c13, c0, 1"::"r"(proc_asid): "memory");
}


void lockdown_print_entry(unsigned idx) {
        trace("   idx=%d\n", idx);
        lockdown_index_set(idx);
        uint32_t va_ent = lockdown_va_get();
        uint32_t pa_ent = lockdown_pa_get();
        unsigned v = bit_get(pa_ent, 0);
    
        if(!v) {
            trace("     [invalid entry %d]\n", idx);
            return;
        }
    
        // 3-149
        uint32_t va, G, asid;
        va = va_ent >> 12; G = (va_ent >> 9) & 1; asid = (va_ent & 0xff) ;
        trace("     va_ent=%x: va=%x|G=%d|ASID=%d\n",
            va_ent, va, G, asid);
    
        // 3-150
        uint32_t pa, nsa, nstid, size, apx;
        pa = pa_ent >> 12; nsa = (pa_ent >> 9) & 1; nstid = (pa_ent >> 8) & 1;
        size = (pa_ent >> 6) & 0b11; apx = (pa_ent >> 1) & 0b111;
        trace("     pa_ent=%x: pa=%x|nsa=%d|nstid=%d|size=%b|apx=%b|v=%d\n",
                    pa_ent, pa, nsa,nstid,size, apx,v);
    
        // 3-151
        uint32_t dom, xn, tex, C, B;
        uint32_t attr = lockdown_attr_get(); 
        dom = (attr >> 7) & 0xf; xn = (attr >> 6) & 1; tex = (attr >> 3) & 0b111;
        C = (attr >> 2) & 1; B = (attr > 1) & 1;

        trace("     attr=%x: dom=%d|xn=%d|tex=%b|C=%d|B=%d\n",
                attr, dom,xn,tex,C,B);
}
    
void lockdown_print_entries(const char *msg) {
    trace("-----  <%s> ----- \n", msg);
    trace("  pinned TLB lockdown entries:\n");
    for(int i = 0; i < 8; i++)
        lockdown_print_entry(i);
    trace("----- ---------------------------------- \n");
}