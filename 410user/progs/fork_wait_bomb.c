/** @file 410user/progs/fork_wait_bomb.c
 *  @author ?
 *  @brief Tests fork, wait, and exit in stress conditions.
 *  @public yes
 *  @for p3
 *  @covers fork wait set_status vanish
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("fork_wait_bomb:");

int main(int argc, char *argv[]) {
    int pid = 0;
    int count = 0;
    int ret_val;
    int wpid;

    report_start(START_CMPLT);
    report_fmt("parent: %d", gettid());

	while(count < 1000) {
		if((pid = fork()) == 0) {
			exit(42);
		}
		if(pid < 0) {
			break;
		}

        count++;

        report_fmt("child: %d", pid);

		wpid = wait(&ret_val);

		if(wpid != pid || ret_val != 42) {
            report_end(END_FAIL);
            exit(42);
		}
	}

    report_end(END_SUCCESS);
	exit(42);
}
