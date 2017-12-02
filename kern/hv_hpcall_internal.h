/** @file hv_hpcall_internal.h
 *
 *  @brief Definitions and utilities used only by hpcall submodules only
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_HPCALL_INTERNAL_H
#define HV_HPCALL_INTERNAL_H

#include "process.h"

// The universe hypercall entry, which will push registers and pass control to
// hyperCallHandler_Internal in hv_hpcall.c
void hyperCallHandler();

// Following are hpc handlers corresponding to different hypercalls

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

int hpc_setpd(int userEsp, tcb* thr);
int hpc_adjustpg(int userEsp, tcb* thr);

// Following are macros used by hypercall dispenser

// HPC_ON pass control to func when hpc number is matched
#define HPC_ON(opNumber, func) \
  if (eax == opNumber) { \
    return func(userEsp, currentThread); \
  } \

// HPC_ON pass control to extended func when hpc number is matched
// extended func consume much richer info about register value before hypercall
// happens
#define HPC_ON_X(opNumber, func) \
  if (eax == opNumber) { \
    return func(userEsp, currentThread, \
                _edi, _esi, _ebp, _ebx, _edx, _ecx, _eax); \
  } \

// Following are macros used by all hypercall handlers

// Declare and set a hypercall parameter from guest
#define DEFINE_PARAM(type, name, pos) \
  type name;    \
  if (!sGetInt(userEsp + thr->process->hyperInfo.baseAddr + 4*pos,  \
               (int*)(&name)))  \
    return -1;  \

#endif
