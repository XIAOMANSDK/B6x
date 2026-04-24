/**
 ****************************************************************************************
 *
 * @file batt.c
 *
 * @brief Battery voltage detection
 *
 ****************************************************************************************
 */

#include "batt.h"
#include "sadc.h"

#if (DBG_BATT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<BATT>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void batt_init(void)
{

}

uint8_t batt_level(void)
{
    uint8_t lvl = 100;

    return lvl;
}
