/** @file hvcall_int.h
 *
 * Hypervisor call numbers
 *
 * @author jwise
 */

#ifndef _HVCALL_INT_H
#define _HVCALL_INT_H

/* Protocol:
 *
 * 1. Parameters on stack in "natural push order"
 * 2. Operation number (HV_xxx_OP) in %eax
 * 3. INT $HV_INT
 */

#define HV_INT            0xDE

#define HV_MAGIC_OP       0x00
#define HV_EXIT_OP        0x01
#define HV_IRET_OP        0x02
#define HV_SETIDT_OP      0x03
#define HV_DISABLE_OP     0x04
#define HV_ENABLE_OP      0x05
#define HV_SETPD_OP       0x06
#define HV_ADJUSTPG_OP    0x07
#define HV_PRINT_OP       0x08
#define HV_SET_COLOR_OP   0x09
#define HV_SET_CURSOR_OP  0x0A
#define HV_GET_CURSOR_OP  0x0B
#define HV_PRINT_AT_OP    0x0C

/* The calls in here, INCLUSIVE, are promised not to be
 * probed by any grading scripts; as such you are welcome
 * to extend the spec by making use of these hvcall numbers
 */
#define HV_RESERVED_START    0x20
#define HV_RESERVED_0        0x20
#define HV_RESERVED_1        0x21
#define HV_RESERVED_2        0x22
#define HV_RESERVED_3        0x23
#define HV_RESERVED_4        0x24
#define HV_RESERVED_5        0x25
#define HV_RESERVED_6        0x26
#define HV_RESERVED_7        0x27
#define HV_RESERVED_END      0x27

#endif /* _HVCALL_INT_H */
