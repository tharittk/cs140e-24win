
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

unsigned write_mailbox(volatile mailbox_t *mbox, fb_config_t *cp, unsigned channel) {
    while(mbox->status & MAILBOX_FULL)
        ;

    // why is this not moving it down?
	// the gpu may not read the written value yet.
    cp->width = cp->virtual_width = 1280;
    cp->height = cp->virtual_height = 960;
    cp->depth = 32;
    cp->x_offset = cp->y_offset = 0;
    cp->pointer = 0;



	// this is different from 4-fb.c because the *cp is passed.
	// the compiler must be conservative and assume that changes in cp
	// can be observed by the external world and therefore judiciously
	// store ri, [addr] - as oppsed to what happen when *cp is declared statically

    mbox->write = ((unsigned)(cp) | channel | 0x40000000);
    return cp->pointer;
}
