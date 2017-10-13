/** @file 410user/progs/wait_getpid.c
 *  @author mpa
 *  @brief Tests gettid()/wait()/fork()
 *  @public yes
 *  @for p3
 *  @covers gettid wait fork set_status vanish
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("wait_getpid:");

int main()
{
  int pid, status;

  report_start(START_CMPLT);

  pid = fork();

  if (pid < 0) {
    report_end(END_FAIL);
    exit(-1);
  }
  
  if (pid == 0) {
    pid = gettid();
    exit(pid);
    report_end(END_FAIL);
  }
  if (wait(&status) != pid) {
    report_end(END_FAIL);
    exit(-1);
  }

  if (status != pid) {
    report_end(END_FAIL);
    exit(-1);
  }

  report_end(END_SUCCESS);
  exit(0);
}
