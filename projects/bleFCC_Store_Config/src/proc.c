/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "reg_rf.h"
#include "fcc.h"
#include "uartRb.h"
#include "sftmr.h"
#include "bledef.h"
#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

#define BLE_MAX_LEN  20
#define NULL_CNT     20
#define HOPPING_INV  _MS(200)
extern bool Set_Config(uint8_t data,uint8_t offset);
extern uint8_t* get_config(void);

enum uart_cmd
{
    CMD_FCC_START         =  0xA0,
    CMD_FCC_STOP,
    
    CMD_FCC_TX_CARR       =  0xB0,
    CMD_FCC_RX_CARR,
    
    CMD_FCC_TX_MOD        =  0xC0,
    CMD_FCC_RX_MOD,
    
    CMD_FCC_TX_HOP        =  0xD0,
    
    CMD_SET_XOSC16_TR     =  0xE0,
    CMD_GET_XOSC16_TR,
    
    CMD_SET_RF_POWER      =  0xF0,
    
    CMD_SET_ADJ00_2402    =  0xF1,
    
    CMD_SET_ADJ12_2440    =  0xF2,
    
    CMD_SET_ADJ05_2480    =  0xF3,
    
    CMD_SET_ADJ_VCO       =  0xF4,
    
    CMD_SET_BG_RES_TRIM   =  0xF5,
};

static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;

volatile uint8_t g_hopping_idx, g_hopping_timer_id;

static tmr_tk_t hopping_timer(uint8_t id)
{
    // 0 ~ 39 (2402M ~ 2480M)
    g_hopping_idx = (g_hopping_idx % 40);

    fcc_tx_carr(g_hopping_idx);
    
    ++g_hopping_idx;
    
    return HOPPING_INV;
}

static void hopping_mode(void)
{
    if ((g_hopping_timer_id & 0x80) == 0)
    {
        g_hopping_timer_id = sftmr_start(HOPPING_INV, hopping_timer);
        
        g_hopping_timer_id |= 0x80;
    }
}
uint8_t crut_mode = 0;
uint8_t mode_data = 0;
/*
 * FUNCTIONS
 ****************************************************************************************
 */
