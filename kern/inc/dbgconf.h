/** @file dbgconf.h
 *
 *  @brief Macros for debugging, and is irrelevant to the functionalities

 *
 *  @author Leiyu Zhao
 */

#ifndef DBGCONF_H
#define DBGCONF_H

#define HYPERVISOR_VERBOSE_PRINT

/* When defined, print more information to simics console. */
#define VERBOSE_PRINT

/* When defined, print all decisions and reasons of scheduler */
// #define SCHEDULER_DECISION_PRINT

/* When defined, print each time a cpulock is acquired and released
   (quite verbose, payattention) */
// #define CPULOCK_PRINT

/* When defined, auto context switch happens on pressing RIGHT_KEY, instead of
   timer ticks */
// #define CONTEXT_SWTICH_ON_RIGHT_KEY

#endif
