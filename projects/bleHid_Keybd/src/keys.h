/**
 ****************************************************************************************
 *
 * @file keys.h
 *
 * @brief Header file - Keys Scanning and Report
 *
 ****************************************************************************************
 */

#ifndef _KEYS_H_
#define _KEYS_H_

#include <stdint.h>
#include <stdbool.h>
#include "hidkey.h"


void keys_init(void);

void keys_scan(void);

#endif  //_KEYS_H_
