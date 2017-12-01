/** @file TODO
 *
 *  Migrated from old graphic drive and keyboard event
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

typedef struct {
  kmutex keyboardHolder;

  bool waitingForAnyChar; // true for char, false for string
  tcb* eventWaiter;
  CrossCPULock latch;
  intMultiplexer kbMul;

  int keyPressBuffer[KEY_BUFFER_SIZE];
  int bufferStart;
  int bufferEnd;
} virtualKeyboard;

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

typedef struct {
  virtualKeyboard i;
  virtualGraphix o;
  int ref;
  bool dead;
  int vcNumber;
} virtualConsole;


#endif
