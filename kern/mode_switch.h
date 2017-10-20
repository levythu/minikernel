/** @file mode_switch.h
 *
 *  @brief Infrastructure for mode switch
 *
 *  Mode switch is diffrent from context switch in that it never switch to
 *  other thread. Instead, this is the point that kernel mode go into user
 *  mode (ring3)
 *
 *  @author Leiyu Zhao
 */

#ifndef MODE_SWITCH_H
#define MODE_SWITCH_H

#include <stdint.h>

// Switch to ring3, given the user-space esp, eip and eflags. This function
// NEVER returns!
// It will set segment registers accordingly
void switchToRing3(uint32_t esp, uint32_t eflags, uint32_t eip);

#endif
