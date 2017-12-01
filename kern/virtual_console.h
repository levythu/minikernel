/** @file TODO
 *  @author Leiyu Zhao
 */

#ifndef VIRTUAL_CONSOLE_H
#define VIRTUAL_CONSOLE_H

#define MAX_LIVE_VIRTUAL_CONSOLE 10

int initVirtualConsole();
int newVirtualConsole();

void switchToVirtualConsole(int vcNumber);
void switchNextVirtualConsole();

void referVirtualConsole(int vcNumber);
void dereferVirtualConsole(int vcNumber);
void* getVirtualConsole(int vcNumber);

#endif
