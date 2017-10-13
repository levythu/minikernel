/** @file 410user/progs/make_crash.c
 *  @author rsa
 *  @author kcaffrey
 *  @brief Forkbombs, sleeps for a while, then exec()s a helper
 *  @public yes
 *  @for p3
 *  @covers fork, exec, wait
 *  @status done
 */

#include <syscall.h>
#include <stdio.h>
#include "410_tests.h"

DEF_TEST_NAME ("make_crash:");

int main(int argc, char **argv) {
	int i, tid, status;
  int parentTID = gettid ();
	char *name = "make_crash_helper";
	char s_parentTID[11];
	sprintf(s_parentTID, "%d", parentTID);
	char *argvec[] = {name, "evil test", s_parentTID, 0};
	char buf[100];
  REPORT_START_CMPLT;

  REPORT_MISC("This test brought to you by Group 35 (Fall 03) - rsa & kcaffrey\n"); 
	REPORT_MISC("Starting forks");

	for(i=0; i<7; i++) {
		if ((tid=fork())==0) {
			sprintf(buf, "   Starting sleep I am %d", gettid());
			REPORT_MISC(buf);
			sleep(100);
		}
	}

	sprintf(buf, "   Done with my forks, I am %d", gettid());
	REPORT_MISC(buf);

	while ((tid=wait(&status))>=0) {
		sprintf(buf, "   Reaped child %d with status %d, I am %d", tid, status, gettid());
		REPORT_MISC(buf);
	}

	sleep(200);
	exec(name, argvec);

	return -40;
}
