/** @file 410user/progs/exec_basic_helper.c
 *  @author mpa
 *  @brief Run by exec_basic to test exec.
 *  @public yes
 *  @for p3
 *  @covers nothing
 *  @status done
 *  @note Helper.
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include "410_tests.h"

DEF_TEST_NAME("exec_basic:");

int main()
{
  REPORT_MISC("exec_basic_helper main() starting...");
  REPORT_END_SUCCESS;
 
  exit(1);
}
