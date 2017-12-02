/** @file hvvm.h
 *
 *  @brief Guest-vm related functions that can be used inside hv module.
 *
 *  see hv_hpcall_vm.c for Implementation
 *
 *  @author Leiyu Zhao
 */

#ifndef HVVM_H
#define HVVM_H

#include "process.h"

// recover originalPD, free its back up and permannently leave paging mode
// Hypercall is not allowed to turn off paging, so this only happens on
// hypervisor exiting
void exitPagingMode(tcb* thr);

// The wrapper of reCompileGuestPD, since reCompileGuestPD needs a in-memory
// copy of guest page directory. This function make a temporary copy, call it,
// and destroy the copy.
// NOTE: this function will access guest physical memory, so caller must
// recover originalPD before calling me
bool swtichGuestPD(tcb* thr);

// Invalidate only one mapping is complicated: we have to clear the current
// page directory, recover originalPD; while we need to preserve most.
// Simply recover originalPD will cause loss to current PD. Therefore, caller
// mustn't recover originalPD before calling this. (it's different from
// swtichGuestPD)
bool invalidateGuestPDAt(tcb* thr, uint32_t guestaddr);

// Discard current guest page directory and activate original PD.
// This is needed before crashing or want to accessing guest page directory
void reActivateOriginalPD(tcb* thr);

#endif
