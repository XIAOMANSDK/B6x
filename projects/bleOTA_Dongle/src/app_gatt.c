/**
 ****************************************************************************************
 *
 * @file app_gatt.c
 *
 * @brief App SubTask of GATT Message - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#if (GATT_CLI)

#include "regs.h"
#include "drvs.h"
#include "app.h"
#include "gatt.h"
#include "gatt_api.h"
#include "prf.h"
#include "utils.h"
#include "leds.h"

#if (DBG_GATT)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#define NEW_LINE           32
#define debugDump(info, dat, len)                                                                  \
    do {                                                                                           \
        debug("<%s,%d>%s(%d):", __MODULE__, __LINE__, info, (len));                                \
        if (((len) > NEW_LINE) || ((len) == 0))                                                    \
            debug("\r\n");                                                                         \
        for (int _i = 0; _i < (len); _i++)                                                         \
        {                                                                                          \
            debug("%02X", *((dat) + _i));                                                          \
            if (((_i + 1) % NEW_LINE == 0) && (_i > 0))                                            \
                debug("\r\n");                                                                     \
        }                                                                                          \
        if ((len % NEW_LINE) != 0)                                                                 \
            debug("\r\n");                                                                         \
    } while (0)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#define debugDump(info, dat, len)
#endif

// 584db6b8-ce89-72cd-9edf-2162ce8fcabf
// 584d09c5-ce89-72cd-9edf-2162ce8fcabf
// 584d0f42-ce89-72cd-9edf-2162ce8fcabf

/// Characteristic Base UUID128
#define OTA_BASE_UUID128(uuid)                                                                     \
    { 0xBF, 0xCA, 0x8F, 0xCE, 0x62, 0x21, 0xDF, 0x9E, 0xCD, 0x72, 0x89, 0xCE,                      \
        ((uuid >> 0) & 0xFF), ((uuid >> 8) & 0xFF), 0x4D, 0x58 }

const uint8_t ota_svc_uuid[] = OTA_BASE_UUID128(0xB6B8);
/// OTA Notify UUID128(Slave -> Master)
const uint8_t ota_char_ntf[] = OTA_BASE_UUID128(0x09C5);
/// OTA Receive Write Command UUID128(Master -> Slave)
const uint8_t ota_char_recv[] = OTA_BASE_UUID128(0x0F42);

/*
 * DEFINES
 ****************************************************************************************
 */
#define OTA_DATA_LEN          RD_32(0x18000010)
#define OTA_DATA_STORE_POS    RD_32(0x18000014)
#define OTA_DATA_STORE_OFFSET (OTA_DATA_STORE_POS - FLASH_BASE)
// 1B(Head) + 1B(CMD) + 2B(IDX) + 2B(Len) + nB(data)
#define OTA_DATA_POS          (6)

#define BUFF_LEN  4096
#define BUFF_WLEN (BUFF_LEN >> 2)

enum prf_char_idx
{
    CHAR_IDX_NTF,
    CHAR_IDX_WR,

    CHAR_IDX_MAX,
};

/// Structure containing the characteristics handles, value handles and
/// descriptors
struct ota_env_tag
{
    uint32_t bin_data[BUFF_WLEN];

    volatile uint16_t head;
    volatile uint16_t tail;

    uint32_t ota_bin_offset;
    uint32_t ota_data_next;
    uint32_t ota_remain_size;
    uint32_t ota_data_sum;
    uint32_t ota_block_pkt_nb;
    uint32_t ota_pkt_idx;

    uint16_t ota_block_size;
    uint16_t ota_pkt_size;

    uint32_t ota_fsm   : 8;
    uint32_t ota_next  : 1;
    uint32_t cccd_cfg  : 1;
    uint32_t buff_full : 1;
    uint32_t reserved  : 21;

    /// Service info:4
    struct prf_svc svc;
    /// Characteristic info:6
    struct prf_char_inf chars[CHAR_IDX_MAX];
    /// Descriptor handles:2
    struct prf_desc_inf descs[CHAR_IDX_MAX];
};

struct ota_env_tag ota_env;

#define ota_fsm_sta ota_env.ota_fsm

