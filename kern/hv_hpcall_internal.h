/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_HPCALL_INTERNAL_H
#define HV_HPCALL_INTERNAL_H

#include "process.h"

void hyperCallHandler();

int hpc_magic(int userEsp, tcb* thr);
int hpc_exit(int userEsp, tcb* thr);

int hpc_print(int userEsp, tcb* thr);
int hpc_cons_set_term_color(int userEsp, tcb* thr);
int hpc_cons_set_cursor_pos(int userEsp, tcb* thr);
int hpc_cons_get_cursor_pos(int userEsp, tcb* thr);
int hpc_print_at(int userEsp, tcb* thr);

int hpc_disable_interrupts(int userEsp, tcb* thr);
int hpc_enable_interrupts(int userEsp, tcb* thr);
int hpc_setidt(int userEsp, tcb* thr);
int hpc_iret(int userEsp, tcb* thr,
    int oedi, int oesi, int oebp, int oebx, int oedx, int oecx, int oeax);

#define HPC_ON(opNumber, func) \
  if (eax == opNumber) { \
    return func(userEsp, currentThread); \
  } \

#define HPC_ON_X(opNumber, func) \
  if (eax == opNumber) { \
    return func(userEsp, currentThread, \
                _edi, _esi, _ebp, _ebx, _edx, _ecx, _eax); \
  } \

#define DEFINE_PARAM(type, name, pos) \
  type name;    \
  if (!sGetInt(userEsp + thr->process->hyperInfo.baseAddr + 4*pos,  \
               (int*)(&name)))  \
    return -1;  \

#endif
