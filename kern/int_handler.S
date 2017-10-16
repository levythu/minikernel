.globl timerIntHandler
.globl keyboardIntHandler

timerIntHandler:
    pusha
    call timerIntHandlerInternal
    popa
    iret

keyboardIntHandler:
    pusha
    call keyboardIntHandlerInternal
    popa
    iret

keyboardIntHandler:
    pusha
    call keyboardIntHandlerInternal
    popa
    iret
