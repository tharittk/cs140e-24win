#include "rpi.h"
#include "pinned-vm.h"
#include "mmu.h"       

// from last lab.
#include "switchto.h"
#include "full-except.h"

enum { OneMB = 1024*1024};

static unsigned dom_heap = 8; // Testing domain id


static void inline remove_permission() {
    uint32_t domains;
    // Read-modify-write, 3-63
    asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(domains)::"memory");
    assert (((domains >> (2 * dom_heap)) & 0b11) == DOM_client); // we enabled as client previously
    // disable it
    domains = bits_clr(domains, 2*dom_heap, 2*dom_heap + 1);
    asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(domains):"memory");
    // trace("remove permission %b \n", (domains >> (2*dom_heap)) & 0b11);
}

static void inline enable_permission () {
    uint32_t domains;
    // Read-modify-write, 3-63
    asm volatile ("mrc p15, 0, %0, c3, c0, 0":"=r"(domains)::"memory");
    assert (((domains >> (2 * dom_heap)) & 0b11) == 0); // we disabled access previously
    // re-enable as client
    domains |= (DOM_client << (2 * dom_heap));
    asm volatile ("mcr p15, 0, %0, c3, c0, 0"::"r"(domains):"memory");
    // trace("enable permission %b \n", (domains >> (2*dom_heap)) & 0b11);
}


static void prefetch_abort_handler (regs_t *r) {
    // the pc + 4 is saved to lr, so the jump instruction that causes this abort is at lr - 4
    trace("PREFETCH FAULT DETECTED pc=%x lr=%x \n", r->regs[15], r->regs[14] - 4);
    // trace("PREFETCH FAULT DETECTED \n");

    // GET32(r->regs[15]) gives the instruction try to execute like add r1, r2 etc.
    
    enable_permission();
    return;
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
    // void *null_pt = kmalloc_aligned(4096*4, 1<<14);
    // assert((uint32_t)null_pt % (1<<14) == 0);

    // armv6 has 16 different domains with their own privileges.
    // just pick one for the kernel.
    enum { 
        dom_kern = 1, // domain id for kernel
    };          

    // initialize everything, after bootup.
    uint32_t domain_reg = (DOM_client << (2*dom_heap)) | (DOM_client << (2*dom_kern));
    pin_mmu_init(domain_reg);

    // current index into the 8 pinned entries in tlb.
    unsigned idx = 0;

    uint32_t no_user = perm_rw_priv;

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
    //
    // protection: same as device.
    // memory rules: uncached access.
    pin_t kern = pin_mk_global(dom_kern, no_user, MEM_uncached);
    pin_t heap_ctl = pin_mk_global(dom_heap, no_user, MEM_uncached);

    pin_mmu_sec(idx++, 0, 0, kern);                         // tlb 3
    pin_mmu_sec(idx++, OneMB, OneMB, heap_ctl);             // tlb 4 (heap)

    // now map kernel stack (or nothing will work)
    uint32_t kern_stack = STACK_ADDR-OneMB;
    pin_mmu_sec(idx++, kern_stack, kern_stack, kern);       // tlb 5
    uint32_t except_stack = INT_STACK_ADDR-OneMB;
    pin_mmu_sec(idx++, except_stack, except_stack, kern);   // tlb 6


    lockdown_print_entries("about to turn on first time");

    full_except_install(0);
    full_except_set_data_abort(fault_handler);
    full_except_set_prefetch(prefetch_abort_handler);

    pin_mmu_enable();

    // Remove permission at heap and do the lead via GET32
    remove_permission();
    uint32_t heap_addr = OneMB + 0x4;
    GET32(heap_addr);
    
    // when returned, the permission is already re-enable
    remove_permission();
    // Write bx lr (0xe12fff1e) so we can jump to it and return.
    // ! After the prefetch_handler, the CPU will retry the instruction that fails.
    // We cannot put the random value (bx lr simply returns)
    PUT32(heap_addr, 0xe12fff1e);
    // PUT32(heap_addr, 0x11);

    remove_permission();
    // Use a function pointer so that LR is set correctly for the return.
    // void (*f)(void) = (void (*)(void))heap_addr;
    // f(); // compiler will generate blx rn (where rn store address of f to jump to)

    // may also do this instead
    asm volatile ("blx %0" ::"r"(heap_addr): "lr");
    clean_reboot();
}
