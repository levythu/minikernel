/** @file 410user/progs/yield_desc_mkrun.c
 *  @author mpa
 *  @brief Tests yield() and deschedule()/make_runnable()
 *  @public yes
 *  @for p3
 *  @covers yield deschedule make_runnable fork set_status vanish
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("yield_desc_mkrun:");

int main() {
  int parenttid, childtid, error;

  report_start(START_CMPLT);

  parenttid = gettid();
  childtid = fork();
  
  if (childtid < 0) {
    lprintf("cannot fork()");
    goto fail;
  }

  if (childtid == 0) {
    /* child */

    /* Wait until the process deschedules itself */
    while(yield(parenttid) == 0) {
        report_misc("parent not asleep yet");
    }

    report_misc("parent presumably asleep");

    /* Wake parent back up */
    if((error = make_runnable(parenttid)) != 0) {
      lprintf("make_runnable() failed %d in child", error);
      goto fail;
    }
    report_misc("parent should be awake");
  } else {
    /* parent */

    int reject = 0;
	if((error = deschedule(&reject)) != 0) {
      lprintf("deschedule() failed %d in parent", error);
      goto fail;
    }

	/* wait() would be better, but we're not testing wait() here */
    yield(childtid);
    yield(childtid);
    yield(childtid);
    report_end(END_SUCCESS);
  }
  
  exit(0);

fail:
  report_end(END_FAIL);
  exit(1);
}
