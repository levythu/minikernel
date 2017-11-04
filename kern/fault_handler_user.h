#ifndef FAULT_HANDLER_USER_H
#define FAULT_HANDLER_USER_H

#include <ureg.h>

#include "process.h"

void registerUserFaultHandler(tcb* thread, uint32_t esp3, uint32_t eip,
    uint32_t customArg);

void deregisterUserFaultHandler(tcb* thread);

bool makeRegisterHandlerStackAndGo(tcb* thread, ureg_t* uregs);

int translateIDTNumToCause(int idtNumber);

#define SWEXN_minimalStackSize (sizeof(ureg_t) + sizeof(uint32_t) * 5)

#endif
