/**
 ****************************************************************************************
 *
 * @file esam.h
 *
 * @brief Header file
 *
 ****************************************************************************************
 */

#ifndef _ISO7816_H_
#define _ISO7816_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * DEFINES
 ****************************************************************************************
 */

#if (SYS_CLK == 1)
    #define ISO7816_BAUD          BRR_DIV(129032, 32M)
#elif (SYS_CLK == 2)
    #define ISO7816_BAUD          BRR_DIV(129032, 48M)
#elif (SYS_CLK == 3)
    #define ISO7816_BAUD          BRR_DIV(129032, 64M)
#else
    #define ISO7816_BAUD          BRR_DIV(129032, 16M)
#endif //SYS_CLK
#define ISO7816_CLK_4M           ((SYS_CLK + 1) * 2 - 1)

//LCR_STOP_BITS_1: 0.5 StopBits; LCR_STOP_BITS_2: 1.5 StopBits
#define ISO7816_LCRS          LCR_BITS(8, 2, even) // default: 8 DataBits, 1.5 StopBits, Even Parity

/// Avoid conflict GPIOs of UART_DBG if diff port

#ifndef PA_ISO7816_RST
#define PA_ISO7816_RST         (17)
#define PA_ISO7816_DATA        (16)
#define PA_ISO7816_CLK         (15)
#endif

/* 
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/// Init UARTx and its RB, enable IRQ.
void iso7816Init(void);

/// Empty RingBuffer to reset.
void iso7816RbReset(void);

/// Get Length of data in RB.
uint16_t iso7816RbLen(void);

/// Read data from RB, return the Length of data copied.
uint16_t iso7816RbRead(uint8_t *buff, uint16_t max);


void iso7816Send(uint16_t len, const uint8_t *data);

#endif //
