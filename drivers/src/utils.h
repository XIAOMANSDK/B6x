/**
 ****************************************************************************************
 *
 * @file utils.h
 *
 * @brief Common Utilities definitions (functions and macros).
 *
 ****************************************************************************************
 */

#ifndef _UTILS_H_
#define _UTILS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef __INLINE__
#define __INLINE__ __forceinline static
#endif

#ifndef ASSERT_ERR
/// Assertions showing a critical error
#define ASSERT_ERR(cond)

/// Assertions showing a critical info
#define ASSERT_INFO(cond, param0, param1)

/// Assertions showing a non-critical problem
#define ASSERT_WARN(cond, param0, param1)

/// DUMP data array present in the SW.
#define DUMP_DATA(data, length)
#endif


/*
 * MACRO
 ****************************************************************************************
 */

/// Value with one bit set
#define BIT(pos)           (1UL<<(pos))

/// Number of '1' bits in a byte, used to fasten bit counting
extern const unsigned char one_bits[]; //{0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
#define ONE_BITS(byte)     (one_bits[byte & 0x0F] + one_bits[byte >> 4])

/// Get the number of elements within an array
#define ARRAY_LEN(array)   (sizeof((array))/sizeof((array)[0]))

/// maroc to Get a structure from one of its structure field
#define CONTAINER_OF(ptr, type, member)    ((type *)((char *)ptr - offsetof(type,member)))

/// Bit checking
#define CHKF(flag, mask)     (((flag)&(mask)) == mask)

/// Get a field from a value containing several fields
/// @param[in] __r bit field value
/// @param[in] __f field name(_MASK & _LSB)
/// @return the value of the register masked and shifted
#define GETF(__r, __f)    (( (__r) & (__f##_MASK) ) >> (__f##_LSB))

