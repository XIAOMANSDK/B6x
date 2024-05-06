#ifndef _PT_ENV_H_
#define _PT_ENV_H_

#include <stdint.h>

struct pt_pkt; // defined in proto.h

/// Timeout(unit in currTickCnt) between PKT received, 0 means keep waiting
#define PT_TOUT_CNT  (5)

enum PT_STATE
{
    PT_STATE_SYNC,
    PT_STATE_HDR,
    PT_STATE_PAYL,
    PT_STATE_OUT_OF_SYNC
};

typedef uint16_t (*read_fnct)(uint8_t *buf, uint16_t len);
typedef uint16_t (*send_fnct)(uint8_t *buf, uint16_t len);
typedef void(*parse_fnct)(struct pt_pkt *pkt, uint8_t status);

typedef struct proto_env_tag
{
    // interface
    read_fnct  read;
    parse_fnct parse;
    // buffer
    uint8_t   *pkt;
    // rx channel
    uint8_t   *rx_ptr;
    uint16_t   rx_size;
    uint8_t    rx_state;
    uint8_t    rx_pfmt;
#if (PT_TOUT_CNT)
    uint16_t   rx_tick;
#endif
} proto_t;

/// pt_pkt API
extern uint8_t pkt_crc8(uint8_t *buff, uint16_t len);
extern uint8_t pkt_hdr_valid(struct pt_pkt *p_hdr);

/// pt_sch API
extern void pt_sch_proc(proto_t *pt);
extern void pt_sch_init(proto_t *pt, uint8_t *buf, read_fnct read, parse_fnct parse);

#endif // _PT_ENV_H_
