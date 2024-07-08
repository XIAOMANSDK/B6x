#include "gap.h"
#include "atcmd.h"
#include "string.h"
#include "drvs.h"
#include <stdio.h>
#include "app.h"
#include "app_actv.h"
#include "gapc_api.h"
#include "gapm_api.h"
#include "att.h"
#include "uartRb.h"
#include "prf_sess.h"
#include "gatt.h"
#include "sftmr.h"
#include "regs.h"

#if (DBG_ATCMD)
#include "dbg.h"
//#define DEBUG(format, ...)    debug("<%s,%d>" format , __MODULE__, __LINE__, ##__VA_ARGS__)
#define DEBUG(format, ...)    debug(format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define BLE_DEV_VERSION "1.0.1"

#define SYS_CONFIG_ALIGNED4_WLEN ((sizeof(SYS_CONFIG) + 3) / 4)
    
extern uint8_t           scan_cnt;
extern struct gap_bdaddr scan_addr_list[];

uint8_t BLE_ADDR_DEFAULT[GAP_BD_ADDR_LEN] = BLE_ADDR;
//struct connected_result connected_list[BLE_NB_SLAVE + BLE_NB_MASTER];

bool scan_time_out  = false;
bool disconnect_all = false;

volatile uint8_t g_cfg_change;

enum cfg_chng_bits
{
    CFG_CHNG_BLE = 0x01,
    CFG_CHNG_SYS = 0x02,
    CFG_CHNG_RST = 0x04,
};

AT_CMD_FORMAT_T at_cmd_head_list[CMD_CODE_MAX] =
{
    [CMD_ECHO]      = {5,  5,  "AT+\r\n"          },
    [CMD_ALL]       = {8,  8,  "AT+ALL\r\n"       },
    [CMD_MAC_R]     = {9,  9,  "AT+MAC?\r\n"      },
    [CMD_VER_R]     = {9,  9,  "AT+VER?\r\n"      },
    [CMD_NAME_R]    = {14, 14, "AT+DEV_NAME?\r\n" },
    [CMD_NAME_S]    = {12, 34, "AT+DEV_NAME="     },
    [CMD_BAUD_R]    = {10, 10, "AT+BAUD?\r\n"     },
    [CMD_BAUD_S]    = {8,  16, "AT+BAUD="         },
    [CMD_DISCON_S]  = {13, 13, "AT+DISCON=1\r\n"  },
    [CMD_SCAN_S]    = {15, 15, "AT+SCAN_BLE=1\r\n"},
    [CMD_CON_MAC_R] = {13, 13, "AT+CON_MAC?\r\n"  },
    [CMD_CON_MAC_S] = {11, 30, "AT+CON_MAC="      },
    [CMD_UUIDS_R]   = {11, 11, "AT+UUIDS?\r\n"    },
    [CMD_UUIDS_S]   = {9,  43, "AT+UUIDS="        },
    [CMD_UUIDN_R]   = {11, 11, "AT+UUIDN?\r\n"    },
    [CMD_UUIDN_S]   = {9,  43, "AT+UUIDN="        },
    [CMD_UUIDW_R]   = {11, 11, "AT+UUIDW?\r\n"    },
    [CMD_UUIDW_S]   = {9,  43, "AT+UUIDW="        },
    [CMD_AINTVL_R]  = {12, 12, "AT+AINTVL?\r\n"   },
    [CMD_AINTVL_S]  = {10, 16, "AT+AINTVL="       },
    [CMD_AMDATA_R]  = {12, 12, "AT+AMDATA?\r\n"   },
    [CMD_AMDATA_S]  = {10, 38, "AT+AMDATA="       },
    [CMD_RENEW_S]   = {10, 10, "AT+RENEW\r\n"     },
    [CMD_RESET_S]   = {10, 10, "AT+RESET\r\n"     },
    [CMD_HELP]      = {9,  9,  "AT+HELP\r\n"      },
};

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void at_uart_baud_cfg(uint8_t port, uint32_t uart_baud)
{
    UART_TypeDef* uart = ((UART_TypeDef *)(UART1_BASE + (port) * 0x1000));
    
    uint16_t uart_baud_div = (uint16_t)((rcc_sysclk_freq() + ((uart_baud) >> 1)) / (uart_baud));

    // update BaudRate
    uart->LCR.BRWEN  = 1; 
    uart->BRR        = uart_baud_div;
    uart->LCR.BRWEN  = 0;
}

