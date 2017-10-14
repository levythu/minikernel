/** @file int_handler.h
 *
 *  @brief definition about interrupt handler entry functions (asm function)
 *  and IDT related macros
 *
 *  @author Leiyu Zhao (leiyuz)
 */

#ifndef INT_HANDLER_H
#define INT_HANDLER_H

// Given fields, encode them into the lower 4 bytes of IDT entry
#define ENCRYPT_IDT_TRAPGATE_LSB(DPL, offset, Present, Selector, D) (   \
    (((Selector) & 0xffff) << 16) | ((offset) & 0xffff))

// Given fields, encode them into the upper 4 bytes of IDT entry
#define ENCRYPT_IDT_TRAPGATE_MSB(DPL, offset, Present, Selector, D) (   \
    ((offset) & 0xffff0000) |                                           \
    (((Present) & 0x1) << 15) |                                         \
    (((DPL) & 0x3) << 13) |                                             \
    (((D) & 0x1) << 11) |                                               \
    0x700)

// The timer interrupt handler, which will store all common register, call
// timerIntHandlerInternal(), and then restore registers. NOTE: this function
// will not send ACK, the handler internal should do that instead.
extern void timerIntHandler();

// The keyboard interrupt handler, which will store all common register, call
// keyboardIntHandlerInternal(), and then restore registers. NOTE: this function
// will not send ACK, the handler internal should do that instead.
extern void keyboardIntHandler();

#endif
