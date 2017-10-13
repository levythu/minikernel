/**
 * @file swexn_basic_test.c
 * @author bblum
 * @brief Tests uninstalling handlers
 * @public yes
 * @for p4
 */

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>
#include "410_tests.h"

DEF_TEST_NAME("swexn_uninstall_test:");

#define STAQ_SIZE (4096)
char exn_staq[STAQ_SIZE];
#define EXN_STAQ_TOP ((void *)(&exn_staq[STAQ_SIZE-8]))

#define BAD_MEMORY ((void *)0x40000000)

volatile int success = 0;

void handler(void *arg, ureg_t *uregs)
{
	REPORT_MISC("Uh-oh, we weren't supposed to be called!");
	REPORT_END_FAIL;
	exit(-1);
	return;
}

int main()
{
	int ret;
	void (*x)() = (typeof(x))NULL;

	REPORT_START_ABORT;

	ret = swexn(EXN_STAQ_TOP, handler, NULL, NULL);
	if (ret < 0) {
		REPORT_MISC("swexn call rejected");
		REPORT_END_FAIL;
		exit(-1);
	}

	/* uninstall */
	ret = swexn(NULL, NULL, NULL, NULL);
	if (ret < 0) {
		REPORT_MISC("swexn call rejected (2)");
		REPORT_END_FAIL;
		exit(-1);
	}

	/* Invoke handler */
	success = *(int *)BAD_MEMORY;

	REPORT_MISC("D:");
	REPORT_END_FAIL;
	exit(-1);
}
