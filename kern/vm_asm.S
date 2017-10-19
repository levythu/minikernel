.globl invalidateTLB

# void invalidateTLB(uint32_t addr);
mov 4(%esp), %eax
invlpg %eax
ret
