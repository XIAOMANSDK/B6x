/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief UART command processor for BLE UART CMD protocol.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "prf_sess.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "uartRb.h"

#include "proto.h"
#include "cmd.h"
#include "regs.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */
/// Characteristic Base UUID128 (User Customize)
#define SES_ATT_UUID128(uuid)     { 0x16, 0x0A, 0x10, 0x40, 0xD1, 0x9F, 0x4C, 0x6C, \
                                    0xB4, 0x55, 0xE3, 0xF7, (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0x00, 0x00 }

/// Magic number replacements
#define BLE_STATUS_MASK           0x07   ///< Visibility/status bits mask (3 bits)
#define UART_MCR_FLOW_MSK         0x0C   ///< UART MCR flow control bits (AFCEN | RTSCTRL)
#define ADC_10BIT_MAX             0x3FF  ///< 10-bit ADC max value
#define WAKE_PIN_MASK             0x1F   ///< Wake GPIO pin field mask (5 bits)
#define WAKE_LEVEL_BIT            0x80   ///< Wake GPIO level bit (bit 7)
#define GPIO_SWD_PIN_MAX          2      ///< PA00-PA01 are SWD pins
#define GPIO_RESET_PIN            19     ///< PA19 is reset pin
#define ADV_DATA_MAX_LEN          31     ///< BLE advertising data max length (GAP_ADV_DATA_LEN)
#define BLE_HANDLE_LEN            2      ///< BLE attribute handle length in bytes
#define VOLTAGE_DOUBLE            2      ///< Voltage divider multiplier

/// Flash config write size (words)
#define FLASH_CFG_WLEN            64

/// Attribute property bit masks
#define ATT_PROP_READ_MASK        0x03   ///< Broadcast & Read
#define ATT_PROP_WRITE_MASK       0x0C   ///< WriteWithoutResponse & Write
#define ATT_PROP_NOTIFY_MASK      0x30   ///< Notify & Indicate
#define ATT_PROP_EXT_MASK         0xC0   ///< AuthenticatedSignedWrites & ExtendedProperties

__attribute__((aligned (4)))
SYS_CONFIG sysCfg =
{
    .baudrate     = 115200,
    .tx_power     = TX_0DB_P,
    .stauts       = { { .BLE_DIS = ADV_ENBLE } },
    .name_info    = { .len = sizeof(BLE_DEV_NAME), .name = BLE_DEV_NAME },
    .addrL        = { BLE_ADDR },
    .uuid_len     = ATT_UUID128_LEN,
    .uuids        = SES_ATT_UUID128(0x0001),
    .uuidw        = SES_ATT_UUID128(0x0002),
    .uuidn        = SES_ATT_UUID128(0x0003),
    .vertion      = FIRM_VERTION,
    .wake_info    = { .iox = PA10 },
};

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/// @brief Handle SET_BLE_ADDR command
static void cmd_set_ble_addr(struct pt_pkt *pkt)
{
    memcpy(sysCfg.addrL.addr, pkt->payl, PLEN_CMD_SET_BLE_ADDR);
    pt_rsp_ok(pkt->code);
}

/// @brief Handle SET_VISIBILITY command
static void cmd_set_visibility(struct pt_pkt *pkt)
{
    sysCfg.stauts.value = pkt->payl[0] & BLE_STATUS_MASK;
    pt_rsp_ok(pkt->code);
}

/// @brief Handle SET_BLE_NAME command
static void cmd_set_ble_name(struct pt_pkt *pkt)
{
    if (pkt->len > sizeof(sysCfg.name_info.name))
    {
        pt_rsp_cmd_res(pkt->code, PT_ERROR, 0, NULL);
        return;
    }
    sysCfg.name_info.len = pkt->len;
    memcpy(sysCfg.name_info.name, pkt->payl, pkt->len);
    pt_rsp_ok(pkt->code);
}

/// @brief Handle SEND_BLE_DATA command
static void cmd_send_ble_data(struct pt_pkt *pkt)
{
    if (app_state_get() == APP_CONNECTED)
    {
        uint16_t handle = pkt->payl[0] | ((uint16_t)pkt->payl[1] << 8);

        if (sess_txd_send1(handle, pkt->len - BLE_HANDLE_LEN, &pkt->payl[BLE_HANDLE_LEN]) == LE_SUCCESS)
            pt_rsp_ok(pkt->code);
    }
}

