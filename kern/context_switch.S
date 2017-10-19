.globl switchTheWorld
.globl checkpointTheWorld


# switchTheWorld(ureg_t* oldURegSavePlace, ureg_t *newUReg, int hint)
switchTheWorld:
    mov 4(%esp), %eax

    mov %edi, 24(%eax)
    mov %esi, 28(%eax)
    mov %ebp, 32(%eax)
    mov %ebx, 40(%eax)
    mov %esp, 72(%eax)
    lea resume, %ecx
    mov %ecx, 60(%eax)

    mov 12(%esp), %ecx

    # Start loading new!

    mov 8(%esp), %eax

    mov 72(%eax), %esp
    mov 40(%eax), %ebx
    mov 32(%eax), %ebp
    mov 28(%eax), %esi
    mov 24(%eax), %edi

    mov 60(%eax), %eax
    jmp *%eax

resume:
    mov %ecx, %eax
    ret

# bool checkpointTheWorld(ureg_t* savePlace,
#     uint32_t memToSnapshot, uint32_t memDst, uint32_t size);
checkpointTheWorld:
    mov 4(%esp), %eax

    mov %edi, 24(%eax)
    mov %esi, 28(%eax)
    mov %ebp, 32(%eax)
    mov %ebx, 40(%eax)
    mov %esp, 72(%eax)
    lea checkpointTheWorld_reenter, %ecx
    mov %ecx, 60(%eax)

    # make snapshot
    # memcpy(dst, src, size)
    mov %esp, %ecx
    mov 16(%ecx), %eax
    push %eax       # size
    mov 8(%ecx), %eax
    push %eax       # src
    mov 12(%ecx), %eax
    push %eax       # dst
    call memcpy
    pop %eax
    pop %eax
    pop %eax

    mov $0, %eax
    ret

checkpointTheWorld_reenter:
    mov $1, %eax
    ret
