/** @file 410user/progs/mem_eat_test.c
 *  @author ?
 *  @brief Grows the stack until it dies.
 *  @public yes
 *  @for p3
 *  @covers oom autostack
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("mem_eat_test:");

#define MEGABYTE (1024*1024)
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

void touch_array(char *p);
void foo(int index);

int main(int argc, char *argv[]) {
    report_start(START_ABORT);
    foo(1);
    report_end(END_FAIL);
    return 42;
}

/* touch one byte per page to run faster.
 * this code is a separate function so gcc doesn't optimize
 * away the array writing.
 */
void touch_array(char *p) {
    int i;
    for(i = MEGABYTE - 1; i >= 0; i -= PAGE_SIZE) {
        p[i] = 'a';
    }
}

void foo(int index)
{
    char local_array[MEGABYTE];

    touch_array(local_array);

    report_fmt("used %d pages = %d MB", index * (MEGABYTE / PAGE_SIZE), index);
	foo(index + 1);
}