/// @brief Handle SET_UART_FLOW command
static void cmd_set_uart_flow(struct pt_pkt *pkt)
{
    if (pkt->payl[0])
        uart_hwfc(UART1_PORT, PA_UART1_RTS, PA_UART1_CTS);
    else
        UART1->MCR.Word &= ~(UART_MCR_FLOW_MSK);

    pt_rsp_ok(pkt->code);
}

/// @brief Handle SET_UART_BAUD command
static void cmd_set_uart_baud(struct pt_pkt *pkt)
{
    int32_t baud = atoi((char *)&pkt->payl);
    if (baud <= 0 || baud > 2000000)
    {
        pt_rsp_cmd_res(pkt->code, PT_ERROR, 0, NULL);
        return;
    }
    sysCfg.baudrate = (uint32_t)baud;
    UART1->LCR.BRWEN  = 1;
    UART1->BRR        = BRR_DIV(sysCfg.baudrate, 16M);
    UART1->LCR.BRWEN  = 0;

    pt_rsp_ok(pkt->code);
}

/// @brief Handle SET_ADV_DATA command (advertising + scan response)
static void cmd_set_adv_data(struct pt_pkt *pkt)
{
    uint8_t lenA = (ADV_DATA_MAX_LEN < pkt->len) ? ADV_DATA_MAX_LEN : pkt->len;
    uint8_t lenS = pkt->len - lenA;

    sysCfg.adv_info.lenADV = lenA;
    memcpy(sysCfg.adv_info.adv, pkt->payl, lenA);

    if (lenS)
    {
        if (lenS > ADV_DATA_MAX_LEN)
            lenS = ADV_DATA_MAX_LEN;

        sysCfg.adv_info.lenSCAN = lenS;
        memcpy(sysCfg.adv_info.scan, &pkt->payl[lenA], lenS);
    }

    pt_rsp_ok(pkt->code);
}

/// @brief Handle POWER_REQ command - read battery voltage
static void cmd_power_req(struct pt_pkt *pkt)
{
    (void)pkt;

    if (SADC->SADC_ANA_CTRL.SADC_EN)
    {
        uint32_t vdd24 = get_trim_vdd12_voltage() * VOLTAGE_DOUBLE;
        uint32_t value = (vdd24 * sadc_read(SADC_CH_AIN0, 0)) / ADC_10BIT_MAX;
        uint8_t adc_data[2];
        adc_data[0] = (uint8_t)(value / 1000);
        adc_data[1] = (uint8_t)((value % 1000) / 10);
        pt_rsp_cmd_res(pkt->code, PT_OK, PLEN_RSP_POWER, adc_data);
    }
    else
    {
        pt_rsp_cmd_res(pkt->code, PT_ERROR, 0, NULL);
    }
}

/// @brief Handle POWER_SET command - enable/disable voltage measurement
static void cmd_power_set(struct pt_pkt *pkt)
{
    if (pkt->payl[0])
    {
        GPIO_DIR_CLR(1UL << PA_POWER_ADC);
        iom_ctrl(PA_POWER_ADC, IOM_ANALOG);
        sadc_init(SADC_ANA_VREF_2V4);
        sadc_conf(SADC_CR_DFLT);
    }
    else
    {
        SADC->SADC_ANA_CTRL.SADC_EN = 0;
    }

    pt_rsp_ok(pkt->code);
}

/// @brief Handle SET_GPIO command
static void cmd_set_gpio(struct pt_pkt *pkt)
{
    uint8_t io_mode = pkt->payl[0];
    uint8_t io_x    = pkt->payl[1];
    uint8_t io_lhud = pkt->payl[2];

    if (io_x < GPIO_SWD_PIN_MAX)
    {
        iospc_swdpin();
    }
    else if (io_x == GPIO_RESET_PIN)
    {
        iospc_rstpin(true);
    }

    if (io_mode)
    {
        if (io_lhud == 0)
            GPIO_DAT_CLR(BIT(io_x));
        else
            GPIO_DAT_SET(BIT(io_x));
        GPIO_DIR_SET(BIT(io_x));
    }
    else
    {
        GPIO_DIR_CLR(BIT(io_x));
        if (io_lhud == 0)
            io_lhud = IOM_PULLUP;
        else
            io_lhud = IOM_PULLDOWN;
        iom_ctrl(io_x, IOM_INPUT | io_lhud);
    }

    pt_rsp_ok(pkt->code);
}

