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
#include <ureg.h>

#include "bool.h"

// Switch to ring3, given the user-space esp, eip and eflags. This function
// NEVER returns!
// It will set segment registers accordingly
void switchToRing3(uint32_t esp, uint32_t eflags, uint32_t eip);

// The same with ring3, but setting more registers
void switchToRing3X(uint32_t esp, uint32_t eflags, uint32_t eip, uint32_t edi,
                    uint32_t esi, uint32_t ebp, uint32_t ebx, uint32_t edx,
                    uint32_t ecx, uint32_t eax, int cs, int ds);

// Validate whether the target ureg is good to be set when entering ring3
bool validateUregs(ureg_t* uregs);

bool validateEFLAGS(uint32_t eflags);

#endif
