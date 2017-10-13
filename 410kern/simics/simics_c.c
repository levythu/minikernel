/** @file simics_c.c
 *  @author elly1 U2009
 *  @brief Simics calls.
 */

#include <simics.h>
#include <stdarg.h>
#include <stdio/stdio.h>

int sim_in_simics(void) {
    return sim_call(SIM_IN_SIMICS);
}

int sim_memsize(void) {
    return sim_call(SIM_MEMSIZE);
}

void sim_puts(const char *str) {
    sim_call(SIM_PUTS, str);
}

void sim_breakpoint(void) {
    sim_call(SIM_BREAKPOINT);
}

void sim_halt(void) {
    sim_call(SIM_HALT);
}

void sim_reg_process(void *cr3, const char *prog) {
    sim_call(SIM_REG_PROCESS, cr3, prog);
}

void sim_reg_child(void *cr3, void *pcr3) {
    sim_call(SIM_REG_CHILD, cr3, pcr3);
}

void sim_unreg_process(void *cr3) {
    sim_call(SIM_UNREG_PROCESS, cr3);
}

void sim_booted(const char *kern) {
    sim_call(SIM_BOOTED, kern);
}

void sim_vprintf(const char* fmt, va_list vl)
{
  char str[256];
  vsnprintf(str, sizeof(str)-1, fmt, vl);
  sim_puts(str);
}

void sim_printf(const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  sim_vprintf(fmt, ap);
  va_end(ap);
}
