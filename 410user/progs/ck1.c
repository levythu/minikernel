/** @file 410user/progs/ck1.c
 *  @author ?
 *  @author elly1
 *  @brief Runs the checkpoint 1 script.
 *  @public yes
 *  @for p3
 *  @covers gettid
 *  @status done
 */

#include <syscall.h>  /* gettid() */
#include <simics.h>   /* lprintf() */

/* Main */
int main()
{
	int tid;

    sim_ck1();
	tid = gettid();

	lprintf("my tid is: %d", tid);
	
	while (1)
		continue;
}
