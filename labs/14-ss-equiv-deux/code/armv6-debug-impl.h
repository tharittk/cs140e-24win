#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__

/*************************************************************************
 * all the different assembly routines.
 */
#include "asm-helpers.h"

#if 0
// all we need for IMB at the moment: prefetch flush.
static inline void prefetch_flush(void) {
    unsigned r = 0;
    asm volatile ("mcr p15, 0, %0, c7, c5, 4" :: "r" (r));
}
#endif

// turn <x> into a string
#define MK_STR(x) #x

// define a general co-processor inline assembly routine to set the value.
// from manual: must prefetch-flush after each set.
#define coproc_mk_set(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline void c ## coproc ## _ ## fn_name ## _set(uint32_t v) {                    \
        asm volatile ("mcr " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) :: "r" (v));               \
        prefetch_flush();                                               \
    }

#define coproc_mk_get(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline uint32_t c ## coproc ## _ ## fn_name ## _get(void) {                      \
        uint32_t ret=0;                                                   \
        asm volatile ("mrc " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) : "=r" (ret));             \
        return ret;                                                     \
    }


// make both get and set methods.
#define coproc_mk(fn, coproc, opcode_1, Crn, Crm, opcode_2)     \
    coproc_mk_set(fn, coproc, opcode_1, Crn, Crm, opcode_2)        \
    coproc_mk_get(fn, coproc, opcode_1, Crn, Crm, opcode_2)

// produces p14_brv_get and p14_brv_set
// coproc_mk(brv, p14, 0, c0, crm, op2)

/*******************************************************************************
 * debug support.
 */
#include "libc/helper-macros.h"     // to check the debug layout.
#include "libc/bit-support.h"           // bit_* and bits_* routines.


// 13-5
struct debug_id {
                                // lower bit pos : upper bit pos [inclusive]
                                // see 0-example-debug.c for how to use macros
                                // to check bitposition and size.  very very easy
                                // to mess up: you should always do.
    uint32_t    revision:4,     // 0:3  revision number
                variant:4,      // 4:7  major revision number
                :4,             // 8:11
                debug_rev:4,   // 12:15
                debug_ver:4,    // 16:19
                context:4,      // 20:23
                brp:4,          // 24:27 --- number of breakpoint register
                                //           pairs+1
                wrp:4          // 28:31 --- number of watchpoint pairs.
        ;
};

// Get the debug id register
static inline uint32_t cp14_debug_id_get(void) {
    // the documents seem to imply the general purpose register
    // SBZ ("should be zero") so we clear it first.
    uint32_t ret = 0;

    asm volatile ("mrc p14, 0, %0, c0, c0, 0" : "=r"(ret));
    return ret;
}

// This macro invocation creates a routine called cp14_debug_id_macro
// that is equivalant to <cp14_debug_id_get>
//
// you can see this by adding "-E" to the gcc compile line and inspecting
// the output.
coproc_mk_get(debug_id_macro, p14, 0, c0, c0, 0)

// enable the debug coproc
static inline void cp14_enable(void);

// get the cp14 status register.
// static inline uint32_t cp14_status_get(void);
// set the cp14 status register.
// static inline void cp14_status_set(uint32_t status);

#if 0
static inline uint32_t cp15_dfsr_get(void);
static inline uint32_t cp15_ifar_get(void);
static inline uint32_t cp15_ifsr_get(void);
static inline uint32_t cp14_dscr_get(void);
#endif

//**********************************************************************
// all your code should go here.  implementation of the debug interface.

// example of how to define get and set for status registers
// coproc_mk(status, p14, 0, c0, c1, 0)
// cp14_status_get, cp14_status_set

