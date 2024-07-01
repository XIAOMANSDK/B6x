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
#include "regs.h"
#include "weChat.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


uint16_t ble_head_nSeq = 1;
/*
 * DEFINES
 ****************************************************************************************
 */
/// Characteristic Base UUID128 (User Customize)
#define SES_ATT_UUID128(uuid)     { 0x16, 0x0A, 0x10, 0x40, 0xD1, 0x9F, 0x4C, 0x6C, \
                                    0xB4, 0x55, 0xE3, 0xF7, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00 }

#define SADC_ANA_DFLT_V33      (SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                                | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) | SADC_EN_BIT) // 136DC 
                                    
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


struct rxd_buffer ble_rxd;

uint16_t ble_rxd_size(void)    
{
    return ((ble_rxd.head + RXD_BUFF_SIZE - ble_rxd.tail) % RXD_BUFF_SIZE);
}

uint16_t ble_rxd_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = ble_rxd.head;
    uint16_t tail = ble_rxd.tail;
    uint16_t tlen, len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }

    if (len > max)
        len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&ble_rxd.data[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&ble_rxd.data[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&ble_rxd.data[0], len - tlen); // head_len
    }
    ble_rxd.tail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/// Override - Callback on received data from peer device
void sess_cb_rxd1(uint16_t handle, uint16_t len, const uint8_t *data)
{
    if (handle == sess_get_att_handle(SES_IDX_RXD_VAL)) // WECHAT
    {
        if (ble_rxd.head + len <= RXD_BUFF_SIZE)
        {
            memcpy(&ble_rxd.data[ble_rxd.head], data, len);
        }
        else
        {
            uint16_t lenEnd = RXD_BUFF_SIZE - ble_rxd.head;
            memcpy(&ble_rxd.data[ble_rxd.head], data, lenEnd);
            memcpy(&ble_rxd.data[0], &data[lenEnd], len - lenEnd);
        }

        ble_rxd.head = (ble_rxd.head +len) % RXD_BUFF_SIZE;    
    }
    else
    {
        pt_rsp_le_data_rep(handle, len, data);
    }
}

uint8_t bleSendBuff[BLE_DATA_LEN_MAX] = {0xFE, 0x01, 0x00, 0x0F,0x27, 0x12, 0x00, 0x03,    0x0A, 0x00, 0x12,  0x01,  0x56,  0x18,0x00};
uint16_t bleSendLen = 0;
uint16_t bleSendCnt = 0;
uint16_t bleSendHandle;

void bleDataSet(uint8_t len, uint8_t *data) // ble send
{
    bleSendLen = len; // 长度
    memcpy(bleSendBuff, data, len);
}

void bleEtcDataSet(uint8_t len, uint8_t *data) // ETC send
{
    pkt_wc *pkt = (pkt_wc *)bleSendBuff;
    if(!(len < 128)) bleSendLen++;
    
    bleSendLen = len + 15; // 微信协议包总长度
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(bleSendLen); 
    pkt->head.nLength[1]      = __SWP16_L(bleSendLen);     
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_sendData);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_sendData);    
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq);
    
    pkt->payl[3] = len; // ETC数据长度
    
    if(len < 128)
    {
        memcpy(&pkt->payl[4], data, len); // ETC数据
    }else{
        pkt->payl[4] = 0x01;
        memcpy(&pkt->payl[5], data, len); // ETC数据
    }
    
    // ETC小程序 固定结尾
    bleSendBuff[bleSendLen - 3] = 0x18;
    bleSendBuff[bleSendLen - 2] = 0x91;
    bleSendBuff[bleSendLen - 1] = 0x4E;
    
    ble_head_nSeq++;
}