#define ota_data_offset  ota_env.ota_bin_offset
#define ota_data_next    ota_env.ota_next
#define ota_total_size   ota_env.ota_remain_size
#define ota_data_sum     ota_env.ota_data_sum
#define ota_block_pkt_nb ota_env.ota_block_pkt_nb
#define ota_pkt_idx      ota_env.ota_pkt_idx
#define ota_block_size   ota_env.ota_block_size
#define ota_pkt_size     ota_env.ota_pkt_size
#define ota_ntf_en       ota_env.cccd_cfg
#define ota_buff_full    ota_env.buff_full

// State/CMD/Head
enum ota_info
{
    /************************************/
    // state
    OTA_OK  = 0x00,
    OTA_ERR = 0x01,

    /************************************/
    // cmd
    OTA_CMD_VER = 0x10,
    OTA_CMD_START,
    OTA_CMD_DATA_WC,
    OTA_CMD_DATA_WR,
    OTA_CMD_END,

    /************************************/
    // head
    OTA_RSP_HEAD = 0xAB,
    OTA_CMD_HEAD = 0xBA,
};

enum ota_fsm
{
    OTA_IDLE,
    OTA_VER,
    OTA_START,
    OTA_WRITE_DATA,
    OTA_END,
};

// 开始查询服务.
void disc_ota_init(uint8_t conidx)
{
    memset((uint8_t *)&ota_env + BUFF_LEN, 0x00, sizeof(ota_env) - BUFF_LEN);
    ota_total_size = OTA_DATA_LEN;
    DEBUG("\r\ndisc start...%d, %d, %X", sizeof(ota_env), ota_total_size, OTA_DATA_STORE_POS);

    // 无效的bin信息.
    if ((ota_total_size == 0xFFFFFFFF) || (OTA_DATA_STORE_POS == 0xFFFFFFFF))
    {
        return;
    }
    gatt_disc(conidx, GATT_DISC_BY_UUID_SVC, 0x0001, 0xFFFF, sizeof(ota_svc_uuid), ota_svc_uuid);
}

void gatt_send_data(uint8_t conidx, uint16_t len, uint8_t *data)
{
    DEBUG("ota_fsm_sta:%d, len:%d", ota_fsm_sta, len);
    debugDump("send", data, len);
    gatt_write(conidx, GATT_WRITE_NO_RESPONSE, ota_env.chars[CHAR_IDX_WR].val_hdl, data, len);
}

void gatt_notify_set(uint8_t conidx, uint16_t handle, bool is_enable)
{
    uint16_t ntf_cfg = (is_enable ? ATT_CCC_START_NTF : ATT_CCC_STOP_NTFIND);
    DEBUG("ntf_cfg:%d", is_enable);
    gatt_write(conidx, GATT_WRITE, handle, (uint8_t *)&ntf_cfg, sizeof(uint16_t));
}

void gatt_notify_get(uint8_t conidx, uint16_t handle)
{
    gatt_read(conidx, handle, sizeof(uint16_t));
}

// 0x3B
__SRAMFN void flash_dread(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY);

    CACHE->CCR.Word = 0;
    CACHE->CIR.Word = BIT(CACHE_INV_ALL_POS);
    fshc_read(offset, buff, wlen, FCM_MODE_DUAL | FSH_CMD_DLRD);
    CACHE->CCR.Word = 1;

    GLOBAL_INT_RESTORE();
}

void read_flash_data(uint32_t block_len)
{
    uint32_t data_offset = OTA_DATA_STORE_OFFSET + ota_data_offset;
    flash_dread(data_offset, ota_env.bin_data, block_len >> 2);

    ota_data_offset += block_len;
    ota_env.head     = 0;
    ota_env.tail     = 0;
    ota_buff_full    = 1; // buffer filled completely from flash
}

uint16_t ota_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = ota_env.head;
    uint16_t tail = ota_env.tail;

    uint16_t tlen, len;

    if (ota_buff_full)
    {
        len = ota_block_size;
    }
    else if (head >= tail)
    {
        len = head - tail;
    }
    else
    {
        len = ota_block_size - tail + head;
    }

    if (len == 0)
    {
        return 0; // empty
    }

    if (len > max)
    {
        len = max;
    }

    uint8_t *p_data = (uint8_t *)ota_env.bin_data;
    if ((tail + len) <= ota_block_size)
    {
        memcpy(buff, p_data + tail, len);
    }
    else
    {
        tlen = ota_block_size - tail;

        memcpy(buff, p_data + tail, tlen);       // tail_len
        memcpy(buff + tlen, p_data, len - tlen); // head_len
    }

    tail          = (tail + len) % ota_block_size;
    ota_env.tail  = tail;
    ota_buff_full = 0; // once data consumed, buffer is no longer full

    return len;        // count
}