// you'll need to define these and a bunch of other routines.
static inline uint32_t cp15_dfsr_get(void) {
    uint32_t ret = 0;
    asm volatile ("mrc p15, 0, %0, c5, c0, 0":"=r"(ret));
    return ret;
}
static inline uint32_t cp15_ifar_get(void) {
    // 3-69
    // address instruction that causes prefetch abort
    // advised to use for true prefetch abort (not the debug event)
    uint32_t ret = 0;
    asm volatile ("mrc p15, 0, %0, c6, c0, 2":"=r"(ret));
    return ret;
}
static inline uint32_t cp15_ifsr_get(void) {
    uint32_t ret = 0;
    asm volatile ("mrc p15, 0, %0, c5, c0, 1":"=r"(ret));
    return ret;
}
static inline uint32_t cp14_dscr_get(void) {
    uint32_t ret = 0;
    asm volatile ("mrc p14, 0, %0, c0, c1, 0" : "=r"(ret));
    return ret;
}

static inline void cp14_dscr_set(uint32_t status){
    asm volatile("mcr p14, 0, %0, c0, c1, 0" :: "r"(status));
}

static inline uint32_t cp15_far_get(){
    uint32_t far = 0;
    asm volatile ("mrc p15, 0, %0, c6, c0, 0":"=r"(far));
    return far;
}

static inline uint32_t cp14_wcr0_get(void) {
    uint32_t ret = 0;
    asm volatile("mrc p14, 0, %0, c0, c0, 7": "=r"(ret));
    return ret;
}
static inline void cp14_wcr0_set(uint32_t r) {
    asm volatile ("mcr p14, 0, %0, c0, c0, 7" ::"r"(r));
    prefetch_flush();
}

static inline uint32_t cp14_wvr0_get(void) {
    uint32_t ret = 0;
    asm volatile("mrc p14, 0, %0, c0, c0, 6": "=r"(ret));
    return ret;
}
static inline void cp14_wvr0_set(uint32_t r) {
    asm volatile ("mcr p14, 0, %0, c0, c0, 6" ::"r"(r));
    prefetch_flush();
}

static inline uint32_t cp14_bcr0_get(void) {
    uint32_t ret = 0;
    asm volatile ("mrc p14, 0, %0, c0, c0, 5" :"=r"(ret));
    return ret;
}
static inline void cp14_bcr0_set(uint32_t r) {
    asm volatile ("mcr p14, 0, %0, c0, c0, 5" ::"r"(r));
    prefetch_flush();
}

static inline uint32_t cp14_bvr0_get(void) {
    uint32_t ret = 0;
    asm volatile ("mrc p14, 0, %0, c0, c0, 4" :"=r"(ret));
    return ret;
}
static inline void cp14_bvr0_set(uint32_t r) {
    asm volatile ("mcr p14, 0, %0, c0, c0, 4" ::"r"(r));
    prefetch_flush();
}


// return 1 if enabled, 0 otherwise.
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is
//      non-zero).
static inline int cp14_is_enabled(void) {
    uint32_t v = cp14_dscr_get();

    // 13-9: Monitor debug-mode enable
    return bit_is_on(v, 15);
}

// enable debug coprocessor
static inline void cp14_enable(void) {
    // if it's already enabled, just return? - seems like it
    if(cp14_is_enabled())
        return;
        // panic("already enabled\n");

    // for the core to take a debug exception, monitor debug mode has to be both
    // selected and enabled --- bit 14 clear and bit 15 set.

    // 13-9
    uint32_t v = cp14_dscr_get();
    v = bit_set(v, 15);
    v = bit_clr(v, 14);

    cp14_dscr_set(v);

    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

    // 13-9
    uint32_t v = cp14_dscr_get();
    v = bit_clr(v, 15);

    cp14_dscr_set(v);

    assert(!cp14_is_enabled());
}


