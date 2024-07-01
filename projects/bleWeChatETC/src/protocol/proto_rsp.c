#include <stdint.h>
#include <string.h>
#include "proto.h"
#include "uartRb.h"

static __inline void pt_fill_rsp(pkt_t *pkt, uint8_t rsp, uint8_t len)
{
    pkt->type = PT_TYPE_RSP;
    pkt->code = rsp;
    pkt->len  = len;
}

/*static __inline*/ void pt_send_rsp(pkt_t *pkt)
{
    uart_send(UART1_PORT, pkt->len + PKT_HDR_SIZE, (uint8_t *)pkt);
}

/* Common: Status */
void pt_rsp_code(uint8_t rsp)
{
    PKT_ALLOC(PLEN_RSP_CODE);
    pt_fill_rsp(pkt, rsp, PLEN_RSP_CODE);

    pt_send_rsp(pkt);
}

void pt_rsp_cmd_res(uint8_t opcode, uint8_t status, uint8_t len, const void *payl)
{
    PKT_ALLOC(PLEN_RSP_CMD_RES);
    pt_fill_rsp(pkt, PT_RSP_CMD_RES, len + 2);
    
    PKT_PARAM(struct pt_rsp_cmd_res);
    param->opcode = opcode;
    param->status = status;
    
    memcpy(param->data, payl, len);

    pt_send_rsp(pkt);
}

void pt_rsp_le_data_rep(uint16_t handle, uint8_t len, const void *payl)
{
    PKT_ALLOC(PLEN_RSP_LE_DATA_REP);
    pt_fill_rsp(pkt, PT_RSP_LE_DATA_REP, len + 2);
    
    PKT_PARAM(struct pt_rsp_le_data_rep);
    param->handle[0] = handle;
    param->handle[1] = handle >> 8;
    
    memcpy(param->data, payl, len);
//    memcpy(&pkt->payl[2], payl, len);
    pt_send_rsp(pkt);
}

void pt_rsp_status_res(uint8_t status)
{
    PKT_ALLOC(PLEN_RSP_STATUS);
    pt_fill_rsp(pkt, PT_RSP_STATUS_RES, PLEN_RSP_STATUS);

    PKT_PARAM(struct pt_rsp_status);
    param->status = status;

    pt_send_rsp(pkt);
}

void pt_rsp_ntfind_res(uint16_t uuids, uint16_t status)
{
    PKT_ALLOC(PLEN_RSP_NTFIND);
    pt_fill_rsp(pkt, PT_RSP_NTFIND_RES, PLEN_RSP_NTFIND);

    PKT_PARAM(struct pt_rsp_ntfind_status);
    param->uuids[0] = uuids;
    param->uuids[1] = uuids >> 8;
    param->state[0] = status;
    param->state[1] = status >> 8;

    pt_send_rsp(pkt);
}

void pt_rsp_nvram_rep(const void *payl)
{
    PKT_ALLOC(PLEN_RSP_NVRAM_REP);
    pt_fill_rsp(pkt, PT_RSP_NVRAM_REP, PLEN_RSP_NVRAM_REP);
    
    memcpy(pkt->payl, payl, PLEN_RSP_NVRAM_REP);

    pt_send_rsp(pkt);
}

void pt_rsp_key(uint8_t opcode, uint8_t len, const void *payl)
{
    PKT_ALLOC(PLEN_RSP_GKEY);
    pt_fill_rsp(pkt, opcode, len);
    
    memcpy(pkt->payl, payl, len);

    pt_send_rsp(pkt);
}

void pt_rsp_le_pairing_state(uint16_t state)
{
    PKT_ALLOC(PLEN_RSP_LE_PAIRING_STATE);
    pt_fill_rsp(pkt, PT_RSP_LE_PAIRING_STATE, PLEN_RSP_LE_PAIRING_STATE);

    PKT_PARAM(struct pt_rsp_le_pairing_state);
    param->state[0] = state;
    param->state[1] = state >> 8;
    
    pt_send_rsp(pkt);
}

void pt_rsp_le_encryption_state(uint8_t state)
{
    PKT_ALLOC(PLEN_RSP_LE_ENCRYPTION_STATE);
    pt_fill_rsp(pkt, PT_RSP_LE_ENCRYPTION_STATE, PLEN_RSP_LE_ENCRYPTION_STATE);

    PKT_PARAM(struct pt_rsp_le_encryption_state);
    param->state = state;

    pt_send_rsp(pkt);
}

void pt_rsp_uuid_handle(uint16_t handle)
{
    PKT_ALLOC(PLEN_RSP_UUID_HANDLE);
    pt_fill_rsp(pkt, PT_RSP_UUID_HANDLE, PLEN_RSP_UUID_HANDLE);

    PKT_PARAM(struct pt_rsp_uuid_handle);
    param->handle[0] = handle;
    param->handle[1] = handle >> 8;
    
    pt_send_rsp(pkt);
}
