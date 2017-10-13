/**
 * @file swexn_basic_test.c
 * @author bblum
 * @brief Tests a simple use case for swexn
 * @public yes
 * @for p4
 */

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"

DEF_TEST_NAME("swexn_basic_test:");

#define STAQ_SIZE (4096)
char exn_staq[STAQ_SIZE];
#define EXN_STAQ_TOP ((void *)(&exn_staq[STAQ_SIZE-7]))

#define GOOD_MEMORY ((void *)0x40000000)

void handler(void *arg, ureg_t *uregs)
{
	int ret;

	REPORT_MISC("Hello from a handler");

	if (uregs == NULL) {
		REPORT_MISC("what use is a handler without a ureg pointer?");
		REPORT_END_FAIL;
		exit(-1);
	}

	ret = new_pages(GOOD_MEMORY, STAQ_SIZE);
	if (ret < 0) {
		REPORT_MISC("new_pages failed");
		REPORT_END_FAIL;
		exit(-1);
	}
	*(int *)GOOD_MEMORY = 42;
	ret = swexn(EXN_STAQ_TOP, handler, NULL, uregs);
	if (ret < 0) {
		REPORT_MISC("second swexn rejected");
	} else {
		REPORT_MISC("restoring old position failed");
	}
	REPORT_END_FAIL;
	exit(-1);
	return;
}

int main()
{
	int ret;

	REPORT_START_CMPLT;

	ret = swexn(EXN_STAQ_TOP, handler, NULL, NULL);
	if (ret < 0) {
		REPORT_MISC("swexn call rejected");
		REPORT_END_FAIL;
		exit(-1);
	}

	/* Invoke handler */
	ret = *(int *)GOOD_MEMORY;

	if (ret == 0) {
		REPORT_MISC("handler not run?");
		REPORT_END_FAIL;
		exit(-1);
	}

	TEST_PROG_PROGRESS;
	REPORT_END_SUCCESS;
	exit(ret);
}
