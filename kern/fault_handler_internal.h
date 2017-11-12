#include <x86/seg.h>
#include <x86/idt.h>

// The macro to define a fault handler asm, provided idt number and forwarded
// internal function
#define MAKE_FAULT_HANDLER_NO_ERRCODE(idtNumber, internalFunction) \
    .globl IDT_Handler_ ## idtNumber; \
                        \
    IDT_Handler_ ## idtNumber:        \
        pushl $0;     \
        pushl $idtNumber;   \
        pusha;  \
        \
        mov %ds, %eax; \
        pushl %eax;   \
        \
        mov %es, %eax; \
        pushl %eax;   \
        \
        mov $SEGSEL_KERNEL_DS, %eax;    \
        mov %ax, %ds;   \
        mov %ax, %es;   \
        \
        mov $0, %ebp;   \
        call internalFunction;  \
        \
        pop %eax;   \
        mov %ax, %es;    \
        \
        pop %eax;   \
        mov %ax, %ds;    \
        popa;   \
        add $8, %esp;   \
        iret;

// The macro to define a fault handler asm, provided idt number and forwarded
// internal function. This one differs in that it support errorcode
#define MAKE_FAULT_HANDLER_WITH_ERRCODE(idtNumber, internalFunction) \
    .globl IDT_Handler_ ## idtNumber; \
                        \
    IDT_Handler_ ## idtNumber:        \
        pushl $idtNumber;   \
        pusha;  \
        \
        mov %ds, %eax; \
        pushl %eax;   \
        \
        mov %es, %eax; \
        pushl %eax;   \
        \
        mov $SEGSEL_KERNEL_DS, %eax;    \
        mov %ax, %ds;   \
        mov %ax, %es;   \
        \
        mov $0, %ebp;   \
        call internalFunction;  \
        \
        pop %eax;   \
        mov %ax, %es;    \
        \
        pop %eax;   \
        mov %ax, %ds;    \
        popa;   \
        add $8, %esp;   \
        iret;

// Used in C
#define DECLARE_FAULT_ENTRANCE(idtNumber) \
    void IDT_Handler_ ## idtNumber ()

// Used in C
#define MAKE_FAULT_IDT(idtNumber) \
  do { \
    int32_t* idtBase = (int32_t*)idt_base(); \
    idtBase[idtNumber  << 1] = ENCRYPT_IDT_TRAPGATE_LSB( \
      3, (int32_t)(IDT_Handler_ ## idtNumber), 1, SEGSEL_KERNEL_CS, 1); \
    idtBase[(idtNumber  << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB( \
      3, (int32_t)(IDT_Handler_ ## idtNumber), 1, SEGSEL_KERNEL_CS, 1); \
  } while (false)
