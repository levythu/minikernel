/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_INTERRUPT_H
#define HV_INTERRUPT_H

#include "kmutex.h"

#define MAX_SUPPORTED_VIRTUAL_INT 134
#define MAX_WAITING_INT 512
#define MAX_CONCURRENT_VM 8

typedef struct {
  int intNum;
} hvInt;

typedef struct {
  // flag is used to ensure data integrity, so pay attention to the order of
  // setting data
  bool present;
} IDTEntry;

bool appendIntTo(void* info, hvInt hvi);

/*****************************************************************************/

typedef struct {
  void* waiter[MAX_CONCURRENT_VM];    // HyperInfo*
  kmutex mutex;
} intMultiplexer;

void initMultiplexer(intMultiplexer* mper);
void broadcastIntTo(intMultiplexer* mper, hvInt hvi);
void addToWaiter(intMultiplexer* mper, void* info);
void removeWaiter(intMultiplexer* mper, void* info);

#endif
