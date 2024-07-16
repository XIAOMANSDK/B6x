/**
 ****************************************************************************************
 *
 * @file micphone.h
 *
 * @brief Micphone Interface of Application
 *
 * @data 2022.05.13
 *
 ****************************************************************************************
 */

#ifndef MICPHONE_H_
#define MICPHONE_H_

#include <stdbool.h>
#include <stdint.h>

#define MODE_SELECT         (0)  // 0:ADPCM TEST  1:PCM TEST

void micInit(void);

void micDeinit(void);

void micPut(void);

void pt_voice_deinit(void);

#endif // SADC_H_
