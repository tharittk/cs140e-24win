#include "rpi.h"
#include "trace.h"

void __wrap_notmain(void);
void __real_notmain(void);

void __wrap_notmain(void) {
  // implement this function!

  printk("TRACE:calling pi code\n");
  __real_notmain();
}
