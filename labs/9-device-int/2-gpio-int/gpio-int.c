// engler, cs140 put your gpio-int implementations in here.
#include "rpi.h"
#include <stdint.h>

enum
{
  GPIO_BASE = 0x20200000,
  gpio_set0 = (GPIO_BASE + 0x1C),
  gpio_set1 = (GPIO_BASE + 0x20),
  gpio_clr0 = (GPIO_BASE + 0x28),
  gpio_clr1 = (GPIO_BASE + 0x2C),
  gpio_lev0 = (GPIO_BASE + 0x34),

  // lab 9: interrupt
  gpio_eds0 = (GPIO_BASE + 0x40),
  gpio_eds1 = (GPIO_BASE + 0x44),
  gpio_ren0 = (GPIO_BASE + 0x4c),
  gpio_ren1 = (GPIO_BASE + 0x50),
  gpio_fen0 = (GPIO_BASE + 0x58),
  gpio_fen1 = (GPIO_BASE + 0x5c)
};

#define Enable_IRQs_2 0x2000b214
#define IRQ_Pending_2 0x2000b208
#define GPIO_INT0_ENTRY 49

static void or32(volatile void* addr, uint32_t val){
    PUT32((unsigned long) addr, GET32((unsigned long) addr) | val);
}

static void OR32(uint32_t addr, uint32_t val){
    or32((volatile void*) (uintptr_t) addr, val);
}

// returns 1 if there is currently a GPIO_INT0 interrupt,
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    unsigned status = GET32(IRQ_Pending_2);
    unsigned v = (status >> (GPIO_INT0_ENTRY % 32)) & 0x1;
    DEV_VAL32(v);
    return v;
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    if (pin >= 32)
        return;
    dev_barrier();
    // read-modify-write
    OR32(gpio_ren0, 1 << pin);
    // next you control the interrupt controller (different device from GPIO)
    // so you need the barrier
    dev_barrier();
    // Enable IRQ 2, entry 49 in the table p113
    PUT32(Enable_IRQs_2, 1 << (GPIO_INT0_ENTRY % 32));
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    if (pin >= 32)
        return;
    dev_barrier();
    OR32(gpio_fen0, 1 << pin);
    dev_barrier();
    PUT32(Enable_IRQs_2, 1 << (GPIO_INT0_ENTRY % 32));
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if (pin >= 32)
        return 0;
    dev_barrier();
    int status =  (GET32(gpio_eds0) >> pin) & 0x1;
    dev_barrier();
    DEV_VAL32(status);
    return status;
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if (pin >= 32)
        return;
    dev_barrier();
    PUT32(gpio_eds0, 1 << pin);
    dev_barrier();
}
