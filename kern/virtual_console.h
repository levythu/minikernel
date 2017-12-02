/** @file virtual_console.h
 *
 *  @brief The functions of virtual console.
 *
 *  @author Leiyu Zhao
 */

#ifndef VIRTUAL_CONSOLE_H
#define VIRTUAL_CONSOLE_H

#define MAX_LIVE_VIRTUAL_CONSOLE 10

// init virtual console, must be called by kernel before install drivers
int initVirtualConsole();

// create a new VC and return its VCNumber. Or -1 when fail
int newVirtualConsole();

// Switch to given virtual console. VCNumber must be a known live one
void switchToVirtualConsole(int vcNumber);
// Switch to the next live virtual console, or remain unchange if there's only
// one
void switchNextVirtualConsole();

// Refer to a VC, happens when forking a new process
void referVirtualConsole(int vcNumber);

// Derefer a VC, happens when a process switches to new console or vanish
void dereferVirtualConsole(int vcNumber);

// Get the virtualConsole* given the VCNumber
void* getVirtualConsole(int vcNumber);

// For debug. Print all virtual console info
void reportVC();

#endif
