.globl get_ss
.globl get_esp

get_ss:
    mov %ss, %eax
    ret

get_esp:
    mov %esp, %eax
    ret
