/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HVVM_H
#define HVVM_H

#include "process.h"

void exitPagingMode(tcb* thr);
bool swtichGuestPD(tcb* thr);
bool invalidateGuestPDAt(tcb* thr, uint32_t guestaddr);
void reActivateOriginalPD(tcb* thr);

#endif
