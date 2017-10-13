/**
 * @file swexn_regs.c
 * @brief Tests a case of the validation of uregs.
 *
 * @author mischnal
 * @for swexn p4
 * @covers swexn
 * @public yes
 */

#include <stddef.h>
#include <stdlib.h>
#include <syscall.h>

#include "simics.h"
#include "410_tests.h"

DEF_TEST_NAME("swexn_regs");

#define SWEXN_STACK_SIZE 2048
char swexn_stack[SWEXN_STACK_SIZE];
#define SWEXN_STACK ((void*)(swexn_stack + SWEXN_STACK_SIZE - 8))

#define EFLAGS_MASK 0x00003000


void handler(void *arg, ureg_t *uregs)
{
    int ret;
    REPORT_MISC(" In handler");

    uregs->eflags |= EFLAGS_MASK;

    ret = swexn(SWEXN_STACK, handler, NULL, uregs);
    if (ret < 0)
    {
        REPORT_END_SUCCESS;
        exit(0);
    }
    else
    {
        REPORT_END_FAIL;
        exit(1);
    }
}

int main(void)
{
    int ret;
    
    REPORT_START_CMPLT;

    ret = swexn(SWEXN_STACK, handler, NULL, NULL);
    if (ret < 0)
    {
        REPORT_END_FAIL;
        exit(1);
    }
    
    *(int*)NULL = 42;

    /* Should not get here */
    REPORT_END_FAIL;
    return 1;
}
