/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Bootloader / firmware loader entry point
 *
 * @details
 * Implements a simple flash-based OTA loader. Runs from SRAM after reset.
 *
 * Flash layout:
 *   LDR_ADDR (0x18000000):
 *     +-------------------------------------------+
 *     |  Loader (APROM)                           |
 *     +-------------------------------------------+
 *
 *   LDR_RUN_ADDR (aligned 0x100):
 *     +-------------------------------------------+
 *     |  APP (Code and Data)                      |
 *     +-------------------------------------------+
 *
 *   LDR_INFO_ADDR (aligned 0x100):
 *     +-------------------------------------------+
 *     |  page+0: ldr_info_t                       |
 *     +-------------------------------------------+
 *     |  page+1: code start (otaAddr)             |
 *     |  page+n: code end   (otaLen = end-start)  |
 *     +-------------------------------------------+
 *
 * Boot flow:
 * 1. Read loader info from LDR_INFO_ADDR
 * 2. If ldrType == TYPE_COPY: copy OTA image to LDR_RUN_ADDR
 * 3. Erase loader info to prevent re-copy on next boot
 * 4. Jump to application at LDR_RUN_ADDR
 *
 ****************************************************************************************
 */

#include "regs.h"
#include "drvs.h"
#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#ifndef PAGE_SIZE
#define PAGE_SIZE            0x100
#endif

#define PAGE_SIZE_WLEN       (PAGE_SIZE >> 2)

/// Loader type
#define LDR_TYPE_NONE        0xFFFFFFFF
#define LDR_TYPE_COPY        0x55AA5AA5

/** Loader info stored at LDR_INFO_ADDR */
typedef struct
{
    uint32_t ldrType;   ///< Loader type: TYPE_COPY or TYPE_NONE
    uint32_t otaLen;    ///< OTA image length in bytes
    uint32_t otaAddr;   ///< OTA image source address (typically INFO_ADDR + 0x100)
} ldr_info_t;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Validate application at given address
 *
 * @param[in] run_addr  Application base address
 *
 * @return true if application appears valid
 *
 * @note Currently always returns true; add checks as needed.
 ****************************************************************************************
 */
static bool app_is_ok(uint32_t run_addr)
{
    (void)run_addr;

    return true;
}

/**
 ****************************************************************************************
 * @brief Copy OTA image from info region to run region (page by page)
 *
 * @param[in] run_addr  Destination address
 * @param[in] cpy_addr  Source address
 * @param[in] cpy_len   Number of bytes to copy
 ****************************************************************************************
 */
static void ota_copy(uint32_t run_addr, uint32_t cpy_addr, uint32_t cpy_len)
{
    uint32_t tmp_data[PAGE_SIZE_WLEN];
    uint32_t wr_page  = (run_addr     & 0x00FFFF00) / PAGE_SIZE;
    uint32_t rd_page  = (cpy_addr     & 0x00FFFF00) / PAGE_SIZE;
    uint32_t nb_pages = (cpy_len + (PAGE_SIZE - 1)) / PAGE_SIZE;

    while (nb_pages--)
    {
        // Read source page
        uint32_t offset = rd_page * PAGE_SIZE;
        fshc_read(offset, tmp_data, PAGE_SIZE_WLEN, FSH_CMD_RD);

        // Erase destination page and write
        offset = wr_page * PAGE_SIZE;
        fshc_erase(offset, FSH_CMD_ER_PAGE);
        fshc_write(offset, tmp_data, PAGE_SIZE_WLEN, FSH_CMD_WR);

        wr_page++;
        rd_page++;
    }
}

/**
 ****************************************************************************************
 * @brief Loader entry point (called from startup_ld.s as __main)
 *
 * @note Runs from SRAM. Overrides default __main to skip C runtime init.
 ****************************************************************************************
 */
int __main(void)
{
    ldr_info_t ldr_info;

    // LDR_XXX_ADDR fixed after __Vectors (Entry at 0x20003600, Size=0x10)
    // @see link_ld.sct and startup_ld.s
    uint32_t ldr_run_addr  = RD_32(0x20003610);
    uint32_t ldr_info_addr = RD_32(0x20003614);

    // Read loader type from info region
    ldr_info.ldrType = RD_32(ldr_info_addr + 0);

    if (ldr_info.ldrType == LDR_TYPE_COPY)
    {
        ldr_info.otaLen  = RD_32(ldr_info_addr + 4);
        ldr_info.otaAddr = RD_32(ldr_info_addr + 8);

        // Disable watchdog during flash copy
        iwdt_disable();

        // Copy OTA image to run address
        ota_copy(ldr_run_addr, ldr_info.otaAddr, ldr_info.otaLen);

        // Erase loader info to prevent re-copy on next boot
        fshc_erase((ldr_info_addr & 0x00FFFF00), FSH_CMD_ER_PAGE);

        // Re-enable watchdog
        iwdt_conf(0x20000);
    }

    // Jump to application
    if (app_is_ok(ldr_run_addr))
    {
        sysJumpTo(ldr_run_addr);
    }

    // Should never reach here
    while (1);
}
