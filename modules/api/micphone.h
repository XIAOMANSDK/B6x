/**
 ****************************************************************************************
 *
 * @file micphone.h
 *
 * @brief Micphone Interface of Application
 *
 * @data 2024.06.21
 *
 ****************************************************************************************
 */

#ifndef _MICPHONE_H_
#define _MICPHONE_H_

#include <stdbool.h>
#include <stdint.h>


void micInit(void);
void micDeinit(void);

uint8_t* micDataGet(void);

#endif // SADC_H_
