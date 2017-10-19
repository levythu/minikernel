/** @file source_untrusted.h
 *
 *  @brief TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef SOURCE_UNTRUSTED_H
#define SOURCE_UNTRUSTED_H

bool verifyUserSpaceAddr(
    uint32_t startAddr, uint32_t endAddr, bool mustWritable);

bool sGetInt(uint32_t addr, int* target);

int sGetString(uint32_t addr, char* target, int size);

int sGeUIntArray(uint32_t addr, uint32_t* target, int size);

#endif
