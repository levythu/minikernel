/** @file graphic_driver.c
 *
 *  @brief implementation for functions about graphic driver. Internally, we
 *  keep replicated version for the whole screen memory as well as color and
 *  scroll position.
 *  For replicated screen memory, we can easily extend it to buffer
 *  out-of-screen contents and implement scrollbar-like things
 *
 *  @author Leiyu Zhao (leiyuz)
 */

// Optional TODO: make it two phase printing: one pushing things into queue,
// another make movement
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <simics.h>
#include <ctype.h>

#include "console.h"
#include "graphic_driver.h"
#include "x86/video_defines.h"
#include "x86/asm.h"
#include "cpu.h"
#include "kmutex.h"
#include "virtual_console_dev.h"
#include "virtual_console.h"

static const int defaultColor = BGND_BLACK | FGND_LGRAY;
static const int blankChar = ' ';

static virtualConsole* currentVC = NULL;
static CrossCPULock syncLock;

#define MAKE_CCHAR(ch, color) (((color) << 8) + (ch))
#define CCHAR_TO_CHAR(cchar) ((cchar) & 0xff)
static void syncCursor(int vcn, bool toggling);

// Synchronzie the buffer to graphic memory, of lines [startX, endX)
// Note that the start/end line number is relative to vc->o.validStartX
static void syncGraphicMemory(int vcn, int relativeStartX, int relativeEndX) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&syncLock);
  if (vc != currentVC) {
    GlobalUnlockR(&syncLock);
    return;
  }
  int i = 0;
  for (i = relativeStartX; i < relativeEndX; i++) {
    int theLine = (vc->o.validStartX + i) % CONSOLE_HEIGHT;
    memcpy((char*)(CONSOLE_MEM_BASE + 2*i*CONSOLE_WIDTH),
           vc->o.characterBuffer[theLine],
           2*CONSOLE_WIDTH);
  }
  GlobalUnlockR(&syncLock);
}

// Move the cursor to the first character of new line. If the current buffer
// is full, scroll happens and the earliest line is discarded
static void moveCursorNewLine(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  vc->o.currentCursorY = 0;
  if (vc->o.currentCursorX < CONSOLE_HEIGHT-1) {
    vc->o.currentCursorX++;
  } else {
    int j;
    for (j = 0; j < CONSOLE_WIDTH; j++) {
      vc->o.characterBuffer[vc->o.validStartX][j] =
          MAKE_CCHAR(blankChar, defaultColor);
    }
    vc->o.validStartX = (vc->o.validStartX + 1) % CONSOLE_HEIGHT;
    // Since scroll happens, everything in the graphic memory may have to be
    // re-synced
    syncGraphicMemory(vcn, 0, CONSOLE_HEIGHT);
  }
  syncCursor(vcn, false);
}

// Move cursor to the next position, typically the next column; if currently
// it is the last column, moveCursorNewLine() os called..
static void moveCursorNext(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  if (vc->o.currentCursorY < CONSOLE_WIDTH-1) {
    vc->o.currentCursorY++;
  } else {
    moveCursorNewLine(vcn);
  }
  syncCursor(vcn, false);
}

static bool checkValidColor(int color) {
  return !(color & ~0xff);
}

// Synchronzie the logical cursor position to physical, depending on whether
// we want to show it. When we decide to hide it, typically we just do nothing
// However, when toggling is set to true, we will forcely hide the cursor by
// moving the physical cursor outside screen.
static void syncCursor(int vcn, bool toggling) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&syncLock);
  if (vc != currentVC) {
    GlobalUnlockR(&syncLock);
    return;
  }
  int pos;
  if (vc->o.showCursor) {
    pos = vc->o.currentCursorY + vc->o.currentCursorX * CONSOLE_WIDTH;
  } else if (toggling) {
    pos = CONSOLE_HEIGHT * CONSOLE_WIDTH;
  } else {
    GlobalUnlockR(&syncLock);
    return;
  }
  outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
  outb(CRTC_DATA_REG, pos & 0xff);
  outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
  outb(CRTC_DATA_REG, (pos >> 8) & 0xff);
  GlobalUnlockR(&syncLock);
}

/******************************************************************************/

int initVirtualVideo(void* _vc) {
  virtualConsole* vc = (virtualConsole*)_vc;
  initCrossCPULock(&vc->o.videoLock);
  kmutexInit(&vc->o.longPrintLock);
  vc->o.currentColor = defaultColor;
  vc->o.showCursor = true;
  for (int i = 0; i < CONSOLE_HEIGHT; i++) {
    for (int j = 0; j < CONSOLE_WIDTH; j++) {
      vc->o.characterBuffer[i][j] = MAKE_CCHAR(blankChar, vc->o.currentColor);
    }
  }
  vc->o.currentCursorX = vc->o.currentCursorY = 0;
  vc->o.validStartX = 0;
  return 0;
}

int install_graphic_driver() {
  initCrossCPULock(&syncLock);
  return 0;
}

void useVirtualVideo(int vcn) {
  GlobalLockR(&syncLock);
  currentVC = (virtualConsole*)getVirtualConsole(vcn);
  syncGraphicMemory(vcn, 0, CONSOLE_HEIGHT);
  GlobalUnlockR(&syncLock);
}

