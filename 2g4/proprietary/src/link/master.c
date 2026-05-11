#include "master.h"
#include "log.h"
#include "task.h"
#include "mdm.h"
#include "rfctrl.h"
#include "rfrssi.h"
#include "crc.h"
#include "circle.h"
#include "drvs.h"

#define LINK_MASTER_MSGBUF_LENGTH                                                                  \
    CIRCLE_BUFF_LENGTH(sizeof(link_master_message_t), MASTER_MESSAGE_COUNT_POWER)

static uint8_t             link_master_msgbuf[LINK_MASTER_MSGBUF_LENGTH];
static link_master_conf_t  link_master_usrconf;
static link_master_param_t link_master_param;
static link_master_state_t link_master_state;
static circle_buff_t       link_master_msgcntr;

static logout_fn   link_master_logger    = 0;
static log_level_t link_master_log_level = LOG_LEVEL_NONE;

static void link_master_mdm_slot_begin(uint32_t slot_n, uint16_t fine_n, int sync_found);
static void link_master_mdm_sync_error(void);
static void link_master_mdm_rx_done(
    uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n);
static void link_master_mdm_tx_done(uint32_t slot_n, uint16_t fine_n);

static void link_master_service(void *param, uint32_t flag);

#define ERROR(format, ...)                                                                         \
    log_printf(link_master_logger, link_master_log_level, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define WARN(format, ...)                                                                          \
    log_printf(link_master_logger, link_master_log_level, LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define INFO(format, ...)                                                                          \
    log_printf(link_master_logger, link_master_log_level, LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define DEBUG(format, ...)                                                                         \
    log_printf(link_master_logger, link_master_log_level, LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define DBGHEX(data, length)                                                                       \
    log_hex(link_master_logger, link_master_log_level, LOG_LEVEL_DEBUG, (data), (length))

void link_master_init(void)
{
    mdm_cb_t mdm_cb;

    /* Initialize MDM */
    mdm_cb.slot_begin = link_master_mdm_slot_begin;
    mdm_cb.sync_error = link_master_mdm_sync_error;
    mdm_cb.rx_done    = link_master_mdm_rx_done;
    mdm_cb.tx_done    = link_master_mdm_tx_done;

    mdm_init(&mdm_cb);

    rf_rssi_init();

    /* Initialize message buff */
    circle_buff_init(&link_master_msgcntr,
        link_master_msgbuf,
        sizeof(link_master_message_t),
        MASTER_MESSAGE_COUNT_POWER);

    /* Initialize task manager */
    task_init();
}

void link_master_conf(link_master_conf_t *conf, link_master_handler_t *handler)
{
    link_master_logger = handler->log_out;

    if (link_master_logger == 0 || conf->log_level > LOG_LEVEL_ALL)
    {
        link_master_log_level = LOG_LEVEL_NONE;
    }
    else
    {
        link_master_log_level = (log_level_t)conf->log_level;
    }

    INFO("config slot window: %d ms\r\n", conf->link.slot_win);
    mdm_conf_t mconf;

    mconf.access_code = conf->link.access_code;
    mconf.slot_win    = conf->link.slot_win;
    mconf.slot_off    = conf->link.rx_pwrup + conf->link.rx_pathdly + conf->rx_leading_time;
    mconf.sync_win    = 0xFFF;
    mconf.rf_rate     = conf->link.rf_rate;

    mdm_conf(&mconf);
    link_master_usrconf           = *conf;
    link_master_param.prefix_time = 8 + (32 >> conf->link.rf_rate);

    link_master_param.chn_n = 0;

    while (++link_master_param.chn_n < LINK_MAX_CHN)
    {
        if (conf->link.chn_list[link_master_param.chn_n] == LINK_INVALID_CHN)
        {
            break;
        }
    }

    if (link_master_param.chn_n == 1)
    {
        link_master_state.chn     = MASTER_CHN_SELECTED;
        link_master_state.chn_sel = conf->link.chn_list[0];
    }
    else
    {
        link_master_state.chn     = MASTER_CHN_NONE;
        link_master_state.chn_sel = LINK_INVALID_CHN;
    }

    link_master_param.max_detect =
        conf->conflict_detect_period * 1000 / (conf->link.slot_win * RSSI_TABLE_SIZE);

    rssi_table_reset(&link_master_state.rssi_tab);

    link_master_state.rtx           = MASTER_IDLE;
    link_master_state.slot_n        = 0;
    link_master_state.detect_count  = 0;
    link_master_state.detect_errors = 0;

    for (int i = 0; i < LINK_MAX_CONN; i++)
    {
        link_master_state.conn[i].address = 0;
    }

    task_conf(link_master_service, 0, handler->user_proc, handler->param, handler->flag);
}

void link_master_schedule(void)
{
    INFO("master start...\r\n");

    task_invoke();
}

int link_master_fetch_message(link_master_message_t *msg)
{
    uint16_t size = sizeof(link_master_message_t);

    if (circle_buff_read(&link_master_msgcntr, msg, &size) < 0)
    {
        return -1;
    }

    return circle_buff_count(&link_master_msgcntr);
}

int link_master_piggyback(uint8_t cid, uint8_t *data, uint16_t length)
{
    if (cid >= LINK_MAX_CONN || length > MASTER_PIGGYBACK_LIMIT)
    {
        return -1;
    }

    link_master_conn_t *conn = &link_master_state.conn[cid - 1];

    if (conn->address == 0 || conn->piggyback[0] != 0)
    {
        return -1;
    }

    uint8_t *p   = conn->piggyback + 1;
    uint8_t  len = length | 0x80;

    while (length--)
    {
        *p++ = *data++;
    }

    conn->piggyback[0] = len;

    return 0;
}

static void link_master_mdm_slot_begin(uint32_t slot_n, uint16_t fine_n, int sync_found)
{
    if (RF->PLL_DYM_CTRL.SW_RX_EN && (!RF->RF_ANA_ST0.PLL_LOCK)) // PLL un-lock
    {
        rf_reset();
        INFO("master rf reset(2)\r\n");
    }

    if (link_master_state.rtx == MASTER_RECV_DATA && !sync_found)
    {
        rssi_table_push(&link_master_state.rssi_tab, rf_rssi_read());
        mdm_rtx_disable();
        mdm_rx_enable(link_master_state.chn_sel, link_master_usrconf.link.rf_rate);
    }

    if (link_master_state.rtx == MASTER_IDLE)
    {
        mdm_rx_enable(link_master_state.chn_sel, link_master_usrconf.link.rf_rate);
        link_master_state.rtx = MASTER_RECV_DATA;
    }

    link_master_state.slot_n = slot_n;
    task_event(MASTER_EVENT_SLOT_BEGIN);
}

static void link_master_mdm_rx_done(
    uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n)
{
    rssi_table_push(&link_master_state.rssi_tab, rf_rssi_read());

    mdm_rtx_disable();

    if (link_master_state.chn == MASTER_CHN_SELECTED)
    {
        link_master_state.recv_buff   = data;
        link_master_state.recv_length = length;
        link_master_state.recv_slot_n = slot_n;
        link_master_state.recv_fine_n = fine_n;

        link_master_state.rtx = MASTER_SEND_ACK;
    }
    else
    {
        link_master_state.rtx = MASTER_IDLE;
    }

    task_event(MASTER_EVENT_RX_DONE);
}

static void link_master_mdm_tx_done(uint32_t slot_n, uint16_t fine_n)
{
    mdm_rtx_disable();
    link_master_state.rtx = MASTER_IDLE;
}

static void link_master_mdm_sync_error(void)
{
    mdm_rtx_disable();
    link_master_state.rtx = MASTER_IDLE;

    rf_ctrl_init();
}

static inline int link_master_put_message_connected(uint32_t address, uint8_t cid, link_tos_t tos)
{
    link_master_message_t msg;

    msg.type          = MASTER_MESSAGE_CONNECTION_CREATED;
    msg.cid           = cid;
    msg.address       = address;
    msg.body.conn.tos = tos;

    return circle_buff_write(&link_master_msgcntr, &msg, sizeof(link_master_message_t));
}

static inline int link_master_put_message_closed(uint32_t address, uint8_t cid, link_tos_t tos)
{
    link_master_message_t msg;

    msg.type          = MASTER_MESSAGE_CONNECTION_CLOSED;
    msg.cid           = cid;
    msg.address       = address;
    msg.body.conn.tos = tos;

    return circle_buff_write(&link_master_msgcntr, &msg, sizeof(link_master_message_t));
}

static inline int link_master_put_message_received(
    uint32_t address, uint8_t cid, uint8_t *data, uint16_t length)
{
    link_master_message_t msg;
    uint8_t              *p = msg.body.buff.data;

    msg.type             = MASTER_MESSAGE_DATA_RECEIVED;
    msg.cid              = cid;
    msg.address          = address;
    msg.body.buff.length = length;

    while (length--)
    {
        *p++ = *data++;
    }

    return circle_buff_write(&link_master_msgcntr, &msg, sizeof(link_master_message_t));
}

static inline int link_master_put_message_sent(uint32_t address, uint8_t cid)
{
    link_master_message_t msg;

    msg.type             = MASTER_MESSAGE_DATA_SENT;
    msg.cid              = cid;
    msg.address          = address;
    msg.body.buff.length = 0;

    return circle_buff_write(&link_master_msgcntr, &msg, sizeof(link_master_message_t));
}

static int link_mastet_validate_packet(void)
{
    link_packet_t *packet = (link_packet_t *)link_master_state.recv_buff;

    if (LINK_PACKET_LENGTH(packet) > link_master_state.recv_length)
    {
        return -1;
    }

    if (packet->dir != LINK_PACKET_DIR_TO_MASTER)
    {
        return -1;
    }

    if (packet->cid && !link_master_state.conn[packet->cid - 1].address)
    {
        return -1;
    }

    if (!packet->cid && packet->type == LINK_PACKET_TYPE_DATA)
    {
        return -1;
    }

    uint8_t *p = ((uint8_t *)(packet + 1)) + packet->len;
#ifdef CFG_CRC8
    uint8_t crc = p[0];

    if (crc != crc8(link_master_state.recv_buff, sizeof(link_packet_t) + packet->len))
#else
    uint32_t crc = p[0] | (p[1] << 8) | (p[2] << 16);

    if (crc != crc24(link_master_state.recv_buff, sizeof(link_packet_t) + packet->len))
#endif
    {
        INFO("master CRC Error\r\n");
        return -1;
    } /*else{

         INFO("master CRC PASS\r\n");
     }*/

    return 0;
}

static link_slot_offset_t link_master_calc_offset(uint16_t target)
{
    link_slot_offset_t offset;
    offset.slot    = (link_master_state.recv_slot_n - target) & 0x03;
    uint16_t begin = link_master_state.recv_fine_n + link_master_param.prefix_time;

    if (begin >= link_master_usrconf.link.slot_win)
    {
        offset.slot = (offset.slot - 1) & 0x03;

        offset.fine = link_master_usrconf.link.slot_win * 2 - begin - 1;
    }
    else
    {
        offset.fine = link_master_usrconf.link.slot_win - begin - 1;
    }

    return offset;
}

static uint8_t link_master_create_connection(uint32_t address, link_tos_t tos, uint32_t slot_n)
{
    uint8_t cid = 0;

    for (int i = 0; i < LINK_MAX_CONN; i++)
    {
        if (link_master_state.conn[i].address == address)
        {
            cid = i + 1;
            break;
        }
        else if (!cid && link_master_state.conn[i].address == 0)
        {
            cid = i + 1;
        }
    }

    if (cid != 0)
    {
        link_master_conn_t *conn = &link_master_state.conn[cid - 1];

        conn->address           = address;
        conn->tos               = tos;
        conn->seq               = 0;
        conn->last_n            = slot_n;
        conn->assign.exclusive  = 0;
        conn->assign.preemptive = 0x0F;
        conn->piggyback[0]      = 0;
    }

    return cid;
}

static void link_master_ack_delay(void)
{
    uint32_t slot_n  = link_master_state.recv_slot_n;
    uint16_t fine_n  = link_master_state.recv_fine_n;
    uint16_t rx_time = link_master_state.recv_length >> link_master_usrconf.link.rf_rate;

    uint16_t delay_n = link_master_usrconf.ack_delay + rx_time;

    if (fine_n >= delay_n)
    {
        fine_n -= delay_n;
    }
    else
    {
        slot_n++;
        fine_n += link_master_usrconf.link.slot_win - delay_n;
    }

    mdm_wait_slot(slot_n, fine_n);
}

static inline void link_master_attach_piggyback(link_master_conn_t *conn, link_packet_t *ack)
{
    uint8_t  len = conn->piggyback[0] & 0x7f;
    uint8_t *p   = (uint8_t *)(ack + 1) + sizeof(link_packet_payload_ack_t);

    for (uint8_t i = 1; i <= len; i++)
    {
        *p++ = conn->piggyback[i];
    }

    ack->len += len;
}

static int link_master_handle_piggyback(
    link_master_conn_t *conn, link_packet_t *packet, link_packet_t *ack)
{
    int rt = 0;

    if (packet->len == 0)
    {
        if ((conn->piggyback[0] & 0x80) == 0)
        {
            if (packet->seq == conn->seq)
            {
                conn->piggyback[0] |= 0x80;
            }
            else
            {
                conn->piggyback[0] = 0;
                rt                 = 1;
            }
        }
    }
    else
    {
        if (conn->piggyback[0] & 0x80)
        {
            link_master_attach_piggyback(conn, ack);
            conn->piggyback[0] &= 0x7f;
        }
        else if (packet->seq == conn->seq)
        {
            link_master_attach_piggyback(conn, ack);
        }
        else
        {
            conn->piggyback[0] = 0;
            rt                 = 1;
        }
    }

    return rt;
}

static int link_master_handle_recv(void)
{
    uint8_t        buff[LINK_PACKET_MAX_LENGTH];
    link_packet_t *ack = (link_packet_t *)buff;

    link_packet_t *packet = (link_packet_t *)link_master_state.recv_buff;

    ack->dir  = LINK_PACKET_DIR_TO_SLAVE;
    ack->cid  = packet->cid;
    ack->pos  = packet->pos;
    ack->type = packet->type;
    ack->seq  = packet->seq;

    if (packet->type == LINK_PACKET_TYPE_CONN)
    {
        // Create connection
        link_packet_payload_req_t *req = (link_packet_payload_req_t *)(packet + 1);

        uint32_t address =
            req->address[0] | req->address[1] << 8 | req->address[2] << 16 | req->address[3] << 24;

        if (address == 0)
        {
            return -1;
        }

        uint8_t cid =
            link_master_create_connection(address, req->tos, link_master_state.recv_slot_n);

        if (cid == 0)
        {
            return -1;
        }

        link_master_conn_t *conn = &link_master_state.conn[cid - 1];

        ack->len                           = sizeof(link_packet_payload_rsp_t);
        link_packet_payload_rsp_t *payload = (link_packet_payload_rsp_t *)(ack + 1);
        payload->assign                    = conn->assign;
        payload->offset                    = link_master_calc_offset(packet->pos);
        payload->address[0]                = req->address[0];
        payload->address[1]                = req->address[1];
        payload->address[2]                = req->address[2];
        payload->address[3]                = req->address[3];
        payload->cid                       = cid;

        link_master_put_message_connected(address, cid, conn->tos);

        if (++link_master_state.mdm_err_cnt > 5)
        {
            link_master_state.mdm_err_cnt = 0;
            rf_reset();
            INFO("master rf reset(1)\r\n");
        }
    }
    else // packet->type == LINK_PACKET_TYPE_DATA
    {
        link_master_conn_t *conn = &link_master_state.conn[packet->cid - 1];

        ack->len                           = sizeof(link_packet_payload_ack_t);
        link_packet_payload_ack_t *payload = (link_packet_payload_ack_t *)(ack + 1);
        payload->assign                    = conn->assign;
        payload->offset                    = link_master_calc_offset(packet->pos);
        conn->last_n                       = link_master_state.recv_slot_n;

        int piggy = 0;

        if (conn->piggyback[0] != 0)
        {
            piggy = link_master_handle_piggyback(conn, packet, ack);
        }

        if (packet->seq != conn->seq && packet->len != 0)
        {
            link_master_put_message_received(conn->address,
                packet->cid,
                link_master_state.recv_buff + sizeof(link_packet_t),
                packet->len);
        }

        if (piggy)
        {
            link_master_put_message_sent(conn->address, packet->cid);
        }

        conn->seq = packet->seq;

        link_master_state.mdm_err_cnt = 0;
    }

#ifdef CFG_CRC8
    uint8_t  crc = crc8(buff, sizeof(link_packet_t) + ack->len);
    uint8_t *p   = &buff[sizeof(link_packet_t) + ack->len];
    *p           = crc;
#else
    uint32_t crc = crc24(buff, sizeof(link_packet_t) + ack->len);
    uint8_t *p   = &buff[sizeof(link_packet_t) + ack->len];
    *p++         = (crc >> 0) & 0xFF;
    *p++         = (crc >> 8) & 0xFF;
    *p           = (crc >> 16) & 0xFF;
#endif
    link_master_ack_delay();

    mdm_tx_send(link_master_state.chn_sel, link_master_usrconf.link.rf_rate,
        link_master_usrconf.link.access_code, buff, sizeof(link_packet_t) + ack->len + 3);

    if (packet->type == LINK_PACKET_TYPE_CONN)
    {
        link_packet_payload_rsp_t *payload = (link_packet_payload_rsp_t *)(ack + 1);
        INFO("connected from %02x%02x%02x%02x, cid=%d, assign=%02x\r\n", payload->address[3],
            payload->address[2], payload->address[1], payload->address[0], payload->cid,
            payload->assign.preemptive << 4 | payload->assign.exclusive);
    }

    return 0;
}

static void link_master_manager_connection(void)
{
    for (int i = 0; i < LINK_MAX_CONN; i++)
    {
        link_master_conn_t *conn = &link_master_state.conn[i];

        if (conn->address != 0)
        {
            uint32_t live = link_master_state.slot_n >= conn->last_n
                                ? link_master_state.slot_n - conn->last_n
                                : MDM_SLOT_N_MASK + 1 + link_master_state.slot_n - conn->last_n;

            if (live > link_master_usrconf.conn_alive)
            {
                link_master_put_message_closed(conn->address, i + 1, conn->tos);

                INFO("disconnect slave, address=%08x\r\n", conn->address);
                conn->address = 0;
            }
        }
    }
}

static void link_master_monitor_chn(void)
{
    median_res_t res;
    rssi_table_median(&link_master_state.rssi_tab, &res);

    if (res.median - link_master_state.chn_ref.median > link_master_usrconf.rssi_median_thresh ||
        res.peak - link_master_state.chn_ref.peak > link_master_usrconf.rssi_peak_thresh)
    {
        link_master_state.detect_errors++;
    }

    link_master_state.detect_count++;

    if (link_master_state.detect_count >= link_master_param.max_detect)
    {
        if (link_master_param.chn_n > 1 &&
            (link_master_state.detect_errors * 100 >=
                link_master_usrconf.conflict_detect_thresh * link_master_state.detect_count))
        {
            link_master_state.chn = MASTER_CHN_NONE;
            WARN("conflict detected, chn=%d, err=%d\r\n", link_master_state.chn_sel,
                link_master_state.detect_errors);
        }

        link_master_state.detect_errors = 0;
        link_master_state.detect_count  = 0;
    }

    rssi_table_reset(&link_master_state.rssi_tab);
}

static void link_master_prepare_scanning(void)
{
    for (int i = 0; i < LINK_MAX_CHN; i++)
    {
        link_master_state.chn_rssi[i].median = 0xFFFF;
        link_master_state.chn_rssi[i].peak   = 0xFFFF;
    }

    rssi_table_reset(&link_master_state.rssi_tab);

    link_master_state.chn     = MASTER_CHN_SCANNING;
    link_master_state.chn_sel = link_master_usrconf.link.chn_list[0];
}

static int link_master_select_chn(void)
{
    uint16_t max_peak, min_peak, ref_peak;
    max_peak = link_master_state.chn_rssi[0].peak;
    min_peak = link_master_state.chn_rssi[0].peak;

    for (int i = 1; i < link_master_param.chn_n; i++)
    {
        if (max_peak < link_master_state.chn_rssi[i].peak)
        {
            max_peak = link_master_state.chn_rssi[i].peak;
        }
        else if (min_peak > link_master_state.chn_rssi[i].peak)
        {
            min_peak = link_master_state.chn_rssi[i].peak;
        }
    }

    ref_peak = (max_peak + min_peak * 2) / 3;

    int      idx    = -1;
    uint16_t lowest = 0xFFFF;

    for (int i = 0; i < link_master_param.chn_n; i++)
    {
        if (link_master_state.chn_rssi[i].peak <= ref_peak &&
            link_master_state.chn_rssi[i].median < lowest)
        {
            lowest = link_master_state.chn_rssi[i].median;
            idx    = i;
        }
    }

    return idx;
}

static void link_master_handle_scanned(void)
{
    int idx = 0;

    while (idx < link_master_param.chn_n)
    {
        if (link_master_state.chn_sel == link_master_usrconf.link.chn_list[idx])
        {
            break;
        }

        idx++;
    }

    rssi_table_median(&link_master_state.rssi_tab, &link_master_state.chn_rssi[idx]);
    rssi_table_reset(&link_master_state.rssi_tab);

    idx = idx + 1;

    if (idx < link_master_param.chn_n)
    {
        link_master_state.chn_sel = link_master_usrconf.link.chn_list[idx];
    }
    else
    {
        int sel                         = link_master_select_chn();
        link_master_state.chn_sel       = link_master_usrconf.link.chn_list[sel];
        link_master_state.chn_ref       = link_master_state.chn_rssi[sel];
        link_master_state.detect_count  = 0;
        link_master_state.detect_errors = 0;
        link_master_state.chn           = MASTER_CHN_SELECTED;
        INFO("channel selected, idx = %d\r\n", link_master_state.chn_sel);
    }
}

static void link_master_service(void *param, uint32_t flag)
{
    if (link_master_state.chn == MASTER_CHN_SELECTED)
    {
        if (flag & MASTER_EVENT_RX_DONE)
        {
            if (link_mastet_validate_packet() || link_master_handle_recv())
            {
                link_master_state.rtx = MASTER_IDLE;
            }
        }

        if (flag & MASTER_EVENT_SLOT_BEGIN)
        {
            link_master_manager_connection();

            if (rssi_table_remains(&link_master_state.rssi_tab) == 0)
            {
                link_master_monitor_chn();
            }
        }
    }
    else
    {
        if (flag & MASTER_EVENT_SLOT_BEGIN)
        {
            if (link_master_state.chn == MASTER_CHN_NONE)
            {
                link_master_prepare_scanning();
            }
            else if (rssi_table_remains(&link_master_state.rssi_tab) == 0)
            {
                link_master_handle_scanned();
            }
        }
    }
}