void ble_data_send(void) // ble 分包发送
{
    if (bleSendLen)
    {
        uint16_t send_mtu = gatt_get_mtu(0) - 3;
        uint16_t sendLen = send_mtu > bleSendLen ? bleSendLen: send_mtu;
        
        if (sess_txd_send1(bleSendHandle, sendLen, &bleSendBuff[bleSendCnt]) == LE_SUCCESS)
        {
            bleSendLen -= sendLen; 
            bleSendCnt += sendLen;
            
            if (bleSendLen == 0)
            {
                bleSendCnt = 0;
                pt_rsp_ok(PT_CMD_SEND_BLE_DATA);
            }
        }
    }
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
                bleSendHandle = pkt->payl[0] | ((uint16_t)pkt->payl[1] << 8);
                
                DEBUG("handle 0x%x" , sess_get_att_handle(SES_IDX_TXD_VAL));
                
                if (bleSendHandle == sess_get_att_handle(SES_IDX_TXD_VAL))  // 0xFEC8
                {
                    // Wechat ETC
                    bleEtcDataSet((pkt->len - 2), &pkt->payl[2]);
                }
                else
                {
                    bleDataSet((pkt->len - 2), &pkt->payl[2]);              
                }
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
            
            if (pkt->payl[0]) // 开启 UART 流控
                uart_hwfc(UART1_PORT, PA_UART1_RTS, PA_UART1_CTS);
            else // 关闭 UART 流控 bit[3:2] (.AFCEN=1, .RTSCTRL=1)                
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
            
            if (pkt->payl[0]) // 密钥不匹配
            {}
            else // 密钥匹配
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
            if (pkt->payl[0])  // 开启电压检测 
            {
                // Analog Enable
                GPIO_DIR_CLR(1UL << PA_POWER_ADC);
                // ADC
                iom_ctrl(PA_POWER_ADC, IOM_ANALOG);           
                // sadc init
                sadc_init(SADC_ANA_DFLT_V33);  

                sadc_conf(SADC_CR_DFLT);                
            }
            else  // 关闭电压检测
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
            
            if (IO_MODE)  // 输出
            {
                IO_LHUD == 0 ? GPIO_DAT_CLR(BIT(IO_x)) : GPIO_DAT_SET(BIT(IO_x));
                GPIO_DIR_SET(BIT(IO_x));
            }
            else // 输入
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
            
            if (pkt->payl[0]) // 密钥不匹配
            {
            
            }
            else // 密钥匹配
            {
            
            }
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_REJECT_JUSTWORK:
        { 
            cfgSave  = false;
            
            if (pkt->payl[0]) // 打开拒绝 justwork
            {
            
            }
            else // 关闭拒绝 justwork
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
            
            #if (SES_UUID_128)
                sess_uuid[sess_uuid_nmb].uuid[12] = pkt->payl[0];
                sess_uuid[sess_uuid_nmb++].uuid[13] = pkt->payl[1];
            #else
                sess_uuid[sess_uuid_nmb].uuid[0] = pkt->payl[0];
                sess_uuid[sess_uuid_nmb++].uuid[1] = pkt->payl[1];
            #endif
            
            SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_SVC;
            
            pt_rsp_ok(pkt->code);
        } break;
     
        case PT_CMD_ADD_CHARACTERISTIC_UUID:
        { 
            cfgSave  = false;
            uint8_t att_val =  pkt->payl[0];
            
            #if (SES_UUID_128)
                sess_uuid[sess_uuid_nmb].uuid[12] = pkt->payl[0];
                sess_uuid[sess_uuid_nmb++].uuid[13] = pkt->payl[1];
            #else
                sess_uuid[sess_uuid_nmb].uuid[0] = pkt->payl[0];
                sess_uuid[sess_uuid_nmb++].uuid[1] = pkt->payl[1];
            #endif
            
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
            
            gatt_exmtu(app_env.curidx, 256);
            
            sysCfg.stauts.BLE_COND = 1;
            pt_rsp_le_conn_rep();
            
            gapc_connect_rsp(conidx, GAP_AUTH_REQ_NO_MITM_NO_BOND);
            // Enable profiles by role
        } break;
        
        case BLE_DISCONNECTED:
        {
            app_state_set(APP_READY);
            ble_head_nSeq = 1;
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

extern const bd_addr_t ble_dev_addr;

void ble_req_auth(void)
{
    uint8_t buff[BLE_REQ_AUTH_LEN] = {0xFE, 0x01, 0x00, 0x1A,0x27, 0x11, 0x00, 0x01,    0x0A, 0x00, 0x18, 0x84, 0x80, 0x04, 0x20, 0x01, 0x28, 0x02, 0x3A, 0x06}; 
    pkt_wc *pkt = (pkt_wc *)buff;
        
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(BLE_REQ_AUTH_LEN); 
    pkt->head.nLength[1]      = __SWP16_L(BLE_REQ_AUTH_LEN); 
        
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_auth);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_auth); 
        
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq); 
        
    ble_head_nSeq++;
    
//    memcpy(&pkt->payl[12], ble_dev_addr_B.addr ,GAP_BD_ADDR_LEN);
    for (uint8_t idx = 0; idx < GAP_BD_ADDR_LEN; idx++)
    {
        pkt->payl[12 + idx] = ble_dev_addr.addr[(GAP_BD_ADDR_LEN - 1) - idx];
    }
     
    sess_txd_send1(sess_get_att_handle(SES_IDX_TXD_VAL), 20, buff);
    sess_txd_send1(sess_get_att_handle(SES_IDX_TXD_VAL), 6, &buff[20]);
