/** @file hvcall.h
 *
 * Prototypes for PebPeb hypervisor calls.
 *
 * @author jwise
 * @author de0u
 * @author relong
 */

#ifndef _HVCALL_H
#define _HVCALL_H

/* vIDT slots match regular x86-32 numbers from x86/idt.h, plus these */
#define HV_TICKBACK 0x20 /**< vIDT slot for virtual timer interrupt */
#define HV_KEYBOARD 0x21 /**< vIDT slot for virtual keyboard interrupt */

#define HV_MAGIC 0xC001C0DE /**< hv_magic() value for v2 of PebPeb API */
#define HV_SETIDT_PRIVILEGED 1
#define HV_PRINT_MAX 410

#define GUEST_LAUNCH_EAX 0x15410DE0U /**< Value for %EAX at guest launch */
#define GUEST_CRASH_STATUS 0xDEADC0DE /**< For simulated hv_exit() */
#define GUEST_INTERRUPT_UMODE 0x13370000 /**< Guest was interrupted in user mode */
#define GUEST_INTERRUPT_KMODE 0x00001337 /**< Guest was interrupted in kernel mode */

#ifndef ASSEMBLER

#ifndef NORETURN
#define NORETURN __attribute__((__noreturn__))
#endif

/** @brief verify we are running under a v2 PebPeb hypervisor
 *  @return HV_MAGIC
 */
unsigned int hv_magic(void);

/** @brief Cease execution and pass status to parent task
 *  @param  status Status code to pass to parent task
 */
void hv_exit(int status) NORETURN;

/** @brief Atomically activate a new privilege level/execution context
 *  @param  eip    New value for %eip
 *  @param  eflags New value for %eflags
 *  @param  esp    New value for %esp
 *  @param  esp0   If zero, IRET into guest-kernel mode;
 *                 else, remember virtual %esp0 for later and IRET
 *                 into guest-user mode
 *  @param  eax    New value for %eax
 *
 *  @note   If eflags is invalid, the hypervisor will crash the guest
 *  @note   eip, esp, esp0 addresses are guest-virtual
 */
void hv_iret(void *eip, unsigned int eflags, void *esp, void *esp0, unsigned int eax);

/** @brief Install a virtual interrupt handler
 *  @param  irqno IDT slot index
 *  @param  eip   Address of first instruction of handler
 *  @param privileged if non-zero only kernel access, else user access permitted
 *  @note If a slot's eip value is zero when that event happens, the guest crashes
 *  @note When guest kernel launches, all slots have eip == 0
 *  @note eip address is guest-virtual
 *  @note See HV_SETIDT_PRIVILEGED definition above
 */
void hv_setidt(int irqno, void *eip, int privileged);

/** @brief Suspend delivery of virtual interrupts (not virtual exceptions)
 */
void hv_disable_interrupts(void);

/** @brief Begin/resume delivery of virtual interrupts (not virtual exceptions)
 */
void hv_enable_interrupts(void);

/** @brief Activate an address space
 *  @param  pdbase Base of page directory
 *  @param      wp If non-zero, guest kernel can't write into user-level
 *                 read-only pages (see Intel's description of bit 16 of %cr0)
 *  @pre    pdbase must be page-aligned
 *  @note   pdbase address is guest-physical
 */
void hv_setpd(void *pdbase, int wp);

/** @brief Re-validate/invalidate/update the mapping
 *         of one virtual-to-physical translation
 *  @param  addr Base of page whose mapping status has changed
 *  @pre    addr must be page-aligned
 *  @note   addr address is guest-virtual
 */
void hv_adjustpg(void *addr);

/** @brief Print to console
 *  @param  len  Count of bytes to print (must be less than HV_PRINT_MAX)
 *  @param  buf  Address of first byte to print
 *  @note   buf  address is guest-virtual
 */
void hv_print(int len, unsigned char *buf);

/** @brief Change printing color
 *  @param  color VGA color code
 */
void hv_cons_set_term_color(int color);

/** @brief Set cursor position
 *  @param  row  Row cursor should be moved to
 *  @param  col  Column cursor should be moved to
 */
void hv_cons_set_cursor_pos(int row, int col);

/** @brief Get cursor position
 *  @param  rowp  Pointer to where cursor row should be stored
 *  @param  colp  Pointer to where cursor column should be stored
 *  @note   both  addresses are guest-virtual
 */
void hv_cons_get_cursor_pos(int *rowp, int *colp);

/** @brief Print to the console
 *  @param  len   Count of bytes to print (must be less than HV_PRINT_MAX)
 *  @param  buf   Address of first byte to print
 *  @param  color VGA color code for printing this string
 *  @param  row   Row this string should be printed on
 *  @param  col   Column this string should be printed at
 *  @note   This moves the cursor, changes the print color, prints,
 *          then restores the cursor position and print color.
 *  @note   buf   address is guest-virtual
 */
void hv_print_at(int len, unsigned char *buf, int row, int col, int color);

#endif /* ASSEMBLER */

#endif /* _HVCALL_H */
