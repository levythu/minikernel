#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>

#include "int_handler.h"
#include "common_kern.h"
#include "console.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "hv.h"
#include "hvlife.h"
#include "source_untrusted.h"
#include "hv_hpcall_internal.h"
#include "zeus.h"

int hpc_print(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, len, 0);
  DEFINE_PARAM(uint32_t, bufPos, 1);
  bufPos += thr->process->hyperInfo.baseAddr;

  char* buf = (char*)smalloc(HV_PRINT_MAX);
  int actualLen = sGetString(bufPos, buf, HV_PRINT_MAX);
  if (actualLen < 0 || actualLen == HV_PRINT_MAX) {
    // Text too long or invalid
    sfree(buf, HV_PRINT_MAX);
    return -1;
  }
  putbytes(buf, actualLen);
  sfree(buf, HV_PRINT_MAX);
  return 0;
}

int hpc_cons_set_term_color(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, color, 0);
  set_term_color(color);
  return 0;
}

int hpc_cons_set_cursor_pos(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, row, 0);
  DEFINE_PARAM(int, col, 1);
  if (set_cursor(row, col) != 0) {
    // TODO crash the guest
  }
  return 0;
}

int hpc_cons_get_cursor_pos(int userEsp, tcb* thr) {
  DEFINE_PARAM(uint32_t, rowAddr, 0);
  DEFINE_PARAM(uint32_t, colAddr, 1);
  rowAddr += thr->process->hyperInfo.baseAddr;
  colAddr += thr->process->hyperInfo.baseAddr;

  if (!verifyUserSpaceAddr(rowAddr, rowAddr + sizeof(int) - 1, true) ||
      !verifyUserSpaceAddr(colAddr, colAddr + sizeof(int) - 1, true)) {
    // row/col destination is not writable
    // TODO crash the guest
    return -1;
  }

  int row, col;
  get_cursor(&row, &col);
  *((int*)rowAddr) = row;
  *((int*)colAddr) = col;

  return 0;
}

int hpc_print_at(int userEsp, tcb* thr) {
  DEFINE_PARAM(int, len, 0);
  DEFINE_PARAM(uint32_t, bufPos, 1);
  DEFINE_PARAM(int, row, 2);
  DEFINE_PARAM(int, col, 3);
  DEFINE_PARAM(int, color, 4);
  bufPos += thr->process->hyperInfo.baseAddr;

  char* buf = (char*)smalloc(HV_PRINT_MAX);
  int actualLen = sGetString(bufPos, buf, HV_PRINT_MAX);
  if (actualLen < 0 || actualLen == HV_PRINT_MAX) {
    // Text too long or invalid
    sfree(buf, HV_PRINT_MAX);
    return -1;
  }

  int _color, _row, _col;
  get_term_color(&_color);
  get_cursor(&_row, &_col);

  set_term_color(color);
  if (set_cursor(row, col) != 0) {
    // TODO crash the guest
    sfree(buf, HV_PRINT_MAX);
    return -1;
  }
  putbytes(buf, actualLen);
  sfree(buf, HV_PRINT_MAX);

  set_term_color(_color);
  set_cursor(_row, _col);

  return 0;
}
