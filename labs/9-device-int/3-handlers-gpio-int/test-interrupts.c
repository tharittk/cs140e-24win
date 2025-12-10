// test code for checking the interrupts.
#include "test-interrupts.h"
#include "gpio.h"
#include "rpi-interrupts.h"
#include "rpi.h"

volatile int n_interrupt;

static interrupt_fn_t interrupt_fn;

void interrupt_vector(unsigned pc) {
    dev_barrier();
    n_interrupt++;

    // if(!interrupt_fn(pc))
    //     panic("should have no other interrupts?\n");

    // check what type of interrupt
    unsigned is_timer = GET32(IRQ_basic_pending) & RPI_BASIC_ARM_TIMER_IRQ;
    dev_barrier();

    // falling and risinging comes in the same interrupt line
    unsigned is_gpio_interrupt0 = GET32(IRQ_pending_2) & (1 << (GPIO_INT0 % 32));
    dev_barrier();

    if (is_timer){
        timer_test_handler(pc);
    } else if (is_gpio_interrupt0){
        // the test code writing to outpin which loops-back and triggers
        // its effect on in_pin
        if (!gpio_event_detected(in_pin))
            panic("gpio-int0 is triggered, expect event detect at in_pin %d \n", in_pin);

        // figure out if it is falling or rising edge.
        // the PRELAB suggests this scheme.
        // trace("gpio_interrupt detected in_pin value: %d\n", gpio_read(in_pin));
        if (gpio_read(in_pin)){
            // trace("rising detected \n");
            rising_handler(pc);
        } else {
            // trace("falling detected \n");
            falling_handler(pc);
        }
    } else {
        panic("should only be either timer, gpio_interrupt_0 \n");
    }

    dev_barrier();
}

#include "vector-base.h"

// initialize all the interrupt stuff.  client passes in the
// gpio int routine <fn>
//
// make sure you understand how this works.
void test_startup(init_fn_t init_fn, interrupt_fn_t int_fn) {
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");

    // initialize.
    extern uint32_t interrupt_vec[];
    int_vec_init(interrupt_vec);

    gpio_set_output(out_pin);
    gpio_set_input(in_pin);

    init_fn();
    interrupt_fn = int_fn;

    // in case there was an event queued up.
    gpio_event_clear(in_pin);

    // start global interrupts.
    cpsr_int_enable();
}


/********************************************************************
 * falling edge.
 */

volatile int n_falling;

// check if there is an event, check if it was a falling edge.
int falling_handler(uint32_t pc) {
    ++n_falling;
    gpio_event_clear(in_pin);
    return 0;
}

void falling_init(void) {
    gpio_write(out_pin, 1);
    gpio_int_falling_edge(in_pin);
}

/********************************************************************
 * rising edge.
 */

volatile int n_rising;

// check if there is an event, check if it was a rising edge.
int rising_handler(uint32_t pc) {
    ++n_rising;
    gpio_event_clear(in_pin);
    return 0;
}

void rising_init(void) {
    gpio_write(out_pin, 0);
    gpio_int_rising_edge(in_pin);
}

/********************************************************************
 * rising edge.
 */

#include "timer-interrupt.h"

void timer_test_init(void) {
    // turn on timer interrupts.
    timer_interrupt_init(0x4);
}

int timer_test_handler(uint32_t pc) {
    dev_barrier();
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();
    return 0;
}
