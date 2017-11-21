/** @file new_shell.c
 *  @brief This program tests the new_console() syscall by spawning a
 *         new shell
 *
 *  @author Geoffrey G. Wilson (ggw)
 *  @author E. Chaos Golubitsky (egolubit)
 *
 */
#include <syscall.h>
#include <stdio.h>

int main()
{
	char *argvec[] = { "shell", 0} ;

	if (!(fork())) {
		printf("i am thread %d; about to switch consoles\n", gettid());
		if (new_console() < 0) {
			printf("failed to create new console\n");
			return(-1);
		} else {
			exec(argvec[0], argvec);
		}
	}

	return(0);
}