/// @brief Handle READ_GPIO command
static void cmd_read_gpio(struct pt_pkt *pkt)
{
    gpio_info_t io_info;
    io_info.iox = 0;

    if (BIT(pkt->payl[0]) & GPIO_PIN_GET())
        io_info.level = OE_HIGH;
    else
        io_info.level = OE_LOW;

    pt_rsp_cmd_res(pkt->code, PT_OK, PLEN_RSP_GPIO, (uint8_t *)&io_info);
}

/// @brief Handle LE_SET_ADV_DATA command
static void cmd_le_set_adv_data(struct pt_pkt *pkt)
{
    sysCfg.adv_info.lenADV = pkt->len;
    memcpy(sysCfg.adv_info.adv, pkt->payl, pkt->len);
    pt_rsp_ok(pkt->code);
}

/// @brief Handle LE_SET_SCAN_DATA command
static void cmd_le_set_scan_data(struct pt_pkt *pkt)
{
    sysCfg.adv_info.lenSCAN = pkt->len;
    memcpy(sysCfg.adv_info.scan, pkt->payl, pkt->len);
    pt_rsp_ok(pkt->code);
}

/// @brief Handle LE_SEND_CONN_UPDATE_REQ command
static void cmd_le_conn_update(struct pt_pkt *pkt)
{
    sysCfg.conn_info.intervalMIN = pkt->payl[0] | ((uint16_t)pkt->payl[1] << 8);
    sysCfg.conn_info.intervalMAX = pkt->payl[2] | ((uint16_t)pkt->payl[3] << 8);
    sysCfg.conn_info.latency     = pkt->payl[4] | ((uint16_t)pkt->payl[5] << 8);
    sysCfg.conn_info.timeout     = pkt->payl[6] | ((uint16_t)pkt->payl[7] << 8);
    pt_rsp_ok(pkt->code);
}

/// @brief Handle SET_WAKE_GPIO command
static void cmd_set_wake_gpio(struct pt_pkt *pkt)
{
    sysCfg.wake_info.iox      = pkt->payl[0] & WAKE_PIN_MASK;
    sysCfg.wake_info.level    = pkt->payl[0] & WAKE_LEVEL_BIT;
    sysCfg.wake_info.delay_us = RD_32(&pkt->payl[1]);

    GPIO_DIR_SET(1UL << sysCfg.wake_info.iox);

    if (sysCfg.wake_info.level)
        GPIO_DAT_SET(1UL << sysCfg.wake_info.iox);
    else
        GPIO_DAT_CLR(1UL << sysCfg.wake_info.iox);

    btmr_delay(16, sysCfg.wake_info.delay_us);

    pt_rsp_ok(pkt->code);
}

/// @brief Handle ADD_CHARACTERISTIC_UUID command
static void cmd_add_char_uuid(struct pt_pkt *pkt)
{
    uint8_t att_val = pkt->payl[0];

    sess_uuid[sess_uuid_nmb].uuid[12] = pkt->payl[1];
    sess_uuid[sess_uuid_nmb].uuid[13] = pkt->payl[2];
    sess_uuid_nmb++;

    pt_rsp_uuid_handle(sess_env[0].start_hdl + sess_inx_nmb + 1);

    ses_atts_perm[ses_perm_nmb++] = ((uint16_t)att_val << 8);

    if (att_val & ATT_PROP_READ_MASK)
    {
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_READ_CHAR;
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_READ_VAL;

        ses_read_info[sess_read_nmb].length = pkt->payl[3];
        if (pkt->payl[3] <= sizeof(ses_read_info[0].data))
        {
            memcpy(ses_read_info[sess_read_nmb].data, &pkt->payl[4], pkt->payl[3]);
        }
        sess_read_nmb++;
    }
    else if (att_val & ATT_PROP_WRITE_MASK)
    {
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_RXD_CHAR;
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_RXD_VAL;
    }
    else if (att_val & ATT_PROP_NOTIFY_MASK)
    {
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_TXD_CHAR;
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_TXD_VAL;
        SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_TXD_NTF_CFG;
    }
    /* else: ATT_PROP_EXT_MASK - no additional attributes */
}


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// Override - Callback on received data from peer device
void sess_cb_rxd1(uint16_t handle, uint16_t len, const uint8_t *data)
{
    pt_rsp_le_data_rep(handle, len, data);
}


