// change from 4-fb.c: flip around 
// structure assignments so the code
// breaks.
typedef struct {
  unsigned read;
  unsigned padding[3];
  unsigned peek;
  unsigned sender;
  unsigned status;
  unsigned configuration;
  unsigned write;
} mailbox_t;

typedef struct {
        unsigned width;
        unsigned height;
        unsigned virtual_width;
        unsigned virtual_height;
        unsigned pitch;
        unsigned depth;
        unsigned x_offset;
        unsigned y_offset;
        unsigned pointer;
        unsigned size;
} __attribute__ ((aligned(16))) fb_config_t;

#define MAILBOX_EMPTY  (1<<30)
#define MAILBOX_FULL   (1<<31)

static fb_config_t cp;

void write_mailbox_x(volatile mailbox_t *mbox, unsigned channel) {
    while(mbox->status & MAILBOX_FULL)
        ;
    // interesting, if you flip around these assignments
    // controls if the write assigment gets hoisted

	// Self: It appears that if you explicitly show that you don't care
	// about the assignment order, the compiler will think it's ok too.
	// -- see 4-fb.c does not break when it should have. Now the compiler,
	// may reorder these assignments + store via mbox->write.
	// and so the GPU may read right away after write while some cp.attr 
	// has not been set yet.

    cp.height = cp.virtual_height = 960;
    cp.x_offset = cp.y_offset = 0;
    cp.depth = 32;
    cp.width = cp.virtual_width = 1280;
    cp.pointer = 0;
    mbox->write = ((unsigned)(&cp) | channel | 0x40000000);
}

/* Compile with -O3
write_mailbox_x:
	@ Function supports interworking.
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	push	{r4, r5}
.L2:
	ldr	r3, [r0, #24]
	cmp	r3, #0
	blt	.L2
	mov	r4, #1280
	mov	r5, #960
	mov	r2, #0
	mov	ip, #32
	ldr	r3, .L6
	orr	r1, r3, r1
	orr	r1, r1, #1073741824
	stm	r3, {r4-r5}  <------ only witdth and height were store when mbox->write is executed.
	str	r4, [r3, #8]
	str	r5, [r3, #12]
	str	r2, [r3, #32]
	str	r2, [r3, #28]
	str	r1, [r0, #32]
	str	r2, [r3, #24]
	str	ip, [r3, #20]
	pop	{r4, r5}
	bx	lr
.L7:
	.align	2
.L6:
	.word	.LANCHOR0
	.size	write_mailbox_x, .-write_mailbox_x
	.bss
	.align	4
	.set	.LANCHOR0,. + 0
	.type	cp, %object
	.size	cp, 48
cp:
	.space	48

/*
