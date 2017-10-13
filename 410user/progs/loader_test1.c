/** @file 410user/progs/loader_test1.c
 *  @author ?
 *  @author nwf
 *  @brief Tests loading of .data and .bss
 *  @public yes
 *  @for p3
 *  @covers exec
 *  @status done
 */

#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("loader_test1:");

int feed = 0xfeed;
int face = 0xface;
char x[8192] = { 1, 2, 3 };
int face2 = 0xface;
void *self = (void *)&self;
const void *cself = (void *)&cself;
int fred;

int main(int argc, char** argv) {
  
  int *ip;
  int ret=0;

  report_start(START_CMPLT);

  if (feed != 0xfeed) {
    /* fatal */
    report_misc(".data contains the wrong value");
    report_end(END_FAIL);
    ret--;
  }

  if (face != 0xface) {
    /* fatal */
    report_misc(".data contains the wrong value");
    report_end(END_FAIL);
    ret--;
  }
  
  ip = &feed;
  ++ip;

#ifdef notdef
// This was not promised by the language but was essentially
// always true... "fixed" by gcc 4.6.1-9 [gbrunsta]
  if (*ip != 0xface) {
    /* fatal */
    report_misc(".data out of order... how did the previous two tests work?");
    report_end(END_FAIL);
    ret--;
  }
#endif

  if (fred) {
    /* fatal */
    report_misc(".bss not zeroed");
    report_end(END_FAIL);
    ret--;
  }

  if (face2 != 0xface) {
    /* fatal */
    report_misc(".data contains wrong value");
    report_end(END_FAIL);
    ret--;
  }

  if (&feed >= &fred) {
    /* fatal -- if even observable */
    report_misc("linker botch???");
    report_end(END_FAIL);
    ret--;
  }

  if ((self != (void *)&self) || (cself != (void *)&cself)) {
    report_misc("One of the flay-rods has gone out askew on the treadle!");
    report_end(END_FAIL);
    ret--;
  }

  if(ret==0)
  {
    report_end(END_SUCCESS);
    return 0;
  }
  exit(42);
}
