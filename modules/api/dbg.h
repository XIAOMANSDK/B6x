/**
 ****************************************************************************************
 *
 * @file dbg.h
 *
 * @brief Debug Output via UART or JLink-RTT, or Disable
 *
 ****************************************************************************************
 */

#ifndef _DBG_H_
#define _DBG_H_

#include <stdint.h>


/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_DISABLE         0
#define DBG_VIA_UART        1
#define DBG_VIA_RTT         2

#if !defined(DBG_MODE)
#define DBG_MODE            (DBG_DISABLE)
#endif


/// Debug Output Functions
#if (DBG_MODE == DBG_VIA_UART)
#include <stdio.h>

void dbgInit(void);

#define debug(format, ...)  printf(format, ##__VA_ARGS__)

#define debugHex(dat, len)  do{ debug("<%s,%d>", __MODULE__, __LINE__);\
                                for (int i=0; i<len; i++){      \
                                    debug("%02X ", *((dat)+i)); \
                                }                               \
                                debug("\r\n");                  \
                            } while (0)

extern void trace_init(void); // HardFault Info

#elif (DBG_MODE == DBG_VIA_RTT)
//#include "RTT.h"
extern void rtt_init(void);
extern  int rtt_printf(const char *format, ...);

#define dbgInit()           rtt_init()

#define debug(format, ...)  rtt_printf(format, ##__VA_ARGS__)

#define debugHex(dat, len)  do{                                 \
                                for (int i=0; i<len; i++){      \
                                    debug("%02X ", *((dat)+i)); \
                                }                               \
                                debug("\r\n");                  \
                            } while (0)

#else

/// Disable via empty marco
#define dbgInit()
#define debug(format, ...)
#define debugHex(dat, len)

#endif


#endif	//_DBG_H_
