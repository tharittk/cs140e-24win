// write code in C to check if stack grows up or down.
// suggestion:
//   - local variables are on the stack.
//   - so take the address of a local, call another routine, and
//     compare addresses.
//   - make sure you check the machine code make sure the
//     compiler didn't optimize the calls away.
#include "rpi.h"

int addr(int *local) {
  int local2 = 2;
  return local > &local2;
};
int stack_grows_down(void) {
  int local = 1;
  return addr(&local);
}

void notmain(void) {
  if (stack_grows_down())
    trace("stack grows down\n");
  else
    trace("stack grows up\n");
}