void atConfigFlashWrite(uint32_t offset, uint8_t len, const uint32_t *data)
{
    flash_page_erase(offset);
    flash_write(offset, (uint32_t *)data, len);
}

void atConfigFlashRead(void)
{
    uint32_t config_data[SYS_CONFIG_ALIGNED4_WLEN];
    flash_read(SYS_CONFIG_OFFSET, config_data, SYS_CONFIG_ALIGNED4_WLEN);
    
    g_cfg_change = 0;
    
    for (uint8_t i = 0; i < SYS_CONFIG_ALIGNED4_WLEN; i++)
    {
        if (config_data[i] != 0xFFFFFFFF)
        {
            memcpy((uint8_t *)&sys_config, (uint8_t *)&config_data, sizeof(SYS_CONFIG));

            break;
        }
    }

    at_uart_baud_cfg(UART1_PORT, sys_config.baudrate);
}

SYS_CONFIG sys_config =
{
    .baudrate = AT_DFT_UART_BAUD,
    .mac_addr = BLE_ADDR,
    .uuid_len = ATT_UUID16_LEN,
    .uuids    = {0xF0, 0xFF},
    .uuidn    = {0xF1, 0xFF},
    .uuidw    = {0xF2, 0xFF},
    .name_len = sizeof(BLE_DEV_NAME) - 1,
    .name     = BLE_DEV_NAME,

    .adv_data_len = AT_DFT_ADV_DATA_LEN,
    .adv_data = 
    {
        0x02, 0x01, 0x06,
        0x03, 0x03, 0xF0, 0xFF,
        0x03, 0x19, 0x00, 0x00
    },
    .adv_intv_time= AT_DFT_ADV_INTV,
    .rssi         = AT_DFT_RSSI,
};

// flag: All_FACTORY_REST:  全部恢复出厂设置
// flag: PAIR_FACTORY_RESET: 清除配对信息
void atSetBleDefault(PARA_SET_FACTORY flag)
{
//    uint8_t uuid16_idx = 12;

    if (flag == All_FACTORY_REST)
    {
        //        sys_config;
        memset(sys_config.adv_data, 0, sizeof(sys_config.adv_data));
        for (uint8_t idx = 0; idx < sys_config.adv_data_len; idx++)
        {
            sys_config.adv_data[idx] = idx;
        }

//        sys_config.rssi = 0;                      //  RSSI 信号值

//        sys_config.adv_intv_time = 20;    //min

//        sys_config.uuid_len = ATT_UUID16_LEN;

//        if (sys_config.uuid_len == ATT_UUID128_LEN) uuid16_idx = 0;

//        memcpy(sys_config.uuids, &ses_uuid_s[uuid16_idx], sys_config.uuid_len);
//        memcpy(sys_config.uuidn, &ses_uuid_n[uuid16_idx], sys_config.uuid_len);
//        memcpy(sys_config.uuidw, &ses_uuid_w[uuid16_idx], sys_config.uuid_len);
    }
//    else if(flag == PAIR_FACTORY_RESET)
//    {
//        //sprintf((char*)sys_config.pass, "000000");      //密码
////        memset(sys_config.mac_addr, 0, sizeof(sys_config.mac_addr));
////        memcpy(sys_config.mac_addr, BLE_ADDR_DEFAULT, GAP_BD_ADDR_LEN);
//        sys_config.ever_connect_peripheral_mac_addr_conut = 0;
//        sys_config.ever_connect_peripheral_mac_addr_index = 0;
//    }

//    GAPBondMgr_SetParameter( GAPBOND_ERASE_ALLBONDS, 0, NULL ); //清除绑定信息

    atConfigFlashWrite(SYS_CONFIG_OFFSET, sizeof(SYS_CONFIG), (uint32_t *)&sys_config);
}

