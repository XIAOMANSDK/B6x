/**
 ****************************************************************************************
 * @file proto_rsp.c
 * @brief Protocol response builder and transmitter
 ****************************************************************************************
 */
#include "drvs.h"
#include "proto.h"
#include "pt_env.h"
#include "uartRb.h"

#if (DBG_PKT)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

__STATIC_INLINE /**
 ****************************************************************************************
 * @brief Fill response packet with header and payload
 * @param[in]  pkt_hdr_t *pkt
 * @param[in]  uint8_t cmd
 * @param[in]  const uint8_t *data
 * @param[in]  uint16_t len
 ****************************************************************************************
 */
void pt_fill_rsp(pkt_t *pkt, uint8_t rsp, uint8_t len)
{
    pkt->type = PT_TYPE_RSP;
    pkt->code = rsp;
    pkt->len  = len;
}

__STATIC_INLINE void pt_send_rsp(pkt_t *pkt)
{
    uart_send(UART1_PORT, pkt->len + PKT_HDR_SIZE, (uint8_t *)pkt);
}

void pt_rsp_cmd_sta(uint8_t cmd, uint8_t rsp_sta)
{
    PKT_ALLOC(PLEN_RSP);
    pt_fill_rsp(pkt, cmd, PLEN_RSP);

    PKT_PARAM(struct pt_rsp_status);
    param->status = rsp_sta;

    pt_send_rsp(pkt);
}
