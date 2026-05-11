#include "slave.h"
#include "log.h"
#include "task.h"
#include "mdm.h"
#include "rfctrl.h"
#include "crc.h"
#include "drvs.h"

#define LINK_SLAVE_SENDBUF_LENGTH CIRCLE_BUFF_LENGTH(SLAVE_SEND_MAX_LENGTH, SLAVE_SEND_COUNT_POWER)
#define LINK_SLAVE_PIGGYBUF_LENGTH                                                                 \
    CIRCLE_BUFF_LENGTH(SLAVE_PIGGYBACK_MAX_LENGTH, SLAVE_PIGGYBACK_COUNT_POWER)

static uint8_t link_slave_sendbuf[LINK_SLAVE_SENDBUF_LENGTH];
static uint8_t link_slave_piggybuf[LINK_SLAVE_PIGGYBUF_LENGTH];

static link_slave_conf_t  link_slave_usrconf;
static link_slave_state_t link_slave_state;
static logout_fn          link_slave_logger    = 0;
static log_level_t        link_slave_log_level = LOG_LEVEL_NONE;

static void link_slave_mdm_slot_begin(uint32_t slot_n, uint16_t fine_n, int sync_found);
static void link_slave_mdm_sync_error(void);
static void link_slave_mdm_rx_done(
    uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n);
static void link_slave_mdm_tx_done(uint32_t slot_n, uint16_t fine_n);

static void link_slave_service(void *param, uint32_t flag);
static void link_slave_conn_proc(void *param, uint32_t flag);
static void link_slave_data_proc(void *param, uint32_t flag);
static void link_slave_build_req(uint32_t address, link_tos_t tos);
static void link_slave_close_connection(void);

