/** @file virtual_console_dev.h
 *
 *  @brief The definition of A vitual console,
 *  migrated from old graphic drive and keyboard event
 *
 *  Due to the good incapsulation of old design, all we need to do to support
 *  virtual console is to migrate those static global variables to member field
 *  inside vc data structure.
 *
 *  Each VC has a VCNumber, and all keyboard and screen related API are changed
 *  to include the VC number, so that it will modify correct VC. The VC number
 *  is kept as the index of actual VC structure in an array
 *
 *  TAB serves as the switching hotkey between VCs.
 *
 *  fork() will make the new process have the same VC. And new_console() will
 *  create another and switch the new one. If any VC has no process in it, it
 *  will display a message on that VC, warning user that the next time the VC
 *  is switched out, it will be closed.
 *
 *  @author Leiyu Zhao
 */

#ifndef VIRTUAL_CONSOLE_DEV_H
#define VIRTUAL_CONSOLE_DEV_H

#include <stdint.h>

#include "bool.h"
#include "cpu.h"
#include "kmutex.h"
#include "process.h"

#define KEY_BUFFER_SIZE 512

// Migrated from key_event.c
typedef struct {
  // Hold this mutex to occupy a keyboard
  kmutex keyboardHolder;

  bool waitingForAnyChar; // true for char, false for string

  // a thread that's waiting the keyboard
  tcb* eventWaiter;
  CrossCPULock latch;
  // the multiplexer to distribute key event to all keyboard in this console
  intMultiplexer kbMul;

  // The circular buffer for keyevent
  int keyPressBuffer[KEY_BUFFER_SIZE];
  int bufferStart;
  int bufferEnd;
} virtualKeyboard;

// Migrated from graphic_driver.c
typedef struct {
  CrossCPULock videoLock;
  kmutex longPrintLock;

  int16_t characterBuffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];
  int currentCursorX, currentCursorY;  // relative cursor position
  // the startX for characterBuffer, relative position should add this to become
  // the absolute position in characterBuffer.
  int validStartX;

  int currentColor;
  bool showCursor;
} virtualGraphix;

// A virtual console
typedef struct {
  virtualKeyboard i;
  virtualGraphix o;
  // The number of processes living inside. When reducing to 0, will turn dead
  int ref;
  // If dead, it will be removed next time it is switched away
  bool dead;
  // Should be the same as its index in vcList
  int vcNumber;
} virtualConsole;


#endif
