/** @file hvseg.c
 *
 *  @brief Segmentation setup just for guest
 *
 *  @author Leiyu Zhao
 */

#include <assert.h>
#include <simics.h>
#include <common_kern.h>
#include <x86/asm.h>

#include "hvseg.h"

#define MAKE_SEG_LOWER(Base, LimitMinusPage, DPL, Type) \
  ( \
    ( ((LimitMinusPage) >> 12) & 0xffff  ) | \
    ( ((Base) & 0xffff) << 16            ) \
  )

#define MAKE_SEG_HIGHER(Base, LimitMinusPage, DPL, Type) \
  ( \
    ( (Base) & 0xff000000                 ) | \
    ( 0xc << 20                           ) | \
    ( ((LimitMinusPage) >> 12) & 0xf0000  ) | \
    ( 0x9 << 12                           ) | \
    ( ((DPL) & 0x3) << 13                 ) | \
    ( ((Type) & 0xf) << 8                 ) | \
    ( ((Base) >> 16) & 0xff               ) \
  )

void setupVirtualSegmentation() {
  uint32_t* gdt = (uint32_t*)gdt_base();

  // Guest code, SEGSEL_SPARE0 -> SEGSEL_GUEST_CS
  gdt[SEGSEL_SPARE0_IDX * 2] = MAKE_SEG_LOWER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, SEG_TYPE_CODE);
  gdt[SEGSEL_SPARE0_IDX * 2 + 1] = MAKE_SEG_HIGHER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, SEG_TYPE_CODE);

  // Guest data, SEGSEL_SPARE1 -> SEGSEL_GUEST_DS
  gdt[SEGSEL_SPARE1_IDX * 2] = MAKE_SEG_LOWER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, SEG_TYPE_DATA);
  gdt[SEGSEL_SPARE1_IDX * 2 + 1] = MAKE_SEG_HIGHER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, SEG_TYPE_DATA);
}
