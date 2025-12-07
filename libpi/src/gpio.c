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

void gpio_set_function(unsigned pin, gpio_func_t function)
{

  if ((pin >= 32 && pin != 47) || function > 7)
    return;
  // 0-9, 10-19, 20-29, 47
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
void gpio_set_on(unsigned pin)
{
  if (pin >= 32 && pin != 47)
    return;
  // writing zero has no effect - see doc.
  // GET32 on this addr is undef. Writing it back
  // may change the state of other pins
  unsigned reg = pin < 32 ? gpio_set0 : gpio_set1;
  PUT32(reg, 0x1 << (pin) % 32);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin)
{
  if (pin >= 32 && pin != 47)
    return;

  unsigned reg = pin < 32 ? gpio_clr0 : gpio_clr1;
  PUT32(reg, 0x1 << (pin) % 32);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v)
{
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
int gpio_read(unsigned pin)
{
  if (pin >= 32)
    return -1;
  uint32_t val = GET32(gpio_lev0);
  return DEV_VAL32((val >> pin) & 1);
}

// lab 9: interrupt
static void or32(volatile void* addr, uint32_t val){
    dev_barrier();
    PUT32(addr, GET32(addr) | val);
    dev_barrier();
}

static void OR32(uint32_t addr, uint32_t val){
    or32((volatile void*) addr, val);
}

// BCM2835 p.97
void gpio_int_rising_edge(unsigned pin) {
    if (pin > 53)
        return;
    unsigned reg = pin < 32 ? gpio_ren0 : gpio_ren1;
    OR32(reg, 1 << (pin % 32));
}

void gpio_int_falling_edge(unsigned int pin){
    if (pin > 53)
        return;
    unsigned reg = pin < 32 ? gpio_fen0 : gpio_fen1;
    OR32(reg, 1 << (pin % 32));
}

void gpio_event_detected(unsigned int pin){
    if (pin > 53)
        return;
    unsigned reg = pin < 32 ? gpio_eds0 : gpio_eds1;
    return GET32(reg) & (1 << (pin % 32));
}

void gpio_event_clear(unsigned int pin){
    if (pin > 53)
        return;
    unsigned reg = pin < 32 ? gpio_eds0 : gpio_eds1;
    OR32(reg, 1 << (pin % 32));
}

/* For later: just when thses stuffs are still in my head */
// void enable_gpio_interrupt(unsigned pin){
//     // BCM 2835 p.113
//     unsigned bit = pin < 32 ?  GPIO_INT0 : GPIO_INT1;
//     OR32(Enable_IRQs_2 ,1 << (bit % 32))
// }

int gpio_has_interrupt(void) {
    // we can only get it from GPIO_INT0 according from the Lab write-up
    return GET32(IRQ_pending_2) & (1 << (GPIO_INT0 % 32));
}
