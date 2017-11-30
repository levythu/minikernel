/** @file keyboard_event.h
 *
 *  @brief the headers for keyboard event handler.
 *
 *  This module is decoupled from keyboard handler. And kernel should attach it
 *  to keyboard event to make things work,
 *
 *  @author Leiyu Zhao
 */

#ifndef KEYBOARD_EVENT_H
#define KEYBOARD_EVENT_H

#include <stdlib.h>
#include <x86/page.h>

#include "hv.h"

intMultiplexer* getKeyboardMultiplexer();

// let kernel call it on startup!
// It must happen before register keyboard driver
void initKeyboardEvent();

// Ocuppy the keyboard, avoid other threads from listening to it (may block due
// to others listening)
void occupyKeyboard();

// release the keyboard and let other listeners in
void releaseKeyboard();

// Get a char or a string from keyboard, otherwise block the current thread.
// No validation is made on the target space, the caller should safeguard it.
// NOTE the two functions can only be called when occupying the keyboard
int getcharBlocking();
int getStringBlocking(char* space, int maxlen);

// Exposed two functions, and kernel.c should use them to register to kayboard
// driver
void onKeyboardSync(int ch);
void onKeyboardAsync(int ch);

#endif
