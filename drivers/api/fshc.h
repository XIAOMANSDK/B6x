/**
 ****************************************************************************************
 *
 * @file fshc.h
 *
 * @brief Header file - Flash Controlor(FSHC) Driver
 *
 ****************************************************************************************
 */

#ifndef _FSHC_H_
#define _FSHC_H_

#include <stdint.h>
#include <stdbool.h>


/*
 * DEFINES
 ****************************************************************************************
 */


typedef uint32_t  flen_t; // uint16->32 reduce size 6vp 1118

enum cache_region_size
{
    CACHE_RS1K             = 0x09,
    CACHE_RS2K             = 0x0A,
    CACHE_RS4K             = 0x0B,
    CACHE_RS8K             = 0x0C,
    CACHE_RS16K            = 0x0D,
    CACHE_RS32K            = 0x0E,
    CACHE_RS64K            = 0x0F,
    CACHE_RS128K           = 0x10,
    CACHE_RS256K           = 0x11,
    CACHE_RS512K           = 0x12,
};

enum bit_num_mode
{
    // 0/1: 1 bit spi mode
    SPI_MODE                 = 1,
    // 2:   2bits dual mode
    DUAL_MODE                = 2,
    // 3:   4bits quad mode
    QUAD_MODE                = 3,
};

/// Bits field of Cmd and Mode
enum fshc_cmd_mode
{
    // Command Code - bit[7:0]
    FCM_CMD_LSB              = 0,
    FCM_CMD_MSK              = 0xFF << FCM_CMD_LSB,
    
    // Line BitMode - bit[9:8]
    FCM_MODE_LSB             = 8,
    FCM_MODE_MSK             = 0x03 << FCM_MODE_LSB,
    
    // Special Bits - bit[11:10]
    // OTP read or write
    FCM_RWOTP_BIT            = 1 << 10,
    // erase chip
    FCM_ERCHIP_BIT           = 1 << 11,
    
    // R&W FIFO Type - bit[15:12]
    // pkt fill: 1 - next fragment, 0 - first of send-en
    FCM_NEXTPKT_BIT          = 1 << 12,
    // multi-packet transmisson mode
    FCM_PACKETS_BIT          = 1 << 13,
    // not wait complete
    FCM_NOTWAIT_BIT          = 1 << 14,
    // enable suspend mode
    FCM_SUSPEND_BIT          = 1 << 15,
};

// BitMode, default SPI_MODE
#define FCM_MODE_DUAL        (DUAL_MODE << FCM_MODE_LSB)
#define FCM_MODE_QUAD        (QUAD_MODE << FCM_MODE_LSB)

// R&W FIFO Type
#define FCM_TYPE_BLOCKING    (0)
#define FCM_TYPE_NOTWAIT     (FCM_NOTWAIT_BIT)
#define FCM_TYPE_SUSPEND     (FCM_SUSPEND_BIT)
#define FCM_TYPE_PKTSTART    (FCM_PACKETS_BIT | FCM_NOTWAIT_BIT)
#define FCM_TYPE_PKTNEXT     (FCM_PACKETS_BIT | FCM_NEXTPKT_BIT)

/// SBUS access flash send_ctrl register config

enum addr_with_arg
{
    // 2'b00/2'b11: address followed by idle
    IDLE_ADR_ARG             = 0,
    // 2'b01:       address followed by wdata
    SDAT_ADR_ARG             = 1,
    // 2'b10:       address followed by rdata
    RDAT_ADR_ARG             = 2,
};

enum cmd_with_arg
{
    // 3'b011: command followed by address
    SADR_CMD_ARG             = 3,
    // 3'b101: command followed by wdata
    SDAT_CMD_ARG             = 5,
    // 3'b110: command followed by rdata
    RDAT_CMD_ARG             = 6,
    // others: command followed by idle
    IDLE_CMD_ARG             = 0,
};

