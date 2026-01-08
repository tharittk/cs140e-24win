#include "rpi.h"
#include "pinned-vm.h"
#include "mmu.h"       

// from last lab.
#include "switchto.h"
#include "full-except.h"

enum { OneMB = 1024*1024};

static unsigned d = 8; // Testing domain id


static void inline remove_permission() {
    uint32_t domains;
    // Read-modify-write, 3-63
    asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(domains)::"memory");
    assert (((domains >> (2 * d)) & 0b11) == 0b11); // we enabled as manager previously
    // disable it
    domains = bits_clr(domains, 2*d, 2*d + 1);
    asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(domains):"memory");
}

static void inline enable_permission () {
    uint32_t domains;
    // Read-modify-write, 3-63
    asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(domains)::"memory");
    assert (((domains >> (2 * d)) & 0b11) == 0); // we disabled access previously
    // re-enable as Manager ?
    domains |= (0b11 << (2 * d));
    asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(domains):"memory");
}


// a trivial fault handler that checks that we got the fault
// we expected.
static void fault_handler(regs_t *r) {
    uint32_t fault_addr;

    // b4-44
    asm volatile("MRC p15, 0, %0, c6, c0, 0" : "=r" (fault_addr));

    // print pc (? means pc that causes fault), ARMv6 "reason" for the fault (dfsr)
    trace("FAULT DETECTED: fault_addr: %x \n", fault_addr);

    // b4-43
    uint32_t dfsr;
    asm volatile ("mrc p15, 0, %0, c5, c0, 0":"=r"(dfsr)::"memory");

    if (bit_get(dfsr, 11))
        trace("write caused data fault \n");
    else
        trace("read caused data fault \n");

    uint32_t dom_fault = bits_get(dfsr, 4, 7);
    trace("fault at domain id: %d \n", dom_fault);

    // B4-20, bit 10, 3:0
    uint32_t reason = ((bit_get(dfsr, 10) << 4) | (bits_get(dfsr, 0, 3)));

    switch (reason){
        case 0b00001:
            trace("alignment \n");
            break;
        case 0b00000:
            trace("PMSA- TLB miss\n");
            break;
        case 0b00100:
            trace("Instruction cache maintenance operation fault \n");
            break;
        case 0b01100:
            trace("External abort on translation (1st level) \n");
            break;
        case 0b01110:
            trace("External abort on translation (2nd level) \n");
            break;
        case 0b00101:
            trace("Translation (Section) \n");
            break;
        case 0b00111:
            trace("Translation (Page) \n");
            break;
        case 0b01001:
            trace("Domain (Section) \n");
            break;
        case 0b01011:
            trace("Domain (Page) \n");
            break;
        case 0b01101:
            trace("Permission (Section) \n");
            break;
        case 0b01111:
            trace("Permission (Page) \n");
            break;
        case 0b01000:
            trace("Percise External Abort \n");
            break;
        case 0b10100:
            trace("TLB Lock \n");
            break;
        case 0b11010:
            trace("Coprocessor Data Abort \n");
            break;
        case 0b10110:
            trace("Imprecise External Abort \n");
            break;
        case 0b11000:
            trace("Parity Error Exception \n");
            break;
        case 0b00010:
            trace("Debug event \n");
            break;
        default:
            trace("Deprecated / not handled ! \n");
            break;
    }

    // re-enable domain permission, returns
    enable_permission();   

    return;
}

void notmain(void) { 
    assert(!mmu_is_enabled());

    // map the heap: for lab cksums must be at 0x100000.
    kmalloc_init_set_start((void*)OneMB, OneMB);

    // if we are correct this will never get accessed.
    // since all valid entries are pinned.
    void *null_pt = kmalloc_aligned(4096*4, 1<<14);
    assert((uint32_t)null_pt % (1<<14) == 0);

    // initialize everything, after bootup.
    staff_mmu_init();

    // definitions in <pinned-vm.h>
    uint32_t AXP = 0;
    uint32_t AP = 1;
    uint32_t no_user = AXP << 2 | 1; // no access user (privileged only)
    assert(perm_rw_priv == no_user);

    // current index into the 8 pinned entries in tlb.
    unsigned idx = 0;

    // armv6 has 16 different domains with their own privileges.
    // just pick one for the kernel.
    enum { 
        dom_kern = 1, // domain id for kernel
    };          

    // ******************************************************
    // 2. setup device memory.
    // 
    // permissions: kernel domain, no user access, 
    // memory rules: strongly ordered, not shared.
    pin_t dev  = pin_mk_global(dom_kern, no_user, MEM_device);
    
    // map all device memory to itself.  ("identity map")
    pin_mmu_sec(idx++, 0x20000000, 0x20000000, dev);   // tlb 0
    pin_mmu_sec(idx++, 0x20100000, 0x20100000, dev);   // tlb 1
    pin_mmu_sec(idx++, 0x20200000, 0x20200000, dev);   // tlb 2

    // ******************************************************
    // 3. setup kernel memory: 

    // protection: same as device.
    // memory rules: uncached access.
    pin_t kern = pin_mk_global(dom_kern, no_user, MEM_uncached);

    pin_mmu_sec(idx++, 0, 0, kern);                    // tlb 3
    pin_mmu_sec(idx++, OneMB, OneMB, kern);            // tlb 4

    // now map kernel stack (or nothing will work)
    uint32_t kern_stack = STACK_ADDR-OneMB;
    pin_mmu_sec(idx++, kern_stack, kern_stack, kern);   // tlb 5
    uint32_t except_stack = INT_STACK_ADDR-OneMB;

    pin_mmu_sec(idx++, except_stack, except_stack, kern);

    // ******************************************************
    // 4. setup vm hardware.
    //  - page table, asid, pid.
    //  - domain permissions.

    // b4-42: give permissions for all domains.
    staff_domain_access_ctrl_set(DOM_client << dom_kern*2); 

    // set address space id, page table, and pid.
    // note:
    //  - pid never matters, it's just to help the os.
    //  - asid doesn't matter for this test b/c all entries 
    //    are global
    //  - the page table is empty (since pinning) and is
    //    just to catch errors.
    enum { ASID = 1, PID = 128 };
    staff_mmu_set_ctx(PID, ASID,null_pt);

    // if you want to see the lockdown entries.
    // lockdown_print_entries("about to turn on first time");

    // ******************************************************
    // 5. turn it on/off, checking that it worked.
    trace("about to enable\n");
    for(int i = 0; i < 10; i++) {
        staff_mmu_enable();

        if(mmu_is_enabled())
            trace("MMU ON: hello from virtual memory!  cnt=%d\n", i);
        else
            panic("MMU is not on?\n");

        staff_mmu_disable();
        assert(!mmu_is_enabled());
        trace("MMU is off!\n");
    }

    // ******************************************************
    // 6. setup exception handling and make sure we get a fault.

    // just like last lab.  setup a data abort handler.
    full_except_install(0);
    full_except_set_data_abort(fault_handler);

    // the address we will write to (2MB) we know this is not mapped.
    illegal_addr = OneMB + OneMB;

    // this <PUT32> should "work" since vm is off.
    assert(!mmu_is_enabled());
    PUT32(illegal_addr, 0xdeadbeef);
    trace("we wrote without vm: got %x\n", GET32(illegal_addr));
    assert(GET32(illegal_addr) == 0xdeadbeef);

    // this should fault.
    staff_mmu_enable();
    assert(mmu_is_enabled());
    PUT32(illegal_addr, 0xdeadbeef);
    panic("should not reach here\n");
}