// Put a byte on the current cursor of screen, with the current color
// If the character is a newline ('\n'), the cursor is moved to the beginning
// of the next line (scrolling if necessary).
// If the character is a carriage return ('\r'), the cursor is immediately
// reset to the beginning of the current line, causing any future output to
// overwrite any existing output on the line.
// If backspace ('\b') is encountered, the previous character is erased and
// cursor is backed. If there is no character in the line, then nothing will
// happen.
int putbyte_(int vcn, char ch) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&vc->o.videoLock);
  if (ch == '\n') {
    moveCursorNewLine(vcn);
  } else if (ch == '\r') {
    vc->o.currentCursorY = 0;
    syncCursor(vcn, false);
  } else if (ch == '\b') {
    if (vc->o.currentCursorY > 0) {
      vc->o.currentCursorY--;
      draw_char(vcn, vc->o.currentCursorX, vc->o.currentCursorY,
          blankChar, vc->o.currentColor);
      syncCursor(vcn, false);
    }
  } else {
    draw_char(vcn, vc->o.currentCursorX, vc->o.currentCursorY, ch,
        vc->o.currentColor);
    moveCursorNext(vcn);
  }
  GlobalUnlockR(&vc->o.videoLock);
  return ch;
}

int putbyte(char ch) {
  return putbyte_(currentVC->vcNumber, ch);
}

// Print the string with length len on the screen by repeatedly calling
// putbyte_(). If s is NULL nothing will happen
// Unlike putbyte, which use global spinlock for atomicity, putbytes use kmutex
// that is, it can be interleaved by some putbyte. (Imagine putbytes put long
// string, so we allow context switch!)
void putbytes(int vcn, const char *s, int len) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  if (len <= 0 || !s) return;
  int i;
  kmutexWLock(&vc->o.longPrintLock);
  for (i = 0; i < len; i++) {
    putbyte_(vcn, s[i]);
  }
  kmutexWUnlock(&vc->o.longPrintLock);
}

// Set the current terminal color, successful call will return 0, otherwise a
// negative integer
int set_term_color(int vcn, int color) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  if (!checkValidColor(color)) return GRAPHIC_INVALID_COLOR;
  GlobalLockR(&vc->o.videoLock);
  vc->o.currentColor = color;
  GlobalUnlockR(&vc->o.videoLock);
  return 0;
}

// Get the current terminal color
void get_term_color(int vcn, int *color) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&vc->o.videoLock);
  *color = vc->o.currentColor;
  GlobalUnlockR(&vc->o.videoLock);
}

// Set the current cursor position, successful call will return 0, otherwise a
// negative integer
int set_cursor(int vcn, int row, int col) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  if (row<0 || row>=CONSOLE_HEIGHT || col<0 || col>=CONSOLE_WIDTH) {
    return GRAPHIC_INVALID_POSITION;
  }
  GlobalLockR(&vc->o.videoLock);
  vc->o.currentCursorX = row;
  vc->o.currentCursorY = col;
  syncCursor(vcn, false);
  GlobalUnlockR(&vc->o.videoLock);
  return 0;
}

// Get the current cursor position
void get_cursor(int vcn, int *row, int *col) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&vc->o.videoLock);
  *row = vc->o.currentCursorX;
  *col = vc->o.currentCursorY;
  GlobalUnlockR(&vc->o.videoLock);
}

// Hide the physical cursor on the screen
void hide_cursor(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&vc->o.videoLock);
  vc->o.showCursor = false;
  syncCursor(vcn, true);
  GlobalUnlockR(&vc->o.videoLock);
}

// Show the physical cursor on the screen
void show_cursor(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&vc->o.videoLock);
  vc->o.showCursor = true;
  syncCursor(vcn, true);
  GlobalUnlockR(&vc->o.videoLock);
}

// Clear the whole console with current background. Reset the cursor to the 1st
// col in the 1st row.
void clear_console(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  GlobalLockR(&vc->o.videoLock);
  int i, j;
  for (i = 0; i < CONSOLE_HEIGHT; i++) {
    for (j = 0; j < CONSOLE_WIDTH; j++) {
      vc->o.characterBuffer[i][j] = MAKE_CCHAR(blankChar, vc->o.currentColor);
    }
  }
  vc->o.currentCursorX = vc->o.currentCursorY = 0;
  syncCursor(vcn, false);
  vc->o.validStartX = 0;
  syncGraphicMemory(vcn, 0, CONSOLE_HEIGHT);
  GlobalUnlockR(&vc->o.videoLock);
}

// Draw a character on given position with given color; if the position or color
// is invalid, nothing will happen; if the character is not printable, nothing
// happens either.
// NOTE this function is lower level and has nothing to do with cursor,
// scrolling, etc.. Unless you know what you are doing, most of time you want to
// use put_char() or put_chars()
void draw_char(int vcn, int row, int col, int ch, int color) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  if (row<0 || row>=CONSOLE_HEIGHT || col<0 || col>=CONSOLE_WIDTH) return;
  if (!isprint(ch)) return;
  if (!checkValidColor(color)) return;

  GlobalLockR(&vc->o.videoLock);
  int absRow = (vc->o.validStartX + row) % CONSOLE_HEIGHT;
  vc->o.characterBuffer[absRow][col] = MAKE_CCHAR(ch, color);
  syncGraphicMemory(vcn, row, row+1);
  GlobalUnlockR(&vc->o.videoLock);
}

// Get the char on a given point of screen
// For any errors a negative value is returnedl; otherwise a valid ASCII char
// is returned.
char get_char(int vcn, int row, int col) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  if (row<0 || row>=CONSOLE_HEIGHT || col<0 || col>=CONSOLE_WIDTH) {
    return GRAPHIC_INVALID_POSITION;
  }
  GlobalLockR(&vc->o.videoLock);
  row = (vc->o.validStartX + row) % CONSOLE_HEIGHT;
  int16_t res = vc->o.characterBuffer[row][col];
  GlobalUnlockR(&vc->o.videoLock);
  return CCHAR_TO_CHAR(res);
}
