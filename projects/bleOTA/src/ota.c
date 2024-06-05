/**
 ****************************************************************************************
 *
 * @file ota.c
 *
 * @brief Main loop of the application.
 *
 ****************************************************************************************
 */
#include "drvs.h"
#include "regs.h"
#include "prf_otas.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "ota.h"

/*
 * DEFINES
 ****************************************************************************************
 */
/// examples\loader
#define CFG_USE_LOAD  0

#if (DBG_OTAS)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#ifndef OTA_LED
#define OTA_LED  (GPIO08|GPIO10)
#endif

#ifndef FLASH_BASE
#define FLASH_BASE            (0x18000000)
#endif

#define FLASH_SIZE            (0x40000) // 256KB
#define FLASH_PAGE_SIZE       (0x100)   // 256B
#define FLASH_SECTOR_SIZE     (0x1000)  // 4KB
#define FLASH_PAGE_SIZE_WLEN  (FLASH_PAGE_SIZE >> 2)

#define FLASH_INFO_PAGE       (0x00)
#define FLASH_INFO_MAGIC      (FLASH_BASE + 0x00)
#define FLASH_INFO_CODE_LEN   (FLASH_BASE + 0x04)
#define FLASH_INFO_CODE_ADDR  (FLASH_BASE + 0x08)

#define OTA_BANK_A_BASE       (0x18004000)
#if (CFG_USE_LOAD)
#define OTA_BANK_B_BASE       (0x18020100)
#else
#define OTA_BANK_B_BASE       (0x18020000)
#endif
#define OTA_BANK_A            (OTA_BANK_A_BASE - FLASH_BASE)
#define OTA_BANK_B            (OTA_BANK_B_BASE - FLASH_BASE)

#define FLASH_ADDR_INVALID    (0xFFFFFFFF)

#define OTA_FW_SECTOR_MAX_SIZE   ((FLASH_SIZE - OTA_BANK_B) / FLASH_SECTOR_SIZE)
#define OTA_FW_MAX_SIZE          (OTA_FW_SECTOR_MAX_SIZE * FLASH_SECTOR_SIZE) / 16 // 112K

#define OTA_BUFF_LEN             (64)
#define OTA_PKT_LEN              (20) // 2B index + 16B ota data + 2B CRC
#define OTA_DATA_LEN             (OTA_PKT_LEN - 4)
#define OTA_DATA_OLD             (0x55)
#define OTA_DATA_NEW             (0xAA)
#define OTA_TIMER_TICK           _MS(3000) // 3s
#define OTA_MAX_TIMEOUT          (1000) // 3000s

enum ota_state
{
    OTA_IDLE,
    OTA_START,
    OTA_BUSY,
    OTA_ERROR,
    OTA_END,
};

enum ota_cmd
{
    OTA_CMD_START     = 0xFF01,
    OTA_CMD_END       = 0xFF02,
};

enum ota_err_code
{
    OTA_ERR_NONE      = 0, // No error
    OTA_ERR_BANK_ADDR = 0x5501,
    OTA_ERR_PKT_LEN   = 0x5502,
    OTA_ERR_CRC       = 0x5504,
    OTA_ERR_FW        = 0x5508,
    OTA_ERR_TIMEOUT   = 0x5510,
    OTA_ERR_INDEX     = 0x5512,
    OTA_ERR_MAC       = 0x5514,
    OTA_ERR_FAIL      = 0x55AA,
};

struct ota_env_tag
{
    uint32_t bank;
    uint16_t data_len;
    uint16_t err_info;
    uint16_t local_data_idx;
    uint16_t peer_data_idx;
    uint8_t  data_sta;
    uint8_t  proc_sta;
    tmr_tk_t tcnt; 
    tmr_id_t tid;
    uint8_t  rsv;
};

uint8_t  ota_recv_buf[OTA_BUFF_LEN];
uint32_t ota_wr_data[FLASH_PAGE_SIZE_WLEN];


volatile struct ota_env_tag ota_env;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
__attribute__((section("ram_func.ota.124")))
static void flash_sector_erase(uint32_t offset)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
    fshc_erase(offset, FSH_CMD_ER_SECTOR);
    
    GLOBAL_INT_RESTORE();
}

