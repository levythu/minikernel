/**
 * @file swexn_stands_for_swextensible.c
 * @author bblum
 * @brief Tests what happens when the handler faults
 * @public yes
 * @for p4
 */

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"

DEF_TEST_NAME("swexn_stands_for_swextensible:");

#define STAQ_SIZE (4096)
char exn_staq[STAQ_SIZE];
#define EXN_STAQ_TOP ((void *)(&exn_staq[STAQ_SIZE-11]))

#define BAD_MEMORY ((void *)0x40000000)

volatile int already_called = 0;

void handler(void *arg, ureg_t *uregs)
{
	REPORT_MISC("Hello from a handler");
	if (already_called == 0) {
		already_called = 1;
		/* Incur another fault (in a way that won't get optimized). */
		exit(*(int *)BAD_MEMORY);
	} else {
		REPORT_MISC("Not supposed to get called again!");
		REPORT_END_FAIL;
		exit(-1);
	}
	return;
}

int main()
{
	int ret;

	REPORT_START_ABORT;

	ret = swexn(EXN_STAQ_TOP, handler, NULL, NULL);
	if (ret < 0) {
		REPORT_MISC("swexn call rejected");
		REPORT_END_FAIL;
		exit(-1);
	}

	/* Invoke handler */
	ret = *(int *)BAD_MEMORY;

	REPORT_MISC("something bad happened!");
	REPORT_END_FAIL;
	exit(ret);
}
