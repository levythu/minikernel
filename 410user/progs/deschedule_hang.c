/** @file 410user/progs/deschedule_hang.c
 *  @author mpa
 *  @brief Tests that deschedule() doesn't return without a corresponding
 *         make_runnable() call.
 *  @public yes
 *  @for p3
 *  @covers deschedule
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("deschedule_hang:");

int main()
{
  int error, reject;
  report_start(START_4EVER);

  reject = 0;
  /* stop running for quite a while */
  error = deschedule(&reject);

  report_fmt("deschedule() returned %d", error);

  report_end(END_FAIL);
  exit(1);
}
