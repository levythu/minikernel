/** @file 410user/progs/fib.c
 *  @author acrichto
 *  @brief Computes Fibonacci series in multiple threads
 *  @public yes
 *  @for p4-smp
 *  @status done
 */

#include <410_tests.h>
#include <stdio.h>
#include <syscall.h>
#include <thread.h>
#include <thrgrp.h>

DEF_TEST_NAME("fib:");

#define THREADS 8
#define FIBN 32

typedef void*(thr_func)(void*);

int fib(int n) {
  if (n < 3) return n;
  return fib(n - 1) + fib(n - 2);
}

int main() {
  thr_init(4096);
  int ticks = get_ticks();
  int i, answer, guess;
  thrgrp_group_t group;

  REPORT_ON_ERR(thrgrp_init_group(&group));
  for (i = 0; i < THREADS; i++) {
    REPORT_ON_ERR(thrgrp_create(&group, (thr_func*) fib, (void*) FIBN));
  }

  REPORT_ON_ERR(thrgrp_join(&group, (void*) &answer));
  for (i = 1; i < THREADS; i++) {
    REPORT_ON_ERR(thrgrp_join(&group, (void*) &guess));
    if (guess != answer) {
      REPORT_END_FAIL;
    }
  }
  printf("fib(%d) = %d\n", FIBN, answer);
  printf("ticks taken: %d\n", get_ticks() - ticks);
  return 0;
}
