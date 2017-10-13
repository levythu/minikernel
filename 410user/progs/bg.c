/** @file 410user/progs/bg.c
 *  @author mjsulliv (2009)
 *  @brief runs a command in the background
 *  @public yes
 *  @for p3,p4
 *  @covers fork
 *  @bug This should arguably be done by the shell.
 */

#include <syscall.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[])
{
	int pid;

	if (argv[argc]) printf("*** fsck: %p!\n", argv[argc]);

	if (argc < 2) {
		printf("usage: %s program [arguments]\n", argv[0]);
		return 1;
	}

	if ((pid = fork()) > 0) {
		printf("Child pid: %d\n", pid);
		return 0;
	} else if (pid < 0) {
		return 1;
	}

	/* Now we are a child, our parent is exiting, so we will be owned by init.
	 * The shell was waiting on our parent and doesn't give a hoot about us. */

	int ret = exec(argv[1], &argv[1]);
	panic("exec failed: %d!", ret);

	return 0;
}
