/** @file keyboard_driver.c
 *
 *  @brief implementation for functions about keyboard driver. The process for
 *  a keyboard event is in two steps:
 *  1. when a keyboard interrupt triggers, the scanCode is put in a buffer (if
 *  the buffer is full, discard it). Then the interrupt handler ends and ACK is
 *  sent
 *  2. each time readchar() is called, DFA will try to figure out the character
 *  by consuming the scanCodes in the buffer.
 *
 *  @author Leiyu Zhao (leiyuz)
 */

#include <stdint.h>
#include <simics.h>

#include "int_handler.h"
#include "timer_driver.h"
#include "keyboard_driver.h"
#include "x86/asm.h"
#include "x86/interrupt_defines.h"
#include "x86/seg.h"
#include "x86/keyhelp.h"
#include "cpu.h"

static int translateScancode(uint8_t scanCode) {
  int ret = -1;
  LocalLockR();
  kh_type augmentedChar = process_scancode(scanCode);
  if (KH_HASDATA(augmentedChar) && KH_ISMAKE(augmentedChar)) {
    ret = KH_GETCHAR(augmentedChar);
  }
  LocalUnlockR();
  return ret;
}

/******************************************************************************/

static KeyboardCallback kbCallbackAsync = NULL;
static KeyboardCallback kbCallbackSync = NULL;

// Install the keyboard driver. For any errors return negative integer;
// otherwise return 0;
int install_keyboard_driver(KeyboardCallback asyncCallback,
    KeyboardCallback syncCallback) {
  kbCallbackAsync = asyncCallback;
  kbCallbackSync = syncCallback;

  int32_t* idtBase = (int32_t*)idt_base();
  idtBase[KEY_IDT_ENTRY  << 1] = ENCRYPT_IDT_TRAPGATE_LSB(
    0, (int32_t)keyboardIntHandler, 1, SEGSEL_KERNEL_CS, 1);
  idtBase[(KEY_IDT_ENTRY  << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB(
    0, (int32_t)keyboardIntHandler, 1, SEGSEL_KERNEL_CS, 1);
  return 0;
}

// The entry for handling the keyboard interrupt event from intHandler
// It will also send ACK after pushing the scancode to buffer.
void keyboardIntHandlerInternal() {
  uint8_t scanCode = inb(KEYBOARD_PORT);
  int ch = translateScancode(scanCode);
  if (kbCallbackSync && ch >= 0) {
    kbCallbackSync(ch);
  }
  outb(INT_CTL_PORT, INT_ACK_CURRENT);
  if (kbCallbackAsync && ch >= 0) {
    kbCallbackAsync(ch);
  }
}
