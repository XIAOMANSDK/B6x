/**
 ****************************************************************************************
 *
 * @file revbit.c
 *
 * @brief Reversing the bits in an integer v.
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include <stddef.h>


uint8_t revbit8(uint8_t v)
{
    #if (OPTM_SIZE)
    uint8_t t = 0;

    for (uint32_t i = 0; i < 8; i++)
    {
        t |= ((v >> i) & 0x01) << (7 - i);
    }

    return t;
    #else
    v = (((v >> 1) & 0x55) | ((v & 0x55) << 1));
    v = (((v >> 2) & 0x33) | ((v & 0x33) << 2));

    return ((v >> 4) | (v << 4));
    #endif
}

uint16_t revbit16(uint16_t v)
{
    #if (OPTM_SIZE)
    uint16_t t = 0;

    for (uint32_t i = 0; i < 16; i++)
    {
        t |= ((v >> i) & 0x01) << (15 - i);
    }

    return t;
    #else
    v = (((v >> 1) & 0x5555) | ((v & 0x5555) << 1));
    v = (((v >> 2) & 0x3333) | ((v & 0x3333) << 2));
    v = (((v >> 4) & 0x0f0f) | ((v & 0x0f0f) << 4));

    return ((v >> 8) | (v << 8));
    #endif
}

uint32_t revbit24(uint32_t v)
{
    #if (OPTM_SIZE)
    uint32_t t = 0;

    for (uint32_t i = 0; i < 24; i++)
    {
        t |= ((v >> i) & 0x01) << (23 - i);
    }

    return t;
    #else
    v = (((v >> 1) & 0x555555) | ((v & 0x555555) << 1));
    v = (((v >> 2) & 0x333333) | ((v & 0x333333) << 2));
    v = (((v >> 4) & 0x0f0f0f) | ((v & 0x0f0f0f) << 4));

    return (((v >> 16) & 0x0000ff) | ((v & 0x0000ff) << 16) | (v & 0x00ff00));
    #endif
}

uint32_t revbit32(uint32_t v)
{
    #if (OPTM_SIZE)
    uint32_t t = 0;

    for (uint32_t i = 0; i < 32; i++)
    {
        t |= ((v >> i) & 0x01) << (31 - i);
    }

    return t;
    #else
    v = (((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1));
    v = (((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2));
    v = (((v >> 4) & 0x0f0f0f0f) | ((v & 0x0f0f0f0f) << 4));
    v = (((v >> 8) & 0x00ff00ff) | ((v & 0x00ff00ff) << 8));

    return ((v >> 16) | (v << 16));
    #endif
}

uint32_t revbit(uint8_t n, uint32_t v)
{
    uint32_t t = 0;
    
    for (uint32_t i = 0; i < n; i++)
    {
        t |= ((v >> i) & 0x01) << (n - 1 - i);
    }

    return t;
}
