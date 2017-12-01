/** @file keyboard_driver.h
 *
 *  @brief The definition for functions and macros for keyboard driver.
 *  Note that all keyboard interfaces are declared in p1kern.h, so here we do
 *  not do it again.
 *
 *  @author Leiyu Zhao (leiyuz)
 */

#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <stdio.h>

#include "bool.h"

// The type for kb callback
typedef void (*KeyboardCallback)(int ch);

// Install the keyboard driver. For any errors return negative integer;
// otherwise return 0;
extern int install_keyboard_driver(KeyboardCallback asyncCallback,
    KeyboardCallback syncCallback);

#endif