#if (0)
// 字符串对比
static uint8_t strCmp(const uint8_t *p1, char *p2, uint8_t len)
{
    uint8_t i = 0;
    while (i < len)
    {
        if (p1[i] != p2[i])
        {
            return 0;
        }
        i++;
    }
    return 1;
}
#endif

// 字符串转数字
uint32_t str2Num(const uint8_t *numStr, uint8_t iLength)
{
    uint32_t rtnInt = 0;
    
    #if (__MICROLIB)
    rtnInt = strtoul((const char *)numStr, NULL, 10);
    #else
    /*
          为代码简单，在确定输入的字符串都是数字的
          情况下，此处未做检查，否则要检查
          numStr[i] - '0'是否在[0, 9]这个区间内
    */

    for (uint8_t  i = 0; (i < iLength) && (numStr[i] != '\0'); i++)
    {
        rtnInt = rtnInt * 10 + (numStr[i] - '0');
    }
    #endif

    return rtnInt;
}

__INLINE__ uint8_t co_hexstr2hex(uint8_t hex_str)
{
    return (hex_str <= '9') ? (hex_str - '0') : 
           ((hex_str <= 'F') ? (hex_str - 'A' + 10) : (hex_str - 'a' + 10));
}

void str2mac(const uint8_t *date)
{
    uint8_t *buff = (uint8_t *)date;
    uint8_t  j    = (GAP_BD_ADDR_LEN - 1);

    for (uint8_t i = 0; i < 17; i++)
    {
        buff[i] = co_hexstr2hex(buff[i]);
        
        if ((i % 3) == 1)
        {
            sys_config.connect_mac_addr[j--] = (buff[i - 1] << 4) + buff[i];
            i++; //跳过:
        }
    }
}

void hexstr2hex(uint8_t str_len, const uint8_t *hex_str, uint8_t *hex)
{
    uint8_t *buff = (uint8_t *)hex_str;

    for (uint8_t i = 0; i < str_len; i++)
    {
        buff[i] = co_hexstr2hex(buff[i]);

        // 两个字符拼成一个hex
        if (i & 0x01)
        {
            hex[i / 2] = (buff[i - 1] << 4) | buff[i];
        }
    }
}
void str2uuid(uint8_t str_len, const uint8_t *date, uint8_t *uuid)
{
    uint8_t *buff = (uint8_t *)date;

    for (uint8_t i = 0; i < str_len; i++)
    {
        buff[i] = co_hexstr2hex(buff[i]);

        // 两个字符拼成一个hex
        if (i & 0x01)
        {
            uuid[(str_len - i) / 2] = (buff[i - 1] << 4) + buff[i];
        }
    }
}

#define debugHexB(dat, len)                   \
    do                                        \
    {                                         \
        for (int i = 1; i <= len; i++)        \
        {                                     \
            DEBUG("%02X", *((dat + len) - i));\
        }                                     \
        DEBUG("\r\n");                        \
    } while (0)

void debugMAC(uint8_t *addr)
{
    for (uint8_t idx = GAP_BD_ADDR_LEN; idx > 0; idx--)
    {
        DEBUG("%02X", addr[idx - 1]);
        DEBUG("%s", idx > 1 ? ":" : "\r\n");
    }
}

