/**
 ****************************************************************************************
 *
 * @file flash.h
 *
 * @brief Header file - Flash API
 *
 ****************************************************************************************
 */

#ifndef _FLASH_H_
#define _FLASH_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

enum flash_vendor_id
{      
    FSH_VID_PUYA           = 0x85,
    FSH_VID_WINBOND        = 0xEF,
    FSH_VID_GD             = 0xC8,
    FSH_VID_FUDAN          = 0xA1,
    FSH_VID_SPANSION       = 0x01,
    FSH_VID_ZBITSEMI       = 0x5E,
    FSH_VID_XMC            = 0x20,
    FSH_VID_BOYA           = 0x68,
};

enum flash_cmd_code
{
    // Reset opcode
    FSH_CMD_RST_EN         = 0x66,
    FSH_CMD_RESET          = 0x99,

    // Read flash id
    FSH_CMD_RD_ID          = 0x9F,

    // State opcode
    FSH_CMD_WR_STA_EN      = 0x50,
    FSH_CMD_WR_STA         = 0x01,
    FSH_CMD_WR_STA1        = 0x31, // Boya Flash
    FSH_CMD_RD_STA0        = 0x05,
    FSH_CMD_RD_STA1        = 0x35,

    // Erase opcode
    FSH_CMD_ER_CHIP        = 0x60,
    FSH_CMD_ER_SECTOR      = 0x20, 
    FSH_CMD_ER_PAGE        = 0x81,

    // Write opcode
    FSH_CMD_WR_EN          = 0x06,
    FSH_CMD_WR             = 0x02,
    FSH_CMD_DLWR           = 0xA2,
    FSH_CMD_QDWR           = 0x32,

    // Read opcode
    FSH_CMD_RD             = 0x03,
    FSH_CMD_DLRD           = 0x3B,
    FSH_CMD_QDRD           = 0x6B,

    // HPM opcode
    FSH_CMD_DLRD_HMP       = 0xBB,
    FSH_CMD_QDRD_HMP       = 0xEB,
    FSH_CMD_EXIT_HMP       = 0xFF,

    // Security Region opcode
    FSH_CMD_WR_OTP         = 0x42,    
    FSH_CMD_ER_OTP         = 0x44,
    FSH_CMD_RD_OTP         = 0x48,

    // Suspend/Resume when in erase & write
    FSH_CMD_SUSPEND        = 0x75,
    FSH_CMD_RESUME         = 0x7A,
};

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Read flash id.
 *
 * @return FlashID - 3 octets
 ****************************************************************************************
 */
#define flashReadId()    \
        fshc_rd_sta(FSH_CMD_RD_ID, 3)

/**
 ****************************************************************************************
 * @brief Read Flash Status Register.
 *
 * @param[in] cmd    Flash Read Status Command(FSH_CMD_RD_STA0, FSH_CMD_RD_STA1).
 *
 * @return Flash Status Register Value - 1 octets
 ****************************************************************************************
 */
#define flashReadSta(cmd)    fshc_rd_sta(cmd, 1)

/**
 ****************************************************************************************
 * @brief Puya Flash Enter Dual Read Mode(2 X IO Read Mod).
 ****************************************************************************************
 */
void puya_enter_dual_read(void);

/**
 ****************************************************************************************
 * @brief Puya Flash Exti Dual Read Mode.
 ****************************************************************************************
 */
void puya_exit_dual_read(void);

/**
 ****************************************************************************************
 * @brief Boya Flash Quad Enable bit set 1.
 ****************************************************************************************
 */
void boya_flash_quad_mode(void);

/**
 ****************************************************************************************
 * @brief Write data to Flash.
 *
 * @param[in] offset Where offset to write.
 * @param[in] data   The pointer of data write to flash.
 * @param[in] wlen   The number of word write to flash.
 *
 * @note  offset Must be 0x100 Aligned.
 * @note  data address Must be 4-byte Aligned.
 * @note  if hpm, must exit_hpm.
 ****************************************************************************************
 */
void flash_write(uint32_t offset, uint32_t *data, uint32_t wlen);

/**
 ****************************************************************************************
 * @brief   Read Flash data.
 *
 * @param[in] offset  Where offset to read.
 * @param[in] buff    The pointer that store data read from flash.
 * @param[in] wlen    The number of word read from flash.
 *
 * @note  buff address Must be 4-byte Aligned.
 * @note  if hpm, must exit_hpm.
 ****************************************************************************************
 */
void flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen);

/**
 ****************************************************************************************
 * @brief Erases the specified FLASH page.
 *
 * @param[in] offset address of the page to erase.
 *
 * @note  offset Must be 0x100 Aligned.
 * @note  if hpm, must exit_hpm.
 ****************************************************************************************
 */
void flash_page_erase(uint32_t offset);

/**
 ****************************************************************************************
 * @brief Write data to Flash.
 *
 * @param[in] offset Where offset to write.
 * @param[in] data   The pointer of data write to flash.
 * @param[in] blen   The number of byte write to flash.(Must be a multiple of 4)
 *
 * @note  offset Must be 0x100 Aligned.
 * @note  if hpm, must exit_hpm.
 ****************************************************************************************
 */
void flash_byte_write(uint32_t offset, uint8_t *data, uint32_t blen);

/**
 ****************************************************************************************
 * @brief   Read Flash data.
 *
 * @param[in] offset  Where offset to read.
 * @param[in] buff    The pointer that store data read from flash.
 * @param[in] blen    The number of byte read from flash.
 *
 * @note  if hpm, must exit_hpm.
 ****************************************************************************************
 */
void flash_byte_read(uint32_t offset, uint8_t *buff, uint32_t blen);

/**
 ****************************************************************************************
 * @brief Get Flash Size.
 *
 * @return  size in bytes.
 *
 * @note   0 is invalid size.
 ****************************************************************************************
 */
uint32_t flash_size(void);
#endif //_FLASH_H_