__attribute__((section("ram_func.ota.136")))
static void flash_wr_protect(uint8_t val)
{
    GLOBAL_INT_DISABLE();
    
    while (SYSCFG->ACC_CCR_BUSY);
    
    uint8_t sta_reg0 = fshc_rd_sta(FSH_CMD_RD_STA0, 1);
    
    if (val != sta_reg0)
    {
        // Ð´±£»¤0x000-0xFFF (4KB)
        // write en singal
        fshc_en_cmd(FSH_CMD_WR_EN);

        // send write sta cmd
        fshc_wr_sta(FSH_CMD_WR_STA, 1, val);
        
        bootDelayUs(12000);
    }
    
    GLOBAL_INT_RESTORE();
}

/**
 ****************************************************************************************
 * @brief Swap bytes of a 16 bits value.
 * The swap is done in every case. Should not be called directly.
 * @param[in] val16 The 16 bit value to swap.
 * @return The 16 bit swapped value.
 ****************************************************************************************
 */
__INLINE__ uint16_t co_bswap16(uint16_t val16)
{
    return ((val16 << 8) & 0xFF00) | ((val16 >> 8) & 0x00FF);
}

static void ota_env_init(void)
{
    ota_env.bank           = FLASH_ADDR_INVALID;
    ota_env.data_len       = 0;
    ota_env.data_sta       = OTA_DATA_OLD;
    ota_env.err_info       = OTA_ERR_NONE;
    ota_env.proc_sta       = OTA_IDLE;
    ota_env.local_data_idx = 0;
    ota_env.peer_data_idx  = 0xFFFF;
    ota_env.tid            = TMR_ID_NONE;
    ota_env.tcnt           = 0;
}

void ota_init(void)
{
    ota_env_init();
    
    #if (CFG_USE_LOAD)
    ota_env.bank = OTA_BANK_B;
    #else
    uint32_t curr_code_addr = RD_32(FLASH_INFO_CODE_ADDR);
    DEBUG("Curr Addr:0x%x", curr_code_addr);
    ota_env.bank = FLASH_ADDR_INVALID;
    if(curr_code_addr == OTA_BANK_A_BASE)
    {
        ota_env.bank = OTA_BANK_B;
    }
    else if(curr_code_addr == OTA_BANK_B_BASE)
    {
        ota_env.bank = OTA_BANK_A;
    }
    #endif

    DEBUG("OTA_BANK:0x%x", ota_env.bank);
}

uint8_t app_name_get(uint8_t size, uint8_t *name)
{
    uint8_t len = sizeof(BLE_DEV_NAME) - 1;

    // eg. prefix(BLE_DEV_NAME) + suffix(Addr[0])
    if (size < len + 2)
    {
        // no enough buffer, short copy
        len = size;
        memcpy(name, BLE_DEV_NAME, len);
    }
    else
    {
        // prefix + suffix
        memcpy(name, BLE_DEV_NAME, len);
        
        if (ota_env.bank)
        {
        
        }
        name[len++] = (ota_env.bank == OTA_BANK_A ? 'B' : 'A');
    }

    return len;
}

