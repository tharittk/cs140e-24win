// Q: does push change sp before writing to it or after?
#include "rpi.h"

enum
{
    val1 = 0xdeadbeef,
    val2 = 0xFAF0FAF0
};

// you write this in <asm-check.S>
//
// should take a few lines:
//  use the "push" instruction to push <val1>
//  onto the memory pointed to by <addr>.
//
// returns the final address

uint32_t *push_one(uint32_t *addr, uint32_t val1);
uint32_t *push_one_self(uint32_t *addr, uint32_t val1)
{
    uint32_t *final_sp;
    asm volatile(
        "mov sp, %[a] \n\t"
        "mov r0, %[v] \n\t"
        "push {r0} \n\t"
        "mov %[fa], sp"
        : [fa] "=r"(final_sp) // output this var to a register and
        // save to C var final_sp
        : [a] "r"(addr), [v] "r"(val1) // load these vars to registers
        : "r0", "memory");
    return final_sp;
}

void notmain()
{

    // v[3]
    // v[2] * <-
    // v[1] <-
    // v[0]
    // TOS
    uint32_t v[4] = {1, 2, 3, 4};

    uint32_t *res = push_one(&v[2], val1);
    assert(res == &v[1]);

    // note this also shows you the order of writes.
    if (v[2] == val1)
    {
        assert(v[3] == 4);
        assert(v[1] == 2);
        assert(v[0] == 1);
        trace("wrote value before modifying pointer\n");
    }
    else if (v[1] == val1)
    {
        assert(v[3] == 4);
        assert(v[2] == 3);
        assert(v[0] == 1);
        trace("wrote value after modifying pointer\n");
    }
    else
        panic("unexpected result\n");
}