/// Set a field value into a value containing several fields.
/// @param[in] __r bit field value
/// @param[in] __f field name(_MASK & _LSB)
/// @param[in] __v value to put in field
#define SETF(__r, __f, __v)                                                      \
    do {                                                                         \
        ASSERT_ERR( ( ( ( (__v) << (__f##_LSB) ) & ( ~(__f##_MASK) ) ) ) == 0);  \
        __r = (((__r) & ~(__f##_MASK)) | (__v) << (__f##_LSB));                  \
    } while (0)

/// Get a bit field from a value containing several fields
/// @param[in] __r bit field value
/// @param[in] __b bit field name(_BIT & _POS)
/// @return the value of the register masked and shifted
#define GETB(__r, __b)    (( (__r) & (__b##_BIT) ) >> (__b##_POS))

/// set a bit field value into a value containing several fields.
/// @param[in] __r bit field value
/// @param[in] __b bit field name
/// @param[in] __v value to put in field
#define SETB(__r, __b, __v)                                                     \
    do {                                                                        \
        ASSERT_ERR( ( ( ( (__v) << (__b##_POS) ) & ( ~(__b##_BIT) ) ) ) == 0 ); \
        __r = (((__r) & ~(__b##_BIT)) | (__v) << (__b##_POS));                  \
    } while (0)


/// Align value on the multiple of 4 equal or nearest higher.
/// @param[in] val Value to align.
#define ALIGN4_HI(_val) (((_val) + 3) & ~3)

/// Align value on the multiple of 4 equal or nearest lower.
/// * @param[in] _val Value to align.
#define ALIGN4_LO(_val) ((_val) & ~3)

/// @brief Align value on the multiple of 2 equal or nearest higher.
/// * @param[in] _val Value to align.
#define ALIGN2_HI(_val) (((_val) + 1) & ~1)

/// @brief Align value on the multiple of 2 equal or nearest lower.
/// * @param[in] _val Value to align.
#define ALIGN2_LO(_val) ((_val) & ~1)


/// Perform a modulo(%) operation
/// @param[in] _val    Dividend
/// @param[in] _div    Divisor
#define MOD(_val, _div) ((_val) % (_div))

/// Perform a division and ceil up the result
/// * @param[in] _val Value to divide
/// * @param[in] _div Divide value
#define DIV_CEIL(_val, _div) (((_val) + ((_div) - 1))/ (_div))

/// Perform a division and round the result
/// @param[in] _val Value to divide
/// @param[in] _div Divide value
#define DIV_ROUND(_val, _div) (((_val) + ((_div) >> 1))/ (_div))

/// Increment value (wrap to 0 if reach max)
/// * @param[in] _val Value to inc
/// * @param[in] _max Max Range
#define VAL_INC(_val, _max)         \
    (_val) = (_val) + 1;            \
    if ((_val) >= (_max)) (_val) = 0


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Function to initialize the random seed.
 * @param[in] seed The seed number to use to generate the random sequence.
 ****************************************************************************************
 */
__INLINE__ void rand_init(uint32_t seed)
{
    srand(seed);
}

/**
 ****************************************************************************************
 * @brief Function to get an 8 bit random number.
 * @return Random byte value.
 ****************************************************************************************
 */
__INLINE__ uint8_t rand_byte(void)
{
    return (uint8_t)(rand() & 0xFF);
}

/**
 ****************************************************************************************
 * @brief Function to get an 16 bit random number.
 * @return Random half word value.
 ****************************************************************************************
 */
__INLINE__ uint16_t rand_hword(void)
{
    return (uint16_t)(rand() & 0xFFFF);
}

/**
 ****************************************************************************************
 * @brief Function to get an 32 bit random number.
 * @return Random word value.
 ****************************************************************************************
 */
__INLINE__ uint32_t rand_word(void)
{
    return (uint32_t)rand();
}


/**
 ****************************************************************************************
 * @brief Read an aligned 32 bit word.
 * @param[in] ptr32 The address of the first byte of the 32 bit word.
 * @return The 32 bit value.
 ****************************************************************************************
 */
__INLINE__ uint32_t read32(const void *ptr32)
{
    return *((uint32_t*)ptr32);
}

/**
 ****************************************************************************************
 * @brief Read an aligned 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.
 ****************************************************************************************
 */
__INLINE__ uint16_t read16(const void *ptr16)
{
    return *((uint16_t*)ptr16);
}

/**
 ****************************************************************************************
 * @brief Write an aligned 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__INLINE__ void write32(const void *ptr32, uint32_t value)
{
    *(uint32_t*)ptr32 = value;
}

/**
 ****************************************************************************************
 * @brief Write an aligned 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__INLINE__ void write16(const void *ptr16, uint32_t value)
{
    *(uint16_t*)ptr16 = value;
}

/**
 ****************************************************************************************
 * @brief Write a 8 bits word.
 * @param[in] ptr8 The address of the first byte of the 8 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__INLINE__ void write8(const void *ptr8, uint32_t value)
{
    *(uint8_t*)ptr8 = value;
}

/**
 ****************************************************************************************
 * @brief Read a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.
 ****************************************************************************************
 */
__INLINE__ uint16_t read16p(const void *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}

/**
 ****************************************************************************************
 * @brief Write a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__INLINE__ void write16p(const void *ptr16, uint16_t value)
{
    uint8_t *ptr=(uint8_t*)ptr16;

    *ptr++ = value&0xff;
    *ptr = (value&0xff00)>>8;
}

/**
 ****************************************************************************************
 * @brief Read a packed 24 bits word.
 * @param[in] ptr24 The address of the first byte of the 24 bits word.
 * @return The 24 bits value.
 ****************************************************************************************
 */
__INLINE__ uint32_t read24p(const void *ptr24)
{
    uint16_t addr_l, addr_h;
    addr_l = read16p(ptr24);
    addr_h = *((uint8_t *)ptr24 + 2) & 0x00FF;
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

/**
 ****************************************************************************************
 * @brief Write a packed 24 bits word.
 * @param[in] ptr24 The address of the first byte of the 24 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__INLINE__ void write24p(const void *ptr24, uint32_t value)
{
    uint8_t *ptr=(uint8_t*)ptr24;

    *ptr++ = (uint8_t)(value&0xff);
    *ptr++ = (uint8_t)((value&0xff00)>>8);
    *ptr++ = (uint8_t)((value&0xff0000)>>16);
}

/**
 ****************************************************************************************
 * @brief Read a packed 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @return The 32 bits value.
 ****************************************************************************************
 */
__INLINE__ uint32_t read32p(const void *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = read16p(ptr32);
    addr_h = read16p((uint8_t *)ptr32 + 2);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

/**
 ****************************************************************************************
 * @brief Write a packed 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__INLINE__ void write32p(const void *ptr32, uint32_t value)
{
    uint8_t *ptr=(uint8_t*)ptr32;

    *ptr++ = (uint8_t)(value&0xff);
    *ptr++ = (uint8_t)((value&0xff00)>>8);
    *ptr++ = (uint8_t)((value&0xff0000)>>16);
    *ptr = (uint8_t)((value&0xff000000)>>24);
}


/**
 ****************************************************************************************
 * @brief Count leading zeros.
 * @param[in] val Value to count the number of leading zeros on.
 * @return Number of leading zeros when value is written as 32 bits.
 ****************************************************************************************
 */
__INLINE__ uint32_t co_clz(uint32_t val)
{
    #if defined(__arm__)
    return __builtin_clz(val);
    #elif defined(__GNUC__)
    if (val == 0)
    {
        return 32;
    }
    return __builtin_clz(val);
    #else
    uint32_t i;
    for (i = 0; i < 32; i++)
    {
        if (val & BIT(31 - i))
            break;
    }
    return i;
    #endif // defined(__arm__)
}

/**
 ****************************************************************************************
 * @brief Count trailing zeros.
 * @param[in] val Value to count the number of trailing zeros on.
 * @return Number of trailing zeros when value is written as 32 bits.
 ****************************************************************************************
 */
__INLINE__ uint32_t co_ctz(uint32_t val)
{
    #if defined(__arm__)
    return __builtin_ctz(val);
    #elif defined(__GNUC__)
    if (val == 0)
    {
        return 32;
    }
    return __builtin_ctz(val);
    #else
    uint32_t i;
    for (i = 0; i < 32; i++)
    {
        if (val & BIT(i))
            break;
    }
    return i;
    #endif // defined(__arm__)
}

/**
 ****************************************************************************************
 * @brief Count binary ones.
 * @param[in] val Value to count the number of ones in the binary representation.
 * @return Number of ones when value is written as 32 bits.
 ****************************************************************************************
 */
__INLINE__ uint32_t co_ones(uint32_t val)
{
    #if defined(__arm__) || defined(__GNUC__)
    return __builtin_popcount(val);
    #else
    uint32_t i, ones=0;
    for (i = 0; i < 32; i++)
    {
        if (val & BIT(i))
            ones++;
    }
    return ones;
    #endif // defined(__arm__)
}

/**
 ****************************************************************************************
 * @brief Function to return the smallest of 2 unsigned 32 bits words.
 * @return The smallest value.
 ****************************************************************************************
 */
__INLINE__ uint32_t co_min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

/**
 ****************************************************************************************
 * @brief Function to return the smallest of 2 signed 32 bits words.
 * @return The smallest value.
 ****************************************************************************************
 */
__INLINE__ int32_t co_min_s(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

/**
 ****************************************************************************************
 * @brief Function to return the greatest of 2 unsigned 32 bits words.
 * @return The greatest value.
 ****************************************************************************************
 */
__INLINE__ uint32_t co_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

/**
 ****************************************************************************************
 * @brief Function to return the absolute value of a signed integer.
 * @return The absolute value.
 ****************************************************************************************
 */
__INLINE__ int co_abs(int val)
{
    return val < 0 ? val*(-1) : val;
}


#endif // _UTILS_H_
