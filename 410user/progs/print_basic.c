/** @file 410user/progs/print_basic.c
 *  @author mpa
 *  @author mtomczak
 *  @brief Tests print().
 *  @public yes
 *  @for p3
 *  @covers print
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"

DEF_TEST_NAME("print_basic:");

int main()
{
  REPORT_START_CMPLT;
  if (print(13, "Hello World!\n") != 0)
    REPORT_END_FAIL;
  else
    REPORT_END_SUCCESS;

  exit(0);
}
