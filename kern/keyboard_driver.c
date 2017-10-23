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

// keyPressBuffer is a cyclic array, with range [bufferStart, bufferEnd)
// When bufferStart==bufferEnd, the buffer is actually empty.
// When bufferStart==bufferEnd+1 (cyclic), the buffer is full, despite that
// there's still one slot left.
int keyPressBuffer[KEY_BUFFER_SIZE];
int bufferStart, bufferEnd;

// Push a new key event (descripted by scanCode) to the end of the queue. If the
// queue is full, discard it. NOTE: the function is not interleavable, so caller
// MUST guarantee that only one instance of pushCharEvent and fetchCharEvent is
// running.
static int pushCharEvent(uint8_t scanCode) {
  int ret = -1;
  LocalLockR();
  kh_type augmentedChar = process_scancode(scanCode);
  if (KH_HASDATA(augmentedChar) && KH_ISMAKE(augmentedChar)) {
    int bufferNext = (bufferEnd + 1) % KEY_BUFFER_SIZE;
    if (bufferNext == bufferStart) {
      // When the buffer is full, we discard any new event.
      LocalUnlockR();
      return ret;
    }
    keyPressBuffer[bufferEnd] = KH_GETCHAR(augmentedChar);
    bufferEnd = bufferNext;
    ret = KH_GETCHAR(augmentedChar);
  }
  LocalUnlockR();
  return ret;
}

// return the earliest char in the queue. If there's nothing, return -1;
// NOTE: the function is not interleavable, so caller MUST guarantee that only
// one instance of pushCharEvent and fetchCharEvent is running.
int fetchCharEvent() {
  LocalLockR();
  if (bufferStart == bufferEnd) {
    LocalUnlockR();
    return -1;
  }
  int ret = keyPressBuffer[bufferStart];
  bufferStart = (bufferStart + 1) % KEY_BUFFER_SIZE;
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
  bufferStart = bufferEnd = 0;
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
  int ch = pushCharEvent(scanCode);
  if (kbCallbackSync && ch >= 0) {
    kbCallbackSync(ch);
  }
  outb(INT_CTL_PORT, INT_ACK_CURRENT);
  if (kbCallbackAsync && ch >= 0) {
    kbCallbackAsync(ch);
  }
}
