/*
 * TODO
 */

#ifndef PM_H
#define PM_H

#include <stdlib.h>
#include <x86/page.h>

void claimUserMem();
uint32_t getUserMemPage();
void freeUserMemPage(uint32_t mem);

#endif
