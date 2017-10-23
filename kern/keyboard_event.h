/** @file keyboard_event.h
 *
 *  @brief TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef KEYBOARD_EVENT_H
#define KEYBOARD_EVENT_H

#include <stdlib.h>
#include <x86/page.h>

void initKeyboardEvent();
void occupyKeyboard();
void releaseKeyboard();
int getcharBlocking();
int getStringBlocking(char* space, int maxlen);

void onKeyboardSync(int ch);
void onKeyboardAsync(int ch);

#endif
