/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief Header file - UART Driver
 *
 ****************************************************************************************
 */

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include "rcc.h"
#include "reg_uart.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Number of Uart port
enum uart_port
{
    UART1_PORT             = 0,
    UART2_PORT             = 1,
};

/// Bits field of line control @see UART_LCR_TypeDef
enum uart_lcr_bfs
{
    // data length select - bit[1:0]
    LCR_DATA_BITS_LSB      = 0,
    LCD_DATA_BITS_8        = (0x03 << LCR_DATA_BITS_LSB),
    LCD_DATA_BITS_7        = (0x02 << LCR_DATA_BITS_LSB),
    LCD_DATA_BITS_6        = (0x01 << LCR_DATA_BITS_LSB),
    LCD_DATA_BITS_5        = (0x00 << LCR_DATA_BITS_LSB),
    
    // stop bits select - bit[2]
    LCR_STOP_BITS_LSB      = 2,
    LCR_STOP_BITS_1        = (0x00 << LCR_STOP_BITS_LSB),
    LCR_STOP_BITS_2        = (0x01 << LCR_STOP_BITS_LSB),
    
    // parity mode select - bit[4:3]
    LCR_PARITY_BITS_LSB    = 3,
    LCR_PARITY_none        = (0x00 << LCR_PARITY_BITS_LSB),
    LCR_PARITY_odd         = (0x01 << LCR_PARITY_BITS_LSB),
    LCR_PARITY_even        = (0x03 << LCR_PARITY_BITS_LSB),
    
    // uart receive enable - bit5
    LCR_RXEN_BIT           = (1 << 5),
    // receive time out enable - bit8
    LCR_RTOEN_BIT          = (1 << 8),
};

/// Bits field of fifo control @see UART_FCR_TypeDef
enum uart_fcr_bfs
{
    // fifo enable - bit0
    FCR_FIFOEN_BIT         = (1 << 0),
    // rx fifo reset - bit1
    FCR_RFRST_BIT          = (1 << 1),
    // tx fifo reset - bit2
    FCR_TFRST_BIT          = (1 << 2),
    
    // rx fifo trigger level - bit[5:4]
    FCR_RXTL_LSB           = 4,
    FCR_RXTL_1BYTE         = (0 << FCR_RXTL_LSB),
    FCR_RXTL_4BYTE         = (1 << FCR_RXTL_LSB),
    FCR_RXTL_8BYTE         = (2 << FCR_RXTL_LSB),
    FCR_RXTL_14BYTE        = (3 << FCR_RXTL_LSB),

    // tx fifo trigger level - bit[7:6]
    FCR_TXTL_LSB           = 6,
    FCR_TXTL_1BYTE         = (0 << FCR_TXTL_LSB),
    FCR_TXTL_4BYTE         = (1 << FCR_TXTL_LSB),
    FCR_TXTL_8BYTE         = (2 << FCR_TXTL_LSB),
    FCR_TXTL_14BBYTE       = (3 << FCR_TXTL_LSB),
};

/// Bits field of uart status @see UART_SR_TypeDef
enum uart_sr_bfs
{
    UART_SR_DR_BIT         = (1 << 0),  // data ready
    UART_SR_OE_BIT         = (1 << 1),  // overrun error
    UART_SR_PE_BIT         = (1 << 2),  // parity error
    UART_SR_FE_BIT         = (1 << 3),  // framing error
    UART_SR_BF_BIT         = (1 << 4),  // break interrupt
    UART_SR_TBEM_BIT       = (1 << 5),  // transmit buffer register empty
    UART_SR_TEM_BIT        = (1 << 6),  // transmitter empty
    UART_SR_RFE_BIT        = (1 << 7),  // receiver fifo data error
    UART_SR_BUSY_BIT       = (1 << 8),  // uart busy
    UART_SR_TFNF_BIT       = (1 << 9),  // transmit fifo not full
    UART_SR_TFEM_BIT       = (1 << 10), // transmit fifo empty
    UART_SR_RFNE_BIT       = (1 << 11), // receive fifo not empty
    UART_SR_RFF_BIT        = (1 << 12), // receive fifo full
    UART_SR_DCTS_BIT       = (1 << 13), // delta clear to send
    UART_SR_CTS_BIT        = (1 << 14), // clear to send
    
