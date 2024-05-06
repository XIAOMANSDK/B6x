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
#include "prf_sess.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "uartRb.h"

#include "proto.h"
#include "cmd.h"
#include "regs.h"

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
/// Characteristic Base UUID128 (User Customize)
#define SES_ATT_UUID128(uuid)     { 0x16, 0x0A, 0x10, 0x40, 0xD1, 0x9F, 0x4C, 0x6C, \
                                    0xB4, 0x55, 0xE3, 0xF7, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00 }

__attribute__((aligned (4))) 
SYS_CONFIG sysCfg = 
{
    .baudrate       = 115200,
    .tx_power       = TX_0DB_P,
    .stauts.BLE_DIS = ADV_ENBLE,
    .name_info.len  = sizeof(BLE_DEV_NAME),
    .name_info.name = BLE_DEV_NAME,
    .addrL          = BLE_ADDR,
    .uuid_len       = ATT_UUID128_LEN,
    .uuids          = SES_ATT_UUID128(0x0001),
    .uuidw          = SES_ATT_UUID128(0x0002),
    .uuidn          = SES_ATT_UUID128(0x0003),
    .vertion        = FIRM_VERTION,
    .wake_info.iox  = PA10,
}; 


/*
 * FUNCTIONS
 ****************************************************************************************
 */
/// Override - Callback on received data from peer device
void sess_cb_rxd1(uint16_t handle, uint16_t len, const uint8_t *data)
{
    pt_rsp_le_data_rep(handle, len, data);
}


void syscfgInit(void)
{    
    uint32_t trim_data[64];         
    flash_read(ADDR_OFFSET_CFG, trim_data, 64); 
    
    if (trim_data[0] != 0xFFFFFFFF)
    {
        memcpy((uint8_t *)&sysCfg, (uint8_t *)trim_data, sizeof(sysCfg));
    }
}

