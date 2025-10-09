/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "gpio.h"
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
  GPIO_BASE = 0x20200000,
  gpio_set0 = (GPIO_BASE + 0x1C),
  gpio_clr0 = (GPIO_BASE + 0x28),
  gpio_lev0 = (GPIO_BASE + 0x34)
};

void gpio_set_function(unsigned pin, gpio_func_t function) {

  if ((pin >= 32 && pin != 47) || function > 7)
    return;
  // 0-9, 10-19, 20-29
  unsigned reg_index = pin / 10;
  unsigned bit_start = (pin % 10) * 3;

  unsigned gpio_fseln = GPIO_BASE + 0x4 * reg_index;

  uint32_t val = GET32(gpio_fseln);

  val &= ~(0x7 << bit_start);     // clear 3 bits
  val |= (function << bit_start); // 3-bit
  PUT32(gpio_fseln, val);
}

void gpio_set_output(unsigned pin) { gpio_set_function(pin, GPIO_FUNC_OUTPUT); }

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return;
  // writing zero has no effect - see doc.
  // GET32 on this addr is undef. Writing it back
  // may change the state of other pins
  PUT32(gpio_set0, 0x1 << pin);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return;

  PUT32(gpio_clr0, 0x1 << pin);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
  if (pin >= 32 && pin != 47)
    return;

  if (v)
    gpio_set_on(pin);
  else
    gpio_set_off(pin);
}

// set <pin> to input.
void gpio_set_input(unsigned pin) { gpio_set_function(pin, GPIO_FUNC_INPUT); }

// return the value of <pin>
int gpio_read(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return -1;
  uint32_t val = GET32(gpio_lev0);
  return DEV_VAL32((val >> pin) & 1);
}
