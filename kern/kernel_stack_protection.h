/** @file kernel_stack_protection.h
 *
 *  @brief Stack protection helpers
 *
 *  This module detect kernel stack misbehaviour and panic the whole system
 *  in time before it makes mysterious undefined behavior.
 *
 *  @author Leiyu Zhao
 */

#ifndef KERNEL_STACK_PRO_H
#define KERNEL_STACK_PRO_H

void checkKernelStackOverflow();

// use KERNEL_STACK_CHECK on the entry of critical system functions (mostly in
// zeus and reaper) help detect stack misbehaviour effectively.
#define KERNEL_STACK_CHECK checkKernelStackOverflow()

#endif
