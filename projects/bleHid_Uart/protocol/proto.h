#ifndef _PROTO_H_
#define _PROTO_H_

#include <stdint.h>
#include "hid_desc.h"


/***************************************************************************
 *  Proto Macro API
 ***************************************************************************/

/// Protocol Defines

enum PT_TYPE
{
    PT_TYPE_CMD          = 0x5A,
    PT_TYPE_RSP          = 0xA5,
};

enum PT_ERR_CODE
{
    PT_OK                = 0x00,

    // proto sch error A0~A4
    PT_ERROR             = 0xA0,
    PT_ERR_CRC           = 0xA0,
    PT_ERR_LEN           = 0xA1,
    PT_ERR_CODE          = 0xA2,
    PT_ERR_HEAD          = 0xA3,
    PT_ERR_TIMEOUT       = 0xA4,
    
    // proto app error A6~AF
    PT_ERR_VERIFY        = 0xA6,
    PT_ERR_STATUS        = 0xA7,
};

/// Packet Defines @see pt_pkt
#define PKT_HDR_POS        0
#define PKT_HDR_SIZE       3
#define HDR_SYNC_POS       (PKT_HDR_POS)
#define HDR_SYNC_SIZE      (0x01)
#define HDR_CMD_POS        (PKT_HDR_POS + HDR_SYNC_SIZE)
#define HDR_CMD_SIZE       (0x01)
#define HDR_LEN_POS        (PKT_HDR_POS + HDR_SYNC_SIZE + HDR_CMD_SIZE)
#define HDR_LEN_SIZE       (0x01)

#define PKT_PAYL_POS       3
#define PAYL_DAT_MAX_SIZE  (0xFF)
#define PKT_PAYL_MAX_SIZE  (PAYL_DAT_MAX_SIZE)

#define PKT_MAX_SIZE       (((PKT_HDR_SIZE + PKT_PAYL_MAX_SIZE)+3)/4)*4

enum PKT_FMT
{
    PKT_FIXLEN           = 0,
    PKT_VARLEN           = 1,

    PKT_FIXLEN_CRC       = 2,
    PKT_VARLEN_CRC       = 3,
    
    PKT_LEN_MSK          = 0x01,
    PKT_CRC_MSK          = 0x02,
};

/// Protocol Packet
typedef struct pt_pkt
{
    /* head(3B): 1-sync; 1-cmd; 1-len */
    uint8_t   type;
    uint8_t   code;
    uint8_t   len;

    /* payl(nB): A set of variable value */
    uint8_t   payl[];
} pkt_t;

typedef void(*parse_fnct)(struct pt_pkt *pkt, uint8_t status);

/// PT Interface
void proto_init(parse_fnct parse);

void proto_schedule(void);


/***************************************************************************
 *  Proto Command & Response Macros
 ***************************************************************************/

enum PT_CMD_RSP
{
    PT_CMD_HID_KB       = 0x00, // keyboard input(device-->host)
    PLEN_CMD_HID_KB     = RPT_LEN_KB,
    PLEN_RSP            = 0x01,

    PT_CMD_HID_KB_LED   = 0x01, // keyboard output(host-->device)
    PLEN_CMD_GET_KB_LED = 0,

    PT_CMD_HID_MEDIA    = 0x02, // media
    PLEN_CMD_HID_MEDIA  = RPT_LEN_MEDIA,

    PT_CMD_HID_SYSTEM   = 0x03, // system control
    PLEN_CMD_HID_SYSTEM = RPT_LEN_SYSTEM,

    PT_CMD_HID_MOUSE    = 0x04, // mouse
    PLEN_CMD_HID_MOUSE  = RPT_LEN_MOUSE,

    PT_CMD_GET_APP_STA  = 0x05, // app status
    PLEN_CMD_GET_APP_STA= 0,
    
    PT_CMD_SLP          = 0x06, // poweroff
    PLEN_CMD_SLP        = 0,
    
    PT_CMD_RST          = 0x07, // reset
    PLEN_CMD_RST        = 0,
};

/***************************************************************************
 *  Proto Command & Response Structs
 ***************************************************************************/
struct pt_rsp_status
{
    uint8_t status;
};

/// HID pkt
struct pt_cmd_hid_kb
{
    uint8_t keys_code[RPT_LEN_KB];
};

struct pt_cmd_hid_media
{
    uint8_t mkeys_code[RPT_LEN_MEDIA];
};

struct pt_cmd_hid_system
{
    uint8_t sys_code[RPT_LEN_SYSTEM];
};

struct pt_cmd_hid_mouse
{
    uint8_t mouse_code[RPT_LEN_MOUSE];
};

/// app state
struct pt_cmd_get_app_sta
{
    uint8_t app_sta;
};

/// lower power mode
struct pt_cmd_slp
{
    uint8_t lp_mode;
};
/***************************************************************************
 *  Proto Command & Response Function
 ***************************************************************************/

#define PKT_ALLOC(payl_len)  uint8_t buff[PKT_HDR_SIZE + payl_len]; pkt_t *pkt = (pkt_t *)buff
#define PKT_PARAM(p_struct)  p_struct *param = (p_struct *)pkt->payl

void uart_proc(struct pt_pkt *pkt, uint8_t status);
void pt_rsp_cmd_sta(uint8_t cmd, uint8_t rsp_sta);
#endif // _PROTO_H_