    UART_SR_ALL_MSK        = (0x7FFF),  // bit[14:0]
};

/// Bits field of uart interrupt @see UART_IER_TypeDef
enum uart_intr_bfs
{
    UART_IR_RXRD_BIT       = (1 << 0),  // receive data available interrupt enable status
    UART_IR_TXS_BIT        = (1 << 1),  // thr empty interrupt enable status
    UART_IR_RXS_BIT        = (1 << 2),  // receiver line status interrupt enable status
    UART_IR_MDS_BIT        = (1 << 3),  // modem status interrupt enable status
    UART_IR_RTO_BIT        = (1 << 4),  // receiver timeout enable status
    UART_IR_BUSY_BIT       = (1 << 5),  // busy detect interrupt enable status
    UART_IR_ABE_BIT        = (1 << 6),  // auto baud rate end interrupt enable status
    UART_IR_ABTO_BIT       = (1 << 7),  // auto baud rate timeout interrupt enable status
    UART_IR_LINBK_BIT      = (1 << 8),  // lin break detection interrupt enable status
    UART_IR_TC_BIT         = (1 << 9),  // transmission complete interrupt enable status
    UART_IR_EOB_BIT        = (1 << 10), // end of block interrupt enable status
    UART_IR_CM_BIT         = (1 << 11), // character match interrupt enable status
    
    UART_IR_ALL_MSK        = (0x0FFF),  // bit[11:0]
};


/*
 * MACROS DECLARATION
 ****************************************************************************************
 */

