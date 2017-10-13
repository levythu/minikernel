/** @file 410user/progs/knife.c
 *  @author mpa
 *  @brief Tests fork and context-switching.
 *  @public yes
 *  @for p3
 *  @covers fork gettid
 *  @status done
 */

#include <syscall.h>
#include <stdio.h>
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

int main() {

  int tid;

  if ((tid = fork()) == 0)
    while(1) {
      slow();
      lprintf("child: %d", gettid());
    }
  else
    while(1) {
      slow();
      lprintf("parent: %d\t\tchild:%d", gettid(), tid);
    }
}