enum send_ctrl_bfs
{
    // bit[1:0] - MCU transmit data bit number (@see enum bit_num_mode)
    SCTRL_DATA_BIT_MODE_LSB  = 0,
    SCTRL_DATA_BIT_MODE_MSK  = (0x03 << SCTRL_DATA_BIT_MODE_LSB),
    // bit[3:2] - MCU transmit address bit number (@see enum bit_num_mode)
    SCTRL_ADDR_BIT_MODE_LSB  = 2,
    SCTRL_ADDR_BIT_MODE_MSK  = (0x03 << SCTRL_ADDR_BIT_MODE_LSB),
    // bit[5:4] - MCU transmit command bit number (most support only 1bit, eg. GD)
    SCTRL_CMD_BIT_MODE_LSB   = 4,
    SCTRL_CMD_BIT_MODE_MSK   = (0x03 << SCTRL_CMD_BIT_MODE_LSB),
    // bit[7:6] - MCU address argument (@see enum addr_with_arg)
    SCTRL_ADDR_WITH_ARG_LSB  = 6,
    SCTRL_ADDR_WITH_ARG_MSK  = (0x03 << SCTRL_ADDR_WITH_ARG_LSB),
    // bit[10:8]- MCU command argument (@see enum cmd_with_arg)
    SCTRL_CMD_WITH_ARG_LSB   = 8,
    SCTRL_CMD_WITH_ARG_MSK   = (0x07 << SCTRL_CMD_WITH_ARG_LSB),
};

#define SEND_CTRL_CONF(datmode, adrmode, cmdarg, adrarg)                               \
    ( ((datmode) << SCTRL_DATA_BIT_MODE_LSB) | ((adrmode) << SCTRL_ADDR_BIT_MODE_LSB)  \
    | (SPI_MODE << SCTRL_CMD_BIT_MODE_LSB)   /* command be mostly 1bit mode */         \
    | ((cmdarg) << SCTRL_CMD_WITH_ARG_LSB)   | ((adrarg) << SCTRL_ADDR_WITH_ARG_LSB) )

// command sent with address for READ, expect receive data.
#define SCTRL_RD_DAT(datmode)  SEND_CTRL_CONF(datmode, SPI_MODE, SADR_CMD_ARG, RDAT_ADR_ARG)
// command sent with address and data for WRITE.
#define SCTRL_WR_DAT(datmode)  SEND_CTRL_CONF(datmode, SPI_MODE, SADR_CMD_ARG, SDAT_ADR_ARG)

// SBUS control conf
enum sbus_ctrl_cfg
{
    // command sent without arguments for ENABLE Only.
    SCTRL_EN_CMD             = SEND_CTRL_CONF(SPI_MODE, SPI_MODE, IDLE_CMD_ARG, IDLE_ADR_ARG),
    // command sent without arguments for READ, expect receive state value(l~4Bytes).
    SCTRL_RD_STA             = SEND_CTRL_CONF(SPI_MODE, SPI_MODE, RDAT_CMD_ARG, IDLE_ADR_ARG),
    // command sent with state value(l~4Bytes) for WRITE.
    SCTRL_WR_STA             = SEND_CTRL_CONF(SPI_MODE, SPI_MODE, SDAT_CMD_ARG, IDLE_ADR_ARG),
    // command sent without arguments for ERASE chip all.
    SCTRL_ER_CHIP            = SEND_CTRL_CONF(SPI_MODE, SPI_MODE, IDLE_CMD_ARG, IDLE_ADR_ARG),
    // commnad sent with address for ERASE page/sector/bank part.
    SCTRL_ER_PART            = SEND_CTRL_CONF(SPI_MODE, SPI_MODE, SADR_CMD_ARG, IDLE_ADR_ARG),
    // Read & Write date command in special mode.
    SCTRL_SIRD_DAT           = SCTRL_RD_DAT(SPI_MODE),
    SCTRL_SIWR_DAT           = SCTRL_WR_DAT(SPI_MODE),
    SCTRL_DLRD_DAT           = SCTRL_RD_DAT(DUAL_MODE),
    SCTRL_DLWR_DAT           = SCTRL_WR_DAT(DUAL_MODE),
    SCTRL_QDRD_DAT           = SCTRL_RD_DAT(QUAD_MODE),
    SCTRL_QDWR_DAT           = SCTRL_WR_DAT(QUAD_MODE),
};

