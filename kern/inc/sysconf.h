/** @file sysconf.h
 *
 *  @brief Basic configuration for LevyOS

 *
 *  @author Leiyu Zhao
 */

#ifndef SYSCONF_H
#define SYSCONF_H

#define MAX_PROCESSES 4096
#define MAX_THREAD_PER_PROCESS 64

// The PID for INIT process.
// 1 for IDLE
// 2 for INIT
// 3 for shell
#define INIT_PID 2

#endif
