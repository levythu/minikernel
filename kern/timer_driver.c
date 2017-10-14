/** @file timer_driver.c
 *
 *  @brief implementation for functions about timer driver. The process for
 *  a timer event is just one step:
 *  1. when a timer event is triggered, the timerIntHandlerInternal() is called
 *  and then pass control to the callback. ACK will not be sent before callback
 *  returns.
 *
 *  @author Leiyu Zhao (leiyuz)
 */

#include <stdint.h>
#include <simics.h>

#include "int_handler.h"
#include "timer_driver.h"
#include "x86/asm.h"
#include "x86/interrupt_defines.h"
#include "x86/seg.h"
#include "x86/timer_defines.h"

static TimerCallback cb;
static unsigned int epoch;

// Install the timer driver. For any errors return negative integer;
// otherwise return 0;
int install_timer_driver(TimerCallback callback) {
  cb = callback;
  epoch = 0;
  int intervalFor10ms = TIMER_RATE / 100;
  int32_t* idtBase = (int32_t*)idt_base();
  idtBase[TIMER_IDT_ENTRY << 1] = ENCRYPT_IDT_TRAPGATE_LSB(
    0, (int32_t)timerIntHandler, 1, SEGSEL_KERNEL_CS, 1);
  idtBase[(TIMER_IDT_ENTRY << 1) + 1] = ENCRYPT_IDT_TRAPGATE_MSB(
    0, (int32_t)timerIntHandler, 1, SEGSEL_KERNEL_CS, 1);
  outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
  outb(TIMER_PERIOD_IO_PORT, intervalFor10ms & 0xff);
  outb(TIMER_PERIOD_IO_PORT, (intervalFor10ms >> 8) & 0xff);
  return 0;
}

// The entry for handling the timer interrupt event from intHandler
// It will also send ACK after the callback returns
void timerIntHandlerInternal() {
  epoch++;
  cb(epoch);
  outb(INT_CTL_PORT, INT_ACK_CURRENT);
}
