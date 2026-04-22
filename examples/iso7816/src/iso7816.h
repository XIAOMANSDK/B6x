/**
 ****************************************************************************************
 *
 * @file iso7816.h
 *
 * @brief ISO7816 smart card interface driver
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
#endif

#define ISO7816_CLK_4M           ((SYS_CLK + 1) * 2 - 1)

/// LCR config: 8 data bits, 1.5 stop bits (LCR_STOP_BITS_2), even parity
#define ISO7816_LCRS          LCR_BITS(8, 2, even)

/// Default pin assignments (overridden by cfg.h)
#ifndef PA_ISO7816_RST
#define PA_ISO7816_RST         (17)
#define PA_ISO7816_DATA        (16)
#define PA_ISO7816_CLK         (15)
#endif

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize ISO7816 UART, ring buffer, DMA, and interrupts
 ****************************************************************************************
 */
void iso7816Init(void);

/**
 ****************************************************************************************
 * @brief Reset the receive ring buffer
 ****************************************************************************************
 */
void iso7816RbReset(void);

/**
 ****************************************************************************************
 * @brief Get number of bytes in receive ring buffer
 *
 * @return Number of bytes available
 ****************************************************************************************
 */
uint16_t iso7816RbLen(void);

/**
 ****************************************************************************************
 * @brief Read data from receive ring buffer
 *
 * @param[out] buff  Destination buffer
 * @param[in]  max   Maximum bytes to read
 *
 * @return Number of bytes actually read
 ****************************************************************************************
 */
uint16_t iso7816RbRead(uint8_t *buff, uint16_t max);

/**
 ****************************************************************************************
 * @brief Send data via ISO7816 interface (blocking, runs from SRAM)
 *
 * @param[in] len   Data length
 * @param[in] data  Data to send
 ****************************************************************************************
 */
void iso7816Send(uint16_t len, const uint8_t *data);

#endif /* _ISO7816_H_ */
