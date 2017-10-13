/** @file 410user/progs/exec_basic.c
 *  @author mpa
 *  @brief Tests basic functionality of exec()
 *  @public yes
 *  @for p3
 *  @covers exec
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

DEF_TEST_NAME("exec_basic:");

int main()
{

  REPORT_LOCAL_INIT;  

  char *name = "exec_basic_helper";
  char *args[] = {name, 0};

  REPORT_START_CMPLT;

  REPORT_ON_ERR(exec(name,args));

	REPORT_END_FAIL;

  exit(-1);
}
