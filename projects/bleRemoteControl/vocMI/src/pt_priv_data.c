/**
 * @file pt_priv_data.c
 * @brief 私有（掉电保持）数据处理
 * @author lijianpeng (bosco@pthyidh.com)
 * @date 2024-05-14 07:31:04
 * 
 * @copyright Copyright (c) 2024 by 深圳市鹏天微智能科技有限公司, All Rights Reserved.
 */

#include "bledef.h"
#include "drvs.h"

#include "pt.h"
#include "pt_priv_data.h"

#define USER_STORE_OFFSET   (0x1200)

#if (USER_STORE_OFFSET < 0x1000)
    #error "User Store Data Offset Must Greater Than or Equal 0x1000"
#endif

typedef struct
{
    struct gapc_ltk ltk;
    uint8_t peer_addr[GAP_BD_ADDR_LEN];
} pt_priv_data_t;

static pt_priv_data_t priv_data;

// 从flash中载入持久数据
void pt_load_priv_data(void)
{
    flash_byte_read(USER_STORE_OFFSET, (uint8_t *)&priv_data, sizeof(pt_priv_data_t));
}

void pt_write_priv_data_to_flash(void)
{
    flash_page_erase(USER_STORE_OFFSET);
    flash_byte_write(USER_STORE_OFFSET, (uint8_t *)&priv_data, sizeof(pt_priv_data_t));
}

void pt_reset_priv_data(void)
{
    memset(&priv_data, 0xff, sizeof(pt_priv_data_t));
    // pt_write_priv_data_to_flash();
    flash_page_erase(USER_STORE_OFFSET);
}

void pt_save_ltk(const struct gapc_ltk *ltk)
{
    pt_load_priv_data();

    memcpy(&priv_data.ltk, ltk, sizeof(struct gapc_ltk));

    pt_write_priv_data_to_flash();
}

const struct gapc_ltk *pt_read_ltk(void)
{
    pt_load_priv_data();
    return &priv_data.ltk;
}

void pt_save_peer_addr(const uint8_t *peer_addr)
{
    pt_load_priv_data();

    memcpy(priv_data.peer_addr, peer_addr, GAP_BD_ADDR_LEN);
    // PT_LOGD("save peer addr");
    // pt_print_addr(priv_data.peer_addr);

    pt_write_priv_data_to_flash();
}

const uint8_t *pt_read_peer_addr(void)
{
    pt_load_priv_data();
    return priv_data.peer_addr;
}

// bool peer_addr_valid(void)
// {
//     for (uint8_t i = 0; i < GAP_BD_ADDR_LEN; ++i)
//     {
//         // != 0x00, != 0xFF
//         if ((priv_data.peer_addr[i] > 0) && (priv_data.peer_addr[i] < 0xFF))
//         {
//             return true;
//         }
//     }
//     return false;
// }

// 判断遥控器是否配对过
bool pt_rcu_is_paired(void)
{
    bool ret = false;
    uint8_t temp[34];
    memset(temp, 0xff, sizeof(temp));
    pt_load_priv_data();

    if (memcmp(&priv_data, temp, sizeof(pt_priv_data_t)) != 0)
    {
        PT_LOGD("rcu is paired\n");
        ret = true;
    }

    return ret;
}
