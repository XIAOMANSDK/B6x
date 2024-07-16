
// #define DONGLE_TEST

typedef struct
{
    uint8_t disc_mode;  /*!< 发现模式 */
    uint16_t prop;  /*!< GAPM_ADV_PROP_UNDIR_CONN_MASK or GAPM_ADV_PROP_DIR_CONN_MASK */
    uint32_t interval;  /*!< min max interval - (n)*0.625ms */
    uint32_t timeout;  /*!< Advertising duration - 0 mean Always ON (in multiple of 10ms) */

    const uint8_t *adv_data;
    uint16_t adv_data_size;
} pt_adv_para_t;

// 广播数据,
// pairing reconnect prompt
static const uint8_t app_adv_data[] = {
    // 0x01 - «Flags»
    2, GAP_AD_TYPE_FLAGS,
    0x06,
#ifdef DONGLE_TEST
    // 0xFF - «Manufacturer Specific Data»
    6, GAP_AD_TYPE_MANU_SPECIFIC_DATA,
    0xAC, 0x06, 0x91, 0x68, 0x2C, 
#else
    3, GAP_AD_TYPE_MANU_SPECIFIC_DATA,
    0x00, 0x00,
#endif
    // 0x08 - «Shortened Local Name»
    6, GAP_AD_TYPE_SHORTENED_NAME,
    0x4D, 0x49, 0x20, 0x52, 0x43,
    // 0x02 - «Incomplete List of 16-bit Service Class UUIDs»
    3, GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID,
    0x12, 0x18,
    // 0x0D - «Class of Device»
    4, GAP_AD_TYPE_CLASS_OF_DEVICE,
    0x04, 0x05, 0x00,
    // 0x0A - «Tx Power Level»
    2, GAP_AD_TYPE_TRANSMIT_POWER,
    0x00,
};
// power key adv
static uint8_t app_adv_data_power[] = {
    // 0x01 - «Flags»
    2, GAP_AD_TYPE_FLAGS,
    0x05,
#ifdef DONGLE_TEST
    // 0xFF - «Manufacturer Specific Data»
    6, GAP_AD_TYPE_MANU_SPECIFIC_DATA,
    0xAC, 0x06, 0x91, 0x68, 0x2C, 
#else
    // 0xFF - «Manufacturer Specific Data»
    3, GAP_AD_TYPE_MANU_SPECIFIC_DATA,
    0x00, 0x01,
#endif
    // 0x08 - «Shortened Local Name»
    6, GAP_AD_TYPE_SHORTENED_NAME,
    0x4D, 0x49, 0x20, 0x52, 0x43,
    // 0x02 - «Incomplete List of 16-bit Service Class UUIDs»
    3, GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID,
    0x12, 0x18,
    // 0x0D - «Class of Device»
    4, GAP_AD_TYPE_CLASS_OF_DEVICE,
    0x04, 0x05, 0x00,
    // 0x0A - «Tx Power Level»
    2, GAP_AD_TYPE_TRANSMIT_POWER,
    0x00,
    // 26
    4, 0xFE,
    // 83 A1 45 3B 3C F4 
    0x83, 0xA1, 0x45, /*!< 小米国内盒子 */
    // 0x8f, 0x6e, 0x66, /*!< 小米国内电视 */
};

static const pt_adv_para_t pt_adv_params[ADV_TYPE_MAX] = {
    { /*!< ADV_REPAIR 配对广播 */
        .interval = 36,
        .prop = GAPM_ADV_PROP_UNDIR_CONN_MASK,
        .disc_mode = GAPM_ADV_MODE_GEN_DISC,
        .adv_data = app_adv_data,
        .adv_data_size = sizeof(app_adv_data),
        .timeout = 50 * 100, /*!< 50s */
    },
    { /*!< ADV_POWER 开机广播 */
        .interval = 36,
        .prop = GAPM_ADV_PROP_UNDIR_CONN_MASK,
        .disc_mode = GAPM_ADV_MODE_GEN_DISC,
        .adv_data = app_adv_data_power,
        .adv_data_size = sizeof(app_adv_data_power),
        .timeout = 10 * 100,
    },
    { /*!< ADV_DIRECT 直连广播 */
        .interval = 36,
        .prop = GAPM_ADV_PROP_DIR_CONN_MASK,
        .disc_mode = GAPM_ADV_MODE_NON_DISC,
        .adv_data = NULL,
        .adv_data_size = 0,
        .timeout = 50 * 100,
    }
};