// 打印所有存储的数据，方便调试代码
void printAllConfigData(void)
{
    DEBUG("Name       = %s\r\n", sys_config.name);

    DEBUG("Version    = %s\r\n", BLE_DEV_VERSION);

    DEBUG("Mac_Addr   = %02X:%02X:%02X:%02X:%02X:%02X\r\n", sys_config.mac_addr[5], sys_config.mac_addr[4],
          sys_config.mac_addr[3], sys_config.mac_addr[2], sys_config.mac_addr[1], sys_config.mac_addr[0]);

    DEBUG("[DA]+AMDATA= 0x");
    debugHexB(sys_config.adv_data, sys_config.adv_data_len);

    DEBUG("UUIDS      = 0x");
    debugHexB(sys_config.uuids, sys_config.uuid_len);

    DEBUG("UUIDN      = 0x");
    debugHexB(sys_config.uuidn, sys_config.uuid_len);

    DEBUG("UUIDW      = 0x");
    debugHexB(sys_config.uuidw, sys_config.uuid_len);

    DEBUG("Rssi       = %d\r\n", sys_config.rssi);

    DEBUG("Adv_Intv   = %d\r\n", (sys_config.adv_intv_time * 5)/8);

    DEBUG("Baudrate   = %d\r\n", sys_config.baudrate);

    DEBUG("Connected  = %d\r\n\r\n", co_ones(app_env.conbits));
}

void printATHelp(void)
{
    for (uint8_t idx = CMD_ECHO; idx < CMD_CODE_MAX; idx++)
    {
//        sprintf((char *)&str_help, "\r\n");

        DEBUG("%d:%s", idx,at_cmd_head_list[idx].str);

        if (at_cmd_head_list[idx].str_len_min != at_cmd_head_list[idx].str_len_max)
        {
            DEBUG("\r\n");
        }
    }
}

tmr_tk_t printScanMac(uint8_t id)
{
    for (uint8_t idx = 0; idx < scan_cnt; idx++)
    {
        DEBUG("[%d]", idx);
        debugMAC((uint8_t *)&scan_addr_list[idx++].addr.addr);
    }
    DEBUG("[AT]Scan end\r\n");
    app_scan_action(ACTV_STOP);

    return 0;
}

void atBleTx(const uint8_t *buff, uint8_t buff_len)
{
    if (app_env.conrole & (0x01 << app_env.curidx)) // Slaver send to Master
    {
        sess_txd_send(app_env.curidx, buff_len, buff);
    }
    else // Master write to Slaver
    {
        // sesc_rxd_write(app_env.curidx, GATT_WRITE_NO_RESPONSE, buff_len, (uint8_t *)&buff);
    }
}

