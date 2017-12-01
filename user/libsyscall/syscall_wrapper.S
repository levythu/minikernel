#include <syscall_int.h>

#include "make_syscall_wrapper.h"

# ##############################################################################
# Multiple parameters syscall:

# int new_pages(void * addr, int len)
MAKE_WRAPPER_MULTIPARAMS(new_pages, NEW_PAGES_INT)

# int readfile(char *filename, char *buf, int count, int offset)
MAKE_WRAPPER_MULTIPARAMS(readfile, READFILE_INT)

# int exec(char *execname, char *argvec[])
MAKE_WRAPPER_MULTIPARAMS(exec, EXEC_INT)

# int swexn(void *esp3, swexn_handler_t eip, void *arg, ureg_t *newureg)
MAKE_WRAPPER_MULTIPARAMS(swexn, SWEXN_INT)

# int readline(int size, char *buf)
MAKE_WRAPPER_MULTIPARAMS(readline, READLINE_INT)

# int print(int size, char *buf)
MAKE_WRAPPER_MULTIPARAMS(print, PRINT_INT)

# int get_cursor_pos(int *row, int *col)
MAKE_WRAPPER_MULTIPARAMS(get_cursor_pos, GET_CURSOR_POS_INT)

# int set_cursor_pos(int row, int col)
MAKE_WRAPPER_MULTIPARAMS(set_cursor_pos, SET_CURSOR_POS_INT)

# int new_console(void)
MAKE_WRAPPER_NOPARAM(new_console, NEW_CONSOLE_INT)



# ##############################################################################
# Single parameter syscall:

# void set_status(int status)
MAKE_WRAPPER_SINGLEPARAM(set_status, SET_STATUS_INT)

# int wait(int *status_ptr)
MAKE_WRAPPER_SINGLEPARAM(wait, WAIT_INT)

# int yield(int pid)
MAKE_WRAPPER_SINGLEPARAM(yield, YIELD_INT)

# int deschedule(int *flag)
MAKE_WRAPPER_SINGLEPARAM(deschedule, DESCHEDULE_INT)

# int make_runnable(int pid)
MAKE_WRAPPER_SINGLEPARAM(make_runnable, MAKE_RUNNABLE_INT)

# int sleep(int ticks)
MAKE_WRAPPER_SINGLEPARAM(sleep, SLEEP_INT)

# int set_term_color(int color)
MAKE_WRAPPER_SINGLEPARAM(set_term_color, SET_TERM_COLOR_INT)

# void task_vanish(int status)
MAKE_WRAPPER_SINGLEPARAM(task_vanish, TASK_VANISH_INT)

# int remove_pages(void * addr)
MAKE_WRAPPER_SINGLEPARAM(remove_pages, REMOVE_PAGES_INT)

# void misbehave(int mode)
MAKE_WRAPPER_SINGLEPARAM(misbehave, MISBEHAVE_INT)



# ##############################################################################
# Zero parameter syscall:

# int fork(void)
MAKE_WRAPPER_NOPARAM(fork, FORK_INT)

# void vanish(void)
MAKE_WRAPPER_NOPARAM(vanish, VANISH_INT)

# int gettid(void)
MAKE_WRAPPER_NOPARAM(gettid, GETTID_INT)

# int getchar(void)
MAKE_WRAPPER_NOPARAM(getchar, GETCHAR_INT)

# void halt(void)
MAKE_WRAPPER_NOPARAM(halt, HALT_INT)

# unsigned int get_ticks()
MAKE_WRAPPER_NOPARAM(get_ticks, GET_TICKS_INT)
