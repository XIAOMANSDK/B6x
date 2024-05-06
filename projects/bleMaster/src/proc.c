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
#include "keys.h"
#include "uartRb.h"

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

enum uart_cmd
{
    CMD_DISCONNECT = 0xA0,
    CMD_CONNECT    = 0xA1,
    CMD_SCAN       = 0xA2,
    
    CMD_GATT_EXMTU = 0xB0,
    CMD_GATT_DISC,
    CMD_GATT_READ,
    CMD_GATT_WRITE,
};

#define CMD_MAX_LEN           20
#define NULL_CNT              20

static uint8_t buff[CMD_MAX_LEN];
static uint16_t buff_len = 0;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void uart_proc(void)
{
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart1Rb_Read(&buff[buff_len], CMD_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < CMD_MAX_LEN)
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
    
    switch (buff[0])
    {
        case CMD_DISCONNECT:
        {
            DEBUG("Disconnect");
            gapc_disconnect(app_env.curidx);
        } break;
        
        case CMD_CONNECT:
        {
            app_init_action(ACTV_STOP);  //20211101
            
            if (buff_len == 1)
            {
                DEBUG("CONN Dflt");
                app_start_initiating(NULL);
            }
            else if (buff_len == 2)
            {
                if (scan_cnt > buff[1])
                {
                    app_start_initiating(&scan_addr_list[buff[1]]);
                }
            }
            else if (buff_len >= 8)
            {
                struct gap_bdaddr peer;
                for(int i = 0; i < 6; i++)
                {
                    //peer.addr.addr[i] = buff[6-i];
                    peer.addr.addr[i] = buff[i+1];
                }
                peer.addr_type = buff[7];

                DEBUG("CONN Peer:");
                debugHex((uint8_t *)&peer, sizeof(peer));
                app_start_initiating(&peer);
                
                finish = true;
            }
            else
            {
                finish = false;
            }
        } break;
        
        case CMD_SCAN:
        {
            DEBUG("Scan");
            app_scan_action(ACTV_START);
        } break; 
        
        case CMD_GATT_DISC:
        {
            if ((buff_len >= 6) && (buff[1] < 6))
            {
                uint8_t disc_op = buff[1] + GATT_DISC_ALL_SVC;
                
                if ((disc_op == GATT_DISC_BY_UUID_SVC) || (disc_op == GATT_DISC_BY_UUID_CHAR))
                {
                    if (buff_len >= 7)
                    {
                        uint8_t ulen = buff[6];
                        if (ulen == 0x02 || ulen == 0x04 || ulen == 0x10)
                        {
                            if (buff_len >= 7 + ulen)
                            {
                                DEBUG("GATT DISC Uuid(typ:%d,shdl:0x%X,ehdl:0x%X,ulen:%d)", buff[1], read16p(&buff[2]), read16p(&buff[4]), ulen);
                                gatt_disc(app_env.curidx, disc_op, read16p(&buff[2]), read16p(&buff[4]), ulen, &buff[7]);
                            }
                            else
                                finish = false;
                        }
                    }
                    else
                        finish = false;
                }
                else
                {
                    DEBUG("GATT DISC(typ:%d,shdl:0x%X,ehdl:0x%X)", buff[1], read16p(&buff[2]), read16p(&buff[4]));
                    gatt_disc(app_env.curidx, disc_op, read16p(&buff[2]), read16p(&buff[4]), 2, NULL);
                }
            }
        } break;
        
        case CMD_GATT_READ:
        {
            if (buff_len >= 5)
            {
                if (buff[1] == 0x00)
                {
                    DEBUG("GATT READ(hdl:0x%X,len:%d)", read16p(&buff[2]), buff[4]);
                    gatt_read(app_env.curidx, read16p(&buff[2]), buff[4]);
                }
                else if (buff[1] == 0x01)
                {
                    uint8_t offset = (buff_len >= 6)? buff[5] : 0x00;
                    DEBUG("GATT READ Long(hdl:0x%X,len:%d,oft:%d)", read16p(&buff[2]), buff[4], offset);
                    gatt_read_long(app_env.curidx, read16p(&buff[2]), buff[4], offset);
                }
                else if (buff[1] == 0x02)
                {
                    uint8_t ulen = buff[2] & 0xFE;
                    if (ulen == 0x02 || ulen == 0x04 || ulen == 0x10)
                    {
                        if (buff[2] & 0x01)
                        {
                            if (buff_len >= ulen+7)
                            {
                                DEBUG("GATT READ Uuid(len:%d,shdl:0x%X,ehdl:0x%X)", ulen, read16p(&buff[ulen+3]), read16p(&buff[ulen+5]));
                                gatt_read_by_uuid(app_env.curidx, ulen, &buff[3], read16p(&buff[ulen+3]), read16p(&buff[ulen+5]));
                            }
                            else
                                finish = false;
                        }
                        else
                        {
                            if (buff_len >= ulen+3)
                            {
                                DEBUG("GATT READ Uuid(len:%d)", ulen);
                                gatt_read_by_uuid(app_env.curidx, ulen, &buff[3], 0x0001, 0xFFFF);
                            }
                            else
                                finish = false;
                        }
                    }
                }
                else if (buff[1] == 0x03)
                {
                    if (buff_len >= buff[2]+3)
                    {
                        DEBUG("GATT READ Multi(hdls:%d)", buff[2]);
                        gatt_read_by_multiple(app_env.curidx, buff[2], &buff[3]);
                    }
                    else
                        finish = false;
                }
            }
        } break;
        
        case CMD_GATT_WRITE:
        {
            if (buff_len >= 5)
            {
                if (buff[1] <= 0x02)
                {
                    DEBUG("GATT WRITE(typ:%d,hdl:0x%X,len:%d)", buff[1], read16p(&buff[2]),buff_len-4);
                    gatt_write(app_env.curidx, buff[1]+GATT_WRITE, read16p(&buff[2]), &buff[4], buff_len-4);
                }
                else if (buff[1] == 0x03)
                {
                    DEBUG("GATT PreWR(hdl:0x%X,len:%d,oft:%d)", read16p(&buff[2]), buff[4], (buff_len == 5)?0x00:buff[5]);
                    gatt_pre_write(app_env.curidx, read16p(&buff[2]), NULL, buff[4], (buff_len == 5)?0x00:buff[5]);
                }
            }
        } break;
        
        case CMD_GATT_EXMTU:
        {           
            uint16_t mtu = (buff_len == 3) ? read16p(&buff[1]) : BLE_MTU;
            
            DEBUG("GATT ExMTU:%d(23~512,blen:%d)", mtu, buff_len);           
            gatt_exmtu(app_env.curidx, mtu);
        } break;

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
    uart_proc();
    keys_scan();
}
