// run a single thread: does a single context switch from
// scheduler thread into this thread.
#include "test-header.h"

static void thread_code(void *arg)
{
    unsigned *x = arg;

    // asm volatile(
    //     " mov r0, sp \t\n"
    //     "blx rpi_print_regs" :::);
    // check tid
    rpi_thread_t *t = rpi_cur_thread();

    // actually i think the thread pointer should be same?
    trace("in thread [%p], tid=%d with x=%x @ [%p]\n", t, t->tid, *x, t);

    // asm volatile(
    //     " mov r0, sp \t\n"
    //     "blx rpi_print_regs" :::);

    assert(t->tid == 1 && *x == 0xdeadbeef);
    trace("SUCCESS: got to the first thread: rebooting\n");
    clean_reboot();
}

void notmain()
{
    test_init();
    trace("about to fork and run one thread\n");

    unsigned x = 0xdeadbeef;
    rpi_fork(thread_code, &x);
    rpi_thread_start();
    not_reached();
}
