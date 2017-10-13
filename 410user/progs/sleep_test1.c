/** @file 410user/progs/sleep_test1.c
 *  @author zra
 *  @brief Sleeps for amount of time given as an argument.
 *  @public yes
 *  @for p3
 *  @covers sleep
 *  @status done
 */

/* Includes */
#include <syscall.h>  /* for sleep, getpid */
#include <stdio.h>
#include <simics.h>   /* for lprintf */
#include <stdlib.h>   /* for atoi, exit */
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("sleep_test1:");

int main(int argc, char *argv[]) {
  report_start(START_CMPLT);
  report_misc("before sleeping");

  if (argc == 2)
	sleep( atoi( argv[1] ) );
  else
	sleep( 3 + (gettid() % 27) );

  report_misc("after sleeping");
  report_end(END_SUCCESS);
  exit( 42 );
}