/// Uart Data procedure
void uart_proc(struct pt_pkt *pkt, uint8_t status)
{
    // Todo Loop-Proc
    bool bleReset = true;
    bool cfgSave  = true;
    
    if (status != PT_OK)
    {
        //pt_send_rsp(pkt); // debug
        pt_rsp_cmd_res(pkt->code, status, pkt->len, pkt->payl);

        return;
    }
    
    switch (pkt->code)
    {
        case PT_CMD_SET_BLE_ADDR:
        { 
            memcpy(sysCfg.addrL.addr,  pkt->payl, PLEN_CMD_SET_BLE_ADDR);
            
            pt_rsp_ok(pkt->code);
        } break; 

        case PT_CMD_SET_VISIBILITY:
        { 
            sysCfg.stauts.value = pkt->payl[0] & 0x07;
            pt_rsp_ok(pkt->code);
        } break;
        case PT_CMD_SET_BLE_NAME:
        { 
            sysCfg.name_info.len = pkt->len;
            memcpy(sysCfg.name_info.name,  pkt->payl, pkt->len);
            pt_rsp_ok(pkt->code);
        } break;
        case PT_CMD_SEND_BLE_DATA:
        {  
            bleReset = false;
            cfgSave  = false;
            
            if (app_state_get() == APP_CONNECTED)
            {
                uint16_t handle = pkt->payl[0] | ((uint16_t)pkt->payl[1] << 8);
                
//                if (sess_txd_send(app_env.curidx, (pkt->len - 2), &pkt->payl[2]) == LE_SUCCESS)
                if (sess_txd_send1(handle, (pkt->len - 2), &pkt->payl[2]) == LE_SUCCESS)
                    pt_rsp_ok(pkt->code);
            }          
        } break; 

        case PT_CMD_STATUS_REQUEST:
        {  
            bleReset = false;
            cfgSave  = false;
            
            pt_rsp_status_res(sysCfg.stauts.value);
        } break; 

        case PT_CMD_SET_UART_FLOW:
        { 
            bleReset = false;
            
            if (pkt->payl[0]) // ¿ªÆô UART Á÷¿Ø
                uart_hwfc(UART1_PORT, PA_UART1_RTS, PA_UART1_CTS);
            else // ¹Ø±Õ UART Á÷¿Ø bit[3:2] (.AFCEN=1, .RTSCTRL=1)                
                UART1->MCR.Word &= ~(0x0000000C);
            
            pt_rsp_ok(pkt->code);
        } break; 

        case PT_CMD_SET_UART_BAUD:
        { 
            bleReset = false;

            sysCfg.baudrate = atoi((char *)&pkt->payl);
            // update BaudRate
            UART1->LCR.BRWEN  = 1; 
            UART1->BRR        = BRR_DIV(sysCfg.baudrate, 16M); // (rcc_sysclk_get() + (baud >> 1)) / baud
            UART1->LCR.BRWEN  = 0;
            
            pt_rsp_ok(pkt->code);
        } break; 

        case PT_CMD_VERSION_REQUEST:
        {
            bleReset = false;
            cfgSave  = false;
            
            pt_rsp_cmd_res(pkt->code, PT_OK, PLEN_RSP_VERTION, (uint8_t *)&sysCfg.vertion);            
        } break;
        case PT_CMD_BLE_DISCONNECT:
        {   
            bleReset = false; 
            cfgSave  = false;
            
            gapc_disconnect(app_env.curidx);
            pt_rsp_ok(pkt->code);
        } break;
        case PT_CMD_CONFIRM_GKEY:
        { 
            bleReset = false;
            cfgSave  = false;
            
            if (pkt->payl[0]) // ÃÜÔ¿²»Æ¥Åä
            {}
            else // ÃÜÔ¿Æ¥Åä
            {}
            pt_rsp_ok(pkt->code);      
        } break;

        case PT_CMD_SET_ADV_DATA:
        {
            uint8_t lenA = (31 < pkt->len) ? 31 : pkt->len;
            uint8_t lenS = pkt->len - lenA;
            
            sysCfg.adv_info.lenADV = lenA;          
            memcpy(sysCfg.adv_info.adv, pkt->payl, lenA);
            
            if (lenS)
            {
                sysCfg.adv_info.lenSCAN = lenS;
                memcpy(sysCfg.adv_info.scan, &pkt->payl[lenA], lenS); 
            }
            
            pt_rsp_ok(pkt->code);            
        } break;

        case PT_CMD_POWER_REQ:
        { 
            bleReset = false;
            cfgSave  = false;
            
            if (SADC->SADC_ANA_CTRL.SADC_EN)
            {
                uint32_t value;
                uint8_t adc_data[2];
                value = (3300UL*sadc_read(SADC_CH_AIN0, 0))/0x3FF;
               
                adc_data[0] = value/1000;
                adc_data[1] = value%1000/10;            
                pt_rsp_cmd_res(pkt->code, PT_OK, PLEN_RSP_VERTION, adc_data);            
            }
            else
            {
                pt_rsp_cmd_res(pkt->code, PT_ERROR, 0, NULL);
            }

        } break;

        case PT_CMD_POWER_SET:
        { 
            bleReset = false;
            if (pkt->payl[0])  // ¿ªÆôµçÑ¹¼ì²â 
            {
                // Analog Enable
                GPIO_DIR_CLR(1UL << PA_POWER_ADC);
                // ADC
                iom_ctrl(PA_POWER_ADC, IOM_ANALOG);           
                // sadc init
                sadc_init(SADC_ANA_VREF_VDD);

                sadc_conf(SADC_CR_DFLT);
            }
            else  // ¹Ø±ÕµçÑ¹¼ì²â
            {
                 SADC->SADC_ANA_CTRL.SADC_EN = 0;   
            }
                    
            pt_rsp_ok(pkt->code);
        } break;

        case PT_CMD_PASSKEY_ENTRY:
        { 
            bleReset = false;
            cfgSave  = false;            
        } break;

        case PT_CMD_SET_GPIO:
        { 
            bleReset = false;
            
            uint8_t IO_MODE = pkt->payl[0];
            uint8_t IO_x    = pkt->payl[1];
            uint8_t IO_LHUD = pkt->payl[2];
            
            if (IO_x < 2)
            {
                //PA00 PA01
                iospc_swdpin();
            }
            else if (IO_x == 19)
            {
                //PA19
                iospc_rstpin(true);            
            }
            
            if (IO_MODE)  // Êä³ö
            {
                IO_LHUD == 0 ? GPIO_DAT_CLR(BIT(IO_x)) : GPIO_DAT_SET(BIT(IO_x));
                GPIO_DIR_SET(BIT(IO_x));
            }
            else // ÊäÈë
            {
                GPIO_DIR_CLR(BIT(IO_x));
                IO_LHUD == 0 ? (IO_LHUD = IOM_PULLUP) : (IO_LHUD = IOM_PULLDOWN); 
                iom_ctrl(IO_x, IOM_INPUT | IO_LHUD);
            }
            
            pt_rsp_ok(pkt->code);
        } break;

        case PT_CMD_READ_GPIO:
        { 
            bleReset = false;
            cfgSave  = false;
            
            gpio_info_t io_info;
            io_info.iox = 0; //pkt->payl[0]; //GPIOx
            
            if (BIT(pkt->payl[0]) & GPIO_PIN_GET())
                io_info.level = OE_HIGH;
            else
                io_info.level = OE_LOW;
            
            pt_rsp_cmd_res(pkt->code, PT_OK, PLEN_RSP_GPIO, (uint8_t *)&io_info);                    
        } break;

        case PT_CMD_LE_SET_PAIRING:
        { 
            bleReset = false;
            
            // enum gap_auth
            sysCfg.pair_info.auth = pkt->payl[0];
            
            pt_rsp_ok(pkt->code);
        } break;

        case PT_CMD_LE_SET_ADV_DATA:
        { 
            sysCfg.adv_info.lenADV = pkt->len;
            memcpy(sysCfg.adv_info.adv, pkt->payl, pkt->len);
            
            pt_rsp_ok(pkt->code);            
        } break;

        case PT_CMD_LE_SET_SCAN_DATA:
        { 
            sysCfg.adv_info.lenSCAN = pkt->len;
            memcpy(sysCfg.adv_info.scan, pkt->payl, pkt->len);
            
            pt_rsp_ok(pkt->code);             
        } break;

        case PT_CMD_LE_SEND_CONN_UPDATE_REQ:
        { 
            bleReset = false;
            cfgSave  = false;
            
            sysCfg.conn_info.intervalMIN = (pkt->payl[0] | (pkt->payl[1] << 8));
            sysCfg.conn_info.intervalMAX = (pkt->payl[2] | (pkt->payl[3] << 8));
            sysCfg.conn_info.latency = (pkt->payl[4] | (pkt->payl[5] << 8));
            sysCfg.conn_info.timeout = (pkt->payl[6] | (pkt->payl[7] << 8));
            
            pt_rsp_ok(pkt->code);
        } break;

        case PT_CMD_LE_SET_ADV_PARM:
        { 
            sysCfg.advr = (pkt->payl[0] | (pkt->payl[1] << 8));
            pt_rsp_ok(pkt->code);
        } break;

        case PT_CMD_LE_START_PAIRING:
        { 
            cfgSave  = false;
            pt_rsp_ok(pkt->code);
        } break;

        case PT_CMD_SET_WAKE_GPIO:
        { 
            bleReset = false;
            sysCfg.wake_info.iox = pkt->payl[0] & 0x1F;
            sysCfg.wake_info.level = pkt->payl[0] & 0x80;
            sysCfg.wake_info.delay_us = RD_32(&pkt->payl[1]);
            
            //
            GPIO_DIR_SET(1UL << sysCfg.wake_info.iox);
            
            if (sysCfg.wake_info.level)
            {
                GPIO_DAT_SET(1UL << sysCfg.wake_info.iox);
            }
            else
            {
                GPIO_DAT_CLR(1UL << sysCfg.wake_info.iox);
            }
            // system 16M
            btmr_delay(16, sysCfg.wake_info.delay_us);
    
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_SET_TX_POWER:
        { 
            sysCfg.tx_power = pkt->payl[0];
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_LE_CONFIRM_GKEY:
        { 
            bleReset = false;
            cfgSave  = false;
            
            if (pkt->payl[0]) // ÃÜÔ¿²»Æ¥Åä
            {
            
            }
            else // ÃÜÔ¿Æ¥Åä
            {
            
            }
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_REJECT_JUSTWORK:
        { 
            cfgSave  = false;
            
            if (pkt->payl[0]) // ´ò¿ª¾Ü¾ø justwork
            {
            
            }
            else // ¹Ø±Õ¾Ü¾ø justwork
            {
            
            }
            pt_rsp_ok(pkt->code);            
        } break;
     
        case PT_CMD_RESET_CHIP_REQ:
        { 
            bleReset = false;
            cfgSave  = false;
            
            NVIC_SystemReset();       
        } break;
     
        case PT_CMD_LE_SET_FIXED_PASSKEY:
        { 
            
        } break;
     
        case PT_CMD_DELETE_CUSTOMIZE_SERVICE:
        { 
            cfgSave  = false;
            sess_inx_nmb = 8;
            sess_uuid_nmb = 4;
            sess_read_nmb = 1;
            ses_perm_nmb = 3;
            
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_ADD_SERVICE_UUID:
        { 
            cfgSave  = false;
            
            sess_uuid[sess_uuid_nmb].uuid[12] = pkt->payl[0];
            sess_uuid[sess_uuid_nmb++].uuid[13] = pkt->payl[1];
            
            SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_SVC;
            
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_ADD_CHARACTERISTIC_UUID:
        { 
            cfgSave  = false;
            uint8_t att_val =  pkt->payl[0];
            sess_uuid[sess_uuid_nmb].uuid[12] = pkt->payl[1];
            sess_uuid[sess_uuid_nmb++].uuid[13] = pkt->payl[2];
            
            pt_rsp_uuid_handle(sess_env[0].start_hdl + sess_inx_nmb + 1);
            
            ses_atts_perm[ses_perm_nmb++] = ((uint16_t)att_val << 8);
            
            if (att_val & 0x03)
            {
                // Broadcast & Read
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_READ_CHAR;
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_READ_VAL;
                
                ses_read_info[sess_read_nmb].length = pkt->payl[3];
                memcpy(ses_read_info[sess_read_nmb++].data, &pkt->payl[4], pkt->payl[3]);
            }
            else if (att_val & 0x0C)
            {
                // WriteWithoutResponse & Write 
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_RXD_CHAR;
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_RXD_VAL;               
            }
            else if (att_val & 0x30)
            {
                // Notify & Indicate  
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_TXD_CHAR;
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_TXD_VAL;
                SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_TXD_NTF_CFG;                
            }
            else if (att_val & 0xC0)
            {
                // AuthenticatedSignedWrites & ExtendedProperties  

            }

        } break;
     
        case PT_TEST_CMD_CLOSE_LPM:
        { 
            cfgSave  = false;
        } break;
     
        default :
        { 
            cfgSave  = false;
        } break; 
    }
    
    if (bleReset)
    gapm_reset();
 
    if (cfgSave)
    {       
        flash_page_erase(ADDR_OFFSET_CFG);
        flash_write(ADDR_OFFSET_CFG, (uint32_t *)&sysCfg, 64);
    }
}

#if (CFG_SLEEP)
static void sleep_proc(void)
{
    uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);

    if (lpsta == BLE_IN_SLEEP)
    {
        uint16_t lpret = core_sleep(CFG_WKUP_BLE_EN);
        //DEBUG("ble sta:%d, wksrc:%X", lpsta, lpret);
    }
    else
    {
        //DEBUG("ble sta:%d", lpsta);
    }
}
#endif //(CFG_SLEEP)

uint8_t app_name_get(uint8_t size, uint8_t *name)
{
    uint8_t len = sysCfg.name_info.len;

    memcpy(name, sysCfg.name_info.name, len);

    return len;
}

void app_conf_fsm(uint8_t evt)
{
    if (evt == BLE_RESET)
    {
        memset(&app_env, 0, sizeof(app_env));
        
        // Set device config
        gapm_set_dev(&ble_dev_config, &sysCfg.addrL, NULL);
    }
    else /*if (evt == BLE_CONFIGURED)*/
    {
        #if (CFG_SLEEP)
        // Set Sleep duration

        #if (RC32K_CALIB_PERIOD)
        ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);
        #endif //(RC32K_CALIB_PERIOD)
        #endif //(CFG_SLEEP)

        app_state_set(APP_IDLE);

        // Create Profiles
        app_prf_create();

        // Create Activities
        if (sysCfg.stauts.BLE_DIS)
        {
            app_actv_create();        
        }

    }
}

void app_conn_fsm(uint8_t evt, uint8_t conidx, const void* param)
{
    switch (evt)
    {
        case BLE_CONNECTED:
        {
            // Connected state, record Index
            app_env.curidx = conidx;
            app_state_set(APP_CONNECTED);
            
            sysCfg.stauts.BLE_COND = 1;
            pt_rsp_le_conn_rep();
            
            gapc_connect_rsp(conidx, GAP_AUTH_REQ_NO_MITM_NO_BOND);

            // Enable profiles by role
        } break;
        
        case BLE_DISCONNECTED:
        {
            app_state_set(APP_READY);
            
            sysCfg.stauts.BLE_COND = 0;
            pt_rsp_le_dis_rep();
            
            #if (BLE_EN_ADV)
            // Slave role, Restart Advertising
            app_adv_action(ACTV_START);
            #endif //(BLE_EN_ADV)
        } break;
        
        case BLE_BONDED:
        {
            // todo, eg. save the generated slave's LTK to flash
        } break;
        
        case BLE_ENCRYPTED:
        {
            // todo
        } break;
        
        default:
            break;
    }
}

void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif //(CFG_SLEEP)

    proto_schedule();
}
