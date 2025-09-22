/**
 * @file ble_priv_data.c
 */

#include "bledef.h"
#include "drvs.h"
#include "dbg.h"
#include "ble_priv_data.h"

#define USER_STORE_OFFSET   (0x1200)

#if (USER_STORE_OFFSET < 0x1000)
    #error "User Store Data Offset Must Greater Than or Equal 0x1000"
#endif

typedef struct
{
    struct gapc_ltk ltk;
    uint8_t peer_addr[GAP_BD_ADDR_LEN];
} ble_priv_data_t;

static ble_priv_data_t priv_data;

// 从flash中载入持久数据
void ble_load_priv_data(void)
{
    flash_byte_read(USER_STORE_OFFSET, (uint8_t *)&priv_data, sizeof(ble_priv_data_t));
}

void ble_write_priv_data_to_flash(void)
{
    flash_page_erase(USER_STORE_OFFSET);
    flash_byte_write(USER_STORE_OFFSET, (uint8_t *)&priv_data, sizeof(ble_priv_data_t));
}

void ble_reset_priv_data(void)
{
    memset(&priv_data, 0xff, sizeof(ble_priv_data_t));
    // ble_write_priv_data_to_flash();
    flash_page_erase(USER_STORE_OFFSET);
}

void ble_save_ltk(const struct gapc_ltk *ltk)
{
    ble_load_priv_data();

    memcpy(&priv_data.ltk, ltk, sizeof(struct gapc_ltk));

    ble_write_priv_data_to_flash();
}

const struct gapc_ltk *ble_read_ltk(void)
{
    ble_load_priv_data();
    return &priv_data.ltk;
}

void ble_save_peer_addr(const uint8_t *peer_addr)
{
    ble_load_priv_data();

    memcpy(priv_data.peer_addr, peer_addr, GAP_BD_ADDR_LEN);
    // DEBUG("save peer addr");
    // ble_print_addr(priv_data.peer_addr);

    ble_write_priv_data_to_flash();
}

const uint8_t *ble_read_peer_addr(void)
{
    ble_load_priv_data();
    return priv_data.peer_addr;
}

// 判断遥控器是否配对过
bool ble_is_paired(void)
{
    bool ret = false;
    uint8_t temp[34];
    memset(temp, 0xff, sizeof(temp));
    ble_load_priv_data();

    if (memcmp(&priv_data, temp, sizeof(ble_priv_data_t)) != 0)
    {
        debug("ble is paired\n");
        ret = true;
    }

    return ret;
}
