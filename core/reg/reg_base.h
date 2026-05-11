#ifndef REG_BASE_H_
#define REG_BASE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define __I                    volatile const /* defines 'read only' permissions */
#define __O                    volatile       /* defines 'write only' permissions */
#define __IO                   volatile       /* defines 'read / write' permissions */

#define REG_RD(addr)           (*(volatile uint32_t *)(addr))
#define REG_WR(addr, value)    (*(volatile uint32_t *)(addr)) = (value)

/*
 *  This macro is for use by other macros to form a fully valid C statement.
 *  Without this, the if/else conditionals could show unexpected behavior.
 * (The while condition below evaluates false without generating a
 *  constant-controlling-loop type of warning on most compilers.)
 */
#if !defined(dowl)
    #define dowl(x)            do { x } while(__LINE__ == -1)
#endif

#if defined ( __CC_ARM )
#pragma anon_unions
#endif

#endif // REG_BASE_H_
