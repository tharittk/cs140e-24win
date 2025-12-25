/*
 * simplified i2c implementation --- no dma, no interrupts.  the latter
 * probably should get added.  the pi's we use can only access i2c1
 * so we hardwire everything in.
 *
 * datasheet starts at p28 in the broadcom pdf.
 *
 */
#include "rpi.h"
#include "libc/helper-macros.h"
#include "i2c.h"
#include "gpio.h"

typedef struct {
	volatile uint32_t control; // "C" register, p29
	volatile uint32_t status;  // "S" register, p31

#	define check_dlen(x) assert(((x) >> 16) == 0)
	volatile uint32_t dlen; 	// p32. number of bytes to xmit, recv
					// reading from dlen when TA=1
					// or DONE=1 returns bytes still
					// to recv/xmit.  
					// reading when TA=0 and DONE=0
					// returns the last DLEN written.
					// can be left over multiple pkts.

    // Today address's should be 7 bits.
#	define check_dev_addr(x) assert(((x) >> 7) == 0)
	volatile uint32_t 	dev_addr;   // "A" register, p 33, device addr 

	volatile uint32_t fifo;  // p33: only use the lower 8 bits.
#	define check_clock_div(x) assert(((x) >> 16) == 0)
	volatile uint32_t clock_div;     // p34
	// we aren't going to use this: fun to mess w/ tho.
	volatile uint32_t clock_delay;   // p34
	volatile uint32_t clock_stretch_timeout;     // broken on pi.
} RPI_i2c_t;
// } __attribute__((packed)) RPI_i2c_t;

// offsets from table "i2c address map" p 28
_Static_assert(offsetof(RPI_i2c_t, control) == 0, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, status) == 0x4, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, dlen) == 0x8, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, dev_addr) == 0xc, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, fifo) == 0x10, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, clock_div) == 0x14, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, clock_delay) == 0x18, "wrong offset");

/*
 * There are three BSC masters inside BCM. The register addresses starts from
 *	 BSC0: 0x7E20_5000 (0x20205000)
 *	 BSC1: 0x7E80_4000
 *	 BSC2 : 0x7E80_5000 (0x20805000)
 * the PI can only use BSC1.
 */
static volatile RPI_i2c_t *i2c = (void*)0x20804000; 	// BSC1

static void i2c_prolog(void){
	// transmit START
	uint32_t old = GET32(i2c->control);
	PUT32(i2c->control, old | (1 << 7));
	// wait until transfer is no more active
	while (GET32(i2c->status) & 0x1){
		output("w");
	}
	// check error and read fifo is empty
	uint32_t status = GET32(i2c->status);
	char clkt_err = status & (1 << 9);
	char err = status & (1 << 8);
	assert (!clkt_err && !err);
}

static void i2c_prepare_rw(unsigned addr, unsigned nbytes){
	uint32_t old = GET32(i2c->control);
	// clear DONE field
	PUT32(i2c->control, old | (1 << 1));

	// 7-bit address
	PUT32(i2c->dev_addr, addr);

	// data length
	PUT32(i2c->dlen, nbytes);
}

static void i2c_epilog(void){
	// should DONE, and clear
	assert(GET32(i2c->status) & (1 << 1));
	PUT32(i2c->status, (1 << 1));

	// check transfer is not active
	assert(!(GET32(i2c->status) & 1));
}

// extend so this can fail.
int i2c_write(unsigned addr, uint8_t data[], unsigned nbytes) {
	i2c_prolog();
	output("prolog ok \n");

	// char txe = GET32(i2c->status) & (1 << 6);
	// assert (txe);

	i2c_prepare_rw(addr, nbytes);
	output("prepare rw ok \n");

	// 0 for WRITE
	uint32_t old = GET32(i2c->control);
	PUT32(i2c->control, old & ~0x1);

	// start transfer
	old = GET32(i2c->control);
	PUT32(i2c->control, old | (1 << 7));

	// wait until start
	// while (!(GET32(i2c->status) & 1)){
	// 	output(".");
	// }

	// write to target
	unsigned i = 0;
	for (unsigned i = 0; i < nbytes; ++i){
		// wait until read FIFO has a space
		while (!(GET32(i2c->status) & (1 << 4))){
			output("..");
			;
		}
		PUT32(i2c->fifo, data[i]);
	}

	i2c_epilog();
	return 1;
}

// extend so it returns failure.
int i2c_read(unsigned addr, uint8_t data[], unsigned nbytes) {
	i2c_prolog();

	char rd_fifo = GET32(i2c->status) & (1 << 5);
	assert (!rd_fifo);

	i2c_prepare_rw(addr, nbytes);

	// 1 for READ
	uint32_t old = GET32(i2c->control);
	PUT32(i2c->control, old | 1);

	// start transfer
	old = GET32(i2c->control);
	PUT32(i2c->control, old | (1 << 7));

	// wait until start
	while (!(GET32(i2c->status) & 1))
		;

	// read from target
	unsigned i = 0;
	for (unsigned i = 0; i < nbytes; ++i){
		// wait until read FIFO has at least one byte
		while (!(GET32(i2c->status) & (1 << 5)))
			;
		uint8_t d = GET32(i2c->fifo) & 0xff;
		data[i] = d;
	}

	i2c_epilog();

	// last one, send STOP
	return 1;
}

void i2c_init(void) {
	// p102. gpio 2 set to alt0 (sda1)
	gpio_set_function(2, GPIO_FUNC_ALT0);
	dev_barrier();

	// gpio 3 set to alt0 (scl1)
	gpio_set_function(3, GPIO_FUNC_ALT0);
	dev_barrier();

	// p29. enable I2C
	uint32_t c = 0;
	c |= (1 << 15);

	// p30. clear fifo
	c |= (1 << 4);

	PUT32(i2c->control, c);
	// dev_barrier();

	// p31. clear status register
	PUT32(i2c->status, 0);

	// do sanity check
	uint32_t div = GET32(i2c->clock_div) & 0xffff;
	dev_barrier();
	// hacky to test other part. This div reset assesrt sometimes fail !
	PUT32(i2c->clock_div, 0x5dc);
	div = GET32(i2c->clock_div) & 0xffff;
	dev_barrier();

	// Buggy reg (lab said) so we put the value directly
	PUT32(i2c->clock_stretch_timeout, 0x40);
	uint32_t st = GET32(i2c->clock_stretch_timeout) & 0xffff;
	dev_barrier();

	printk("clock_div @ start: %x \n", div);
	// printk("clock_stretch_timeout @ start: %x \n", st); 
	assert(div == 0x5dc);
	assert(st == 0x40);
	// printk("init ok \n");
}

// shortest will be 130 for i2c accel.
void i2c_init_clk_div(unsigned clk_div) {
	i2c_init();
	PUT32(i2c->clock_div, clk_div & 0xffff);
	dev_barrier();
}