void ota_send_data(uint8_t conidx)
{
    uint16_t i;

    if (app_state_get() >= APP_CONNECTED && ota_data_next)
    {
        uint16_t pkt_len = ota_pkt_size;
        DEBUG("idx[%d, %d], pkt_len:%d, total:%d", ota_pkt_idx, ota_block_pkt_nb, pkt_len,
            ota_total_size);
        uint8_t data_pkt[BLE_MTU];

        data_pkt[0] = OTA_CMD_HEAD;
        data_pkt[1] = OTA_CMD_DATA_WC;

        if (ota_total_size < ota_pkt_size ||
            (ota_block_pkt_nb - 1 == (ota_pkt_idx % ota_block_pkt_nb)))
        {
            pkt_len = ota_block_size - (ota_pkt_idx % ota_block_pkt_nb) * ota_pkt_size;

            if (ota_total_size < ota_pkt_size)
            {
                pkt_len = ota_total_size;
                DEBUG("OTA_END:%d, %d", ota_total_size, ota_pkt_size);
            }

            // last packet in block. wait for ack
            data_pkt[1]   = OTA_CMD_DATA_WR;
            ota_data_next = 0;
        }

        write16p(data_pkt + 2, ota_pkt_idx);
        write16p(data_pkt + 4, pkt_len);
        ota_read(data_pkt + OTA_DATA_POS, pkt_len);
        ota_total_size -= pkt_len;

        for (i = 0; i < pkt_len; ++i)
        {
            ota_data_sum += data_pkt[OTA_DATA_POS + i];
        }

        gatt_send_data(conidx, pkt_len + OTA_DATA_POS, data_pkt);
        ++ota_pkt_idx;
    }
}

