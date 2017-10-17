.globl switchTheWorld


# switchTheWorld(ureg_t* oldURegSavePlace, ureg_t *newUReg)
switchTheWorld:
    mov 4(%esp), %eax

    mov %edi, 24(%eax)
    mov %esi, 28(%eax)
    mov %ebp, 32(%eax)
    mov %ebx, 40(%eax)
    mov %edx, 44(%eax)
    mov %ecx, 48(%eax)
    mov %esp, 72(%eax)
    lea resume, %ecx
    mov %ecx, 60(%eax)


    # Start loading new!

    mov 8(%esp), %eax

    mov 72(%eax), %esp
    mov 48(%eax), %ecx
    mov 44(%eax), %edx
    mov 40(%eax), %ebx
    mov 32(%eax), %ebp
    mov 28(%eax), %esi
    mov 24(%eax), %edi

    mov 60(%eax), %eax
    jmp *%eax

resume:
    ret
