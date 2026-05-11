/**
 ****************************************************************************************
 *
 * @file crc.h
 *
 * @brief Header file - CRC Util
 *
 ****************************************************************************************
 */

#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
#define CFG_CRC8

#ifdef CFG_CRC8
uint8_t crc8(uint8_t *data, uint16_t length);
#define CRC_LEN 1
#else
uint32_t crc24(uint8_t *data, uint16_t length);
#define CRC_LEN 3
#endif

#endif // _CRC_H_