void ota_ntf_event_proc(uint8_t conidx, const uint8_t *data, uint16_t len)
{
    (void)len;
    switch (ota_fsm_sta)
    {
        case OTA_VER:
        {
            ota_block_size = read16p(data + 4);
            if (ota_block_size > BUFF_LEN)
            {
                ota_fsm_sta = OTA_IDLE;
                gapc_disconnect(conidx);
                break;
            }
            ota_pkt_size     = read16p(data + 6) - OTA_DATA_POS;
            ota_block_pkt_nb = ota_block_size / ota_pkt_size + 1;

            uint32_t reset_hdl = RD_32(OTA_DATA_STORE_POS + 4);
            DEBUG("bin_size:%d, rst_hdl:0x%X", ota_total_size, reset_hdl);
            DEBUG("sz:%d,%d, nb:%d", ota_pkt_size, ota_block_size, ota_block_pkt_nb);
            uint8_t req_start[10] = { OTA_CMD_HEAD, OTA_CMD_START };
            write32p(req_start + 2, ota_total_size);
            write32p(req_start + 6, reset_hdl);
            gatt_send_data(conidx, sizeof(req_start), req_start);
            ota_fsm_sta = OTA_START;

            leds_play(LED_BUSY_BL);
        }
        break;

        case OTA_START:
        {
            read_flash_data(ota_block_size);
            ota_data_next = 1;
            ota_send_data(conidx);
            ota_fsm_sta = OTA_WRITE_DATA;
        }
        break;

        case OTA_WRITE_DATA:
        {
            DEBUG("OTA_WRITE_DATA:%d,%d,%d", ota_pkt_idx, ota_block_pkt_nb, ota_total_size);

            if (ota_total_size == 0)
            {
                ota_data_next = 0;
                ota_fsm_sta   = OTA_END;

                uint32_t total_len     = OTA_DATA_LEN;
                uint8_t  req_block[10] = { OTA_CMD_HEAD, OTA_CMD_END };
                write32p(req_block + 2, total_len);
                write32p(req_block + 6, ota_data_sum);
                gatt_send_data(conidx, sizeof(req_block), req_block);
            }
            else
            {
                read_flash_data(ota_block_size);
                ota_data_next = 1;
                ota_send_data(conidx);
                ota_fsm_sta = OTA_WRITE_DATA;
            }
        }
        break;

        case OTA_END:
        {
            ota_fsm_sta = OTA_IDLE;
            gapc_disconnect(conidx);
        }
        break;

        default:
        {
        }
        break;
    }
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
static void gatt_cmp_proc(uint8_t conidx, uint8_t gatt_op)
{
    switch (gatt_op)
    {
        case GATT_MTU_EXCH:
        {
            uint8_t req_ver[] = { OTA_CMD_HEAD, OTA_CMD_VER };
            gatt_send_data(conidx, sizeof(req_ver), req_ver);
            ota_fsm_sta = OTA_VER;
        }
        break;

        case GATT_DISC_BY_UUID_SVC:
        {
            gatt_disc(conidx, GATT_DISC_ALL_CHAR, ota_env.svc.shdl, ota_env.svc.ehdl,
                ATT_UUID128_LEN, NULL);
        }
        break;

        case GATT_DISC_ALL_CHAR:
        {
            gatt_disc(conidx, GATT_DISC_DESC_CHAR, ota_env.chars[0].val_hdl + 1,
                ota_env.chars[CHAR_IDX_MAX - 1].val_hdl + 1, ATT_UUID16_LEN, NULL);
        }
        break;

        case GATT_DISC_DESC_CHAR:
        {
            gatt_notify_set(conidx, ota_env.descs[CHAR_IDX_NTF].desc_hdl, true);
            ota_ntf_en = 1;
        }
        break;

        case GATT_WRITE:
        {
            if (ota_ntf_en)
            {
                ota_ntf_en = 0;

                gatt_exmtu(conidx, BLE_MTU);
            }
        }
        break;

        default:
        {
        }
        break;
    }
}

APP_MSG_HANDLER(gatt_cmp_evt)
{
    (void)msgid;(void)dest_id;
    uint8_t conidx = TASK_IDX(src_id);
    // operation @see enum gatt_operation
    DEBUG("Cmp_evt(op:%d,sta:%d, fsm:(%d,%d)\r\n", param->operation, param->status, ota_fsm_sta,
        ota_data_next);

    if (ota_data_next && (GATT_WRITE_NO_RESPONSE == param->operation))
    {
        ota_send_data(conidx);
    }
    else if (param->status == GATT_ERR_NO_ERROR)
    {
        gatt_cmp_proc(conidx, param->operation);
    }
}

APP_MSG_HANDLER(gatt_mtu_changed_ind)
{
    (void)msgid;(void)param;(void)dest_id;(void)src_id;
    DEBUG("mtu_chg:%d,seq:%d", param->mtu, param->seq_num);
}

APP_MSG_HANDLER(gatt_disc_svc_ind)
{
    (void)msgid;(void)dest_id;(void)src_id;
    DEBUG("disc_svc(shdl:%d,ehdl:%d,ulen:%d)", param->start_hdl, param->end_hdl, param->uuid_len);
    debugDump("disc_svc", param->uuid, param->uuid_len);

    if ((param->uuid_len == sizeof(ota_svc_uuid)) &&
        (memcmp(param->uuid, ota_svc_uuid, sizeof(ota_svc_uuid)) == 0))
    {
        ota_env.svc.shdl = param->start_hdl;
        ota_env.svc.ehdl = param->end_hdl;
    }
}

APP_MSG_HANDLER(gatt_disc_svc_incl_ind)
{
    (void)msgid;(void)param;(void)dest_id;(void)src_id;
    DEBUG("disc_incl(ahdl:%d,shdl:%d,ehdl:%d,ulen:%d)", param->attr_hdl, param->start_hdl,
        param->end_hdl, param->uuid_len);
    debugDump("disc_incl", param->uuid, param->uuid_len);
}

APP_MSG_HANDLER(gatt_disc_char_ind)
{
    (void)msgid;(void)dest_id;(void)src_id;
    DEBUG("disc_char(ahdl:%d,phdl:%d,prop:%d,ulen:%d)", param->attr_hdl, param->pointer_hdl,
        param->prop, param->uuid_len);
    debugDump("disc_char", param->uuid, param->uuid_len);

    if ((param->uuid_len == sizeof(ota_char_ntf)) &&
        (memcmp(param->uuid, ota_char_ntf, sizeof(ota_char_ntf)) == 0))
    {
        ota_env.chars[CHAR_IDX_NTF].char_hdl = param->attr_hdl;
        ota_env.chars[CHAR_IDX_NTF].val_hdl  = param->pointer_hdl;
        ota_env.chars[CHAR_IDX_NTF].prop     = param->prop;
        DEBUG("Char NTF(chdl:%d, vhdl:%d)", param->attr_hdl, param->pointer_hdl);
    }
    else if ((param->uuid_len == sizeof(ota_char_recv)) &&
             (memcmp(param->uuid, ota_char_recv, sizeof(ota_char_recv)) == 0))
    {
        ota_env.chars[CHAR_IDX_WR].char_hdl = param->attr_hdl;
        ota_env.chars[CHAR_IDX_WR].val_hdl  = param->pointer_hdl;
        ota_env.chars[CHAR_IDX_WR].prop     = param->prop;
        DEBUG("Char WR(chdl:%d, vhdl:%d)", param->attr_hdl, param->pointer_hdl);
    }
}

APP_MSG_HANDLER(gatt_disc_char_desc_ind)
{
    (void)msgid;(void)dest_id;(void)src_id;
    // uint8_t conidx = TASK_IDX(src_id);
    DEBUG("disc_char_desc(ahdl:%d,ulen:%d)", param->attr_hdl, param->uuid_len);
    debugDump("disc_char_desc", param->uuid, param->uuid_len);

    if (param->uuid_len == ATT_UUID16_LEN)
    {
        if ((ATT_DESC_CLIENT_CHAR_CFG == read16p(param->uuid)) &&
            (param->attr_hdl == (ota_env.chars[CHAR_IDX_NTF].val_hdl + 1)))
        {
            ota_env.descs[CHAR_IDX_NTF].desc_hdl = param->attr_hdl;
            DEBUG("desc NTF");
        }
    }
}

APP_MSG_HANDLER(gatt_read_ind)
{
    (void)msgid;(void)param;(void)dest_id;(void)src_id;
    DEBUG("Read_ind(hdl:%d,oft:%d,len:%d)", param->handle, param->offset, param->length);
    debugDump("read_ind", param->value, param->length);
}

APP_MSG_HANDLER(gatt_event_ind)
{
    (void)msgid;(void)dest_id;
    uint8_t conidx = TASK_IDX(src_id);
    DEBUG("notify(typ:%d,hdl:%d,len:%d, fsm:%d, %d)", param->type, param->handle, param->length,
        ota_fsm_sta, ota_total_size);
    debugDump("notify", param->value, param->length);

    uint8_t head = param->value[0];
    uint8_t sta  = param->value[1];

    if ((OTA_RSP_HEAD == head) && (OTA_OK == sta))
    {
        ota_ntf_event_proc(conidx, param->value, param->length);
    }
    else
    {
        gapc_disconnect(conidx);
    }
}

APP_MSG_HANDLER(gatt_event_req_ind)
{
    (void)msgid;(void)dest_id;
    uint8_t conidx = TASK_IDX(src_id);

    DEBUG("indication(typ:%d,hdl:%d,len:%d)", param->type, param->handle, param->length);
    debugDump("indication", param->value, param->length);

    gatt_evt_cfm(conidx, param->handle);
}

/**
 ****************************************************************************************
 * @brief SubTask Handler of GATT Message.
 ****************************************************************************************
 */
APP_SUBTASK_HANDLER(gatt_msg)
{
    switch (msgid)
    {
        case GATT_CMP_EVT:
        {
            APP_MSG_FUNCTION(gatt_cmp_evt);
        }
        break;

        case GATT_MTU_CHANGED_IND:
        {
            APP_MSG_FUNCTION(gatt_mtu_changed_ind);
        }
        break;

        case GATT_DISC_SVC_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_svc_ind);
        }
        break;

        case GATT_DISC_SVC_INCL_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_svc_incl_ind);
        }
        break;

        case GATT_DISC_CHAR_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_char_ind);
        }
        break;

        case GATT_DISC_CHAR_DESC_IND:
        {
            APP_MSG_FUNCTION(gatt_disc_char_desc_ind);
        }
        break;

        case GATT_READ_IND:
        {
            APP_MSG_FUNCTION(gatt_read_ind);
        }
        break;

        case GATT_EVENT_IND:
        {
            APP_MSG_FUNCTION(gatt_event_ind);
        }
        break;

        case GATT_EVENT_REQ_IND:
        {
            APP_MSG_FUNCTION(gatt_event_req_ind);
        }
        break;

        default:
        {
            DEBUG("Unknow MsgId:%d", msgid);
        }
        break;
    }

    return (MSG_STATUS_FREE);
}
#endif //(GATT_CLI)
