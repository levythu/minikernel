/** @file 410user/progs/merchant.c
 *  @author mpa
 *  @brief Tests basic functionality of exec()
 *  @public yes
 *  @for p3
 *  @covers exec
 *  @status done
 */

#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <simics.h>

#define DELAY (16*1024)

int gcc_please_do_not_optimize_too_much = 0;

void foo() {
  ++gcc_please_do_not_optimize_too_much;
}

/* this function eats up a bunch of cpu cycles */
void slow() {
  int i;

  for (i = 0; i < DELAY; i++) foo();
}

int main(int argc, char **argv) {

  int tid = gettid();

  char buf[16];
  char msg[128];

  sprintf(buf, "%d", tid);

  /* select a message to print out */
  if (argc != 4)
    sprintf(msg,
	    "WARNING: incorrect number of arguments - got %d, expected %d",
	    argc,
	    4);
  else if (strcmp("merchant", argv[0]) != 0 ||
	   strcmp("13", argv[1]) != 0 ||
	   strcmp("foo bar", argv[2]) != 0)
    sprintf(msg, "WARNING: incorrect arguments");
  else if (strcmp(argv[3], buf))
    sprintf(msg, "WARNING: tid changed");
  else
    sprintf(msg, "Hello!");

  /* print message forever */
  while(1) {
    slow();
    lprintf("Message from merchant #%d:\t%s", tid, msg);
  }
}
