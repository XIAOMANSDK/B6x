/**
 ****************************************************************************************
 *
 * @file i2c.h
 *
 * @brief Header file - I2C Driver
 *
 ****************************************************************************************
 */

#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>
#include "reg_i2c.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// master status->MS, slave status->SS, General Call Address->GCA,  Arbitration lost->ARL
enum i2c_status_def
{
    I2C_MS_BUS_ERROR             = 0x00, // Bus error (Master mode only)
    I2C_MS_START_OK              = 0x08, // Master: start signal
    I2C_MS_RESTART_OK            = 0x10, // Master: restart signal
    I2C_MS_ADDR_W_ACK            = 0x18, // Master: addr+write bit send, ACK received
    I2C_MS_ADDR_W_NAK            = 0x20, // Master: addr+write bit send, Not ACK received
    I2C_MS_SEND_DAT_ACK          = 0x28, // Master: send data, ACK received OK
    I2C_MS_SEND_DAT_NAK          = 0x30, // Master: send data, Not ACK received 
    I2C_MS_TRANS_FAIL            = 0x38, // Master: Arbitration lost in address or data byte
    I2C_MS_ADDR_R_ACK            = 0x40, // Master: addr+read bit send, ACK received
    I2C_MS_ADDR_R_NAK            = 0x48, // Master: addr+read bit send, Not ACK received
    I2C_MS_READ_DAT_ACK          = 0x50, // Master: get data, send ACK
    I2C_MS_READ_DAT_NAK          = 0x58, // Master: get data, No send ACK
    I2C_SS_RECV_ADDR_W           = 0x60, // Slave: get addr+write, send ACK
    I2C_MS_ARL_RECV_ADDR_W       = 0x68, // Master: address arbit fail and get addr+write, send ACK
    I2C_SS_RECV_GCA_ACK          = 0x70, // Slave: get General Call Address and send ACK
    I2C_MS_ARL_RECV_GCA_ACK      = 0x78, // Master: address arbit fail get radio addresss, send ACK
    I2C_SS_RECV_DAT_ACK          = 0x80, // Slave: Get address and get data, send ACK
    I2C_SS_RECV_DAT_NAK          = 0x88, // Slave: Get address and get data, No send ACK
    I2C_SS_GCA_RECV_DAT_ACK      = 0x90, // Slave: Get General Call Address and get data, send ACK
    I2C_SS_GCA_RECV_DAT_NAK      = 0x98, // Slave: Get General Call Address and get data, No send ACK
    I2C_SS_RECV_STOP_OR_RESTART  = 0xA0, // Slave: get stop or restsrt signal
    I2C_SS_RECV_ADDR_R_ACK       = 0xA8, // Slave: get slave addr+read, send ACK
    I2C_MS_ARL_RECV_ADDR_R_ACK   = 0xB0, // Master: address arbit error get slave addr+read, send ACK
    I2C_SS_SEND_DAT_ACK          = 0xB8, // Slave: send data, ACK received
    I2C_SS_SEND_DAT_NAK          = 0xC0, // Slave: send data, Not ACK received
    I2C_SS_SEND_LAST_DAT_ACK     = 0xC8, // Slave: send LB, ACK received
    I2C_SS_SEND_LAST_DAT_NAK     = 0xD0, // Slave: send LB, Not ACK received
    I2C_MS_SEND_SECOND_ADDR_ACK  = 0xE0, // Master: second address send, ACK received 
    I2C_MS_SEND_SECOND_ADDR_NAK  = 0xE8, // Master: second address send, Not ACK received
    I2C_IFLG_IDLE                = 0xF8, // No Status information and IFLG =0 
};

/// Bits field of I2C Contrl @see I2C_CONSET_TypeDef
enum i2c_ctrl_bfs
{
    I2C_CR_AAK_BIT               = (1 << 2),
    I2C_CR_IFLG_BIT              = (1 << 3),
    I2C_CR_STOP_BIT              = (1 << 4),
    I2C_CR_START_BIT             = (1 << 5),
    I2C_CR_I2C_EN_BIT            = (1 << 6),
    I2C_CR_INT_EN_BIT            = (1 << 7),
};

/// Bits field of I2C Clock @see I2C_CLK_TypeDef
enum i2c_clk_bfs
{
    // bit[3:0] --- i2c master mode scl frequency
    I2C_CLK_M_LSB                = 0,
    I2C_CLK_M_MSK                = (0x0F << I2C_CLK_M_LSB),
    // bit[6:4] --- i2c samp frequency
    I2C_CLK_N_LSB                = 4,
    I2C_CLK_N_MSK                = (0x07 << I2C_CLK_N_LSB),
};

#if (SYS_CLK == 1)
    #define SCLK_100K          (0x27)
    #define SCLK_200K          (0x23)
    #define SCLK_400K          (0x21)
#elif (SYS_CLK == 2)
    #define SCLK_100K          (0x2B)
    #define SCLK_200K          (0x25)
    #define SCLK_400K          (0x22)
#elif (SYS_CLK == 3)
    #define SCLK_100K          (0x2F)
    #define SCLK_200K          (0x27)
    #define SCLK_400K          (0x23)
#else
    #define SCLK_100K          (0x23)
    #define SCLK_200K          (0x21)
    #define SCLK_400K          (0x11)
