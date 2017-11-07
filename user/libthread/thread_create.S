#include <syscall_int.h>

.global thread_create

/*
int thread_create(
    uint32_t stackTop,
    ThreadCreateCallback callback,
    ThreadPayload func,
    void* args);
*/
thread_create:
    # set up the new stack, which is starting from %eax
    movl    4(%esp), %ecx
    addl    $12, %ecx
    # now %ecx will be the %esp for new thread
    # retaddr = dead code
    movl    $0xdeadbeef, (%ecx)
    # 1st argument = func
    movl    12(%esp), %edx
    movl    %edx, 4(%ecx)
    # 2nd argument = args
    movl    16(%esp), %edx
    movl    %edx, 8(%ecx)
    # Finally, store the callback address in %edx
    movl    8(%esp), %edx
    # Call thread_fork!
    int     $THREAD_FORK_INT
    cmp     $0, %eax
    jg      thread_create_old_thread    # if (eax > 0) GOTO thread_create_old_thread
    je      thread_create_new_thread    # if (eax == 0) GOTO thread_create_new_thread
    ret
thread_create_old_thread:
    # for old thread, do nothing and return the tid stored in eax
    ret
thread_create_new_thread:
    # for new thread, rebase to stackTop
    movl    %ecx, %esp
    jmp     *%edx
