#include <stdint.h>
#include <stdbool.h>
#include "proto.h"

#if (DBG_PKT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

typedef struct pkt_desc
{
    uint8_t  code; // pkt code
    uint8_t  pfmt; // pkt format
    uint16_t plen; // payl length
} pkt_desc_t;

#define PKT_CRC_SEED      (0xFF)
#define PKT_CMD_CNT       (sizeof(pt_cmd_desc)/sizeof(struct pkt_desc))

const struct pkt_desc pt_cmd_desc[] =
{
    { PT_CMD_HID_KB,      PKT_FIXLEN, PLEN_CMD_HID_KB      },
    { PT_CMD_HID_KB_LED,  PKT_FIXLEN, PLEN_CMD_GET_KB_LED  },
    { PT_CMD_HID_MEDIA,   PKT_FIXLEN, PLEN_CMD_HID_MEDIA   },
    { PT_CMD_HID_SYSTEM,  PKT_FIXLEN, PLEN_CMD_HID_SYSTEM  },
    { PT_CMD_HID_MOUSE,   PKT_FIXLEN, PLEN_CMD_HID_MOUSE   },
    { PT_CMD_GET_APP_STA, PKT_FIXLEN, PLEN_CMD_GET_APP_STA },
    { PT_CMD_SLP,         PKT_FIXLEN, PLEN_CMD_SLP         },
    { PT_CMD_RST,         PKT_FIXLEN, PLEN_CMD_RST         },
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

uint8_t pkt_bcc8(uint8_t *buff, uint16_t len)
{
    uint16_t i;
    uint8_t bcc = buff[0];
    
    for (i = 1; i < len; i++)
    {
        bcc ^= buff[i];
    }
    
    return bcc;
}
