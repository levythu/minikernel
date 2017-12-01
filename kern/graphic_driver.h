/** @file graphic_driver.h
 *
 *  @brief The definition for functions and macros for graphic driver.
 *  Note that all graphic interfaces are declared in p1kern.h, so here we do
 *  not do it again.
 *
 *  @author Leiyu Zhao (leiyuz)
 */

#ifndef GRAHPIC_DRIVER_H
#define GRAHPIC_DRIVER_H

#include <stdio.h>

#include "bool.h"
#include "cpu.h"

#define GRAPHIC_INVALID_COLOR -1
#define GRAPHIC_INVALID_POSITION -2

// Install the graphic driver. For any errors return negative integer;
// otherwise return 0;
extern int install_graphic_driver();
int initVirtualVideo(void* _vc);

void useVirtualVideo(int vcn);

#endif
