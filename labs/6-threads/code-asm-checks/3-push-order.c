// Q: what order does a push instruction write its registers?
#include "rpi.h"

enum { val1 = 0xdeadbeef, val2 = 0xFAF0FAF0 };

// you write this in <asm-check.S>
//
// should take a few lines:
//  use the "push" instruction to push <val1>  and <val2>
//  onto the memory pointed to by <addr>.
//
// returns the final address

uint32_t *push_two(uint32_t *addr, uint32_t val1, uint32_t val2);
// uint32_t *push_two_self(uint32_t *addr, uint32_t val1, uint32_t val2) {
//   uint32_t *final_addr;

//   asm volatile("mov sp, %[a] \t\n"
//                "mov r0, %[v1] \t\n"
//                "mov r1, %[v2] \t\n"
//                "push {r1, r2} \t\n"
//                "mov %[fa], sp"
//                : [fa] "=r"(final_addr)
//                : [a] "r"(addr), [v1] "r"(val1), [v2] "r"(val2)
//                : "r0", "r1", "memory");
//   return final_addr;
// }

void notmain() {
  uint32_t v[4] = {1, 2, 3, 4};
  uint32_t *res = push_two(&v[2], val1, val2);
  assert(res == &v[0]);

  // note this also shows you the order of writes.
  if (v[2] == val2 && v[1] == val1) {
    // v[3]
    // v[2] * <-
    // v[1] <-
    // v[0]
    // TOS
    assert(v[3] == 4);
    assert(v[0] == 1);
    trace("push {r1, r2}, writes r2 and decrements sp \n");
  } else if (v[1] == val2 && v[0] == val1) {
    // v[3]
    // v[2] *
    // v[1] <-
    // v[0] <-
    // TOS
    assert(v[3] == 4);
    assert(v[2] == 3);
    trace("push {r1, r2} decrements sp and writes r2 first \n");
  } else
    panic("unexpected result\n");
}
