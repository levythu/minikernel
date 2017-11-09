/** @file kernel_stack_protection.h
 *
 *  @brief Physical Memory manager, only for non-kernel space.
 *
 *  This module manages all non-kernel physical memory. And provide them to
 *  kernel to set vm-to-pm mappings
 *
 *  All functions except claimUserMem are interrupt-safe and multicore-safe.
 *
 *  @author Leiyu Zhao
 */

#ifndef KERNEL_STACK_PRO_H
#define KERNEL_STACK_PRO_H

// TODO disable it in non-debug mode
void checkKernelStackOverflow();

#define KERNEL_STACK_CHECK checkKernelStackOverflow()

#endif
