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
#include "cmsis_compiler.h"

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

/// Value with one bit set (pos is masked to [0..31] to prevent UB)
#ifndef BIT
    #define BIT(pos)              (1UL << ((pos) & 31))
#endif

/// Number of '1' bits in a 32-bit value
#ifndef NB_1BITS
    #define NB_1BITS(val)         co_ones(val)
#endif

#ifndef ONE_BITS
    #define ONE_BITS(val)         co_ones(val)
#endif

/// Get the number of elements within an array
#ifndef ARRAY_LEN
    #define ARRAY_LEN(array)      (sizeof((array)) / sizeof((array)[0]))
#endif

/// macro to Get a structure from one of its structure field
#ifndef CONTAINER_OF
    #define CONTAINER_OF(ptr, type, member) \
        ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

/// Bit checking
#ifndef CHKF
    #define CHKF(flag, mask)      (((flag) & (mask)) == (mask))
#endif

/// Get a field from a value containing several fields
/// @param[in] __r bit field value
/// @param[in] __f field name(_MASK & _LSB)
/// @return the value of the register masked and shifted
#ifndef GETF
    #define GETF(__r, __f)        (((__r) & (__f##_MASK)) >> (__f##_LSB))
#endif

/// Set a field value into a value containing several fields.
/// @param[in] __r bit field value
/// @param[in] __f field name(_MASK & _LSB)
/// @param[in] __v value to put in field
#ifndef SETF
    #define SETF(__r, __f, __v)                                                         \
        do {                                                                            \
            ASSERT_ERR((((__v) << (__f##_LSB)) & (~(__f##_MASK))) == 0);                \
            (__r) = (((__r) & ~(__f##_MASK)) | ((__v) << (__f##_LSB)));                 \
        } while (0)
#endif

/// Get a bit field from a value containing several fields
/// @param[in] __r bit field value
/// @param[in] __b bit field name(_BIT & _POS)
/// @return the value of the register masked and shifted
#ifndef GETB
    #define GETB(__r, __b)        (((__r) & (__b##_BIT)) >> (__b##_POS))
#endif

/// set a bit field value into a value containing several fields.
/// @param[in] __r bit field value
/// @param[in] __b bit field name
/// @param[in] __v value to put in field
#ifndef SETB
    #define SETB(__r, __b, __v)                                                         \
        do {                                                                            \
            ASSERT_ERR((((__v) << (__b##_POS)) & (~(__b##_BIT))) == 0);                 \
            (__r) = (((__r) & ~(__b##_BIT)) | ((__v) << (__b##_POS)));                  \
        } while (0)
#endif

/// Align value on the multiple of 4 equal or nearest higher.
/// @param[in] _val Value to align.
#ifndef ALIGN4_HI
    #define ALIGN4_HI(_val)      (((_val) + 3) & ~3)
#endif

/// Align value on the multiple of 4 equal or nearest lower.
/// @param[in] _val Value to align.
#ifndef ALIGN4_LO
    #define ALIGN4_LO(_val)      ((_val) & ~3)
#endif

/// @brief Align value on the multiple of 2 equal or nearest higher.
/// @param[in] _val Value to align.
#ifndef ALIGN2_HI
    #define ALIGN2_HI(_val)      (((_val) + 1) & ~1)
#endif

/// @brief Align value on the multiple of 2 equal or nearest lower.
/// @param[in] _val Value to align.
#ifndef ALIGN2_LO
    #define ALIGN2_LO(_val)      ((_val) & ~1)
#endif

/// Perform a modulo(%) operation
/// @param[in] _val    Dividend
/// @param[in] _div    Divisor
#ifndef MOD
    #define MOD(_val, _div)      ((_val) % (_div))
#endif

/// Perform a division and ceil up the result
/// @param[in] _val Value to divide
/// @param[in] _div Divide value
#ifndef DIV_CEIL
    #define DIV_CEIL(_val, _div) \
        (((_div) == 0) ? 0 : (((_val) == 0) ? 0 : (((_val) - 1) / (_div) + 1)))
#endif

/// Perform a division and round the result
/// @param[in] _val Value to divide
/// @param[in] _div Divide value
#ifndef DIV_ROUND
    #define DIV_ROUND(_val, _div) \
        (((_div) == 0) ? 0 : (((_val) + ((_div) >> 1)) / (_div)))
#endif

/// Count leading zeros in a 32-bit value.
/// @param[in] _val Value to count leading zeros for.
/// @return Number of leading zeros.
#ifndef co_clz
    #define co_clz(_val)         __CLZ(_val)
#endif

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
__STATIC_FORCEINLINE void rand_init(uint32_t seed)
{
    srand(seed);
}

/**
 ****************************************************************************************
 * @brief Function to get an 8 bit random number.
 * @return Random byte value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint8_t rand_byte(void)
{
    return (uint8_t)(rand() & 0xFF);
}

/**
 ****************************************************************************************
 * @brief Function to get an 16 bit random number.
 * @return Random half word value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint16_t rand_hword(void)
{
    return (uint16_t)(rand() & 0xFFFF);
}

/**
 ****************************************************************************************
 * @brief Function to get an 32 bit random number.
 * @return Random word value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t rand_word(void)
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
__STATIC_FORCEINLINE uint32_t read32(const void *ptr32)
{
    ASSERT_ERR(ptr32 != NULL);
    return *((const uint32_t *)ptr32);
}

/**
 ****************************************************************************************
 * @brief Read an aligned 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint16_t read16(const void *ptr16)
{
    ASSERT_ERR(ptr16 != NULL);
    return *((const uint16_t *)ptr16);
}

/**
 ****************************************************************************************
 * @brief Write an aligned 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE void write32(void *ptr32, uint32_t value)
{
    ASSERT_ERR(ptr32 != NULL);
    *(uint32_t *)ptr32 = value;
}

/**
 ****************************************************************************************
 * @brief Write an aligned 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE void write16(void *ptr16, uint16_t value)
{
    ASSERT_ERR(ptr16 != NULL);
    *(uint16_t *)ptr16 = value;
}

/**
 ****************************************************************************************
 * @brief Write a 8 bits word.
 * @param[in] ptr8 The address of the first byte of the 8 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE void write8(void *ptr8, uint8_t value)
{
    ASSERT_ERR(ptr8 != NULL);
    *(uint8_t *)ptr8 = value;
}

/**
 ****************************************************************************************
 * @brief Read a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint16_t read16p(const void *ptr16)
{
    ASSERT_ERR(ptr16 != NULL);
    const uint8_t *p = (const uint8_t *)ptr16;
    return (uint16_t)(p[0] | ((uint32_t)p[1] << 8));
}

/**
 ****************************************************************************************
 * @brief Write a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE void write16p(void *ptr16, uint16_t value)
{
    ASSERT_ERR(ptr16 != NULL);
    uint8_t *ptr = (uint8_t *)ptr16;

    *ptr++ = (uint8_t)(value);
    *ptr   = (uint8_t)(value >> 8);
}

/**
 ****************************************************************************************
 * @brief Read a packed 24 bits word.
 * @param[in] ptr24 The address of the first byte of the 24 bits word.
 * @return The 24 bits value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t read24p(const void *ptr24)
{
    ASSERT_ERR(ptr24 != NULL);
    uint16_t addr_l, addr_h;
    addr_l = read16p(ptr24);
    addr_h = *((const uint8_t *)ptr24 + 2);
    return ((uint32_t)addr_l | ((uint32_t)addr_h << 16));
}

/**
 ****************************************************************************************
 * @brief Write a packed 24 bits word.
 * @param[in] ptr24 The address of the first byte of the 24 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE void write24p(void *ptr24, uint32_t value)
{
    ASSERT_ERR(ptr24 != NULL);
    uint8_t *ptr = (uint8_t *)ptr24;

    *ptr++ = (uint8_t)(value);
    *ptr++ = (uint8_t)(value >> 8);
    *ptr++ = (uint8_t)(value >> 16);
}

/**
 ****************************************************************************************
 * @brief Read a packed 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @return The 32 bits value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t read32p(const void *ptr32)
{
    ASSERT_ERR(ptr32 != NULL);
    uint16_t addr_l, addr_h;
    addr_l = read16p(ptr32);
    addr_h = read16p((const uint8_t *)ptr32 + 2);
    return ((uint32_t)addr_l | ((uint32_t)addr_h << 16));
}

/**
 ****************************************************************************************
 * @brief Write a packed 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @param[in] value The value to write.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE void write32p(void *ptr32, uint32_t value)
{
    ASSERT_ERR(ptr32 != NULL);
    uint8_t *ptr = (uint8_t *)ptr32;

    *ptr++ = (uint8_t)(value);
    *ptr++ = (uint8_t)(value >> 8);
    *ptr++ = (uint8_t)(value >> 16);
    *ptr   = (uint8_t)(value >> 24);
}

/**
 ****************************************************************************************
 * @brief Count trailing zeros.
 * @param[in] val Value to count the number of trailing zeros on.
 * @return Number of trailing zeros when value is written as 32 bits.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t co_ctz(uint32_t val)
{
    #if defined(__arm__)
    return (val == 0) ? 32 : (uint32_t)__builtin_ctz(val);
    #else
    uint32_t i;
    for (i = 0; i < 32; i++)
    {
        if (val & BIT(i))
            break;
    }
    return i;
    #endif
}

/**
 ****************************************************************************************
 * @brief Count binary ones.
 * @param[in] val Value to count the number of ones in the binary representation.
 * @return Number of ones when value is written as 32 bits.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t co_ones(uint32_t val)
{
    val = val - ((val >> 1) & 0x55555555U);
    val = (val & 0x33333333U) + ((val >> 2) & 0x33333333U);
    return (((val + (val >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
}

/**
 ****************************************************************************************
 * @brief Function to return the smallest of 2 unsigned 32 bits words.
 * @return The smallest value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t co_min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

/**
 ****************************************************************************************
 * @brief Function to return the smallest of 2 signed 32 bits words.
 * @return The smallest value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE int32_t co_min_s(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

/**
 ****************************************************************************************
 * @brief Function to return the greatest of 2 unsigned 32 bits words.
 * @return The greatest value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t co_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

/**
 ****************************************************************************************
 * @brief Function to return the absolute value of a signed integer.
 * @return The absolute value.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE int32_t co_abs(int32_t val)
{
    return val < 0 ? -val : val;
}

/**
 ****************************************************************************************
 * @brief Perform a modulo operation
 * @return  val%div
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint32_t co_mod(uint32_t val, uint32_t div)
{
    ASSERT_ERR(div);
    return ((val) % (div));
}

/**
 ****************************************************************************************
 * @brief Function to return the hex char of a 4bit.
 * @return The hex char.
 ****************************************************************************************
 */
__STATIC_FORCEINLINE uint8_t co_hex(uint8_t b4)
{
    ASSERT_ERR(b4 < 16);
    return b4 < 10 ? b4 + '0' : b4 - 10 + 'A';
}

#endif // _UTILS_H_
