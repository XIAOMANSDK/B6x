/**
 ****************************************************************************************
 *
 * @file uartRb.h
 *
 * @brief Header file for UARTx Interrupt-Mode with RingBuffer.
 *
 ****************************************************************************************
 */

#ifndef _UARTRB_H_
#define _UARTRB_H_

#include <stdint.h>
#include "uart.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#ifndef USE_UART1
#define USE_UART1               (1) //default ON
#endif

#ifndef USE_UART2
#define USE_UART2               (0) //default OFF
#endif


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

#if (USE_UART1)

#if !defined(PA_UART1_TX)
    #define PA_UART1_TX         (6) //PA06
#endif
#if !defined(PA_UART1_RX)
    #define PA_UART1_RX         (7) //PA07
#endif

/// Init UARTx and its RB, enable IRQ.
void uart1Rb_Init(void);

/// Empty RingBuffer to reset.
void uart1Rb_Reset(void);

/// Get Length of data in RB.
uint16_t uart1Rb_Len(void);

/// Read data from RB, return the Length of data copied.
uint16_t uart1Rb_Read(uint8_t *buff, uint16_t max);

#endif  //(USE_UART1)

#if (USE_UART2)

#if !defined(PA_UART2_TX)
    #define PA_UART2_TX         (11) //PA11
#endif
#if !defined(PA_UART2_RX)
    #define PA_UART2_RX         (12) //PA11
#endif

/// Init UARTx and its RB, enable IRQ.
void uart2Rb_Init(void);

/// Empty RingBuffer to reset.
void uart2Rb_Reset(void);

/// Get Length of data in RB.
uint16_t uart2Rb_Len(void);

/// Read data from RB, return the Length of data copied.
uint16_t uart2Rb_Read(uint8_t *buff, uint16_t max);

#endif  //(USE_UART2)

#endif  // _UARTRB_H_
