/** @file 410user/progs/exec_nonexist.c
 *  @author mpa
 *  @brief Tests exec()ing a nonexistent file.
 *  @public yes
 *  @for p3
 *  @covers exec
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("exec_nonexist:");

int main() {
  char *name = "exec_a_random_file_that_does_not_exist";
  char *args[] = {name, 0};
  report_start(START_CMPLT);

  if (exec(name,args) >= 0) {
    report_misc("exec() did not return an error code");
    report_end(END_FAIL);
    exit(-1);
  }

  report_end(END_SUCCESS);
  exit(0);
}