bool atProc(const uint8_t *buff, uint8_t buff_len)
{
    uint8_t code_idx = CMD_NULL;
//    debug("buff_len[%d]\r\n", buff_len);

    if (memcmp(buff, "AT+", 3) != 0)
    {
        return 0;
    }

    for (uint8_t idx = CMD_NULL; idx < CMD_CODE_MAX; idx++)
    {
        if ((at_cmd_head_list[idx].str_len_min <= buff_len) && (buff_len <= at_cmd_head_list[idx].str_len_max))
        {
            if (memcmp(buff, at_cmd_head_list[idx].str, at_cmd_head_list[idx].str_len_min) == 0)
            {
                if (at_cmd_head_list[idx].str_len_min != at_cmd_head_list[idx].str_len_max)
                {
                    if ((memcmp(&buff[buff_len - 2], "\r\n", 2) == 0) || (idx == CMD_AMDATA_S))
                    {
                        code_idx = idx;
                    }
                }
                else
                {
                    code_idx = idx;
                }
            }
        }
    }

    switch (code_idx)
    {
        case CMD_NULL:
        {
            DEBUG("[AT]ERR[%d]\r\n", ERR_PROTOCOL);
        } break;
        
        case CMD_ECHO:
        {
            DEBUG("[AT]OK\r\n");
        } break;
        
        case CMD_ALL:
        {
            printAllConfigData();
        } break;
        
        case CMD_MAC_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+MAC=");
            debugMAC(sys_config.mac_addr);
        } break;
        
        case CMD_VER_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]%s\r\n", BLE_DEV_VERSION);
        } break;
        
        case CMD_NAME_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+DEV_NAME=%s\r\n", sys_config.name);
        } break;
        
        case CMD_NAME_S:
        {
            memset(sys_config.name, 0, sizeof(sys_config.name));
            sys_config.name_len = buff_len - 14;
            memcpy(sys_config.name, &buff[12], sys_config.name_len); //  "\r\n"
            DEBUG("[AT]OK %s\r\n", sys_config.name);

            g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_BLE);
        } break;

        case CMD_BAUD_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+Baudrate=%d\r\n", sys_config.baudrate);
        } break;
        
        case CMD_BAUD_S:
        {
            uint32_t baud = str2Num(&buff[8], buff_len - 10);

            if ((4800 <= baud) && (baud <= 921600))
            {
                if (sys_config.baudrate != baud)
                {
                    sys_config.baudrate = baud;
                }
                DEBUG("[AT]OK %d\r\n", sys_config.baudrate);

                g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_RST);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
            }
        } break;

        case CMD_DISCON_S:
        {
            if (app_state_get() == APP_CONNECTED)
            {
                DEBUG("[AT]OK\r\n");
                disconnect_all = true;
                gapc_disconnect(app_env.curidx);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_OPERATION);
            }
        } break;
        
        case CMD_SCAN_S:
            // 开始扫描
            if (app_state_get() >= APP_IDLE)
            {
                DEBUG("[AT]OK\r\n");
                DEBUG("[AT]Scanning\r\n");

                app_scan_action(ACTV_START);
                
                #if ((LED_PLAY) || (CFG_SFT_TMR))
                sftmr_start(500, printScanMac);
                #endif
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_OPERATION);
            } break;

        case CMD_CON_MAC_R:
        {
            DEBUG("[AT]OK\r\n");

            uint8_t con_num_bit = app_env.conbits;
            if (con_num_bit == 0)
            {
                DEBUG("No Connected\r\n");
                break;
            }
            
            uint8_t con_num = 0;

            while (con_num_bit)
            {
                DEBUG("[DA]Connected=");

                if (con_num_bit & 0x01)
                {
                    struct gap_bdaddr* peer_bdaddr = gapc_get_bdaddr(con_num, GAPC_SMP_INFO_PEER);
//                    memcpy(&connected_list[con_num].paddr, peer_bdaddr, sizeof(struct gap_bdaddr));
//                    debugMAC(connected_list[con_num].paddr.addr.addr);
                    debugMAC(peer_bdaddr->addr.addr);
                }

                con_num++;
                con_num_bit >>= 1;
            }
        } break;

        case CMD_CON_MAC_S:
        {
//            app_init_action(ACTV_STOP); // 20211101
            str2mac(&buff[11]);

            struct gap_bdaddr peer;

            memcpy(peer.addr.addr, sys_config.connect_mac_addr, GAP_BD_ADDR_LEN);

            peer.addr_type = (peer.addr.addr[GAP_BD_ADDR_LEN-1] < 0xC0 ? 0 : 1);

            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]Connecting\r\n");
            DEBUG("[DA]Connected=");

            debugMAC(sys_config.connect_mac_addr);

            app_start_initiating(&peer);
        } break;

        case CMD_UUIDS_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+UUIDS=0x");
            debugHexB(sys_config.uuids, sys_config.uuid_len);
        } break;
        
        case CMD_UUIDS_S:
        {
            uint8_t id_len = (buff_len - 11) / 2;

            if ((id_len == ATT_UUID16_LEN) || (id_len == ATT_UUID128_LEN))
            {
                sys_config.uuid_len = id_len;
                str2uuid(2 * id_len, &buff[9], sys_config.uuids);

                DEBUG("[AT]OK\r\n");
                g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_BLE);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
            }
        } break;
        
        case CMD_UUIDN_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+UUIDN=0x");
            debugHexB(sys_config.uuidn, sys_config.uuid_len);
        } break;
        
        case CMD_UUIDN_S:
        {
            uint8_t id_len = (buff_len - 11) / 2;

            if (id_len == sys_config.uuid_len)
            {
                str2uuid(2 * id_len, &buff[9], sys_config.uuidn);
                DEBUG("[AT]OK\r\n");
                
                g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_BLE);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
            }
        } break;
        
        case CMD_UUIDW_R:
        {
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+UUIDW=0x");
            debugHexB(sys_config.uuidw, sys_config.uuid_len);
        } break;
        
        case CMD_UUIDW_S:
        {
            uint8_t id_len = (buff_len - 11) / 2;

            if (id_len == sys_config.uuid_len)
            {
                str2uuid(2 * id_len, &buff[9], sys_config.uuidw);
                DEBUG("[AT]OK\r\n");
                
                g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_BLE);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
            }
        } break;
        
        case CMD_AINTVL_R:
        {
            // adv intv unit 0.625
            uint16_t adv_intv_val = (uint16_t)(sys_config.adv_intv_time * 5)/8;
            DEBUG("[AT]OK\r\n");
            DEBUG("[DA]+AINTVL=%d\r\n", adv_intv_val);
        } break;
        
        case CMD_AINTVL_S:
        {
            uint16_t value = str2Num(&buff[10], buff_len - 12);
            DEBUG("value:%d\r\n", value);
            // adv intv unit 0.625
            value = (value * 8 / 5);
            if ((0x20 <= value) && (value <= 5000))
            {
                sys_config.adv_intv_time = value;
                DEBUG("[AT]OK\r\n");
                
                g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_BLE);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
            }
        } break;
        
        case CMD_AMDATA_R:
        {
            DEBUG("[DA]+AMDATA=");
            debugHex(sys_config.adv_data, sys_config.adv_data_len);
        } break;
        
        case CMD_AMDATA_S:
        {
            uint8_t len = buff_len - 12;

            if (len <= 30)
            {
                #if (1)
                if (len & 0x01)
                {
                    DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
                    break;
                }
                sys_config.adv_data_len = len/2;
                hexstr2hex(len, (buff + 10), sys_config.adv_data);
                
                #else
                memcpy(sys_config.adv_data, &buff[10], sys_config.adv_data_len);
                #endif
                DEBUG("[AT]OK\r\n");
                
                g_cfg_change = (CFG_CHNG_SYS | CFG_CHNG_BLE);
            }
            else
            {
                DEBUG("[AT]ERR[%d]\r\n", ERR_INVALID);
            }
        } break;
        
        case CMD_RENEW_S:
        {
//            atSetBleDefault(All_FACTORY_REST);
            flash_page_erase(SYS_CONFIG_OFFSET);
        };// break; // no break, used CMD_RESET_S break
        
        case CMD_RESET_S:
        {
            DEBUG("[AT]OK\r\n");
            g_cfg_change = CFG_CHNG_RST;// 直接重启即可
        } break;

        case CMD_HELP:
            printATHelp();
            break;

        default:
            DEBUG("[AT]ERR[%d]\r\n", ERR_PROTOCOL);
            break;
    }
    
//    DEBUG("cfg:%X\r\n", g_cfg_change);
    
    if (g_cfg_change & CFG_CHNG_BLE)
    {
        gapm_reset(); 
    }
    
    if (g_cfg_change & CFG_CHNG_SYS)
    {
        atConfigFlashWrite(SYS_CONFIG_OFFSET, SYS_CONFIG_ALIGNED4_WLEN, (uint32_t *)&sys_config);
    }
    
    if (g_cfg_change & CFG_CHNG_RST)
    {
        #if ((LED_PLAY) || (CFG_SFT_TMR))
        sftmr_wait(6); // 设置参数后，适当延时,以便上一次发送的数据正常发送出去
        #endif
        NVIC_SystemReset();
    }
    
    return (code_idx ? 1 : 0);
}
