.globl invalidateTLB

# void invalidateTLB(uint32_t addr);
invalidateTLB:
    mov 4(%esp), %eax
    invlpg (%eax)
    ret
