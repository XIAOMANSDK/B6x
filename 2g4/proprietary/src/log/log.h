/**
 ****************************************************************************************
 *
 * @file log.h
 *
 * @brief Header file - Log Util
 *
 ****************************************************************************************
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

typedef enum
{
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ALL
} log_level_t;

typedef void (*logout_fn)(const char*);

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

int log_printf(logout_fn logout, log_level_t level, log_level_t current, const char *format, ...);
int log_hex(logout_fn logout, log_level_t level, log_level_t current, uint8_t *data, uint16_t length);

#endif // _LOG_H_
