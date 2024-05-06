/**
 ****************************************************************************************
 *
 * @file revbit.h
 *
 * @brief Head File of Bit Reversal functions.
 *
 ****************************************************************************************
 */
#ifndef _REVBIT_H_
#define _REVBIT_H_

#include <stdint.h>


uint8_t revbit8(uint8_t v);

uint16_t revbit16(uint16_t v);

uint32_t revbit24(uint32_t v);

uint32_t revbit32(uint32_t v);

uint32_t revbit(uint8_t n, uint32_t v);

#endif  // _REVBIT_H_