enum adrcmd_bitlen_bfs
{
    // bit[5:0]  --- MCU adr cycle length,include dummy cycle, only 24 bit is valid
    ACBIT_ADR_LEN_LSB        = 0,
    ACBIT_ADR_LEN_MSK        = 0x1F << ACBIT_ADR_LEN_LSB,
    // bit[11:6] --- MCU cmd cycle length,include dummy cycle, only 8 bit is vaild,
    ACBIT_CMD_LEN_LSB        = 6,
    ACBIT_CMD_LEN_MSK        = 0x1F << ACBIT_CMD_LEN_LSB,
};

#define ACBIT_ADR_LEN(len)     ((len) << ACBIT_ADR_LEN_LSB)
#define ACBIT_CMD_LEN(len)     ((len) << ACBIT_CMD_LEN_LSB)

enum acbit_len_cfg
{
    // 0x1C0 bit: cmd=8-1, adr=0
    ACBIT_SI_IDLE            = ACBIT_CMD_LEN(7) | ACBIT_ADR_LEN(0x00),
    // 0x1D7 bit: cmd=8-1, adr=24-1, 0dumy=0
    ACBIT_SI_0DUMY           = ACBIT_CMD_LEN(7) | ACBIT_ADR_LEN(0x17),
    // 0x1DF bit: cmd=8-1, adr=24-1, 1dumy=8
    ACBIT_SI_1DUMY           = ACBIT_CMD_LEN(7) | ACBIT_ADR_LEN(0x1F),
    // 0x1CF bit: cmd=8-1, adr=24/2-1, 1dumy=4
    ACBIT_DL_1DUMY           = ACBIT_CMD_LEN(7) | ACBIT_ADR_LEN(0x0F),
};

// SBUS access dummy
enum sbus_dumy_cfg
{
    // adr23 + NDUMY
    SBUS_SI_0DUMY            = 0x17,
    // adr23 + 8(1DUMY)
    SBUS_SI_1DUMY            = 0x1F,
    // adr24/2 - 1 + 4(1DUMY)
    SBUS_DL_1DUMY            = 0xF,
};


/// IBUS access flash(only support read data) fshc ctrl configure

enum cmd_len_dumy
{
    // cmd(7) + dumy(0)
    CLEN_0DUMY               = 7,
    // cmd(7) + dumy(8)
    CLEN_1DUMY               = 15,
};

enum delay_set_bfs
{
    // bit[5:0]  --- hpm command length, default is 6'h1f, 
    DLYSET_HPM_CLEN_LSB       = 0,
    // bit[9:6]  --- only use in qpi read cmd, default is 4'h7,
    DLYSET_CMD_LEN_LSB        = 6,
    // bit[13:10]--- use in suspend/resume/exit_hpm cmd, default is 4'h7,
    DLYSET_0DUMY_CLEN_LSB     = 10,
    // bit[15:14]--- CACHE transmit address bit number(@see enum bit_num_mode)
    DLYSET_ADDR_BIT_MODE_LSB  = 14,
    // bit[17:16]--- CACHE transmit command bit number(@see enum bit_num_mode)
    DLYSET_CMD_BIT_MODE_LSB   = 16,
    // bit[19:18]--- CACHE transmit data bit number(@see enum bit_num_mode)
    DLYSET_DATA_BIT_MODE_LSB  = 18,
};

#define IBUS_CTRL_CONF(adrmode, datmode, hlen)                                      \
    ( ((adrmode) << DLYSET_ADDR_BIT_MODE_LSB) | (SPI_MODE << DLYSET_CMD_BIT_MODE_LSB)  \
    | ((datmode) << DLYSET_DATA_BIT_MODE_LSB) | (CLEN_0DUMY << DLYSET_CMD_LEN_LSB)     \
    | (CLEN_0DUMY << DLYSET_0DUMY_CLEN_LSB)   | ((hlen) << DLYSET_HPM_CLEN_LSB) )

// IBUS control conf
enum ibus_ctrl_cfg
{
    // spi read data via IBUS
    IBUS_SIRD_CFG            = IBUS_CTRL_CONF(SPI_MODE, SPI_MODE, CLEN_0DUMY),
    //dual read data via IBUS
    IBUS_DLRD_CFG            = IBUS_CTRL_CONF(SPI_MODE, DUAL_MODE, CLEN_0DUMY),
    //quad read data via IBUS
    IBUS_QDRD_CFG            = IBUS_CTRL_CONF(SPI_MODE, QUAD_MODE, CLEN_0DUMY),
    //dual hpm read data via IBUS
    IBUS_DLRD_HPM            = IBUS_CTRL_CONF(DUAL_MODE, DUAL_MODE, CLEN_1DUMY),
    //quad hpm read data via IBUS
    IBUS_QDRD_HPM            = IBUS_CTRL_CONF(QUAD_MODE, QUAD_MODE, CLEN_1DUMY),
};

