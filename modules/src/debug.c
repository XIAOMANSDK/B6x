/**
 ****************************************************************************************
 *
 * @file debug.c
 *
 * @brief Debug Interface of Application
 *
 ****************************************************************************************
 */
 
#include "dbg.h"

#if (DBG_MODE == DBG_VIA_UART)
#include "uart.h"

#if !defined(DBG_UART_BAUD)
#if (SYS_CLK == 1)
    #define DBG_UART_BAUD       BRR_DIV(115200, 32M)
#elif (SYS_CLK == 2)
    #define DBG_UART_BAUD       BRR_DIV(115200, 48M)
#elif (SYS_CLK == 3)
    #define DBG_UART_BAUD       BRR_DIV(115200, 64M)
#else
    #define DBG_UART_BAUD       (BRR_115200)
#endif //SYS_CLK
#endif

#if !defined(DBG_UART_PORT)
    #define DBG_UART_PORT       (0) //UART1 
#endif
#if !defined(DBG_UART_TXD)
    #define DBG_UART_TXD        (6) //PA06
#endif
#if !defined(DBG_UART_RXD)
    #define DBG_UART_RXD        (7) //PA07
#endif

#if !defined(DBG_HARDFAULT)
    #define DBG_HARDFAULT       (0)
#endif

// mdk(__CC_ARM) & iar(__ICCARM__)
#if defined ( __CC_ARM ) || defined ( __ICCARM__ )
int fputc(int ch, FILE *f) {
    // Remap printf(...) to UART
    uart_putc(DBG_UART_PORT, ch);
    return ch;
}
#else
// gcc(__GNUC__)
int _write (int fd, char *ptr, int len)
{
    for( int i = 0; i < len; i++)
    {
        uart_putc(DBG_UART_PORT, ptr[i]);
    }

    return len;
}
#endif

void dbgInit(void)
{
    uart_init(DBG_UART_PORT, DBG_UART_TXD, DBG_UART_RXD);
    uart_conf(DBG_UART_PORT, DBG_UART_BAUD, LCR_BITS_DFLT);
    
    #if (DBG_UART_RXEN)
    uart_fctl(DBG_UART_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 20, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif

    #if (DBG_HARDFAULT)
    trace_init();
    #endif
}

#endif // (DBG_VIA_UART)