#define ERROR(format, ...)                                                                         \
    log_printf(link_slave_logger, link_slave_log_level, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define WARN(format, ...)                                                                          \
    log_printf(link_slave_logger, link_slave_log_level, LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define INFO(format, ...)                                                                          \
    log_printf(link_slave_logger, link_slave_log_level, LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define DEBUG(format, ...)                                                                         \
    log_printf(link_slave_logger, link_slave_log_level, LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define DBGHEX(data, length)                                                                       \
    log_hex(link_slave_logger, link_slave_log_level, LOG_LEVEL_DEBUG, (data), (length))

void link_slave_init(void)
{
    mdm_cb_t mdm_cb;
    /* Initialize MDM */
    mdm_cb.slot_begin = link_slave_mdm_slot_begin;
    mdm_cb.sync_error = link_slave_mdm_sync_error;
    mdm_cb.rx_done    = link_slave_mdm_rx_done;
    mdm_cb.tx_done    = link_slave_mdm_tx_done;

    mdm_init(&mdm_cb);

    /* Initialize task manager */
    task_init();
}

void link_slave_conf(link_slave_conf_t *conf, link_slave_handler_t *handler)
{
    link_slave_logger = handler->log_out;

    if (link_slave_logger == 0 || conf->log_level > LOG_LEVEL_ALL)
    {
        link_slave_log_level = LOG_LEVEL_NONE;
    }
    else
    {
        link_slave_log_level = (log_level_t)conf->log_level;
    }

    mdm_conf_t mconf;

    mconf.access_code = conf->link.access_code;
    mconf.slot_win    = conf->link.slot_win;
    mconf.slot_off    = conf->link.tx_pwrup;
    mconf.sync_win    = conf->link.slot_win >> 1;

    mdm_conf(&mconf);
    link_slave_usrconf = *conf;

    link_slave_state.conn = SLAVE_DISCONNECTED;
    link_slave_state.cid  = 0;

    link_slave_state.rtx = SLAVE_IDLE;

    circle_buff_init(&link_slave_state.send_queue, link_slave_sendbuf, SLAVE_SEND_MAX_LENGTH,
        SLAVE_SEND_COUNT_POWER);

    link_slave_state.send_seq   = 0;
    link_slave_state.send_retry = 0;

    circle_buff_init(&link_slave_state.piggy_queue, link_slave_piggybuf, SLAVE_PIGGYBACK_MAX_LENGTH,
        SLAVE_PIGGYBACK_COUNT_POWER);

    link_slave_state.slot_n = 0;

    link_slave_build_req(conf->address, conf->tos);

    link_slave_state.chn_sel = conf->link.chn_list[0];

    task_conf(link_slave_service, 0, handler->user_proc, handler->param, handler->flag);
}

void link_slave_schedule(void)
{
    INFO("slave start...\r\n");

    task_invoke();
}

int link_slave_connect(void)
{
    if (link_slave_state.conn == SLAVE_DISCONNECTED)
    {
        task_syscall(SLAVE_SYSCALL_CONNECT);
    }
    while (link_slave_state.conn == SLAVE_CONNECTING)
    {
    }

    return (link_slave_state.conn == SLAVE_CONNECTED ? link_slave_state.cid : -1);
}

int link_slave_send(uint8_t *payload, uint16_t length)
{
    link_slave_state.send_payload = payload;
    link_slave_state.send_length  = length;
    link_slave_state.send_rtcode  = 0;

    task_syscall(SLAVE_SYSCALL_SEND);

    return link_slave_state.send_rtcode;
}

int link_slave_recv(link_slave_piggy_t *piggy)
{
    piggy->length = SLAVE_PIGGYBACK_MAX_LENGTH;

    return circle_buff_read(&link_slave_state.piggy_queue, piggy->data, &piggy->length);
}

static void link_slave_mdm_slot_begin(uint32_t slot_n, uint16_t fine_n, int sync_found)
{
    link_slave_state.slot_n = slot_n + 1;

    task_event(SLAVE_EVENT_SLOT_BEGIN);
}

static void link_slave_mdm_sync_error(void)
{
    mdm_rtx_disable();

    task_event(SLAVE_EVENT_SYNC_ERROR);
}

static void link_slave_mdm_rx_done(uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n)
{
    mdm_rtx_disable();

    link_slave_state.recv_buff   = data;
    link_slave_state.recv_length = length;
    link_slave_state.recv_slot_n = slot_n;
    link_slave_state.recv_fine_n = fine_n;

    task_event(SLAVE_EVENT_RX_DONE);
}

static void link_slave_mdm_tx_done(uint32_t slot_n, uint16_t fine_n)
{
    mdm_rtx_disable();

    link_slave_state.sent_slot_n = slot_n;
    link_slave_state.sent_fine_n = fine_n;

    task_event(SLAVE_EVENT_TX_DONE);
}

static void link_slave_build_req(uint32_t address, link_tos_t tos)
{
    link_packet_t *packet = (link_packet_t *)link_slave_state.req_buff;

    packet->dir  = LINK_PACKET_DIR_TO_MASTER;
    packet->cid  = 0;
    packet->pos  = 0;
    packet->type = LINK_PACKET_TYPE_CONN;
    packet->seq  = 0;
    packet->len  = sizeof(link_packet_payload_req_t);

    link_packet_payload_req_t *payload = (link_packet_payload_req_t *)(packet + 1);

    payload->address[0] = address & 0xFF;
    payload->address[1] = (address >> 8) & 0xFF;
    payload->address[2] = (address >> 16) & 0xFF;
    payload->address[3] = (address >> 24) & 0xFF;

    payload->tos = tos;

#ifdef CFG_CRC8
    uint8_t crc =
        crc8(link_slave_state.req_buff, sizeof(link_packet_t) + sizeof(link_packet_payload_req_t));

    uint8_t *p = (uint8_t *)(payload + 1);
    *p         = crc;
#else
    uint32_t crc =
        crc24(link_slave_state.req_buff, sizeof(link_packet_t) + sizeof(link_packet_payload_req_t));

    uint8_t *p = (uint8_t *)(payload + 1);
    *p++       = (crc >> 0) & 0xFF;
    *p++       = (crc >> 8) & 0xFF;
    *p         = (crc >> 16) & 0xFF;
#endif
}

static void link_slave_create_connection(void)
{
    INFO("start connect to master\r\n");

    link_slave_state.conn      = SLAVE_CONNECTING;
    link_slave_state.req_retry = 0;

    mdm_retrieve_slot(&link_slave_state.req_slot_n, &link_slave_state.req_fine_n);

    mdm_tx_send(link_slave_state.chn_sel,
        link_slave_usrconf.link.rf_rate,
        link_slave_usrconf.link.access_code,
        link_slave_state.req_buff,
        SLAVE_CONN_REQ_LENGTH);
}

static void link_slave_close_connection(void)
{
    INFO("close connection, retry=%d\r\n", link_slave_state.send_retry);

    link_slave_state.conn = SLAVE_DISCONNECTED;
    link_slave_state.cid  = 0;
}

static void link_slave_send_userdata(void)
{
    uint8_t  buff[SLAVE_SEND_MAX_LENGTH];
    uint8_t *payload = link_slave_state.send_payload;
    uint16_t length  = link_slave_state.send_length;

    link_packet_t *packet = (link_packet_t *)buff;

    packet->dir  = LINK_PACKET_DIR_TO_MASTER;
    packet->type = LINK_PACKET_TYPE_DATA;
    packet->len  = length;

    uint8_t *p = &buff[sizeof(link_packet_t)];
    while (length--)
    {
        *p++ = *payload++;
    }

    link_slave_state.send_rtcode =
        circle_buff_write(&link_slave_state.send_queue, buff, p - buff + LINK_PACKET_CRC_LENGTH);
    if (link_slave_state.send_rtcode == 0)
    {
        link_slave_state.heartbeat = 0;
    }
}

static void link_slave_heartbeat(void)
{
    uint8_t        buff[SLAVE_SEND_MAX_LENGTH];
    link_packet_t *packet = (link_packet_t *)buff;

    packet->dir  = LINK_PACKET_DIR_TO_MASTER;
    packet->type = LINK_PACKET_TYPE_DATA;
    packet->len  = 0;

    circle_buff_write(
        &link_slave_state.send_queue, buff, sizeof(link_packet_t) + LINK_PACKET_CRC_LENGTH);

    link_slave_state.send_retry = 0;
    link_slave_state.heartbeat++;
}

static void link_slave_service(void *param, uint32_t flag)
{
    if (flag & TASK_SYSCALL_EVENT(SLAVE_SYSCALL_CONNECT))
    {
        link_slave_create_connection();
    }

    if (flag & TASK_SYSCALL_EVENT(SLAVE_SYSCALL_SEND))
    {
        link_slave_send_userdata();
    }

    if (link_slave_state.conn == SLAVE_CONNECTING)
    {
        link_slave_conn_proc(param, flag);
    }
    else if (link_slave_state.conn == SLAVE_CONNECTED)
    {
        link_slave_data_proc(param, flag);
    }
    else if ((flag & SLAVE_EVENT_SLOT_BEGIN) &&
             circle_buff_count(&link_slave_state.send_queue) != 0)
    {
        link_slave_create_connection();
    }
}

static void link_slave_rtx_gap(void)
{
    uint32_t slot_n = link_slave_state.sent_slot_n;
    uint16_t fine_n = link_slave_state.sent_fine_n;
    if (fine_n >= link_slave_usrconf.rtx_gap)
    {
        fine_n -= link_slave_usrconf.rtx_gap;
    }
    else
    {
        slot_n++;
        fine_n += link_slave_usrconf.link.slot_win - link_slave_usrconf.rtx_gap;
    }
    mdm_wait_slot(slot_n, fine_n);
}

static int link_slave_conn_validate_packet(void)
{
    link_packet_t *packet = (link_packet_t *)link_slave_state.recv_buff;

    if (LINK_PACKET_LENGTH(packet) > link_slave_state.recv_length)
    {
        DEBUG("conn ack len err\r\n");

        return -1;
    }
    if (packet->dir != LINK_PACKET_DIR_TO_SLAVE)
    {
        DEBUG("conn ack dir err\r\n");

        return -1;
    }
    if (packet->cid != 0)
    {
        DEBUG("conn ack cid err\r\n");

        return -1;
    }
    if (packet->type != LINK_PACKET_TYPE_CONN)
    {
        DEBUG("conn ack type err\r\n");

        return -1;
    }

    uint8_t *p = ((uint8_t *)(packet + 1)) + packet->len;

#ifdef CFG_CRC8
    uint8_t crc = p[0];
    if (crc != crc8(link_slave_state.recv_buff, sizeof(link_packet_t) + packet->len))
#else
    uint32_t crc = p[0] | (p[1] << 8) | (p[2] << 16);
    if (crc != crc24(link_slave_state.recv_buff, sizeof(link_packet_t) + packet->len))
#endif
    {
        DEBUG("conn ack crc err\r\n");
        return -1;
    }

    return 0;
}

static int link_slave_conn_handle_ack(void)
{
    link_packet_t             *packet = (link_packet_t *)link_slave_state.recv_buff;
    link_packet_payload_rsp_t *rsp    = (link_packet_payload_rsp_t *)(packet + 1);
    uint32_t                   address =
        rsp->address[0] | rsp->address[1] << 8 | rsp->address[2] << 16 | rsp->address[3] << 24;
    if (address != link_slave_usrconf.address)
    {
        DEBUG("conn ack addr err\r\n");

        return -1;
    }
    if (rsp->cid == 0)
    {
        DEBUG("conn ack alloc cid err\r\n");

        return -1;
    }

    link_slave_state.assign = rsp->assign;
    link_slave_state.cid    = rsp->cid;

    uint32_t slot_off = (~link_slave_state.req_slot_n + rsp->offset.slot) & 0x03;
    uint16_t fine_off = link_slave_state.req_fine_n + rsp->offset.fine;

    if (fine_off >= link_slave_usrconf.link.tx_pwrup)
    {
        fine_off -= link_slave_usrconf.link.tx_pwrup;
    }
    else
    {
        fine_off += link_slave_usrconf.link.slot_win - link_slave_usrconf.link.tx_pwrup;
        slot_off  = (slot_off - 1) & 0x03;
    }

    mdm_update_slot(slot_off, fine_off);

    link_slave_state.send_seq   = 1;
    link_slave_state.send_retry = 0;

    link_slave_state.conn = SLAVE_CONNECTED;

    INFO("connection created, chn=%d, cid=%d, assign=%2.2x, retry=%d\r\n",
        link_slave_state.chn_sel,
        link_slave_state.cid,
        link_slave_state.assign.preemptive << 4 | link_slave_state.assign.exclusive,
        link_slave_state.req_retry);

    return 0;
}

static inline void link_slave_conn_next_chn(void)
{
    int idx = 0;
    while (idx < LINK_MAX_CHN)
    {
        if (link_slave_state.chn_sel == link_slave_usrconf.link.chn_list[idx])
        {
            break;
        }
        idx++;
    }
    idx++;
    if (idx >= LINK_MAX_CHN || link_slave_usrconf.link.chn_list[idx] == LINK_INVALID_CHN)
    {
        idx = 0;
    }
    link_slave_state.chn_sel = link_slave_usrconf.link.chn_list[idx];
}

static void link_slave_conn_handle_retry(void)
{
    DEBUG("connect retry %d\r\n", link_slave_state.req_retry + 1);

    link_slave_state.req_retry++;
    if (link_slave_state.req_retry >= link_slave_usrconf.chn_retry)
    {
        link_slave_state.req_retry = 0;
        link_slave_conn_next_chn();
    }

    mdm_retrieve_slot(&link_slave_state.req_slot_n, &link_slave_state.req_fine_n);

    mdm_tx_send(link_slave_state.chn_sel,
        link_slave_usrconf.link.rf_rate,
        link_slave_usrconf.link.access_code,
        link_slave_state.req_buff,
        SLAVE_CONN_REQ_LENGTH);
}

static void link_slave_conn_proc(void *param, uint32_t flag)
{
    if (flag & SLAVE_EVENT_SYNC_ERROR)
    {
        DEBUG("conn no ack\r\n");

        link_slave_conn_handle_retry();
    }
    else if (flag & SLAVE_EVENT_RX_DONE)
    {
        if (link_slave_conn_validate_packet() || link_slave_conn_handle_ack())
        {
            DBGHEX(link_slave_state.recv_buff, link_slave_state.recv_length);

            link_slave_conn_handle_retry();
        }
    }
    else if (flag & SLAVE_EVENT_SLOT_BEGIN)
    {
        uint32_t slot_n = link_slave_state.slot_n;
        if (slot_n < link_slave_state.req_slot_n)
        {
            slot_n += 1UL << 28;
        }

        if (slot_n > link_slave_state.req_slot_n + 2)
        {
            rf_reset();
            link_slave_conn_handle_retry();
            // INFO("rf reset\r\n");
        }
    }

    if (flag & SLAVE_EVENT_TX_DONE)
    {
        link_slave_rtx_gap();

        mdm_rx_enable(link_slave_state.chn_sel, link_slave_usrconf.link.rf_rate);
    }
}

static int link_slave_data_validate_packet(void)
{
    link_packet_t *packet = (link_packet_t *)link_slave_state.recv_buff;
    if (LINK_PACKET_LENGTH(packet) > link_slave_state.recv_length)
    {
        DEBUG("data ack len err\r\n");
        return -1;
    }
    if (packet->dir != LINK_PACKET_DIR_TO_SLAVE)
    {
        DEBUG("data ack dir err\r\n");
        return -1;
    }
    if (packet->cid != link_slave_state.cid)
    {
        DEBUG("data ack cid err\r\n");
        return -1;
    }
    if (packet->type != LINK_PACKET_TYPE_DATA)
    {
        DEBUG("data ack type err\r\n");
        return -1;
    }

    uint8_t *p = ((uint8_t *)(packet + 1)) + packet->len;
#ifdef CFG_CRC8
    uint8_t crc = p[0];
    if (crc != crc8(link_slave_state.recv_buff, sizeof(link_packet_t) + packet->len))
#else
    uint32_t crc = p[0] | (p[1] << 8) | (p[2] << 16);
    if (crc != crc24(link_slave_state.recv_buff, sizeof(link_packet_t) + packet->len))
#endif
    {
        DEBUG("data ack crc err\r\n");
        return -1;
    }

    return 0;
}

static void link_slave_data_adjust_timing(uint32_t slot_off, uint16_t fine_off)
{
    uint16_t interval = 8 + (((link_slave_state.send_length + LINK_PACKET_CRC_LENGTH) * 8 + 32) >>
                                link_slave_usrconf.link.rf_rate);
    int16_t send_off = link_slave_usrconf.link.slot_win - (link_slave_state.sent_fine_n + interval);

    if (fine_off > send_off && send_off > 0)
    {
        fine_off -= send_off;
    }

    if (slot_off != 0 || fine_off > link_slave_usrconf.debounce_window)
    {
        mdm_update_slot(slot_off, fine_off);
    }
}

static void link_slave_data_handle_ack(void)
{
    link_packet_t *packet = (link_packet_t *)link_slave_state.recv_buff;

    link_packet_payload_ack_t *ack = (link_packet_payload_ack_t *)(packet + 1);
    link_slave_state.assign        = ack->assign;

    link_slave_data_adjust_timing(ack->offset.slot, ack->offset.fine);
    /*
        if (ack->offset.slot != 0 || ack->offset.fine > link_slave_usrconf.debounce_window)
        {
            mdm_update_slot(ack->offset.slot, ack->offset.fine);
        }
    */
    link_slave_state.send_seq++;
    link_slave_state.send_retry = 0;
    circle_buff_pop(&link_slave_state.send_queue);

    uint16_t piggy = packet->len - sizeof(link_packet_payload_ack_t);
    if (piggy && piggy <= SLAVE_PIGGYBACK_MAX_LENGTH)
    {
        circle_buff_write(&link_slave_state.piggy_queue, (uint8_t *)(ack + 1), piggy);
    }

    link_slave_state.heartbeat = 0;
    link_slave_state.rtx       = SLAVE_IDLE;
}

static void link_slave_data_handle_retry(void)
{
    if (++link_slave_state.send_retry > link_slave_usrconf.tx_retry)
    {
        link_slave_close_connection();
    }
    link_slave_state.rtx = SLAVE_IDLE;
}

static void link_slave_data_handle_send(void)
{
    uint16_t       length;
    link_packet_t *packet = (link_packet_t *)circle_buff_top(&link_slave_state.send_queue, &length);
    packet->cid           = link_slave_state.cid;
    packet->seq           = link_slave_state.send_seq;
    packet->pos           = LINK_TX_POS(link_slave_state.slot_n);
#ifdef CFG_CRC8
    uint8_t  crc = crc8((uint8_t *)packet, sizeof(link_packet_t) + packet->len);
    uint8_t *p   = (uint8_t *)(packet + 1) + packet->len;
    ;
    *p = crc;
#else
    uint32_t crc = crc24((uint8_t *)packet, sizeof(link_packet_t) + packet->len);
    uint8_t *p   = (uint8_t *)(packet + 1) + packet->len;
    ;
    *p++ = (crc >> 0) & 0xFF;
    *p++ = (crc >> 8) & 0xFF;
    *p   = (crc >> 16) & 0xFF;
#endif
    link_slave_state.rtx = SLAVE_SEND_DATA;

    mdm_tx_send(link_slave_state.chn_sel,
        link_slave_usrconf.link.rf_rate,
        link_slave_usrconf.link.access_code,
        (uint8_t *)packet,
        length);
}

static void link_slave_data_proc(void *param, uint32_t flag)
{
    if (flag & SLAVE_EVENT_SYNC_ERROR)
    {
        link_slave_data_handle_retry();

        DEBUG("data no ack\r\n");
    }
    else if (flag & SLAVE_EVENT_RX_DONE)
    {
        if (link_slave_data_validate_packet() == 0)
        {
            link_slave_data_handle_ack();
        }
        else
        {
            link_slave_data_handle_retry();
        }
    }

    if (flag & SLAVE_EVENT_TX_DONE)
    {
        link_slave_state.rtx = SLAVE_RECV_ACK;
        link_slave_rtx_gap();
        mdm_rx_enable(link_slave_state.chn_sel, link_slave_usrconf.link.rf_rate);
    }
    else if (flag & SLAVE_EVENT_SLOT_BEGIN)
    {
        if (link_slave_state.rtx == SLAVE_IDLE)
        {
            if (circle_buff_count(&link_slave_state.send_queue) != 0)
            {
                link_slave_data_handle_send();
            }
            else
            {
                uint32_t live = link_slave_state.slot_n >= link_slave_state.recv_slot_n
                                    ? link_slave_state.slot_n - link_slave_state.recv_slot_n
                                    : MDM_SLOT_N_MASK + 1 + link_slave_state.slot_n -
                                          link_slave_state.recv_slot_n;
                if (live > link_slave_usrconf.conn_alive)
                {
                    link_slave_state.conn = SLAVE_DISCONNECTED;
                    link_slave_state.cid  = 0;
                    INFO("connection closed\r\n");
                }
                else if (link_slave_usrconf.keep_alive != 0 &&
                         live >= link_slave_usrconf.keep_alive)
                {
                    link_slave_heartbeat();
                }
            }
            link_slave_state.mdm_err_cnt = 0;
        }
        else
        {
            if (++link_slave_state.mdm_err_cnt > 2)
            {
                link_slave_state.rtx = SLAVE_IDLE;
                rf_reset();
                link_slave_state.mdm_err_cnt = 0;
                // rf_ctrl_init();
                // INFO("MDM tx/rx state error\r\n");
            }
        }
    }
}
