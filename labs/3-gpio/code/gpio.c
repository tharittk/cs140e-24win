/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum
{
  GPIO_BASE = 0x20200000,
  gpio_set0 = (GPIO_BASE + 0x1C),
  gpio_clr0 = (GPIO_BASE + 0x28),
  gpio_lev0 = (GPIO_BASE + 0x34)
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin)
{
  if (pin >= 32)
    return;

  // use <gpio_fsel0>
  // 0-9, 10-19, 20-29
  unsigned reg_index = pin / 10;
  unsigned bit_start = (pin % 10) * 3;

  unsigned gpio_fseln = GPIO_BASE + 0x4 * reg_index;

  // 001
  uint32_t val = GET32(gpio_fseln);
  val &= ~(0x7 << bit_start); // clear 3 bits
  val |= (0x1 << bit_start);  // set 001
  PUT32(gpio_fseln, val);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin)
{
  if (pin >= 32)
    return;

  uint32_t val = GET32(gpio_set0);
  val |= (0x1 << pin);
  PUT32(gpio_set0, val);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin)
{
  if (pin >= 32)
    return;
  // implement this
  // use <gpio_clr0>
  uint32_t val = GET32(gpio_clr0);
  val |= (0x1 << pin); // write 1 to clear
  PUT32(gpio_clr0, val);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v)
{
  if (v)
    gpio_set_on(pin);
  else
    gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin)
{
  // implement.
  // set 000 through fsel

  if (pin >= 32)
    return;

  // use <gpio_fsel0>
  // 0-9, 10-19, 20-29
  unsigned reg_index = pin / 10;
  unsigned bit_start = (pin % 10) * 3;

  unsigned gpio_fseln = GPIO_BASE + 0x4 * reg_index;

  uint32_t val = GET32(gpio_fseln);
  val &= ~(0x7 << bit_start); // clear 3 bits - set to --000--
  PUT32(gpio_fseln, val);
}

// return the value of <pin>
int gpio_read(unsigned pin)
{
  unsigned v = 0;

  // implement.
  return v;
}
