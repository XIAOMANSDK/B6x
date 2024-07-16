/**
 * @file pt_def.h
 * @brief 
 * @author lijianpeng (bosco@pthyidh.com)
 * @date 2024-02-29 04:50:44
 * 
 * @copyright Copyright (c) 2024 by 深圳市鹏天微智能科技有限公司, All Rights Reserved.
 */
#ifndef PT_DEF_H
#define PT_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Utils */
#define PT_ARRAY_SIZE(a)        (sizeof(a) / sizeof(a[0]))
#define PT_MIN(a, b)            ((a) < (b) ? (a) : (b))
#define PT_MAX(a, b)            ((a) > (b) ? (a) : (b))
#define PT_ABS(val)             (((val) > 0) ? (val) : -(val))

#define PT_SET_BIT(val, n)      ((val) |= (1UL << (n)))
#define PT_CLEAR_BIT(val, n)    ((val) &= ~(1UL << (n)))
#define PT_READ_BIT(val, n)     (((val) >> (n)) & 0x01)

#define PT_ENDIAN_SWAP_16(u16)    (((u16) & 0xff00) >> 8) | (((u16) & 0x00ff) << 8)
#define PT_ENDIAN_SWAP_32(u32)    ((((u32) & 0xff000000) >> 24) | (((u32) & 0x00ff0000) >> 8) \
                                    | (((u32) & 0x0000ff00) << 8) | (((u32) & 0x000000ff) << 24))


#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define __CLANG_ARM
#endif

/* Compiler Related Definitions */
#if defined(__CC_ARM) || defined(__CLANG_ARM)           /* ARM Compiler */
    #include <stdarg.h>
    #define SECTION(x)                  __attribute__((section(x)))
    #define PT_UNUSED                   __attribute__((unused))
    #define PT_USED                     __attribute__((used))
    #define ALIGN(n)                    __attribute__((aligned(n)))

    #define PT_WEAK                     __attribute__((weak))
    #define pt_inline                   static __inline
    /* module compiling */
    #ifdef PT_USING_MODULE
        #define RTT_API                 __declspec(dllimport)
    #else
        #define RTT_API                 __declspec(dllexport)
    #endif

#elif defined (__IAR_SYSTEMS_ICC__)     /* for IAR Compiler */
    #include <stdarg.h>
    #define SECTION(x)                  @ x
    #define PT_UNUSED
    #define PT_USED                     __root
    #define PRAGMA(x)                   _Pragma(#x)
    #define ALIGN(n)                    PRAGMA(data_alignment=n)
    #define PT_WEAK                     __weak
    #define pt_inline                   static inline
    #define RTT_API

#elif defined (__GNUC__)                /* GNU GCC Compiler */
    #ifdef PT_USING_NEWLIB
        #include <stdarg.h>
    #else
        /* the version of GNU GCC must be greater than 4.x */
        typedef __builtin_va_list       __gnuc_va_list;
        typedef __gnuc_va_list          va_list;
        #define va_start(v,l)           __builtin_va_start(v,l)
        #define va_end(v)               __builtin_va_end(v)
        #define va_arg(v,l)             __builtin_va_arg(v,l)
    #endif

    #define SECTION(x)                  __attribute__((section(x)))
    #define PT_UNUSED                   __attribute__((unused))
    #define PT_USED                     __attribute__((used))
    #define ALIGN(n)                    __attribute__((aligned(n)))
    #define PT_WEAK                     __attribute__((weak))
    #define pt_inline                   static __inline
    #define RTT_API
#elif defined (__ADSPBLACKFIN__)        /* for VisualDSP++ Compiler */
    #include <stdarg.h>
    #define SECTION(x)                  __attribute__((section(x)))
    #define PT_UNUSED                   __attribute__((unused))
    #define PT_USED                     __attribute__((used))
    #define ALIGN(n)                    __attribute__((aligned(n)))
    #define PT_WEAK                     __attribute__((weak))
    #define pt_inline                   static inline
    #define RTT_API
#elif defined (_MSC_VER)
    #include <stdarg.h>
    #define SECTION(x)
    #define PT_UNUSED
    #define PT_USED
    #define ALIGN(n)                    __declspec(align(n))
    #define PT_WEAK
    #define pt_inline                   static __inline
    #define RTT_API
#elif defined (__TI_COMPILER_VERSION__)
    #include <stdarg.h>
    /* The way that TI compiler set section is different from other(at least
     * GCC and MDK) compilers. See ARM Optimizing C/C++ Compiler 5.9.3 for more
     * details. */
    #define SECTION(x)
    #define PT_UNUSED
    #define PT_USED
    #define PRAGMA(x)                   _Pragma(#x)
    #define ALIGN(n)
    #define PT_WEAK
    #define pt_inline                   static inline
    #define RTT_API
#else
    #error not supported tool chain
#endif

/**
 * @addtogroup Error
 */

/**@{*/

/* error code definitions */
#define PT_EOK                          0               /**< There is no error */
#define PT_ERROR                        1               /**< A generic error happens */
#define PT_ETIMEOUT                     2               /**< Timed out */
#define PT_EFULL                        3               /**< The resource is full */
#define PT_EEMPTY                       4               /**< The resource is empty */
#define PT_ENOMEM                       5               /**< No memory */
#define PT_ENOSYS                       6               /**< No system */
#define PT_EBUSY                        7               /**< Busy */
#define PT_EIO                          8               /**< IO error */
#define PT_EINTR                        9               /**< Interrupted system call */
#define PT_EINVAL                       10              /**< Invalid argument */

/**@}*/

/**
 * @ingroup BasicDef
 *
 * @def PT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. PT_ALIGN(13, 4)
 * would return 16.
 */
#define PT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def PT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. PT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define PT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def PT_NULL
 * Similar as the \c NULL in C library.
 */
#define PT_NULL                         (0)



#ifdef __cplusplus
}
#endif

#endif /* PT_DEF_H */
