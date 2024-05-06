#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "proto.h"
#include "pt_env.h"
#include "sftmr.h"

/// Protocol Parse
static void proto_read_sync(proto_t *pt)
{
    pt->rx_state = PT_STATE_SYNC;
    pt->rx_size  = HDR_SYNC_SIZE;
    pt->rx_ptr   = pt->pkt + HDR_SYNC_POS;
}

static void proto_read_hdr(proto_t *pt)
{
    pt->rx_state = PT_STATE_HDR;
    pt->rx_size  = HDR_CMD_SIZE + HDR_LEN_SIZE;
    pt->rx_ptr   = pt->pkt + HDR_CMD_POS;
}

static void proto_read_payl(proto_t *pt)
{
    pt->rx_state = PT_STATE_PAYL;
    pt->rx_size  = pt->pkt[2];
    pt->rx_ptr   = pt->pkt + PKT_PAYL_POS;
}

static void proto_received(proto_t *pt)
{
    bool finish = false;
    struct pt_pkt *pkt = (struct pt_pkt *)(pt->pkt);

    switch (pt->rx_state)
    {
        case PT_STATE_SYNC:
        {
            if (pkt->type == PT_TYPE_CMD)
            {
                proto_read_hdr(pt);
            }
            else
            {
                proto_read_sync(pt);
            }
        } break;

        case PT_STATE_HDR:
        {
            pt->rx_pfmt = pkt_hdr_valid(pkt);

            if ((pt->rx_pfmt >= PT_ERROR) || (pkt->len == 0))
            {
                // Hdr error or Short code
                finish = true;
            }
            else
            {
                proto_read_payl(pt); // more data
            }
        } break;

        case PT_STATE_PAYL:
        {
            finish = true;

        } break;

        default:
            break;
    }

    if (finish)
    {
        uint8_t status = (pt->rx_pfmt >= PT_ERROR) ? pt->rx_pfmt : PT_OK;
        
        pt->parse(pkt, status);
                
        proto_read_sync(pt);
    }
}

#if (PT_TOUT_CNT)

static void proto_timeout(proto_t *pt)
{
    if (pt->rx_state == PT_STATE_SYNC)
        return;

    // calc timeout
    if ((tmr_tk_t)(sftmr_tick() - pt->rx_tick) > PT_TOUT_CNT)
    {
        
        if (pt->rx_state == PT_STATE_PAYL)
        {
            struct pt_pkt *pkt = (struct pt_pkt *)(pt->pkt);

            pt->parse(pkt, PT_ERR_TIMEOUT);
        }
              
        proto_read_sync(pt);
        
    }
    
}
#endif

/// Protocol API
void pt_sch_proc(proto_t *pt)
{
    if (pt->rx_size > 0)
    {        
        uint16_t len = pt->read(pt->rx_ptr, pt->rx_size);

        if (len > 0)
        {
            pt->rx_size -= len;
            pt->rx_ptr  += len;
      
            if (pt->rx_size == 0)
            {                
                proto_received(pt);                
            }
#if (PT_TOUT_CNT)
            pt->rx_tick = sftmr_tick();
        }
        else
        {
            proto_timeout(pt);
        }
#else
        }
#endif
    }
}

void pt_sch_init(proto_t *pt, uint8_t *buf, read_fnct read, parse_fnct parse)
{
    memset(pt, 0, sizeof(proto_t));

    pt->read  = read;
    pt->parse = parse;
    pt->pkt   = buf;

    proto_read_sync(pt);
}

