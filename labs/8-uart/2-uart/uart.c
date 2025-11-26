// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define AUX_IRQ  0x20215000
#define AUX_ENB  0x20215004

#define PUT32(x, y) PUT32((unsigned) x, y)
#define GET32(x) GET32((unsigned) x)

#define PUT8(x, y) PUT8((unsigned) x, y)
#define GET8(x) GET8((unsigned) x)

typedef struct {
    unsigned IO;
    unsigned IER;
    unsigned IIR;
    unsigned LCR;
    unsigned MCR;
    unsigned LSR;
    unsigned MSR;
    unsigned SCRATCH;
    unsigned CNTL;
    unsigned STAT;
    unsigned BAUD;
} mini_uart_t;

static mini_uart_t *MU = (mini_uart_t*) 0x20215040;

static int tx_pin = 14;
static int rx_pin = 15;

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // set gpio pin, in sw uart, we set it as output and input pin.
    gpio_set_function(tx_pin, GPIO_FUNC_ALT5);
    gpio_set_function(rx_pin, GPIO_FUNC_ALT5);

    assert(gpio_read(rx_pin) == 1);
    assert(gpio_read(tx_pin) == 1);
    // turn on the global enabler - do read-modify-write (protect SPIm)
    PUT32(AUX_ENB, GET32(AUX_ENB) | 0b1);

    // disable tx, rx
    PUT32(&MU->CNTL, GET32(&MU->CNTL) & ~(0b11));

    // clear FIFO state for both rx, tx
    PUT32(&MU->IIR, 0b110); 

    // TODO: disable interrupt
    // make sure DLAB == 0


    // configure 115,200 baudrate, 8n1
    PUT32(&MU->BAUD, 115200);

    // 8bit mode
    PUT32(&MU->LCR, GET32(&MU->LCR) | 0b1);

    // enable tx, rx
    PUT32(&MU->CNTL, GET32(&MU->CNTL) | (0b11));
}

// disable the uart.
void uart_disable(void) {
}

int uart_can_get8(void){
    // redundant check 2 regs ?
    // rx FIFO hold at least one symbol
    return (GET32(&MU->LSR) & 1) && (GET32(&MU->STAT) & 1);
}

// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
    while (!uart_can_get8())
        ;
    return (int) GET8(&MU->IO);
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    // redundant check 2 regs ?
    // tx FIFO can take at least one byte
    return (GET32(&MU->LSR) & (1 << 5)) && (GET32(&MU->STAT) & (1 << 1));
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    while (!uart_can_put8())
        ;
    PUT8(&MU->IO, c);
    return 1;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    todo("Must implement");
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    // both empty and idle
    return GET32(&MU->LSR) & (1 << 6);
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        ;
}