// IBUS access dummy
enum ibus_dumy_cfg
{
    IBUS_SI_0DUMY            = 0x17,
    IBUS_SI_1DUMY            = 0x1F,
    
    IBUS_DL_1DUMY            = 0xF,
    IBUS_QD_1DUMY            = 0xB,
};


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

#if (ROM_UNUSED)
/**
 ****************************************************************************************
 * @brief Send control/enable command without value
 *
 * @param[in] cmd  Command opcode(eg. FSH_CMD_RST_EN FSH_CMD_RESET FSH_CMD_EXIT_HMP 
 *                                    FSH_CMD_WR_EN FSH_CMD_WR_STA_EN)
 *
 ****************************************************************************************
 */
void fshc_en_cmd(uint8_t cmd);

/**
 ****************************************************************************************
 * @brief Write value to flash state register
 *
 * @param[in] cmd  Command opcode(eg. FSH_CMD_WR_STA)
 * @param[in] len  Length of value, range 1~4 Bytes
 * @param[in] val  State value, valid range 8/16/24/32bits by 'len'
 *
 ****************************************************************************************
 */
void fshc_wr_sta(uint8_t cmd, uint8_t len, uint32_t val);

/**
 ****************************************************************************************
 * @brief Read value from flash state register
 *
 * @param[in] cmd  Command opcode(eg. FSH_CMD_RD_ID FSH_CMD_RD_STA0 FSH_CMD_RD_STA1)
 * @param[in] len  Length of value, range 1~4 Bytes
 *
 * @return value  State value, valid range 8/16/24/32bits by 'len'
 ****************************************************************************************
 */
uint32_t fshc_rd_sta(uint8_t cmd, uint8_t len);

/**
 ****************************************************************************************
 * @brief Perpare write data to flash
 *
 * @param[in] cmd     fshc access cmd (example read cmd: 0x03)
 * @param[in] offset  access flash addr offset
 * @param[in] len     access flash data len, unit is byte
 * @param[in] sctrl   access flash mode config(example: spi mode, dual mode...)
 * @param[in] acbit   dummy cycle
 *
 ****************************************************************************************
 */
void fshc_wr_cfg(uint8_t cmd, uint32_t offset, flen_t len, uint16_t sctrl, uint16_t acbit);

flen_t fshc_wr_fifo(const uint32_t *data, flen_t wlen, uint16_t fcmd);

/**
 ****************************************************************************************
 * @brief Perpare read data from flash
 *
 * @param[in] cmd     fshc access cmd (example read cmd: 0x03)
 * @param[in] offset  access flash addr offset
 * @param[in] len     access flash data len, unit is byte
 * @param[in] sctrl   access flash mode config(example: spi mode, dual mode...)
 * @param[in] acbit   dummy cycle
 *
 ****************************************************************************************
 */
void fshc_rd_cfg(uint8_t cmd, uint32_t offset, flen_t len, uint16_t sctrl, uint16_t acbit);

flen_t fshc_rd_fifo(uint32_t *buff, flen_t wlen, uint16_t fcmd);

void fshc_xip_conf(uint8_t rdCmd, uint8_t adrBits, uint32_t dlySet);

void fshc_hpm_conf(bool en, uint8_t crIdx, uint8_t crCmd);

void fshc_erase(uint32_t offset, uint16_t fcmd);

flen_t fshc_read(uint32_t offset, uint32_t *buff, flen_t wlen, uint16_t fcmd);

flen_t fshc_write(uint32_t offset, const uint32_t *data, flen_t wlen, uint16_t fcmd);

void fshc_cache_conf(uint32_t base_addr);
void fshc_suspend_conf(uint8_t susCmd, uint8_t rsmCmd, uint16_t susTime, uint32_t rsmTime);
#endif

#endif // _FSHC_H_