#endif //SYS_CLK

/// Master RD/WRn bit to be 'OR'ed with Slave address
#define I2C_RD_OP              (0x01)


/// Enable I2C bus / interrupt
#define I2C_ENABLE()           dowl( I2C->CONSET.Word = I2C_CR_I2C_EN_BIT; )
#define I2C_DISABLE()          dowl( I2C->CONCLR.Word = I2C_CR_I2C_EN_BIT; )
#define I2C_INT_EN()           dowl( I2C->CONSET.Word = I2C_CR_INT_EN_BIT; )
#define I2C_INT_DIS()          dowl( I2C->CONCLR.Word = I2C_CR_INT_EN_BIT; )

/// Set I2C Ack / NoAck
#define I2C_SET_ACK()          dowl( I2C->CONSET.Word = I2C_CR_AAK_BIT; )
#define I2C_SET_NAK()          dowl( I2C->CONCLR.Word = I2C_CR_AAK_BIT; )

/// Get/Clear I2C Interrupt Flag
#define I2C_IFLG_CLR()         dowl( I2C->CONCLR.Word = I2C_CR_IFLG_BIT; )
#define I2C_IFLG_GET()         (I2C->CONSET.Word & I2C_CR_IFLG_BIT)

/// Get I2C Status @see enum i2c_status_def
#define I2C_STAT_GET()         (I2C->SR.Word)

/// Get/Put I2C Data
#define I2C_DATA_GET(_X_)      dowl( (_X_) = I2C->DATA.Word; )
#define I2C_DATA_PUT(_X_)      dowl( I2C->DATA.Word = (_X_); )

// Set I2C Slave Addr
#define I2C_SLV_ADR(addr)      dowl( I2C->SADR0.Word = (addr) << 1; )

// Configure I2C Clock
#define I2C_CLK_CFG(samp, scl) dowl( I2C->CLK.Word = ((samp) << I2C_CLK_SAMP_N_LSB) | (scl); )


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
 
/**
 ****************************************************************************************
 * @brief Init I2C Module.
 *
 * @param[in] io_scl  io used for I2C scl @see enum pad_idx.
 * @param[in] io_sda  io used for I2C sda @see enum pad_idx.
 * @param[in] sclk    i2c clk. Bits field of I2C Clock @see enumi2c_clk_bfs.
 * 
 * @note sclk = CLK/(2^clk_samp * (clk_scl+1) *10). Macro SCLK_100K/SCLK_200K/SCLK_400K
 ****************************************************************************************
 */
void i2c_init(uint8_t io_scl, uint8_t io_sda, uint8_t sclk);

/**
 ****************************************************************************************
 * @brief Deinit I2C. Disable I2C Module.
 ****************************************************************************************
 */
void i2c_deinit(void);

/**
 ****************************************************************************************
 * @brief I2C Send Start.
 *
 * @param[in] slv_addr  device Address.
 *
 * @return I2C status @see enum i2c_status_def.
 ****************************************************************************************
 */
uint8_t i2c_start(uint8_t slv_addr);

/**
 ****************************************************************************************
 * @brief I2C Send Stop.
 ****************************************************************************************
 */
void i2c_stop(void);

/**
 ****************************************************************************************
 * @brief I2C Send data.
 *
 * @param[in] len    data length.
 * @param[in] data   data pointer.
 *
 * @return send data cnt.
 ****************************************************************************************
 */
uint16_t i2c_send(uint16_t len, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief I2C Receive data.
 *
 * @param[in] len    buff length.
 * @param[in] buff   buff pointer.
 *
 * @return receive data cnt.
 ****************************************************************************************
 */
uint16_t i2c_recv(uint16_t len, uint8_t *buff);

/**
 ****************************************************************************************
 * @brief I2C Read data.
 *
 * @param[in] addr_r device Address.
 * @param[in] len    buff length.
 * @param[in] buff   buff pointer.
 *
 * @return read data cnt.
 ****************************************************************************************
 */
uint16_t i2c_read(uint8_t addr_r, uint16_t len, uint8_t *buff);

/**
 ****************************************************************************************
 * @brief I2C Write data.
 *
 * @param[in] addr_w device Address.
 * @param[in] len    buff length.
 * @param[in] buff   buff pointer.
 *
 * @return write data cnt.
 ****************************************************************************************
 */
uint16_t i2c_write(uint8_t addr_w, uint16_t len, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief I2C Read eeprom data.
 *
 * @param[in] eep    eeprom device Address.
 * @param[in] addr   Where eeprom addr to read.
 * @param[in] len    buff length.
 * @param[in] buff   buff pointer.
 *
 * @return read data cnt.
 ****************************************************************************************
 */
uint16_t eeprom_read(uint8_t eep, uint16_t addr, uint16_t len, uint8_t *buff);

/**
 ****************************************************************************************
 * @brief I2C Write eeprom data.
 *
 * @param[in] eep    eeprom device Address.
 * @param[in] addr   Where eeprom addr to write.
 * @param[in] len    data length.
 * @param[in] data   data pointer.
 *
 * @return write data cnt.
 ****************************************************************************************
 */
uint16_t eeprom_write(uint8_t eep, uint16_t addr, uint16_t len, const uint8_t *data);

#endif // _I2C_H_