static inline int cp14_bcr0_is_enabled(void) {
    uint32_t v = cp14_bcr0_get();
    // 13-19 table 13-12
    return bit_get(v, 0);
}
static inline void cp14_bcr0_enable(void) {
    if (!cp14_bcr0_is_enabled()){
        // 13-18
        uint32_t v = cp14_bcr0_get();
        // address matches
        v = bits_set(v, 21, 22, 0b00);
        // no linking
        v = bit_clr(v,  20);
        // match both secure and non-secure world
        v = bits_set(v, 14, 15, 0b00);
        // match all byte offset
        v = bits_set(v, 5, 8, 0b1111);
        // allow for both supervisor and user priviledge
        v = bits_set(v, 1, 2, 0b11);
        // enable it
        v = bit_set(v, 0);
        cp14_bcr0_set(v);
    }
}

static inline void cp14_bcr0_enable_mismatch(void) {
    if (!cp14_bcr0_is_enabled()){
        // 13-18
        uint32_t v = cp14_bcr0_get();
        // address MISMATCH
        v = bits_set(v, 21, 22, 0b11);
        // no linking
        v = bit_clr(v,  20);
        // match both secure and non-secure world
        v = bits_set(v, 14, 15, 0b00);
        // match all byte offset
        v = bits_set(v, 5, 8, 0b1111);
        // ? USER ONLY ?
        v = bits_set(v, 1, 2, 0b10);
        // enable it
        v = bit_set(v, 0);
        cp14_bcr0_set(v);
    }
}
static inline void cp14_bcr0_disable(void) {
    uint32_t v = cp14_bcr0_get();
    v = bit_clr(v, 0);
    cp14_bcr0_set(v);
}

// was this a brkpt fault?
static inline int was_brkpt_fault(void) {
    // use IFSR and then DSCR
    uint32_t ifsr = cp15_ifsr_get();
    // 3-67 bit[10] == 0 and bit[3:0] == 0b0010, instruction debug fault event
    if (bit_is_off(ifsr, 10) && bits_get(ifsr, 0, 3) == 0b0010){
        uint32_t dscr = cp14_dscr_get();
        // 13-11 bit[5:2] == 0b0001 is when breakpoint debug occurs
        if (bits_get(dscr, 2, 5) == 0b0001)
            return 1;
    }
    return 0;
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}


// 13-33: tabl 13-23
static inline int was_watchpt_fault(void) {
    // use DFSR then DSCR
    uint32_t dfsr = cp15_dfsr_get();
    // 3-65 bit[10] == 0 and bit[3:0] == 0b0010, debug fault event
    if (bit_is_off(dfsr, 10) && bits_get(dfsr, 0, 3) == 0b0010){
        uint32_t dscr = cp14_dscr_get();
        // 13-11 bit[5:2] == 0b0010 is when watchpoint debug occurs
        if (bits_get(dscr, 2, 5) == 0b0010)
            return 1;
    }
    return 0;
}

static inline int cp14_wcr0_is_enabled(void) {
    // 13-22
    uint32_t wcr0 = cp14_wcr0_get();
    return bit_is_on(wcr0, 0);
}

static inline void cp14_wcr0_enable(void) {
    if (!cp14_wcr0_is_enabled()){
        // 13-21
        uint32_t v = cp14_wcr0_get();
        // disable link
        v = bit_clr(v, 20);
        // match both secure and non-secure world
        v = bits_set(v, 14, 15, 0b00);
        // match all byte offset
        v = bits_set(v, 5, 8, 0b1111);
        // either save or load
        v = bits_set(v, 3, 4, 0b11);
        // allow for both supervisor and user priviledge
        v = bits_set(v, 1, 2, 0b11);
        // enable it
        v = bit_set(v, 0);
        cp14_wcr0_set(v);
    }
}
static inline void cp14_wcr0_disable(void) {
    if (cp14_wcr0_is_enabled()){
        uint32_t v = cp14_wcr0_get();
        v = bit_clr(v, 0);
        cp14_wcr0_set(v);
    }
}

// Get watchpoint fault using WFAR
static inline uint32_t watchpt_fault_pc(void) {

    // 13-12 - should I check previledge ?
    uint32_t wfar = 0;
    asm volatile ("mrc p14, 0, %0, c0, c6, 0":"=r"(wfar));
    return (wfar - 8);
}

#endif
