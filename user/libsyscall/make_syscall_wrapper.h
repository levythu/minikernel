
// The macro is used to batch-create syscall wrappers for multiple parameters
// Before making syscall, the wrapper will put address of `parameter package`
// to %esi.
// Since the package has already been setupped by C Caller on the stack, what
// we need is just reference the address
// %esi is restored before returning
#define MAKE_WRAPPER_MULTIPARAMS(globalName, intNumber) \
    .globl globalName;                                  \
    globalName:                                         \
        pushl   %esi;                                   \
        lea     8(%esp), %esi;                          \
                                                        \
        int     $intNumber;                             \
                                                        \
        popl    %esi;                                   \
                                                        \
        ret;

// The macro is used to batch-create syscall wrappers for single parameter
// Before making syscall, the wrapper will put the value of single parameter
// to %esi
// %esi is restored before returning
#define MAKE_WRAPPER_SINGLEPARAM(globalName, intNumber) \
    .globl globalName;                                  \
    globalName:                                         \
        pushl   %esi;                                   \
        mov     8(%esp), %esi;                          \
                                                        \
        int     $intNumber;                             \
                                                        \
        popl    %esi;                                   \
                                                        \
        ret;

// The macro is used to batch-create syscall wrappers for no parameter
#define MAKE_WRAPPER_NOPARAM(globalName, intNumber)     \
    .globl globalName;                                  \
    globalName:                                         \
        int     $intNumber;                             \
                                                        \
        ret;
