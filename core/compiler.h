#ifndef BX_COMPILER_H_
#define BX_COMPILER_H_

/*============================================================================
 *                        Compiler Identification
 *============================================================================*/

/* Compiler detection - order matters! */
#if defined(__ARMCC_VERSION)
    #if (__ARMCC_VERSION >= 6010050)
        #define COMPILER_AC6            1
        #define COMPILER_NAME           "AC6"
    #elif (__ARMCC_VERSION >= 5000000)
        #define COMPILER_AC5            1
        #define COMPILER_NAME           "AC5"
    #endif
#elif defined(__clang__)
    #define COMPILER_CLANG              1
    #define COMPILER_NAME               "Clang"
#elif defined(__ICCARM__) || defined(__IAR_SYSTEMS_ICC__)
    #define COMPILER_IAR                1
    #define COMPILER_NAME               "IAR"
#elif defined(__GNUC__)
    #define COMPILER_GCC                1
    #define COMPILER_NAME               "GCC"
#else
    #define COMPILER_UNKNOWN            1
    #define COMPILER_NAME               "Unknown"
#endif

/* Define as 0 if not detected */
#ifndef COMPILER_AC6
    #define COMPILER_AC6                0
#endif
#ifndef COMPILER_AC5
    #define COMPILER_AC5                0
#endif
#ifndef COMPILER_CLANG
    #define COMPILER_CLANG              0
#endif
#ifndef COMPILER_GCC
    #define COMPILER_GCC                0
#endif
#ifndef COMPILER_IAR
    #define COMPILER_IAR                0
#endif
#ifndef COMPILER_UNKNOWN
    #define COMPILER_UNKNOWN            0
#endif

/*============================================================================
 *                        Compiler Specific Definitions
 *============================================================================*/

#define COMPILER_ARMCC                  (COMPILER_AC6 || COMPILER_AC5)

/*============================================================================
 *                        Unused Attribute Macros
 *============================================================================*/

/**
 * @brief  Mark variable/function as unused to suppress compiler warnings
 * @note   Usage: UNUSED static int val;
 */
#ifndef UNUSED
    #if defined(__GNUC__) || defined(__clang__) || defined(__ARMCC_VERSION) || defined(__ICCARM__)
        #define UNUSED              __attribute__((unused))
    #else
        #define UNUSED
    #endif
#endif

/**
 * @brief  Mark function parameter as unused
 * @note   Usage: void func(int param) { UNUSED_PARAM(param); }
 */
#ifndef UNUSED_PARAM
    #define UNUSED_PARAM(x)    ((void)(x))
#endif

#endif /* BX_COMPILER_H_ */
