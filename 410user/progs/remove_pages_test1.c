/** @file 410user/progs/remove_pages_test1.c
 *  @author mpa
 *  @brief Tests remove_pages()
 *  @public yes
 *  @for p3
 *  @covers remove_pages new_pages
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

DEF_TEST_NAME("remove_pages_test1:");

int main() {
  int i;
  int size;
  int failed = 0;

  report_start(START_CMPLT);
  lprintf("This test exercises remove_pages\n");
  lprintf("It assumes that new_pages works as spec\n");

  for(size = 1; size < 50; size++) {
    /* allocate pages */
    if (new_pages((void *)ADDR, size * PAGE_SIZE) != 0) {
      lprintf("new_pages failed to allocated %d bytes at %x",
	      size * PAGE_SIZE, ADDR);
      failed = 1;
    }
    
    /* verify pages */
    for (i = 0; i < size; i++) {
      if (*(int *)(ADDR + i * PAGE_SIZE) != 0) {
	lprintf("allocated page not zeroed at %x", ADDR + i * PAGE_SIZE);
	failed = 1;
      }
      *(int *)(ADDR + i * PAGE_SIZE) = i+1;
    }
    
    /* remove pages */
    if (remove_pages((void *)ADDR) != 0) {
      lprintf("remove_pages failed to remove pages at %x", ADDR);
      failed = 1;
    }

  }

  if (failed) {
    report_end(END_FAIL);
    return -1;
  }

  report_end(END_SUCCESS);
  return 0;
}
