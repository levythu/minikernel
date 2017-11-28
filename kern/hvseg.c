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
      3, 0xb);
  gdt[SEGSEL_SPARE0_IDX * 2 + 1] = MAKE_SEG_HIGHER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, 0xb);

  // Guest data, SEGSEL_SPARE1 -> SEGSEL_GUEST_DS
  gdt[SEGSEL_SPARE1_IDX * 2] = MAKE_SEG_LOWER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, SEG_TYPE_DATA);
  gdt[SEGSEL_SPARE1_IDX * 2 + 1] = MAKE_SEG_HIGHER(
      GUEST_PHYSICAL_START,
      GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE,
      3, SEG_TYPE_DATA);
  // assert(MAKE_SEG_LOWER(0, 0xfffff000, 0, 0xb) ==
  //     gdt[SEGSEL_KERNEL_CS_IDX * 2]);
  // assert(MAKE_SEG_HIGHER(0, 0xfffff000, 0, 0xb) ==
  //     gdt[SEGSEL_KERNEL_CS_IDX * 2 + 1]);
  //
  // assert(MAKE_SEG_LOWER(0, 0xfffff000, 0, SEG_TYPE_DATA) ==
  //     gdt[SEGSEL_KERNEL_DS_IDX * 2]);
  // assert(MAKE_SEG_HIGHER(0, 0xfffff000, 0, SEG_TYPE_DATA) ==
  //     gdt[SEGSEL_KERNEL_DS_IDX * 2 + 1]);
  //
  // assert(MAKE_SEG_LOWER(0, 0xfffff000, 3, 0xb) ==
  //     gdt[SEGSEL_USER_CS_IDX * 2]);
  // assert(MAKE_SEG_HIGHER(0, 0xfffff000, 3, 0xb) ==
  //     gdt[SEGSEL_USER_CS_IDX * 2 + 1]);
  //
  // assert(MAKE_SEG_LOWER(0, 0xfffff000, 3, SEG_TYPE_DATA) ==
  //     gdt[SEGSEL_USER_DS_IDX * 2]);
  // assert(MAKE_SEG_HIGHER(0, 0xfffff000, 3, SEG_TYPE_DATA) ==
  //     gdt[SEGSEL_USER_DS_IDX * 2 + 1]);
}
