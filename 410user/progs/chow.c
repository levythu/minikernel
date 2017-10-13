/** @file 410user/progs/chow.c
 *  @author acrichto
 *  @brief "Continuous hours of operation (work)" -
 *         runs a bunch of user-space code, not just syscalls
 *  @public yes
 *  @for p4-smp
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("chow:");

/* Every UPDATE_FREQUENCY reaps, print out an update of
 * how many children of each type are left.
 */
#define UPDATE_FREQUENCY 50

struct prog {
	char *name;
	int pid;
	int count;
} progs[] = {
	{"fib", -1, 20},
	{"ack", -1, 20},
	{"yield_desc_mkrun", -1, 100},
	{"fork_wait", -1, 100}
};

char numbers[10][5][10] = {
  {"  ___  ", " / _ \\ ", "| | | |", "| |_| |", " \\___/ "},
  {" _ ", "/ |", "| |", "| |", "|_|"},
  {" ____  ", "|___ \\ ", "  __) |", " / __/ ", "|_____|"},
  {" _____ ", "|___ / ", "  |_ \\ ", " ___) |", "|____/ "},
  {" _  _   ", "| || |  ", "| || |_ ", "|__   _|", "   |_|  "},
  {" ____  ", "| ___| ", "|___ \\ ", " ___) |", "|____/ "},
  {"  __   ", " / /_  ", "| '_ \\ ", "| (_) |", " \\___/ "},
  {" _____ ", "|___  |", "   / / ", "  / /  ", " /_/   "},
  {"  ___  ", " ( _ ) ", " / _ \\ ", "| (_) |", " \\___/ "},
  {"  ___  ", " / _ \\ ", "| (_) |", " \\__, |", "   /_/ "},
};

int main()
{
	int active_progs = sizeof (progs) / sizeof (progs[0]);
	int active_processes = 0;
	struct prog *p, *fence;
	int reap_count = 0;

    report_start(START_CMPLT);

	fence = progs + active_progs;
  int ticks = get_ticks();

	while ((active_progs > 0) || (active_processes > 0)) {

		/* launch some processes */
		for (p = progs; p < fence; ++p) {
			if ((p->count > 0) && (p->pid == -1)) {
				int pid = fork();

				if (pid < 0) {
					sleep(1);
				} else if (pid == 0) {
					char *args[2];

                    report_misc("After fork(): I am a child!");

					args[0] = p->name;
					args[1] = 0;
					exec(p->name, args);
                    report_misc("exec() failed (missing object?)");
                    report_end(END_FAIL);
					exit(0xdeadbeef);
				} else {
					p->pid = pid;
					++active_processes;
				}
			}
		}

		/* reap a child */
		if (active_processes > 0) {
			int pid, status;
			pid = wait(&status);
			if (status == 0xdeadbeef)
			  exit(0);
            report_fmt("wait() done: %d with status %d", pid, status);
			++reap_count;
			for (p = progs; p < fence; ++p) {
				if (p->pid == pid) {
					p->pid = -1;
					--p->count;
					if (p->count <= 0)
						--active_progs;
					--active_processes;
				}
				if (reap_count % UPDATE_FREQUENCY == 0) {
                    report_fmt("%s: %d left", p->name, p->count);
                }
			}
		}
	}

  ticks = get_ticks() - ticks;

  int pow = 10;
  while (pow * 10 < ticks) pow *= 10;

  printf("\n"
         "*****************************************************************\n"
         "\n"
         "                      Total ticks taken\n"
         "\n");

  int i;
  for (i = 0; i < 5; i++) {
    printf("                     ");
    int tmp = ticks;
    int tmppow = pow;

    while (tmppow > 0) {
      printf("%s ", numbers[(tmp / tmppow) % 10][i]);
      tmppow /= 10;
    }
    printf("\n");
  }
  printf("\n"
         "*****************************************************************\n");

    report_end(END_SUCCESS);
	exit(0);
}
