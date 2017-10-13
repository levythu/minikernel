/** @file 410user/progs/coolness.c
 *  @author de0u
 *  @brief Tests fork, exec, and context-switching.
 *  @public yes
 *  @for p3
 *  @covers fork gettid exec
 *  @status done
 */

#include <syscall.h>
#include <simics.h>

char *program = "peon";
char *args[2];

int main() {

  fork();
  args[0] = program;
  fork();

  exec(program, args);

  while(1)
      lprintf("ULTIMATE BADNESS");
}
