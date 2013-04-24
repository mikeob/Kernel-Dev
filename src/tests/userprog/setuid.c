/* Tests the exit system call. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
	msg ("uid: %d", getuid ()); 
  setuid (1);
	msg ("uid: %d", getuid ()); 
}
