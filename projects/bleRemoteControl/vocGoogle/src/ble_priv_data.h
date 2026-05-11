/**
 * @file ble_priv_data.h
 */
#ifndef BLE_PRIV_DATA_H
#define BLE_PRIV_DATA_H

#include "stdbool.h"
#include "gapc.h"

#ifdef __cplusplus
extern "C" {
#endif

void ble_load_priv_data(void);
void ble_write_priv_data_to_flash(void);
void ble_reset_priv_data(void);

void ble_save_ltk(const struct gapc_ltk *ltk);
const struct gapc_ltk *ble_read_ltk(void);
void ble_save_peer_addr(const uint8_t *peer_addr);
const uint8_t *ble_read_peer_addr(void);
// bool peer_addr_valid(void);
bool ble_is_paired(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_PRIV_DATA_H */