void syscfgInit(void)
{
    uint32_t trim_data[FLASH_CFG_WLEN];
    flash_read(ADDR_OFFSET_CFG, trim_data, FLASH_CFG_WLEN);

    if (trim_data[0] != 0xFFFFFFFF)
    {
        memcpy((uint8_t *)&sysCfg, (uint8_t *)trim_data, sizeof(sysCfg));
    }
}

/// @brief UART command dispatch table
void uart_proc(struct pt_pkt *pkt, uint8_t status)
{
    bool bleReset = true;
    bool cfgSave  = true;

    if (status != PT_OK)
    {
        pt_rsp_cmd_res(pkt->code, status, pkt->len, pkt->payl);
        return;
    }

    switch (pkt->code)
    {
        case PT_CMD_SET_BLE_ADDR:
            cmd_set_ble_addr(pkt);
            break;

        case PT_CMD_SET_VISIBILITY:
            cmd_set_visibility(pkt);
            break;

        case PT_CMD_SET_BLE_NAME:
            cmd_set_ble_name(pkt);
            break;

        case PT_CMD_SEND_BLE_DATA:
            bleReset = false;
            cfgSave  = false;
            cmd_send_ble_data(pkt);
            break;

        case PT_CMD_STATUS_REQUEST:
            bleReset = false;
            cfgSave  = false;
            pt_rsp_status_res(sysCfg.stauts.value);
            break;

        case PT_CMD_SET_UART_FLOW:
            bleReset = false;
            cmd_set_uart_flow(pkt);
            break;

        case PT_CMD_SET_UART_BAUD:
            bleReset = false;
            cmd_set_uart_baud(pkt);
            break;

        case PT_CMD_VERSION_REQUEST:
            bleReset = false;
            cfgSave  = false;
            pt_rsp_cmd_res(pkt->code, PT_OK, PLEN_RSP_VERSION, (uint8_t *)&sysCfg.vertion);
            break;

        case PT_CMD_BLE_DISCONNECT:
            bleReset = false;
            cfgSave  = false;
            gapc_disconnect(app_env.curidx);
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_CONFIRM_GKEY:
            bleReset = false;
            cfgSave  = false;
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_SET_ADV_DATA:
            cmd_set_adv_data(pkt);
            break;

        case PT_CMD_POWER_REQ:
            bleReset = false;
            cfgSave  = false;
            cmd_power_req(pkt);
            break;

        case PT_CMD_POWER_SET:
            bleReset = false;
            cmd_power_set(pkt);
            break;

        case PT_CMD_PASSKEY_ENTRY:
            bleReset = false;
            cfgSave  = false;
            break;

        case PT_CMD_SET_GPIO:
            bleReset = false;
            cmd_set_gpio(pkt);
            break;

        case PT_CMD_READ_GPIO:
            bleReset = false;
            cfgSave  = false;
            cmd_read_gpio(pkt);
            break;

        case PT_CMD_LE_SET_PAIRING:
            bleReset = false;
            sysCfg.pair_info.auth = pkt->payl[0];
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_LE_SET_ADV_DATA:
            cmd_le_set_adv_data(pkt);
            break;

        case PT_CMD_LE_SET_SCAN_DATA:
            cmd_le_set_scan_data(pkt);
            break;

        case PT_CMD_LE_SEND_CONN_UPDATE_REQ:
            bleReset = false;
            cfgSave  = false;
            cmd_le_conn_update(pkt);
            break;

        case PT_CMD_LE_SET_ADV_PARM:
            sysCfg.advr = pkt->payl[0] | ((uint16_t)pkt->payl[1] << 8);
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_LE_START_PAIRING:
            cfgSave  = false;
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_SET_WAKE_GPIO:
            bleReset = false;
            cmd_set_wake_gpio(pkt);
            break;

        case PT_CMD_SET_TX_POWER:
            sysCfg.tx_power = pkt->payl[0];
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_LE_CONFIRM_GKEY:
            bleReset = false;
            cfgSave  = false;
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_REJECT_JUSTWORK:
            cfgSave  = false;
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_RESET_CHIP_REQ:
            bleReset = false;
            cfgSave  = false;
            NVIC_SystemReset();
            break;

        case PT_CMD_LE_SET_FIXED_PASSKEY:
            break;

        case PT_CMD_DELETE_CUSTOMIZE_SERVICE:
            cfgSave  = false;
            sess_inx_nmb = 8;
            sess_uuid_nmb = 4;
            sess_read_nmb = 1;
            ses_perm_nmb = 3;
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_ADD_SERVICE_UUID:
            cfgSave  = false;
            sess_uuid[sess_uuid_nmb].uuid[12] = pkt->payl[0];
            sess_uuid[sess_uuid_nmb].uuid[13] = pkt->payl[1];
            sess_uuid_nmb++;
            SES_ATT_IDX[sess_inx_nmb++] = SES_IDX_SVC;
            pt_rsp_ok(pkt->code);
            break;

        case PT_CMD_ADD_CHARACTERISTIC_UUID:
            cfgSave  = false;
            cmd_add_char_uuid(pkt);
            break;

        case PT_TEST_CMD_CLOSE_LPM:
            cfgSave  = false;
            break;

        default:
            cfgSave  = false;
            break;
    }

    if (bleReset)
        gapm_reset();

    if (cfgSave)
    {
        flash_page_erase(ADDR_OFFSET_CFG);
        flash_write(ADDR_OFFSET_CFG, (uint32_t *)&sysCfg, FLASH_CFG_WLEN);
    }
}

