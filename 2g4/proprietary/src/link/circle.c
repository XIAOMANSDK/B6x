#include <string.h>
#include "circle.h"

void circle_buff_init(circle_buff_t *circle, void *buff, uint16_t block_size, uint16_t count_power)
{
    circle->index_mask  = (1UL << count_power) - 1;
    circle->block_size  = CIRCLE_BUFF_ALIGNED(block_size);
    circle->start_ptr   = (uint8_t *)(CIRCLE_BUFF_ALIGNED((uint32_t)buff));
    circle->index_read  = 0;
    circle->index_write = 0;
}

void circle_buff_reset(circle_buff_t *circle)
{
    circle->index_write = circle->index_read;
}

uint16_t circle_buff_count(circle_buff_t *circle)
{
    return (
        (circle->index_mask + 1 - circle->index_read + circle->index_write) & circle->index_mask);
}

void *circle_buff_top(circle_buff_t *circle, uint16_t *size)
{
    if (circle->index_read == circle->index_write)
    {
        return ((void *)0);
    }

    uint8_t *ptr = circle->start_ptr + (circle->block_size + 4) * circle->index_read;
    *size        = *((uint32_t *)ptr);

    return ((void *)(ptr + 4));
}

int circle_buff_write(circle_buff_t *circle, void *data, uint16_t size)
{
    uint16_t next = (circle->index_write + 1) & circle->index_mask;

    if (next == circle->index_read)
    {
        return -1;
    }

    if (size > circle->block_size)
    {
        return -2;
    }

    uint8_t *ptr       = circle->start_ptr + (circle->block_size + 4) * circle->index_write;
    *((uint32_t *)ptr) = size;
    memcpy(ptr + 4, data, size);

    circle->index_write = next;

    return 0;
}

int circle_buff_read(circle_buff_t *circle, void *data, uint16_t *size)
{
    if (circle->index_read == circle->index_write)
    {
        return -1;
    }

    uint8_t *ptr = circle->start_ptr + (circle->block_size + 4) * circle->index_read;

    if (*size > *((uint32_t *)ptr))
    {
        *size = *((uint32_t *)ptr);
    }

    memcpy(data, ptr + 4, *size);

    circle->index_read = (circle->index_read + 1) & circle->index_mask;

    return 0;
}

int circle_buff_pop(circle_buff_t *circle)
{
    if (circle->index_read == circle->index_write)
    {
        return -1;
    }

    circle->index_read = (circle->index_read + 1) & circle->index_mask;

    return 0;
}
