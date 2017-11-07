/**
 * @file startle.c
 *
 * @brief simple test of thread creation
 *
 * @author Dave Eckhardt
 *
 */

#include <thread.h>
#include <syscall.h>
#include <simics.h>
#include <stdio.h>
#include <stdlib.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("startle:");

void *child(void *param);

#define STACK_SIZE 3072

#define DEFAULT_NTHREADS 30
#define SOMETIMES 4

int *ktids;

/** @brief Create a bunch of threads, and test that they all run. */
int
main(int argc, char *argv[])
{
  int t, done = 0;
  int nthreads = DEFAULT_NTHREADS;
  if (argc > 1) nthreads = atoi(argv[1]);

  report_start(START_CMPLT);
  if (thr_init(STACK_SIZE) < 0 || !(ktids = calloc(nthreads, sizeof (ktids[0])))) {
    printf("Initialization failure\n");
	report_end(END_FAIL);
    return -1;
  }

  for (t = 0; t < nthreads; ++t) {
    (void) thr_create(child, (void *)t);
    if (t % SOMETIMES == 0) {
      yield(-1);
    }
  }

  while (!done) {
    int nregistered, slot;

    for (nregistered = 0, slot = 0; slot < nthreads; ++slot) {
      if (ktids[slot] != 0) {
        ++nregistered;
      }
    }

    if (nregistered == nthreads) {
      done = 1;
    } else {
      sleep(1);
    }
  }

  printf("Success!\n"); lprintf("Success!\n");
  report_end(END_SUCCESS);

  task_vanish(0);
}

/** @brief Declare that we have run, then twiddle thumbs. */
void *
child(void *param)
{
  int slot = (int) param;

  ktids[slot] = gettid();

  while (1) {
    yield(-1);
    sleep(10);
  }
}
