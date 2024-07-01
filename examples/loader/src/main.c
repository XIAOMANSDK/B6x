/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the Loader.
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

/****************** FLASH STORAGE AREAS *******************

LDR_ADDR(0x1800000):
  _________________________________________________________
  |                                                       |
  |  Loader(APROM)                                        |
  |_______________________________________________________|

LDR_RUN_ADDR(Align 0x100):
  _________________________________________________________
  |                                                       |
  |  APP (Code and Data)                                  |
  |                                                       |
  |_______________________________________________________|
 
LDR_INFO_ADDR(Align 0x100):
  _________________________________________________________
  |                                                       |
  |  page+0: ldr_info_t                                   |
  |_______________________________________________________|
  |                                                       |
  |  page+1: code start (otaAddr = INFO_ADDR + 0x100)     |
  |  page+n: code end   (otaLen  = end - start)           |
  |_______________________________________________________|

***********************************************************/

#ifndef PAGE_SIZE
#define PAGE_SIZE            0x100
#endif

#define PAGE_SIZE_WLEN       (PAGE_SIZE >> 2)

/// Loader type
#define LDR_TYPE_NONE        0xFFFFFFFF
#define LDR_TYPE_COPY        0x55AA5AA5

/// Loader's Info
typedef struct
{
    // Loader type  - TYPE_COPY or TYPE_NONE
    uint32_t ldrType;
    // Code length  - copy code from INFO_ADDR to RUN_ADDR
    uint32_t otaLen;
    // Code Address - suggest at LDR_INFO_ADDR + 0x100
    uint32_t otaAddr;

    /* add other info if need */

} ldr_info_t;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static bool app_is_ok(uint32_t run_addr)
{
    // Judge app is right at LDR_RUN_ADDR if need.
    #if (0)
    uint32_t stack_top = RD_32(run_addr + 0);
    uint32_t reset_hdl = RD_32(run_addr + 4);
    
    if (((reset_hdl >> 24) != 0x18) || (stack_top < 0x20003000) || (stack_top > 0x2000B000))
        return false;
    #endif

    return true;
}

static void ota_copy(uint32_t run_addr, uint32_t cpy_addr, uint32_t cpy_len)
{
    uint32_t tmp_data[PAGE_SIZE_WLEN];
    uint32_t wr_page  = (run_addr     & 0x00FFFF00) / PAGE_SIZE;
    uint32_t rd_page  = (cpy_addr     & 0x00FFFF00) / PAGE_SIZE;
    uint32_t nb_pages = (cpy_len + (PAGE_SIZE - 1)) / PAGE_SIZE;

    // Copy Flash page by page
    //for (uint32_t pg_idx = 0; pg_idx < nb_pages; pg_idx++)
    while (nb_pages--) 
    {
        // Read out 
        uint32_t offset = rd_page * PAGE_SIZE;
        fshc_read(offset,  tmp_data, PAGE_SIZE_WLEN, FSH_CMD_RD);

        // Erase, Write in
        offset = wr_page * PAGE_SIZE;
        fshc_erase(offset, FSH_CMD_ER_PAGE);
        fshc_write(offset, tmp_data, PAGE_SIZE_WLEN, FSH_CMD_WR);

        // Next Page
        wr_page++;
        rd_page++;
    }
}

/// Override __main - remove complier func, eg. __scatterload
int __main(void)
{
    ldr_info_t ldr_info;

    // LDR_XXX_ADDR fixed after __Vectors(Entry at 0x20003600 Size=0x10)
    // for easy to external modify.  @see link_ld.sct and startup_ld.s
    uint32_t ldr_run_addr  = RD_32(0x20003610);
    uint32_t ldr_info_addr = RD_32(0x20003614);
    
    // AONLDO VOL > CORELDO VOL
    //AON->BKHOLD_CTRL.CORELDO_TRIM_RUN = 0x1B;
    //AON->BKHOLD_CTRL.AONLDO_TRIM_RUN  = 0x01;
    
//    if ((AON->BKHOLD_CTRL.Word & 0x01FF0000U) == 0)
    {
        // 10ms iwdt rst. 
        // fixbug--Low temperature poweron run fly. 2024.06.27 --- 6vp.
//        iwdt_conf(160); 
//        AON->BKHOLD_CTRL.Word = 0x01B12008;
//        // Re-enable WatchDog
//        iwdt_conf(0x20000);
    }
    ///Program Size: Code=176 RO-data=24 RW-data=0 ZI-data=1536  
    ///Program Size: Code=188 RO-data=24 RW-data=0 ZI-data=1536  
    // Close interrupt, already
    //__disable_irq();

    // Read Loader's Info
    //memcpy(&ldr_info, (ldr_info_t *)(ldr_info_addr), sizeof(ldr_info_t));
    ldr_info.ldrType  = RD_32(ldr_info_addr + 0);

    // Judge Loader Type
    if (ldr_info.ldrType == LDR_TYPE_COPY)
    {
        ldr_info.otaLen  = RD_32(ldr_info_addr + 4);
        ldr_info.otaAddr = RD_32(ldr_info_addr + 8);

        // Check Validity, eg. Fixed (otaAddr = INFO_ADDR + 0x100)
        //if (ldr_info.otaAddr == ldr_info_addr + PAGE_SIZE)
        {
            // Disable WatchDog
            iwdt_disable();

            // Copy code from INFO_ADDR to RUN_ADDR.
            ota_copy(ldr_run_addr, ldr_info.otaAddr, ldr_info.otaLen);

            // Erase Loader's Info to avoid copy again
            fshc_erase((ldr_info_addr & 0x00FFFF00), FSH_CMD_ER_PAGE);

            // Re-enable WatchDog
            iwdt_conf(0x20000);
        }
    }

    // Jump to run
    if (app_is_ok(ldr_run_addr))
    {
        sysJumpTo(ldr_run_addr);
    }

    // Never 
    while (1);
}
