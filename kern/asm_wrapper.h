/** @file asm_wrapper.h
 *
 *  @brief Some c wrapper for basic assembly helper functions
 *
 *  @author Leiyu Zhao
 */

#ifndef ASM_WRAPPER_H
#define ASM_WRAPPER_H

// Get current %ss
int get_ss();

// Get current %esp
int get_esp();

#endif
