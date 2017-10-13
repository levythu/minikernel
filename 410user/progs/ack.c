/** @file 410user/progs/ack.c
 *  @author acrichto
 *  @brief Computes Ackermann function in multiple threads
 *  @public yes
 *  @for p4-smp
 *  @status done
 */

/* "Because not all total computable functions are primitive recursive" */

#include <thread.h>
#include <thrgrp.h>
#include <410_tests.h>
#include <stdio.h>
#include <syscall.h>

DEF_TEST_NAME("ack:");

#define ACKN 8
#define THREADS 8

typedef void*(thr_func)(void*);

int ack(int m, int n) {
  if (m == 0) return n + 1;
  if (n == 0) return ack(m - 1, 1);
  return ack(m - 1, ack(m, n - 1));
}

void* ackstart(void *n) {
  return (void*) ack(3, (int) n);
}

int main() {
  thr_init(1024 * 128);
  int ticks = get_ticks();
  int i, answer, guess;
  thrgrp_group_t group;

  REPORT_ON_ERR(thrgrp_init_group(&group));
  for (i = 0; i < THREADS; i++) {
    REPORT_ON_ERR(thrgrp_create(&group, (thr_func*) ackstart, (void*) ACKN));
  }

  REPORT_ON_ERR(thrgrp_join(&group, (void*) &answer));
  for (i = 1; i < THREADS; i++) {
    REPORT_ON_ERR(thrgrp_join(&group, (void*) &guess));
    if (guess != answer) {
      REPORT_END_FAIL;
    }
  }
  printf("ack(3, %d) = %d\n", ACKN, answer);
  printf("ticks taken: %d\n", get_ticks() - ticks);
  return 0;
}
