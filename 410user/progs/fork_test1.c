/** @file 410user/progs/fork_test1.c
 *  @author zra
 *  @brief Tests basic functionality of fork()
 *  @public yes
 *  @for p3
 *  @covers fork
 *  @status done
 */

/* Includes */
#include <syscall.h>   /* for fork, getpid */
#include <stdlib.h>    /* for exit */
#include <stdio.h>     /* for lprintf */
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("fork_test1:");

/* Main */
int main(int argc, char *argv[])
{
  int pid;
  
  report_start(START_CMPLT);
  pid = fork();
	
  if( pid < 0 ) {
    report_end(END_FAIL);
    exit(-1);
  }
	
  if( pid > 0 ) {
    report_fmt("parent: tid = %d", gettid());
  }
  else {
    report_fmt("child: tid = %d", gettid());
  }

  report_end(END_SUCCESS);
  exit( 1 );
}
