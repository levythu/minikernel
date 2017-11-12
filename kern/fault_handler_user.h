/** @file fault_handler_user.h
 *
 *  @brief User-caused fault handler and related helper functions
 *
 *  @author Leiyu Zhao
 */

#ifndef FAULT_HANDLER_USER_H
#define FAULT_HANDLER_USER_H

#include <ureg.h>

#include "process.h"

// Register a user-space fault handler to one thread. Thread should be owned.
// Won't do validation on any param -- the caller should do it
void registerUserFaultHandler(tcb* thread, uint32_t esp3, uint32_t eip,
    uint32_t customArg);

// Remove a user-space fault handler from one thread. Thread should be owned.
void deregisterUserFaultHandler(tcb* thread);

// Consume uregs, return to ring3 with target registers.
// Should never return on success, or false on failure
bool makeRegisterHandlerStackAndGo(tcb* thread, ureg_t* uregs);

// Helper function, translate a idtnumber (provided in kernel fault handler)
// to a ureg_t.cause
int translateIDTNumToCause(int idtNumber);

#define SWEXN_minimalStackSize (sizeof(ureg_t) + sizeof(uint32_t) * 5)

#endif