#if (CFG_SLEEP)
static void sleep_proc(void)
{
    uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);

    if (lpsta == BLE_IN_SLEEP)
    {
        uint16_t lpret = core_sleep(CFG_WKUP_BLE_EN);
        (void)lpret;
    }
}
#endif //(CFG_SLEEP)

uint8_t app_name_get(uint8_t size, uint8_t *name)
{
    (void)size;
    uint8_t len = sysCfg.name_info.len;

    memcpy(name, sysCfg.name_info.name, len);

    return len;
}

void app_conf_fsm(uint8_t evt)
{
    if (evt == BLE_RESET)
    {
        memset(&app_env, 0, sizeof(app_env));
        gapm_set_dev(&ble_dev_config, &sysCfg.addrL, NULL);
    }
    else
    {
        #if (CFG_SLEEP)
        #if (RC32K_CALIB_PERIOD)
        ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);
        #endif //(RC32K_CALIB_PERIOD)
        #endif //(CFG_SLEEP)

        app_state_set(APP_IDLE);
        app_prf_create();

        if (sysCfg.stauts.BLE_DIS)
        {
            app_actv_create();
        }
    }
}

void app_conn_fsm(uint8_t evt, uint8_t conidx, const void* param)
{
    (void)param;
    switch (evt)
    {
        case BLE_CONNECTED:
        {
            app_env.curidx = conidx;
            app_state_set(APP_CONNECTED);

            sysCfg.stauts.BLE_COND = 1;
            pt_rsp_le_conn_rep();

            gapc_connect_rsp(conidx, GAP_AUTH_REQ_NO_MITM_NO_BOND);
        } break;

        case BLE_DISCONNECTED:
        {
            app_state_set(APP_READY);

            sysCfg.stauts.BLE_COND = 0;
            pt_rsp_le_dis_rep();

            #if (BLE_EN_ADV)
            app_adv_action(ACTV_START);
            #endif //(BLE_EN_ADV)
        } break;

        case BLE_BONDED:
            break;

        case BLE_ENCRYPTED:
            break;

        default:
            break;
    }
}

void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif //(CFG_SLEEP)

    proto_schedule();
}