/// Macro for uart line-control bits configure
/// @param[in] _dbit  data bits value in[8, 7, 6, 5]
/// @param[in] _sbit  stop bits value in[1, 2]
/// @param[in] _pbit  parity mode value in[none, odd, even]
/// @return the value of the line-control bits @see enum uart_lcr_bfs
#define LCR_BITS(_dbit, _sbit, _pbit)  \
    ((uint16_t)(LCD_DATA_BITS_##_dbit | LCR_STOP_BITS_##_sbit | LCR_PARITY_##_pbit))

/// Macro for uart baudrate divider configure (manual input sysclk_freq)
/// @param[in] _baud  baudrate value, eg. 115200
/// @param[in] _freq  sysclk freq name in[16M, 32M, 48M, 64M] @see enum sys_clk_freq
/// @return the value of the BRR
#define BRR_DIV(_baud, _freq)  \
    ((uint16_t)((SYS_FREQ_##_freq + ((_baud) >> 1)) / (_baud)))

/// Macro for uart baudrate divider configure (auto get sysclk_freq, more size)
/// @param[in] _baud  baudrate value, eg. 115200
/// @return the value of the BRR
#define BRR_BAUD(_baud)  \
    ((uint16_t)((rcc_sysclk_freq() + ((_baud) >> 1)) / (_baud)))

/// Common uart params to easy call
#define LCR_BITS_DFLT        LCR_BITS(8, 1, none) // default: 8 DataBits, 1 StopBits, None Parity

#if (SYS_CLK == 1)
#define BRR_115200           BRR_DIV(115200, 32M) // default use SYS_FREQ_32M
#define BRR_921600           BRR_DIV(921600, 32M) // default use SYS_FREQ_32M
#elif (SYS_CLK == 2)
#define BRR_115200           BRR_DIV(115200, 48M) // default use SYS_FREQ_48M
#define BRR_921600           BRR_DIV(921600, 48M) // default use SYS_FREQ_48M
#elif (SYS_CLK == 3)
#define BRR_115200           BRR_DIV(115200, 64M) // default use SYS_FREQ_64M
#define BRR_921600           BRR_DIV(921600, 64M) // default use SYS_FREQ_64M
#else
#define BRR_115200           BRR_DIV(115200, 16M) // default use SYS_FREQ_16M
#define BRR_921600           BRR_DIV(921600, 16M) // default use SYS_FREQ_16M
#endif //SYS_CLK

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init uart port (enable clock, reset, IO func map).
 *
 * @param[in] port   uart port @see enum uart_port
 * @param[in] io_tx  iopad of TXD @see enum pad_idx
 * @param[in] io_rx  iopad of RXD @see enum pad_idx
 *
 ****************************************************************************************
 */
void uart_init(uint8_t port, uint8_t io_tx, uint8_t io_rx);

/**
 ****************************************************************************************
 * @brief Deinit uart port(disable clock).
 *
 * @param[in] port   uart port @see enum uart_port
 *
 ****************************************************************************************
 */
void uart_deinit(uint8_t port);

/**
 ****************************************************************************************
 * @brief Config uart hardware flow control (IO func map, enable auto RTS).
 *
 * @param[in] port   uart port @see enum uart_port
 * @param[in] io_rts  iopad of RTS @see enum pad_idx
 * @param[in] io_cts  iopad of CTS @see enum pad_idx
 *
 ****************************************************************************************
 */
void uart_hwfc(uint8_t port, uint8_t io_rts, uint8_t io_cts);

/**
 ****************************************************************************************
 * @brief Config uart params, baudrate and ctrl bits.
 *
 * @param[in] port     index of port @see enum uart_port
 * @param[in] cfg_BRR  baudrate divider @see BRR_DIV() or BRR_BAUD()
 * @param[in] cfg_LCR  line-control bits @see LCR_BITS()
 *
 ****************************************************************************************
 */
void uart_conf(uint8_t port, uint16_t cfg_BRR, uint16_t cfg_LCR);

/**
 ****************************************************************************************
 * @brief Config uart fifo level and receive timeout.
 *
 * @param[in] port      index of port @see enum uart_port
 * @param[in] fifo_ctl  fifo control @see enum uart_fcr_bfs
 * @param[in] bits_rto  rx timeout (unit in bits)
 * @param[in] intr_en   interrupt enable @see enum uart_intr_bfs
 *
 ****************************************************************************************
 */
void uart_fctl(uint8_t port, uint8_t fifo_ctl, uint16_t bits_rto, uint16_t intr_en);

/**
 ****************************************************************************************
 * @brief Config uart mode control.
 *
 * @param[in] port  index of port @see enum uart_port
 * @param[in] dma   enable DMA or not
 *
 ****************************************************************************************
 */
void uart_mctl(uint8_t port, uint8_t dma);

/**
 ****************************************************************************************
 * @brief Transmits one byte data through the UART port.
 *
 * @param[in] port  index of port @see enum uart_port
 * @param[in] ch    the data transmited
 *
 ****************************************************************************************
 */
void uart_putc(uint8_t port, uint8_t ch);

/**
 ****************************************************************************************
 * @brief Get the most recent received data by UART port.
 *
 * @param[in] port  index of port @see enum uart_port
 *
 * @return received data
 ****************************************************************************************
 */
uint8_t uart_getc(uint8_t port);

/**
 ****************************************************************************************
 * @brief Wait uart transmit finish, in idle state.
 *
 * @param[in] port  index of port @see enum uart_port
 *
 ****************************************************************************************
 */
void uart_wait(uint8_t port);

/**
 ****************************************************************************************
 * @brief Transmits bytes data through the UART port(Blocking Mode).
 *
 * @param[in] port  index of port @see enum uart_port
 * @param[in] len    data length
 * @param[in] data   data pointer
 *
 ****************************************************************************************
 */
void uart_send(uint8_t port, uint16_t len, const uint8_t *data);

#endif  //_UART_H_
