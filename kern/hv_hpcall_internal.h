/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_HPCALL_INTERNAL_H
#define HV_HPCALL_INTERNAL_H

void hyperCallHandler();

#define HPC_ON(opNumber, func) \
  if (eax == opNumber) { \
    return func(userEsp); \
  } \


#endif