//    bleDataSet(BLE_REQ_AUTH_LEN, buff);
}

void ble_req_init(void)
{
    uint8_t buff[BLE_REQ_INIT_LEN] = {0xFE, 0x01, 0x00, 0x0A,0x27, 0x13, 0x00, 0x02,    0x0A, 0x00};
    pkt_wc *pkt = (pkt_wc *)buff;
        
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(BLE_REQ_INIT_LEN); 
    pkt->head.nLength[1]      = __SWP16_L(BLE_REQ_INIT_LEN);     
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_init);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_init);    
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq);
    ble_head_nSeq++;
    
    sess_txd_send1(sess_get_att_handle(SES_IDX_TXD_VAL), BLE_REQ_INIT_LEN, buff);
}

void ble_req_data(void)
{
    uint8_t buff[BLE_REQ_DATA_LEN] = {0xFE, 0x01, 0x00, 0x0F,0x27, 0x12, 0x00, 0x03,    0x0A, 0x00, 0x12,  0x01,  0x56,  0x18,0x00};
    pkt_wc *pkt = (pkt_wc *)buff;
        
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(BLE_REQ_DATA_LEN); 
    pkt->head.nLength[1]      = __SWP16_L(BLE_REQ_DATA_LEN);     
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_sendData);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_sendData);    
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq);
    ble_head_nSeq++;
    
    sess_txd_send1(sess_get_att_handle(SES_IDX_TXD_VAL), BLE_REQ_DATA_LEN, buff);
}

uint8_t buffR[BLE_DATA_LEN_MAX]; 

void ble_wc_sch(void)
{
     if (ble_rxd.head != ble_rxd.tail)
     {
        if (ble_rxd.data[ble_rxd.tail] == FIX_HEAD_MAGIC)
        {
            uint16_t length = ble_rxd_size();
            uint16_t pkt_len = (((uint16_t)ble_rxd.data[(ble_rxd.tail+2)%RXD_BUFF_SIZE] << 8) | ble_rxd.data[(ble_rxd.tail+3)%RXD_BUFF_SIZE]);
            
            if (length >= pkt_len)
            {                             
              ble_rxd_read(buffR, pkt_len);
                
              ble_parser_rsp((struct pt_pkt_wc *)buffR, ECI_none);
            }
            //Timeout
        }
        else
        {
            ble_rxd.tail++;
        }
     }         
     
     ble_data_send();
}

