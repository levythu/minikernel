
#define MAKE_SYSCALL_WRAPPER(syscallName)                          \
    .globl syscallName ## _Handler;                         \
    syscallName ## _Handler:                                \
      pusha;                                                       \
      call syscallName ## _Internal;                        \
      movl %eax, (%esp);                                            \
      popa;                                                        \
      iret;


#define DECLARE_SYSCALL_WRAPPER(syscallName)    \
    void syscallName ## _Handler ();             \
    int syscallName ## _Internal ();
