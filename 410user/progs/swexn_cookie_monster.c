/**
 * @file swexn_cookie_monster.c
 * @brief Tests the correct passing of arguments to swexn handlers
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

DEF_TEST_NAME("swexn_cookie_monster");

#define SWEXN_STACK_SIZE 2048
char swexn_stack[SWEXN_STACK_SIZE];
#define SWEXN_STACK ((void*)(swexn_stack + SWEXN_STACK_SIZE - 8))

void *cookies[] = {(void*)0xfeedbeef, (void*)0x8badf00d, (void*)0xbaddcafe,
                 (void*)0xc0ffee, (void*)0xface, (void*) 0x12345678,
                 (void*)42, (void*)57, (void*)2953575118U};
int curr_cookie = 0;
int first_pass = 1;



void handler(void *arg, ureg_t *uregs)
{
    int ret;

    if (cookies[curr_cookie] != arg)
    {
        REPORT_MISC(" Cookie incorrect");
        REPORT_END_FAIL;
        exit(1);
    }
    
    curr_cookie++;
    if (curr_cookie < sizeof(cookies) / sizeof(cookies[0]))
    {
        ret = swexn(SWEXN_STACK, handler, cookies[curr_cookie], uregs);
        if (ret < 0)
        {
            REPORT_MISC(" Failed to install handler");
            REPORT_END_FAIL;
            exit(1);
        }
    }
    else
    {
        /* Out of cookies */
        REPORT_END_SUCCESS;
        exit(0);
    }
}

int main(void)
{
    int ret;
    if (first_pass)
    {
        REPORT_START_CMPLT;
        first_pass = 0;
    }

    ret = swexn(SWEXN_STACK, handler, cookies[curr_cookie], NULL);
    if (ret < 0)
    {
        REPORT_MISC(" Failed to install handler");
        REPORT_END_FAIL;
        exit(1);
    }
    
    *(int*)NULL = 42;

    /* Should not get here */
    REPORT_MISC(" Something is very wrong");
    REPORT_END_FAIL;
    return 1;
}
