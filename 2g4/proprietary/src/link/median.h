/**
 ****************************************************************************************
 *
 * @file median.h
 *
 * @brief Header file - Median Calculation
 *
 ****************************************************************************************
 */

#ifndef _MEDIAN_H_
#define _MEDIAN_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */
typedef struct
{
    uint16_t median;
    uint16_t peak;
} median_res_t;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void     median_find(uint16_t *val, int n, median_res_t *res);
uint16_t median_calc(uint16_t *val, int n);

#endif // _MEDIAN_H_
