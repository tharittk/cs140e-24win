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

#define C_START (1 << 7)
#define C_ENB (1 << 15)
#define C_CLEAR_FIFO (1 << 4)
#define C_READ 1
#define S_DONE (1 << 1)
#define S_RXD (1 << 5) // FIFO has at least 1 byte
#define S_TXD (1 << 4) // FIFO can accept data
#define S_TA 1 // Transfer Active
#define S_CLKT (1 << 9) // clock stretch timeout
#define S_ERR (1 << 8) // Ack error


// extend so this can fail.
int i2c_write(unsigned addr, uint8_t data[], unsigned nbytes) {
	output("in i2c write\n");
	// addr and data size
	PUT32(i2c->dev_addr, addr & 0x7f);
	PUT32(i2c->dlen, nbytes);

	// clear existing flag
	// PUT32(i2c->status,  S_ERR | S_CLKT | S_DONE);

	PUT32(i2c->control, C_ENB | C_START | C_CLEAR_FIFO);

	// wait until transfer is no more active
	// while (GET32(i2c->status) & S_TA)
		// ;

	// char txe = GET32(i2c->status) & (1 << 6);
	// assert (txe);

	// write to target
	unsigned count = 0;
	while (!(GET32(i2c->status) & S_DONE)){
		output ("first inner write \n");
		while (count < nbytes && (GET32(i2c->status) & S_TXD)){
			PUT32(i2c->fifo, data[count++]);
			output("write: %x \n", data[count-1]);
		}
	}

	uint32_t status = GET32(i2c->status);
	char clkt_err = status & S_CLKT;
	char err = status & S_ERR;
	char done = status & S_DONE;
	char ta = status & S_TA;
	assert (!clkt_err && !err && done && !ta);

	return 1;
}

// extend so it returns failure.
int i2c_read(unsigned addr, uint8_t data[], unsigned nbytes) {
	output("in i2c read\n");

	// address and data length
	PUT32(i2c->dev_addr, addr & 0x7f);
	PUT32(i2c->dlen, nbytes);

	// clear flag
	// PUT32(i2c->status,  S_ERR | S_CLKT | S_DONE);

	// start a transfer
	PUT32(i2c->control, C_ENB | C_START | C_READ | C_CLEAR_FIFO);

	// read from target
	unsigned count = 0;
	while (!(GET32(i2c->status) & S_DONE)){
		output ("first inner read \n");
		while (count < nbytes && (GET32(i2c->status) & S_RXD)){
			uint8_t d = GET32(i2c->fifo) & 0xff;
			data[count++] = d;
			output("read: %x \n", d);
		}
	}

	// leftover data after DONE is set
	while (count < nbytes && (GET32(i2c->status) & S_RXD)) {
        data[count++] = GET32(i2c->fifo) & 0xFF;
    }

	uint32_t status = GET32(i2c->status);
	char clkt_err = status & S_CLKT;
	char err = status & S_ERR;
	char done = status & S_DONE;
	char ta = status & S_TA;
	assert (!clkt_err && !err && done && !ta);
	return 1;
}

void i2c_init(void) {
	// p102. gpio 2 set to alt0 (sda1)
	gpio_set_function(2, GPIO_FUNC_ALT0);
	gpio_set_pullup(2);
	dev_barrier();

	// gpio 3 set to alt0 (scl1)
	gpio_set_function(3, GPIO_FUNC_ALT0);
	gpio_set_pullup(3);
	dev_barrier();

	// p29. enable I2C
	// p30. clear fifo
	PUT32(i2c->control, C_ENB | C_CLEAR_FIFO);
	dev_barrier();

	// p31. clear status register
	PUT32(i2c->status,  S_ERR | S_CLKT | S_DONE);

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
