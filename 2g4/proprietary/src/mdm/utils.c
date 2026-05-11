#include "link_mdm.h"
#include "mdm.h"
#include "task.h"
#include "rfrssi.h"
#include "crc.h"

#define MDM_EVENT_SLOT_BEGIN        0x00000001
#define MDM_EVENT_SYNC_ERROR        0x00000002
#define MDM_EVENT_TX_DONE           0x00000004
#define MDM_EVENT_RX_DONE           0x00000100

#define MDM_SYSCALL_LISTEN          ( 1 )
#define MDM_SYSCALL_SCAN            ( 2 )
typedef enum
{
    LINK_MDM_STATE_IDLE = 0,
    LINK_MDM_STATE_PENDING,
    LINK_MDM_STATE_SENDING,
    LINK_MDM_STATE_COMPLETED,
    LINK_MDM_STATE_SYNC_ERR,
} link_mdm_state_t;

typedef struct
{
    link_mdm_role_t role;
    link_conf_t conf;
    uint8_t chn_sel;
} link_mdm_param_t;

typedef struct
{
    uint8_t *buff;
    uint16_t length;
    uint32_t slot_n;
    uint16_t fine_n;
} link_mdm_rx_ctxt_t;

static volatile link_mdm_param_t link_mdm_param;

static volatile link_mdm_state_t link_mdm_state;
static volatile link_mdm_rx_ctxt_t link_mdm_rx_ctxt;
static volatile uint16_t link_mdm_rssi;

static uint8_t link_mdm_buff[MDM_RX_DATA_LEN] = {0x7F, 0x00,};

static void link_mdm_slot_begin(uint32_t slot_n, uint16_t fine_n, int sync_found);
static void link_mdm_sync_error(void);
static void link_mdm_rx_done(uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n);
static void link_mdm_tx_done(uint32_t slot_n, uint16_t fine_n);
static void link_mdm_service(void *param, uint32_t flag);

void link_mdm_init(link_mdm_role_t role)
{
    mdm_cb_t mdm_cb;

    /* Initialize MDM */
    mdm_cb.slot_begin = link_mdm_slot_begin;
    mdm_cb.sync_error = link_mdm_sync_error;
    mdm_cb.rx_done    = link_mdm_rx_done;
    mdm_cb.tx_done    = link_mdm_tx_done;

    mdm_init(&mdm_cb);

    if (role == MDM_ROLE_SCANNER)
    {
        rf_rssi_init();
    }

    link_mdm_param.role = role;
    link_mdm_state = LINK_MDM_STATE_IDLE;

    /* Initialize task manager */
    task_init();
}

void link_mdm_conf(link_conf_t *conf, link_mdm_handler_t *handler)
{
    mdm_conf_t mconf;

    mconf.access_code = conf->access_code;
    mconf.slot_win = conf->slot_win;
    mconf.slot_off = link_mdm_param.role != MDM_ROLE_SENDER ?
                        conf->rx_pwrup + conf->rx_pathdly : conf->tx_pwrup;

    mconf.sync_win = 0xFFF;
    mconf.rf_rate  = conf->rf_rate;

    mdm_conf(&mconf);

    link_mdm_param.conf = *conf;
    link_mdm_param.chn_sel = conf->chn_list[0];

    task_conf(link_mdm_service, 0, handler->user_proc, handler->param, handler->flag);
}

int link_mdm_listen(uint8_t *data, uint16_t limit)
{
    if (link_mdm_param.role != MDM_ROLE_RECEIVER || link_mdm_state != LINK_MDM_STATE_IDLE)
    {
        return -1;
    }

    link_mdm_state = LINK_MDM_STATE_PENDING;

    task_syscall(MDM_SYSCALL_LISTEN);

//    while (link_mdm_state != LINK_MDM_STATE_COMPLETED)
    while (link_mdm_state < LINK_MDM_STATE_COMPLETED)
    {
    }

    if (link_mdm_state == LINK_MDM_STATE_SYNC_ERR)
    {
        link_mdm_state = LINK_MDM_STATE_IDLE;
        return -3;
    }

    uint8_t *p = link_mdm_rx_ctxt.buff + 2;
    uint16_t count = link_mdm_rx_ctxt.length - 2 - CRC_LEN;

    uint8_t *crc_p = link_mdm_rx_ctxt.buff + link_mdm_rx_ctxt.length - CRC_LEN;
#ifdef  CFG_CRC8
    if (crc8(link_mdm_rx_ctxt.buff, count + 2) != crc_p[0])
#else
    uint32_t crc_rx = ((uint32_t)crc_p[2] << 16) | ((uint32_t)crc_p[1] << 8) | crc_p[0];
    if (crc24(link_mdm_rx_ctxt.buff, count + 2) != crc_rx)
#endif
    {
        link_mdm_state = LINK_MDM_STATE_IDLE;
        return -4;
    }

    if (count > limit)
    {
        count = limit;
    }
    while (count--)
    {
        *data++ = *p++;
    }

    link_mdm_state = LINK_MDM_STATE_IDLE;

    return (link_mdm_rx_ctxt.length - 2 - CRC_LEN);
}

