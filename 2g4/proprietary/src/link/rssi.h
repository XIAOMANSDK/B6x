/**
 ****************************************************************************************
 *
 * @file rssi.h
 *
 * @brief Header file - RSSI table
 *
 ****************************************************************************************
 */

#ifndef _RSSI_H_
#define _RSSI_H_

#include <stdint.h>

#include "median.h"
#include "cmsis_compiler.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define RSSI_TABLE_SIZE (15)

typedef struct
{
    uint16_t val[RSSI_TABLE_SIZE];
    int      idx;
} rssi_table_t;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

__STATIC_INLINE void rssi_table_reset(rssi_table_t *tab)
{
    tab->idx = 0;
}

__STATIC_INLINE void rssi_table_push(rssi_table_t *tab, uint16_t rssi)
{
    if (tab->idx < RSSI_TABLE_SIZE)
    {
        tab->val[tab->idx++] = rssi;
    }
}

__STATIC_INLINE int rssi_table_remains(rssi_table_t *tab)
{
    return (RSSI_TABLE_SIZE - tab->idx);
}

__STATIC_INLINE void rssi_table_median(rssi_table_t *tab, median_res_t *res)
{
    median_find(tab->val, tab->idx, res);
}

#endif // _RSSI_H_
