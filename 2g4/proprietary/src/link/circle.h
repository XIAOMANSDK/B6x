/**
 ****************************************************************************************
 *
 * @file circle.h
 *
 * @brief Header file - Circle Buff
 *
 ****************************************************************************************
 */

#ifndef _CIRCLE_H_
#define _CIRCLE_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

#define CIRCLE_BUFF_ALIGNED(block_size) (((block_size) + 3) & 0xFFFFFFFC)
#define CIRCLE_BUFF_LENGTH(block_size, count_power)                                                \
    (((CIRCLE_BUFF_ALIGNED(block_size) + 4) << (count_power)) + 4)

typedef struct
{
    uint16_t index_mask;
    uint16_t block_size;
    uint8_t *start_ptr;

    volatile uint16_t index_read;
    volatile uint16_t index_write;

} circle_buff_t;

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void circle_buff_init(circle_buff_t *circle, void *buff, uint16_t block_size, uint16_t count_power);
void circle_buff_reset(circle_buff_t *circle);
void    *circle_buff_top(circle_buff_t *circle, uint16_t *size);
uint16_t circle_buff_count(circle_buff_t *circle);
int      circle_buff_write(circle_buff_t *circle, void *data, uint16_t size);
int      circle_buff_read(circle_buff_t *circle, void *data, uint16_t *size);
int      circle_buff_pop(circle_buff_t *circle);

#endif // _CIRCLE_H_
