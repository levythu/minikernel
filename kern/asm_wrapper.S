.globl get_ss

get_ss:
    mov %ss, %eax
    ret

get_retAddr:
    mov (%esp), %eax
    ret
