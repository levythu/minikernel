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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "console.h"
#include "graphic_driver.h"
#include "x86/video_defines.h"
#include "x86/asm.h"

static int16_t characterBuffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];
static int currentCursorX, currentCursorY;  // relative cursor position
// the startX for characterBuffer, relative position should add this to become
// the absolute position in characterBuffer.
static int validStartX;

static int currentColor;
static bool showCursor;

static const int defaultColor = BGND_BLACK | FGND_LGRAY;
static const int blankChar = ' ';

#define MAKE_CCHAR(ch, color) (((color) << 8) + (ch))
#define CCHAR_TO_CHAR(cchar) ((cchar) & 0xff)
static void syncCursor(bool toggling);

// Synchronzie the buffer to graphic memory, of lines [startX, endX)
// Note that the start/end line number is relative to validStartX
static void syncGraphicMemory(int relativeStartX, int relativeEndX) {
  int i = 0;
  for (i = relativeStartX; i < relativeEndX; i++) {
    int theLine = (validStartX + i) % CONSOLE_HEIGHT;
    memcpy((char*)(CONSOLE_MEM_BASE + 2*i*CONSOLE_WIDTH),
           characterBuffer[theLine],
           2*CONSOLE_WIDTH);
  }
}

// Move the cursor to the first character of new line. If the current buffer
// is full, scroll happens and the earliest line is discarded
static void moveCursorNewLine() {
  currentCursorY = 0;
  if (currentCursorX < CONSOLE_HEIGHT-1) {
    currentCursorX++;
  } else {
    int j;
    for (j = 0; j < CONSOLE_WIDTH; j++) {
      characterBuffer[validStartX][j] = MAKE_CCHAR(blankChar, defaultColor);
    }
    validStartX++;
    // Since scroll happens, everything in the graphic memory may have to be
    // re-synced
    syncGraphicMemory(0, CONSOLE_HEIGHT);
  }
  syncCursor(false);
}

// Move cursor to the next position, typically the next column; if currently
// it is the last column, moveCursorNewLine() os called..
static void moveCursorNext() {
  if (currentCursorY < CONSOLE_WIDTH-1) {
    currentCursorY++;
  } else {
    moveCursorNewLine();
  }
  syncCursor(false);
}

static bool checkValidColor(int color) {
  return !(color & ~0xff);
}

// Synchronzie the logical cursor position to physical, depending on whether
// we want to show it. When we decide to hide it, typically we just do nothing
// However, when toggling is set to true, we will forcely hide the cursor by
// moving the physical cursor outside screen.
static void syncCursor(bool toggling) {
  int pos;
  if (showCursor) {
    pos = currentCursorY + currentCursorX * CONSOLE_WIDTH;
  } else if (toggling) {
    pos = CONSOLE_HEIGHT * CONSOLE_WIDTH;
  } else {
    return;
  }
  outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
  outb(CRTC_DATA_REG, pos & 0xff);
  outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
  outb(CRTC_DATA_REG, (pos >> 8) & 0xff);
}

/******************************************************************************/

int install_graphic_driver() {
  currentColor = defaultColor;
  showCursor = true;
  clear_console();
  return 0;
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
int putbyte(char ch) {
  if (ch == '\n') {
    moveCursorNewLine();
  } else if (ch == '\r') {
    currentCursorY = 0;
    syncCursor(false);
  } else if (ch == '\b') {
    if (currentCursorY > 0) {
      currentCursorY--;
      draw_char(currentCursorX, currentCursorY, blankChar, currentColor);
      syncCursor(false);
    }
  } else {
    draw_char(currentCursorX, currentCursorY, ch, currentColor);
    moveCursorNext();
  }
  return ch;
}

// Print the string with length len on the screen by repeatedly calling
// putbyte(). If s is NULL nothing will happen
void putbytes(const char *s, int len) {
  if (len <= 0 || !s) return;
  int i;
  for (i = 0; i < len; i++) {
    putbyte(s[i]);
  }
}

// Set the current terminal color, successful call will return 0, otherwise a
// negative integer
int set_term_color(int color) {
  if (!checkValidColor(color)) return GRAPHIC_INVALID_COLOR;
  currentColor = color;
  return 0;
}

// Get the current terminal color
void get_term_color(int *color) {
  *color = currentColor;
}

// Set the current cursor position, successful call will return 0, otherwise a
// negative integer
int set_cursor(int row, int col) {
  if (row<0 || row>=CONSOLE_HEIGHT || col<0 || col>=CONSOLE_WIDTH) {
    return GRAPHIC_INVALID_POSITION;
  }
  currentCursorX = row;
  currentCursorY = col;
  syncCursor(false);
  return 0;
}

// Get the current cursor position
void get_cursor(int *row, int *col) {
  *row = currentCursorX;
  *col = currentCursorY;
}

// Hide the physical cursor on the screen
void hide_cursor() {
  showCursor = false;
  syncCursor(true);
}

// Show the physical cursor on the screen
void show_cursor() {
  showCursor = true;
  syncCursor(true);
}

// Clear the whole console with current background. Reset the cursor to the 1st
// col in the 1st row.
void clear_console() {
  int i, j;
  for (i = 0; i < CONSOLE_HEIGHT; i++) {
    for (j = 0; j < CONSOLE_WIDTH; j++) {
      characterBuffer[i][j] = MAKE_CCHAR(blankChar, currentColor);
    }
  }
  currentCursorX = currentCursorY = 0;
  syncCursor(false);
  validStartX = 0;
  syncGraphicMemory(0, CONSOLE_HEIGHT);
}

// Draw a character on given position with given color; if the position or color
// is invalid, nothing will happen; if the character is not printable, nothing
// happens either.
// NOTE this function is lower level and has nothing to do with cursor,
// scrolling, etc.. Unless you know what you are doing, most of time you want to
// use put_char() or put_chars()
void draw_char(int row, int col, int ch, int color) {
  if (row<0 || row>=CONSOLE_HEIGHT || col<0 || col>=CONSOLE_WIDTH) return;
  if (!isprint(ch)) return;
  if (!checkValidColor(color)) return;

  int absRow = (validStartX + row) % CONSOLE_HEIGHT;
  characterBuffer[absRow][col] = MAKE_CCHAR(ch, color);
  syncGraphicMemory(row, row+1);
}

// Get the char on a given point of screen
// For any errors a negative value is returnedl; otherwise a valid ASCII char
// is returned.
char get_char(int row, int col) {
  if (row<0 || row>=CONSOLE_HEIGHT || col<0 || col>=CONSOLE_WIDTH) {
    return GRAPHIC_INVALID_POSITION;
  }
  row = (validStartX + row) % CONSOLE_HEIGHT;
  return CCHAR_TO_CHAR(characterBuffer[row][col]);
}