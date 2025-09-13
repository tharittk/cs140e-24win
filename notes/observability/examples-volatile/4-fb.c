// 2023: interesting!  The new version of gcc 
// (10.3.1 20210824) no longer breaks this code.   
// However: if you reorder the cp struct field 
// assignments, it will break (see 5-fb.c).  
// I'm guessing is an interaction b/n register
// assignment and peephole optimization.
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

void write_mailbox_x(mailbox_t *mbox, unsigned channel) {
    while(mbox->status & MAILBOX_FULL)
        ;
	// there is a chance that the compiler thinks
	// these stores are dead
	// there could be mov ri 1280 etc
    cp.width = cp.virtual_width = 1280;
    cp.height = cp.virtual_height = 960;
    cp.depth = 32;
    cp.x_offset = cp.y_offset = 0;
    cp.pointer = 0;

	// but there is no
	// store ri, [cp offset]
	
	// the compiler may simply write these OR result to mbox->write
    mbox->write = ((unsigned)(&cp) | channel | 0x40000000);
	// without store those attribute. Because it thinks those stores are dead.
}
















