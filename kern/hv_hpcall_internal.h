/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_HPCALL_INTERNAL_H
#define HV_HPCALL_INTERNAL_H

void hyperCallHandler();

#define HPC_ON(opNumber, func) \
  if (eax == opNumber) { \
    return func(userEsp, currentThread); \
  } \

#define DEFINE_PARAM(type, name, pos) \
  type name;    \
  if (!sGetInt(userEsp + thr->process->hyperInfo.baseAddr + 4*pos,  \
               (int*)(&name)))  \
    return -1;  \

#endif
