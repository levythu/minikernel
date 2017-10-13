/** @file 410user/progs/readline_basic.c
 *  @author mpa
 *  @brief Tests readline()
 *  @public yes
 *  @for p3
 *  @covers readline
 *  @status done
 */

#include <syscall.h>
#include <stdlib.h>
#include <simics.h>
#include "410_tests.h"
#include <report.h>

DEF_TEST_NAME("readline_basic:");

int main() {
    char buf[100];
    report_start(START_CMPLT);
    REPORT_FAILOUT_ON_ERR(magic_readline(100, buf));
    report_misc(buf);
    report_end(END_SUCCESS);
    exit(0);
}
