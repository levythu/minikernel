/** @file TODO
 *  @author Leiyu Zhao
 */

#ifndef VIRTUAL_CONSOLE_H
#define VIRTUAL_CONSOLE_H

#define MAX_LIVE_VIRTUAL_CONSOLE 10

int initVirtualConsole();

void useVirtualConsole(int vcNumber);
int newVirtualConsole();

void referVirtualConsole(int vcNumber);
void dereferVirtualConsole(int vcNumber);
void* getVirtualConsole(int vcNumber);

#endif