/// Uart Data procedure
static void data_proc(void)
{
    // Todo Loop-Proc
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart1Rb_Read(&buff[buff_len], BLE_MAX_LEN - buff_len);
    
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < BLE_MAX_LEN)
        {
            return; // wait full
        }
    }
    else
    {
        if ((buff_len > 0) && (null_cnt++ > NULL_CNT))
        {
            //finish = true;
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }
    
    if ((buff[0] != CMD_FCC_TX_HOP) && (g_hopping_timer_id != 0))
    {
        g_hopping_timer_id &= ~0x80U;
        sftmr_clear(g_hopping_timer_id);
        g_hopping_timer_id = 0;
    }

    switch (buff[0])
    {
        case CMD_FCC_START:
        {
            DEBUG("fcc_start");
            fcc_init();
            crut_mode =0;
            mode_data =0;
        } break;
        
        case CMD_FCC_STOP:
        {
            DEBUG("fcc_stop");
            fcc_stop();
            crut_mode =0;
            mode_data =0;
        } break;
        
        case CMD_FCC_TX_CARR:
        {
            DEBUG("fcc_tx_carr");
            if (buff_len > 1)
            {
                fcc_tx_carr(buff[1]);
                crut_mode =CMD_FCC_TX_CARR;
                mode_data =buff[1];
            }
        } break;
        
        case CMD_FCC_RX_CARR:
        {
            DEBUG("fcc_rx_carr");
            if (buff_len > 1)
            {
                //fcc_rx_carr(buff[1]);
                uint8_t *p = get_config();
                fcc_rx_carr_vco(buff[1], p[VCO_ADJ_OFFSET]);
                crut_mode =CMD_FCC_RX_CARR;
                mode_data =buff[1];
            }
        } break;
        
        case CMD_FCC_TX_MOD:
        {
            DEBUG("fcc_tx_mod");
            if (buff_len > 1)
            {
                fcc_tx_mod(buff[1]);
                crut_mode =CMD_FCC_TX_MOD;
                mode_data =buff[1];
            }
        } break;
        
        case CMD_FCC_RX_MOD:
        {
            DEBUG("fcc_rx_mod");
            if (buff_len > 1)
            {
                uint8_t *p = get_config();
                //fcc_rx_mod(buff[1]);
                fcc_rx_mod_vco(buff[1],p[VCO_ADJ_OFFSET]);
                crut_mode =CMD_FCC_RX_MOD;
                mode_data =buff[1];
            }
        } break;
        
        case CMD_FCC_TX_HOP:
        {
            DEBUG("hopping_mode");
            g_hopping_idx = 0;
            hopping_mode();
            crut_mode =CMD_FCC_TX_HOP;
            mode_data =0;
        } break;
        
        case CMD_SET_XOSC16_TR:
        {
            if (buff_len > 1)
            {
                DEBUG("SET_XOSC_TR:0x%02x", buff[1]);
                if(Set_Config(buff[1],CAP_OFFSET))
                {
                    uint8_t *p = get_config();
                    flash_page_erase(CONFIG_STORE_OFFESET);
                    flash_byte_write(CONFIG_STORE_OFFESET,p,sizeof(Fcc_Config_t));
                }
                APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = buff[1];
            }
        } break;
        
        case CMD_GET_XOSC16_TR:
        {
            DEBUG("GET_XOSC_TR:0x%02x", APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR);
        } break;
        case CMD_SET_RF_POWER:
        {
            DEBUG("RF_POWER_SET:0x%02x", buff[1]);
            if(buff[1]<0x10)
            {
                if(Set_Config(buff[1],PA_OFFSET))
                {
                    uint8_t *p = get_config();
                    flash_page_erase(CONFIG_STORE_OFFESET);
                    flash_byte_write(CONFIG_STORE_OFFESET,p,sizeof(Fcc_Config_t));
                }

                rf_pa_set(buff[1]);
                
                switch(crut_mode)
                {
                    case CMD_FCC_TX_CARR:
                    {
                        fcc_tx_carr(mode_data);                            
                        
                    } break;
                    
                    case CMD_FCC_RX_CARR:
                    {
                        fcc_rx_carr(mode_data);
                        
                    } break;
                    
                    case CMD_FCC_TX_MOD:
                    {
                        fcc_tx_mod(mode_data);                       
                        
                    } break;
                    
                    case CMD_FCC_RX_MOD:
                    {
                        fcc_rx_mod(mode_data);
                    } break;
                    
                    case CMD_FCC_TX_HOP:
                    {
                        g_hopping_idx = 0;
                        hopping_mode();
                    } break;                   
                }
                    
            }
            else
            {
                DEBUG("Parameter error\r\n");
            }
            
        }break;
        case CMD_SET_ADJ00_2402:
        case CMD_SET_ADJ12_2440:
        case CMD_SET_ADJ05_2480:
        {
            DEBUG("ADJ00:0x%02x", buff[1]);
            if (buff_len > 1)
            {
                uint8_t idex =  buff[0]-CMD_SET_ADJ00_2402;
                if(Set_Config(buff[1],ADJ00_OFFSET+idex))
                {
                    if(idex==0)
                    {
                        DEBUG("ADJ00_2402:0x%02x", buff[1]);
                        RF->PLL_DAC_TAB0.PLL_DAC_ADJ00=buff[1];
                        rf_dac_tab0_set(RF->PLL_DAC_TAB0.Word);
                    }
                    else if(idex==1)
                    {
                        DEBUG("ADJ12_2440:0x%02x", buff[1]);
                        RF->PLL_DAC_TAB1.PLL_DAC_ADJ12=buff[1];
                        rf_dac_tab1_set(RF->PLL_DAC_TAB1.Word);
                    }
                    else 
                    {
                        DEBUG("ADJ05_2480:0x%02x", buff[1]);
                        RF->PLL_DAC_TAB2.PLL_DAC_ADJ05 = buff[1];
                        rf_dac_tab2_set(RF->PLL_DAC_TAB2.Word);
                    }
                    
                    uint8_t *p = get_config();
                    flash_page_erase(CONFIG_STORE_OFFESET);
                    flash_byte_write(CONFIG_STORE_OFFESET,p,sizeof(Fcc_Config_t));
                }
                
            }
        }break;
        case CMD_SET_ADJ_VCO:
        {
            DEBUG("ADJ12_VCO:0x%02x", buff[1]);
            if (buff_len > 1)
            {
                if(Set_Config(buff[1],VCO_ADJ_OFFSET))
                {
                    
                    uint8_t *p = get_config();
                    if(crut_mode == CMD_FCC_RX_MOD)
                    {
                        fcc_rx_mod_vco(mode_data,p[VCO_ADJ_OFFSET]);
                    }
                    else if(crut_mode==CMD_FCC_RX_CARR)
                    {
                        fcc_rx_carr_vco(mode_data,p[VCO_ADJ_OFFSET]);
                    }                                     
                    flash_page_erase(CONFIG_STORE_OFFESET);
                    flash_byte_write(CONFIG_STORE_OFFESET,p,sizeof(Fcc_Config_t));
                }
            }
        }break;
        case CMD_SET_BG_RES_TRIM:
        {
            
            if (buff_len > 1)
            {
                DEBUG("RF->ANA_TRIM.BG_RES_TRIM:0x%02x", buff[1]);
                if(Set_Config(buff[1],BG_RES_TRIM_OFFSET))
                {
                    RF->ANA_TRIM.BG_RES_TRIM = buff[1];
                    uint8_t *p = get_config();
                    flash_page_erase(CONFIG_STORE_OFFESET);
                    flash_byte_write(CONFIG_STORE_OFFESET,p,sizeof(Fcc_Config_t));
                }
            }

        }
        break;
        default:
        {

        } break;
    }
    
    if (finish)
    {
        buff_len = 0;
    }
}

void user_procedure(void)
{
    data_proc();
}