int link_mdm_send(uint8_t *data, uint16_t length)
{
    if (link_mdm_param.role != MDM_ROLE_SENDER || link_mdm_state != LINK_MDM_STATE_IDLE)
    {
        return -1;
    }
    if (length + 2 + CRC_LEN > MDM_RX_DATA_LEN)
    {
        return -2;
    }

    link_mdm_buff[1] = length;
    uint8_t *p = link_mdm_buff + 2;
    while (length--)
    {
        *p++ = *data++;
    }
#ifdef  CFG_CRC8
    *p++ = crc8(link_mdm_buff, link_mdm_buff[1] + 2);
#else
    uint32_t crc = crc24(link_mdm_buff, link_mdm_buff[1] + 2);
    *p++ = (uint8_t)(crc);
    *p++ = (uint8_t)(crc >> 8);
    *p++ = (uint8_t)(crc >> 16);
#endif


    link_mdm_state = LINK_MDM_STATE_PENDING;

    while (link_mdm_state != LINK_MDM_STATE_COMPLETED)
    {
    }

    link_mdm_state = LINK_MDM_STATE_IDLE;

    return 0;
}

int link_mdm_scan(void)
{
    if (link_mdm_param.role != MDM_ROLE_SCANNER || link_mdm_state != LINK_MDM_STATE_IDLE)
    {
        return -1;
    }

    link_mdm_state = LINK_MDM_STATE_PENDING;
    task_syscall(MDM_SYSCALL_SCAN);

    while (link_mdm_state != LINK_MDM_STATE_COMPLETED)
    {
    }

    link_mdm_state = LINK_MDM_STATE_IDLE;

    return link_mdm_rssi;
}

void link_mdm_schedule(void)
{
    task_invoke();
}

static void link_mdm_slot_begin(uint32_t slot_n, uint16_t fine_n, int sync_found)
{
    task_event(MDM_EVENT_SLOT_BEGIN);
}

static void link_mdm_rx_done(uint8_t *data, uint16_t length, uint32_t slot_n, uint16_t fine_n)
{
    link_mdm_rssi = rf_rssi_read();

    mdm_rtx_disable();

    link_mdm_rx_ctxt.buff   = data;
    link_mdm_rx_ctxt.length = length;
    link_mdm_rx_ctxt.slot_n = slot_n;
    link_mdm_rx_ctxt.fine_n = fine_n;

    task_event(MDM_EVENT_RX_DONE);
}

static void link_mdm_tx_done(uint32_t slot_n, uint16_t fine_n)
{
    mdm_rtx_disable();

    task_event(MDM_EVENT_TX_DONE);
}

static void link_mdm_sync_error(void)
{
    link_mdm_rssi = 0;//rf_rssi_read();

    mdm_rtx_disable();

    task_event(MDM_EVENT_SYNC_ERROR);
}

static void link_mdm_service(void *param, uint32_t flag)
{
    if (flag & TASK_SYSCALL_EVENT(MDM_SYSCALL_LISTEN))
    {
        mdm_rx_enable(link_mdm_param.chn_sel, link_mdm_param.conf.rf_rate);
    }
    else if (flag & TASK_SYSCALL_EVENT(MDM_SYSCALL_SCAN))
    {
        mdm_rx_enable(link_mdm_param.chn_sel, link_mdm_param.conf.rf_rate);
    }

    if (flag & MDM_EVENT_TX_DONE)
    {
        link_mdm_state = LINK_MDM_STATE_COMPLETED;
    }

    if (flag & MDM_EVENT_SYNC_ERROR)
    {
        if (link_mdm_param.role == MDM_ROLE_RECEIVER)
        {
            link_mdm_state = LINK_MDM_STATE_SYNC_ERR;
//            mdm_rx_enable(link_mdm_param.chn_sel, link_mdm_param.conf.rf_rate);
        }
        else
        {
            link_mdm_state = LINK_MDM_STATE_COMPLETED;
        }
    }
    else if (flag & MDM_EVENT_RX_DONE)
    {
        link_mdm_state = LINK_MDM_STATE_COMPLETED;
    }
    if (flag & MDM_EVENT_SLOT_BEGIN)
    {
        if (link_mdm_param.role == MDM_ROLE_SENDER && link_mdm_state == LINK_MDM_STATE_PENDING)
        {
            link_mdm_state = LINK_MDM_STATE_SENDING;

            mdm_tx_send(link_mdm_param.chn_sel,
                        link_mdm_param.conf.rf_rate,
                        link_mdm_param.conf.access_code,
                        link_mdm_buff,
                        link_mdm_buff[1] + 2 + CRC_LEN);
        }
    }
}
