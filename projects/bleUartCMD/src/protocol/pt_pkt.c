#include <stdint.h>
#include <stdbool.h>
#include "proto.h"

typedef struct pkt_desc
{
    uint8_t  code; // pkt code
    uint8_t  pfmt; // pkt format
    uint16_t plen; // payl length
} pkt_desc_t;

#define PKT_CRC_SEED      (0xFF)
#define PKT_CMD_CNT       (sizeof(pt_cmd_desc)/sizeof(struct pkt_desc))
#define PKT_RSP_CNT       (sizeof(pt_rsp_desc)/sizeof(struct pkt_desc))

const struct pkt_desc pt_cmd_desc[] =
{
    { PT_CMD_SET_BLE_ADDR,             PKT_FIXLEN ,     PLEN_CMD_SET_BLE_ADDR},
    { PT_CMD_SET_VISIBILITY,           PKT_FIXLEN ,     PLEN_CMD_SET_VISIBILITY},
    { PT_CMD_SET_BLE_NAME,             PKT_VARLEN ,     PLEN_CMD_DFT},
                                            
    { PT_CMD_SEND_BLE_DATA,            PKT_VARLEN ,     PLEN_CMD_DFT},
                                            
    { PT_CMD_STATUS_REQUEST,           PKT_FIXLEN ,     PLEN_CMD_STATUS_REQUEST},
    { PT_CMD_SET_UART_FLOW,            PKT_VARLEN ,     PLEN_CMD_DFT},   
    { PT_CMD_SET_UART_BAUD,            PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_VERSION_REQUEST,          PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_BLE_DISCONNECT,           PKT_VARLEN ,     PLEN_CMD_DFT},
                                            
    { PT_CMD_CONFIRM_GKEY,             PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_SET_CREDIT_GIVEN,         PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_SET_ADV_DATA,             PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_POWER_REQ,                PKT_FIXLEN ,     0},    
    { PT_CMD_POWER_SET,                PKT_VARLEN ,     PLEN_CMD_DFT},
                                            
    { PT_CMD_PASSKEY_ENTRY,            PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_SET_GPIO,                 PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_READ_GPIO,                PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_LE_SET_PAIRING,           PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_LE_SET_ADV_DATA,          PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_LE_SET_SCAN_DATA,         PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_LE_SEND_CONN_UPDATE_REQ,  PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_LE_SET_ADV_PARM,          PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_LE_START_PAIRING,         PKT_VARLEN ,     PLEN_CMD_DFT},
                                            
    { PT_CMD_SET_WAKE_GPIO,            PKT_VARLEN ,     PLEN_CMD_DFT},
    { PT_CMD_SET_TX_POWER,             PKT_VARLEN ,     PLEN_CMD_DFT},    
    { PT_CMD_LE_CONFIRM_GKEY,          PKT_VARLEN ,     PLEN_CMD_DFT},   
    { PT_CMD_REJECT_JUSTWORK,          PKT_VARLEN ,     PLEN_CMD_DFT}, 
                                            
    { PT_CMD_RESET_CHIP_REQ,           PKT_VARLEN ,     PLEN_CMD_DFT}, 
                                            
    { PT_CMD_LE_SET_FIXED_PASSKEY,     PKT_VARLEN ,     PLEN_CMD_DFT},
                                           
    { PT_CMD_DELETE_CUSTOMIZE_SERVICE, PKT_VARLEN ,     PLEN_CMD_DFT},    
    { PT_CMD_ADD_SERVICE_UUID,         PKT_VARLEN ,     PLEN_CMD_DFT},   
    { PT_CMD_ADD_CHARACTERISTIC_UUID,  PKT_VARLEN ,     PLEN_CMD_DFT},
                                            
};

static uint8_t pkt_find(struct pt_pkt *hdr, const struct pkt_desc *desc, uint8_t desc_size)
{
    uint8_t idx;
    bool find = false;

    for (idx = 0; idx < desc_size; idx++)
    {
        if (hdr->code > desc[idx].code)
        {
            continue;
        }
        else
        {
            if (hdr->code == desc[idx].code)
            {
                find = true;
            }
            break;
        }
    }

    if (!find)
        return PT_ERR_CODE;

    if ((desc[idx].pfmt & PKT_LEN_MSK) == PKT_VARLEN)
    {
        if (hdr->len > desc[idx].plen)
            return PT_ERR_LEN;
    }
    else // PKT_FIXLEN
    {
        if (hdr->len != desc[idx].plen)
            return PT_ERR_LEN;
    }

    return desc[idx].pfmt; //PT_OK;
}

uint8_t pkt_hdr_valid(struct pt_pkt *p_hdr)
{
    uint8_t status = PT_ERR_HEAD;

    if (p_hdr->type == PT_TYPE_CMD)
    {
        return pkt_find(p_hdr, pt_cmd_desc, PKT_CMD_CNT);
    }

    return status;
}

uint8_t pkt_crc8(uint8_t *buff, uint16_t len)
{
    uint16_t i;
    uint8_t crc = PKT_CRC_SEED;

    for (i = 0; i < len; i++)
    {
        crc ^= buff[i];
    }
    return crc;
}
