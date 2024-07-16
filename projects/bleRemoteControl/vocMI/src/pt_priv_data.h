/**
 * @file pt_priv_data.h
 * @brief 
 * @author lijianpeng (bosco@pthyidh.com)
 * @date 2024-05-14 07:38:56
 * 
 * @copyright Copyright (c) 2024 by 深圳市鹏天微智能科技有限公司, All Rights Reserved.
 */
#ifndef PT_PRIV_DATA_H
#define PT_PRIV_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

void pt_load_priv_data(void);
void pt_write_priv_data_to_flash(void);
void pt_reset_priv_data(void);

void pt_save_ltk(const struct gapc_ltk *ltk);
const struct gapc_ltk *pt_read_ltk(void);
void pt_save_peer_addr(const uint8_t *peer_addr);
const uint8_t *pt_read_peer_addr(void);
// bool peer_addr_valid(void);
bool pt_rcu_is_paired(void);

#ifdef __cplusplus
}
#endif

#endif /* PT_PRIV_DATA_H */
