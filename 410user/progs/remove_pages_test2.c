/** @file 410user/progs/remove_pages_test2.c
 *  @author mpa
 *  @brief Tests remove_pages()
 *  @public yes
 *  @for p3
 *  @covers new_pages remove_pages
 *  @status done
 */

#include <syscall.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

#define ADDR 0x40000000
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

DEF_TEST_NAME("remove_pages_test2:");

int main() {
  report_start(START_ABORT);
  lprintf("This test checks that remove_pages removed the pages\n");
  lprintf("It assumes that new_pages works as spec\n");

  /* allocate a couple pages */
  if (new_pages((void *)ADDR, 5 * PAGE_SIZE) != 0) {
    lprintf("new_pages failed at %x", ADDR);
    report_end(END_FAIL);
    return -1;
  }

  if (*(int *)(ADDR + 2 * PAGE_SIZE) != 0) {
    lprintf("allocated page not zeroed at %x", ADDR + 2 * PAGE_SIZE);
    report_end(END_FAIL);
    return -1;
  }
  *(int *)(ADDR + 2 * PAGE_SIZE) = 5;

  if (remove_pages((void *)ADDR) != 0) {
    lprintf("remove_pages failed to remove pages at %x", ADDR);
    report_end(END_FAIL);
    return -1;
  }  

  report_misc("pages removed");
    
  switch (*(int *)(ADDR + 2 * PAGE_SIZE)) {
  case 5:
    report_misc("pages not really removed");
    break;

  default:
    report_fmt("pages still mapped, got %d", *(int *)(ADDR + 2 * PAGE_SIZE));
    break;
  }
  report_end(END_FAIL);
  return -1;
}
