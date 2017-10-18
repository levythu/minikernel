/** @file driver.c
 *
 *  @brief Skeleton implementation of device-driver library.

 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>

#include "graphic_driver.h"
#include "keyboard_driver.h"
#include "timer_driver.h"
#include "x86/asm.h"
#include "cpu.h"

int handler_install(void (*tickback)(unsigned int), void (*keyback)()) {
  int rc;
  lprintf("Installing video driver...");
  if ((rc = install_graphic_driver()) != 0) {
    return rc;
  }
  lprintf("Installing timer driver...");
  // We skip install timer callback if tickback is not provided
  if ((rc = install_timer_driver(tickback)) != 0) {
    return rc;
  }
  lprintf("Installing keyboard driver...");
  if ((rc = install_keyboard_driver(keyback)) != 0) {
    return rc;
  }
  EnableInterrupts();
  return 0;
}