/******************************************************************************
 * Name:    CRC-16/MODBUS
 * Poly:    0x8005  ( x16+x15+x2+1 )
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
static uint16_t crc16_modbus(uint8_t *data, uint16_t length)
{
    uint8_t  i;
    uint16_t crc = 0xFFFF; // Initial value

    while (length--)
    {
        crc ^= *data++;
        
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xA001; // 0xA001 = reverse 0x8005
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }

    return crc;
}

void otas_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    app_env.curidx   = conidx;
    ota_env.data_len = len;

    if (ota_env.data_len > OTA_BUFF_LEN)
    {
        ota_env.data_len = OTA_BUFF_LEN;
    }

    memcpy(ota_recv_buf, data, ota_env.data_len);

    ota_env.data_sta = OTA_DATA_NEW;
}

tmr_tk_t ota_time_handle(uint8_t timeid)
{
    ota_env.tcnt++;

    if (ota_env.tcnt > OTA_MAX_TIMEOUT)
    {
        ota_env.tcnt = 0;
        ota_env.proc_sta = OTA_ERROR;
        ota_env.err_info = OTA_ERR_TIMEOUT;

        DEBUG("OTA_TIMEOUT");
        sftmr_clear(ota_env.tid);
    }

    return OTA_TIMER_TICK;
}

void ota_ok_led(void)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        GPIO_DAT_TOG(OTA_LED);
        sftmr_wait(20);
    }
}

void ota_proc(void)
{
    uint16_t peer_crc16, local_crc16;
    
    switch (ota_env.proc_sta)
    {
        case OTA_IDLE:
        {
            if ((co_bswap16(read16p(ota_recv_buf)) == OTA_CMD_START) && (ota_env.data_len == OTA_PKT_LEN))
            {
                ota_env.data_sta = OTA_DATA_OLD;

                peer_crc16  = read16p(ota_recv_buf + OTA_PKT_LEN - 2);
                local_crc16 = crc16_modbus(ota_recv_buf, OTA_PKT_LEN - 2);
                
                ota_env.proc_sta = OTA_ERROR;
                ota_env.err_info = OTA_ERR_CRC;
                
                if (local_crc16 == peer_crc16)
                {
                    uint8_t i = 0;
                    for (; i < 6; i++)
                    {
                        // First Packet Check Connect BLE Mac
                        if (ota_recv_buf[2 + i] != ble_dev_addr.addr[5 - i])
                        {
                            break;
                        }
                    }

                    if (i == 6)
                    {
                        if (ota_env.bank != FLASH_ADDR_INVALID)
                        {
                            // Erase 
                            uint32_t erase_offset  = (ota_env.bank & 0xFFFFF000UL);
                            uint32_t erase_end     = (ota_env.bank == OTA_BANK_B ? FLASH_SIZE : OTA_BANK_B);
                            for (;erase_offset < erase_end; erase_offset += FLASH_SECTOR_SIZE)
                            {
                                flash_sector_erase(erase_offset);
                            }

                            ota_env.proc_sta = OTA_START;
                            ota_env.err_info = OTA_ERR_NONE;
                            otas_txd_send(app_env.curidx, 2, ota_recv_buf);
                        }
                        else
                        {
                            ota_env.err_info = OTA_ERR_BANK_ADDR;
                            DEBUG("OTA_ERR_BANK_ADDR");
                            return;
                        }
                    }
                    else
                    {
                        ota_env.err_info = OTA_ERR_MAC;
                        DEBUG("OTA_ERR_MAC");
                        return;
                    }
                }
            }
        } break;

        case OTA_START:
        {
            ota_env.err_info       = OTA_ERR_NONE;
            ota_env.proc_sta       = OTA_BUSY;
            ota_env.local_data_idx = 0;
            ota_env.peer_data_idx  = 0xFFFF;
            ota_env.tcnt           = 0;
            ota_env.tid            = sftmr_start(OTA_TIMER_TICK, ota_time_handle);

            leds_play(LED_BUSY_BL);
        } break;

        case OTA_BUSY:
        {
            if (app_state_get() < APP_CONNECTED)
            {
                ota_env.err_info       = OTA_ERR_NONE;
                ota_env.proc_sta       = OTA_IDLE;
                ota_env.local_data_idx = 0;
                ota_env.peer_data_idx  = 0xFFFF;
                ota_env.tcnt           = 0;
                sftmr_clear(ota_env.tid);

                return;
            }
            
            if (ota_env.data_sta == OTA_DATA_OLD)
            {
                return;
            }
            
            ota_env.proc_sta = OTA_ERROR;
            ota_env.data_sta = OTA_DATA_OLD;

            //(ota_recv_buf[OTA_PKT_LEN - 1] << 8) | ota_recv_buf[OTA_PKT_LEN - 2]);
            peer_crc16 = read16p(ota_recv_buf + OTA_PKT_LEN - 2);
            
            if (ota_env.data_len == OTA_PKT_LEN)
            {
                local_crc16 = crc16_modbus(ota_recv_buf, OTA_PKT_LEN - 2);

                if (co_bswap16(read16p(ota_recv_buf)) == OTA_CMD_END)
                {
                    if (local_crc16 == peer_crc16)
                    {
                        otas_txd_send(app_env.curidx, 2, ota_recv_buf);

                        if (ota_env.local_data_idx > OTA_FW_MAX_SIZE)
                        {
                            ota_env.err_info = OTA_ERR_FW;
                        }
                        else
                        {
                            uint16_t index_check = co_bswap16(read16p(ota_recv_buf+4));//((ota_recv_buf[4] << 8) | ota_recv_buf[5]);
                            index_check = ~index_check;
                            ota_env.peer_data_idx = co_bswap16(read16p(ota_recv_buf+2));//((ota_recv_buf[2] << 8) | ota_recv_buf[3]);
                            
                            if ((index_check == ota_env.peer_data_idx) && (ota_env.local_data_idx == (ota_env.peer_data_idx + 1)))
                            {
                                // write remain data
                                if ((ota_env.local_data_idx % OTA_DATA_LEN) != 0)
                                {
                                    uint32_t wr_addr = (ota_env.peer_data_idx / OTA_DATA_LEN) * FLASH_PAGE_SIZE + ota_env.bank;
                                    DEBUG("ADDRW:0x%X,ota_app_idx:%d", wr_addr, ota_env.peer_data_idx);
                                    flash_write(wr_addr, ota_wr_data, FLASH_PAGE_SIZE_WLEN);
                                }

                                ota_env.err_info = OTA_ERR_NONE;
                                ota_env.proc_sta = OTA_END;
                                leds_play(LED_CONT_ON);
                            }
                            else
                            {
                                ota_env.err_info = OTA_ERR_INDEX;
                                DEBUG("OTA_ERR_INDEX");
                            }
                        }

                        return;
                    }
                    else
                    {
                        ota_env.err_info = OTA_ERR_CRC;
                        DEBUG("OTA_ERR_CRC");
                        return;
                    }
                }
            }
            else
            {
                ota_env.err_info = OTA_ERR_PKT_LEN;
                DEBUG("OTA_ERR_PKT_LEN");
                return;
            }
            
            ota_env.peer_data_idx = co_bswap16(read16p(ota_recv_buf));//((ota_recv_buf[0] << 8) | ota_recv_buf[1]);
            if ((ota_env.local_data_idx == ota_env.peer_data_idx) && (ota_env.proc_sta != OTA_END))
            {
                if (local_crc16 == peer_crc16)
                {
                    ota_env.proc_sta = OTA_BUSY;
                    ota_env.err_info = OTA_ERR_NONE;
                    uint8_t wr_data_pos = (ota_env.local_data_idx % OTA_DATA_LEN) * OTA_DATA_LEN;
                    memcpy(((uint8_t *)ota_wr_data + wr_data_pos), ota_recv_buf + 2, OTA_DATA_LEN);

                    uint32_t wr_addr = (ota_env.peer_data_idx / OTA_DATA_LEN) * FLASH_PAGE_SIZE + ota_env.bank;

                    if ((ota_env.local_data_idx % OTA_DATA_LEN) == (OTA_DATA_LEN - 1))
                    {
                        DEBUG("ADDRW:0x%X,peer_idx:%d", wr_addr, ota_env.peer_data_idx);
                        flash_write(wr_addr, ota_wr_data, FLASH_PAGE_SIZE_WLEN);
                    }
                }
                else
                {
                    ota_env.err_info = OTA_ERR_CRC;
                    DEBUG("CRC_ERROR");
                }

                ota_env.local_data_idx++;
            }
            else
            {
                ota_env.err_info = OTA_ERR_INDEX;
                DEBUG("INDEX_ERROR");
            }
        } break;

        case OTA_ERROR:
        {
            DEBUG("ERROR_CODE:0x%x", ota_env.err_info);
            otas_txd_send(app_env.curidx, 2, (uint8_t *)&ota_env.err_info);
            ota_env.err_info = OTA_ERR_FAIL;
            otas_txd_send(app_env.curidx, 2, (uint8_t *)&ota_env.err_info);

            NVIC_SystemReset();
        } break;

        case OTA_END:
        {
            DEBUG("OTA_success");

            ota_env.proc_sta = OTA_IDLE;
            ota_env.err_info = OTA_ERR_NONE;
            
#if (CFG_USE_LOAD)
            DEBUG("data_idx:%d", ota_env.local_data_idx);
            memset((uint8_t *)ota_wr_data, 0xFF, sizeof(ota_wr_data));
            ota_wr_data[0] = 0x55AA5AA5;
            ota_wr_data[1] = (ota_env.local_data_idx * 16);
            ota_wr_data[2] = OTA_BANK_B_BASE;

            flash_page_erase(OTA_BANK_B - FLASH_PAGE_SIZE);
            flash_write(OTA_BANK_B - FLASH_PAGE_SIZE, ota_wr_data, FLASH_PAGE_SIZE_WLEN);
#else
            flash_wr_protect(0x00);
            
            flash_read(FLASH_INFO_PAGE, ota_wr_data, FLASH_PAGE_SIZE_WLEN);
            ota_wr_data[1] = ota_env.local_data_idx * 16;
            ota_wr_data[2] = ota_env.bank | FLASH_BASE;

            flash_page_erase(FLASH_INFO_PAGE);
            flash_write(FLASH_INFO_PAGE, ota_wr_data, FLASH_PAGE_SIZE_WLEN);
            
            flash_wr_protect(0x64);
#endif
            ota_ok_led();
            
            NVIC_SystemReset();
        } break;
    }
}

bool is_ota_proc(void)
{
    return (ota_env.proc_sta != OTA_IDLE);
}
