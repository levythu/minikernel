/** @file 410user/progs/work.c
 *  @author acrichto
 *  @brief Factors some numbers in some tasks
 *  @public yes
 *  @for p4-smp
 *  @status done
 */

#include <410_tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <stdint.h>

DEF_TEST_NAME("work:");

#define AMT 50
#define MAX 8000000

int work(uint32_t num) {
  uint32_t i;
  uint32_t factors = 0;

  for (i = 1; i <= num; i++) {
    if (num % i == 0) factors ++;
  }

  return factors;
}

int main() {
  int ticks = get_ticks();
  int i;
  int last = 0;
  srand(0x1401);

  for (i = 0; i < AMT; i++) {

    int next = rand() % MAX;
    int pid = fork();
    REPORT_ON_ERR(pid);
    if (pid == 0) {
      return work(next);
    }
    printf("factoring: %u\n", next);
  }
  print(1, ">");
  for (i = 0; i < AMT; i++) {
    wait(NULL);
    if (i * 79 / AMT > last) {
      print(3, "\b=>");
      last++;
    }
  }

  printf("\nticks taken: %d\n", get_ticks() - ticks);
  return 0;
}
