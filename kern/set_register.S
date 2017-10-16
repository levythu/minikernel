.globl setContext

# setContext(new_esp, new_eip);
setContext:
    mov 8(%esp), %eax
    mov 4(%esp), %esp
    jmp *%eax
