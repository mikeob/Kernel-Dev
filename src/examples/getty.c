#include <syscall.h>

int main (void)
{
	for (;;)
	{
		wait (exec ("login"));
	}
}