#if (ETC_TEST)
uint8_t test_etc_cnt = 0;
const uint8_t ETC_DATA[15][95] = 
{
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x01,0x80,0x17,0xBA,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x81},
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x02,0x80,0x17,0xBA,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x82},
    {0x01,0x09,0x29,0x12,0x00,0x33,0x03,0x80,0x22,0xBC,0x00,0x00,0x1D,0x00,0x81,0x1B,0x01,0x19,0x6F,0x15,0x84,0x0E,0x31,0x50,0x41,0x59,0x2E,0x53,0x59,0x53,0x2E,0x44,0x44,0x46,0x30,0x31,0xA5,0x03,0x88,0x01,0x01,0x90,0x00,0xAB}, 
    {0x01,0x09,0x2D,0x12,0x00,0x33,0x04,0x80,0x26,0xBC,0x00,0x00,0x21,0x00,0x81,0x1F,0x01,0x1D,0xCC,0xEC,0xBD,0xF2,0x00,0x01,0x00,0x01,0x16,0x10,0x86,0xF8,0x00,0x4A,0x02,0x02,0xF1,0x79,0x20,0x08,0x04,0x08,0x20,0x20,0x01,0x01,0x00,0x90,0x00,0xDC}, 
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x05,0x80,0x17,0xBA,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x85},
    {0x01,0x09,0x29,0x12,0x00,0x33,0x06,0x80,0x22,0xBC,0x00,0x00,0x1D,0x00,0x81,0x1B,0x01,0x19,0x6F,0x15,0x84,0x0E,0x31,0x50,0x41,0x59,0x2E,0x53,0x59,0x53,0x2E,0x44,0x44,0x46,0x30,0x31,0xA5,0x03,0x88,0x01,0x01,0x90,0x00,0xAE},
    {0x01,0x09,0x2D,0x12,0x00,0x33,0x07,0x80,0x26,0xBC,0x00,0x00,0x21,0x00,0x81,0x1F,0x01,0x1D,0xCC,0xEC,0xBD,0xF2,0x00,0x01,0x00,0x01,0x16,0x10,0x86,0xF8,0x00,0x4A,0x02,0x02,0xF1,0x79,0x20,0x08,0x04,0x08,0x20,0x20,0x01,0x01,0x00,0x90,0x00,0xDF},
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x08,0x80,0x17,0xB9,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x8B},
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x09,0x80,0x17,0xBA,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x89},
    {0x01,0x09,0x21,0x12,0x00,0x33,0x0A,0x80,0x1A,0xBC,0x00,0x00,0x15,0x00,0x81,0x13,0x01,0x11,0x6F,0x0D,0x84,0x09,0xA0,0x00,0x00,0x00,0x03,0x86,0x98,0x07,0x01,0xA5,0x00,0x90,0x00,0xDA},
    {0x01,0x09,0x1A,0x12,0x00,0x33,0x0B,0x80,0x13,0xBC,0x00,0x00,0x0E,0x00,0x81,0x0C,0x01,0x0A,0x88,0xCD,0x46,0x8C,0xA6,0x63,0x42,0xFC,0x90,0x00,0xC8},
    {0x01,0x09,0x5A,0x12,0x00,0x33,0x0C,0x80,0x53,0xBC,0x00,0x00,0x4E,0x00,0x81,0x4C,0x01,0x4A,0x84,0xDF,0xB6,0x5E,0xD9,0xA9,0xF3,0x45,0xE1,0x22,0xAD,0x9A,0xE5,0xD5,0xE0,0xC6,0xCF,0x1A,0x32,0x3A,0x70,0xB1,0xE1,0x62,0xCF,0x1A,0x32,0x3A,0x70,0xB1,0xE1,0x62,0xCF,0x1A,0x32,0x3A,0x70,0xB1,0xE1,0x62,0xCF,0x1A,0x32,0x3A,0x70,0xB1,0xE1,0x62,0xCF,0x1A,0x32,0x3A,0x70,0xB1,0xE1,0x62,0xCF,0x1A,0x32,0x3A,0x70,0xB1,0xE1,0x62,0xFB,0xDD,0x4C,0xF2,0x3B,0x24,0xB7,0xEC,0x90,0x00,0x70},
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x0D,0x80,0x17,0xB9,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x8E},
    {0x01,0x09,0x1E,0x12,0x00,0x33,0x0E,0x80,0x17,0xB9,0x00,0x14,0x3B,0x7F,0x18,0x00,0x00,0x4A,0x86,0xF8,0x00,0x4A,0x30,0x01,0x22,0x01,0x01,0x00,0x02,0x02,0xF1,0x79,0x8D},
    {0x01,0x09,0x12,0x12,0x00,0x33,0x0F,0x80,0x0B,0xB3,0xE3,0x00,0x06,0x00,0x81,0x04,0x01,0x02,0x14,0x1B,0x5B},
};
void test_etc(void)
{
    if (!bleSendHandle)
        bleSendHandle = sess_get_att_handle(SES_IDX_TXD_VAL);
    
    bleEtcDataSet(ETC_DATA[test_etc_cnt][2]-2, (uint8_t *)&ETC_DATA[test_etc_cnt][5]);
        
    test_etc_cnt++;
}
#endif

void ble_parser_rsp(struct pt_pkt_wc *pkt, uint16_t status)
{
    uint16_t pkt_len = (((uint16_t)pkt->head.nLength[0] << 8) | pkt->head.nLength[1]);
    uint16_t pkt_cmd = (((uint16_t)pkt->head.nCmdId[0] << 8) | pkt->head.nCmdId[1]);
    
    if (status == ECI_err_decode)
    {
        debugHex((uint8_t *)pkt, pkt_len);       
        return;
    }

    switch (pkt_cmd)
    {
        case ECI_resp_auth:
        {
            #if (ETC_TEST)
            test_etc_cnt = 0;
            #endif
            ble_req_init();
        } break;

        case ECI_resp_sendData:
        {
           // Sync Echo Data
            
        } break;

        case ECI_resp_init:
        {
//            ble_req_data();
        } break;

        case ECI_push_recvData:
        { 
            pt_rsp_le_data_rep(sess_get_att_handle(SES_IDX_RXD_VAL), (pkt_len - 14), &pkt->payl[4]);
            
            #if (ETC_TEST)
            test_etc();
            #endif
            
        } break;
        
        case ECI_push_switchView:
        {
        } break;
        
        case ECI_push_switchBackgroud:
        {
            debugHex((uint8_t *)pkt, pkt_len);
        } break;
        
        default:
        {

        } break;
    }
}

void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif //(CFG_SLEEP)

    proto_schedule();
    
    ble_wc_sch();
}
