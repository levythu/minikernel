/*
 * TODO
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void yieldToNext();
tcb* pickNextRunnableThread(tcb* currentThread);

#endif
