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
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "prf_sess.h"
#include "uartRb.h"
#include "weChat.h"

#include "prf_sess_weChat.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

struct rxd_buffer uart1_rxd;

uint16_t uart1_size(void)    
{
    return ((uart1_rxd.head + RXD_BUFF_SIZE - uart1_rxd.tail) % RXD_BUFF_SIZE);
}

uint16_t uart1_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = uart1_rxd.head;
    uint16_t tail = uart1_rxd.tail;
    uint16_t tlen, len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len == 0 || max == 0)
    {
        return 0; // empty
    }

    if (len > max)
        len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&uart1_rxd.data[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&uart1_rxd.data[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&uart1_rxd.data[0], len - tlen); // head_len
    }
    uart1_rxd.tail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}

/*
 * DEFINES
 ****************************************************************************************
 */

#define BLE_MAX_LEN           20
#define NULL_CNT              20

static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;

extern const bd_addr_t ble_dev_addr_B;

uint16_t ble_head_nSeq = 1;
/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if !(DBG_SESS)
/// Override - Callback on received data from peer device
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    uart_send(UART1_PORT, len, data);
}
#endif //!(DBG_SESS)

/// Uart Data procedure
static void data_proc(void)
{
    // Todo Loop-Proc
    static uint8_t null_cnt = 0;
    uint16_t len;

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

    if (app_state_get() == APP_CONNECTED)
    {
        if (buff[0] == 0xAA)
        {
            DEBUG("GAP Disc!\r\n");
            gapc_disconnect(app_env.curidx);
            buff_len = 0;
        }
        else if (sess_txd_send(app_env.curidx, buff_len, buff) == LE_SUCCESS)
        {
            debugHex(buff, buff_len);
            buff_len = 0;
        }
    }
    else
    {
        // goto reset
        if (buff[0] == 0xAA)
        {
            DEBUG("GAP Reset!\r\n");
            gapm_reset();
        }

        buff_len = 0;
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
            
            gapc_connect_rsp(conidx, GAP_AUTH_REQ_NO_MITM_NO_BOND);

            // Enable profiles by role
        } break;
        
        case BLE_DISCONNECTED:
        {
            ble_head_nSeq = 1;
            
            app_state_set(APP_READY);
            
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


void ble_req_auth(void)
{
    uint8_t buff[BLE_REQ_AUTH_LEN] = {0xFE, 0x01, 0x00, 0x1A,0x27, 0x11, 0x00, 0x01,    0x0A, 0x00, 0x18, 0x84, 0x80, 0x04, 0x20, 0x01, 0x28, 0x02, 0x3A, 0x06}; 
    pkt_t *pkt = (pkt_t *)buff;
        
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(BLE_REQ_AUTH_LEN); 
    pkt->head.nLength[1]      = __SWP16_L(BLE_REQ_AUTH_LEN); 
        
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_auth);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_auth); 
        
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq); 
        
    ble_head_nSeq++;
        
    memcpy(&pkt->payl[12], ble_dev_addr_B.addr ,GAP_BD_ADDR_LEN);
        
    sess_ind_send(app_env.curidx, BLE_REQ_AUTH_LEN, buff);
}

void ble_req_init(void)
{
    uint8_t buff[BLE_REQ_INIT_LEN] = {0xFE, 0x01, 0x00, 0x0A,0x27, 0x13, 0x00, 0x02,    0x0A, 0x00};
    pkt_t *pkt = (pkt_t *)buff;
        
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(BLE_REQ_INIT_LEN); 
    pkt->head.nLength[1]      = __SWP16_L(BLE_REQ_INIT_LEN);     
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_init);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_init);    
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq);
    ble_head_nSeq++;
    
    sess_ind_send(app_env.curidx, BLE_REQ_INIT_LEN, buff);
}

void ble_req_data(void)
{
    uint8_t buff[BLE_REQ_DATA_LEN] = {0xFE, 0x01, 0x00, 0x0F,0x27, 0x12, 0x00, 0x03,    0x0A, 0x00, 0x12,  0x01,  0x56,  0x18,0x00};
    pkt_t *pkt = (pkt_t *)buff;
        
    pkt->head.bMagicNumber = FIX_HEAD_MAGIC;
    pkt->head.bVer         = FIX_HEAD_VER;
    pkt->head.nLength[0]      = __SWP16_H(BLE_REQ_DATA_LEN); 
    pkt->head.nLength[1]      = __SWP16_L(BLE_REQ_DATA_LEN);     
    pkt->head.nCmdId[0]       = __SWP16_H(ECI_req_sendData);
    pkt->head.nCmdId[1]       = __SWP16_L(ECI_req_sendData);    
    pkt->head.nSeq[0]         = __SWP16_H(ble_head_nSeq);
    pkt->head.nSeq[1]         = __SWP16_L(ble_head_nSeq);
    ble_head_nSeq++;
    
    sess_ind_send(app_env.curidx, BLE_REQ_DATA_LEN, buff);
}

uint8_t buffR[BLE_DATA_LEN_MAX]; 

void ble_parser_sch(void)
{
     if (uart1_rxd.head != uart1_rxd.tail)
     {
        if (uart1_rxd.data[uart1_rxd.tail] == FIX_HEAD_MAGIC)
        {
            uint16_t length = uart1_size();
            uint16_t pkt_len = (((uint16_t)uart1_rxd.data[(uart1_rxd.tail+2)%RXD_BUFF_SIZE] << 8) | uart1_rxd.data[(uart1_rxd.tail+3)%RXD_BUFF_SIZE]);
            
            if (length >= pkt_len)
            {                             
              uart1_read(buffR, pkt_len);
                
              ble_parser_rsp((struct pt_pkt *)buffR, ECI_none);
            }
            //Timeout
        }
        else
        {
            uart1_rxd.tail++;
        }
     }         
}

void ble_parser_rsp(struct pt_pkt *pkt, uint16_t status)
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
            ble_req_init();
        } break;

        case ECI_resp_sendData:
        {
           // Sync Echo Data
            
        } break;

        case ECI_resp_init:
        {
            ble_req_data();
        } break;

        case ECI_push_recvData:
        { 
        }
        
        case ECI_push_switchView:
        {
        }
        
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

    data_proc();
    
    ble_parser_sch();
}

