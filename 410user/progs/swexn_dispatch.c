/**
 * @file swexn_dispatch.c
 * @author bblum
 * @brief Tests swexn basic function dispatch
 * @public yes
 * @for p4
 */

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"

DEF_TEST_NAME("swexn_dispatch:");

#define STAQ_SIZE (4096)
char exn_staq[STAQ_SIZE];
#define EXN_STAQ_TOP ((void *)(&exn_staq[STAQ_SIZE-4]))

#define COOKIE ((void *)0x0dd0ad9b)

void handler(void *arg, ureg_t *uregs)
{
	REPORT_MISC("Hello from a handler");

	if (arg != COOKIE) {
		/* this fight is not worth fighting */
		REPORT_MISC("C is for cookie; it's not good enough for me...");
		REPORT_END_FAIL;
		exit(-1);
	}

	REPORT_END_SUCCESS;
	exit(42);
}

int main()
{
	int ret;

	REPORT_START_CMPLT;

	ret = swexn(EXN_STAQ_TOP, handler, COOKIE, NULL);
	if (ret < 0) {
		REPORT_MISC("swexn call rejected");
		REPORT_END_FAIL;
		exit(-1);
	}

	/* Invoke handler */
	ret = *(int *)NULL;
	REPORT_MISC("should not get here!");
	REPORT_END_FAIL;
	exit(ret);
}
