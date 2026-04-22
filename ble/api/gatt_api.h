/**
 ****************************************************************************************
 *
 * @file gatt_api.h
 *
 * @brief GATT API functions.
 *
 ****************************************************************************************
 */

#ifndef GATT_API_H_
#define GATT_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "gatt.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Gets the negotiated MTU.
 *
 * @param[in] conidx   connection index
 *
 * @return MTU negotiated
 ****************************************************************************************
 */
uint16_t gatt_get_mtu(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Exchange MTU Command.
 *
 * @param[in] idx   connection index
 * @param[in] mtu   Max MTU value (Minimum is 23)
 *
 * @return state in GATT_MTU_CHANGED_IND
 ****************************************************************************************
 */
void gatt_exmtu(uint8_t conidx, uint16_t mtu);

/**
 ****************************************************************************************
 * @brief Change service.
 *
 * @param[in] idx   connection index
 * @param[in] shdl  Start handle
 * @param[in] ehdl  End handle
 * @return state in GATT_SVC_CHANGED_CFG_IND
 ****************************************************************************************
 */
void gatt_svc_chg(uint8_t conidx, uint16_t shdl, uint16_t ehdl);

/**
 ****************************************************************************************
 * @brief Discovery command.
 *
 * @param[in] idx        connection index
 * @param[in] operation  Operation flags  @see GATT_OPERATION about discovery operation
 * @param[in] shdl       Start handle
 * @param[in] ehdl       End handle
 * @param[in] ulen       UUID length
 * @param[in] uuid       Pointer to the uuid data
 * @return Info in GATT_DISC_SVC_IND,GATT_DISC_SVC_INCL_IND,GATT_DISC_CHAR_IND or GATT_DISC_CHAR_DESC_IND
 ****************************************************************************************
 */
void gatt_disc(uint8_t conidx, uint8_t operation, uint16_t shdl, uint16_t ehdl, uint8_t ulen, const uint8_t *uuid);

/**
 ****************************************************************************************
 * @brief Write command request.
 *
 * @param[in] idx        Connection index
 * @param[in] operation  Operation flags  @see GATT_OPERATION about Operation flags for writing/modifying attributes
 * @param[in] handle     Handle to Write
 * @param[in] value      Pointer to the value data
 * @param[in] length     Value length
 * @return state in GATT_WRITE_REQ_IND
 ****************************************************************************************
 */
void gatt_write(uint8_t conidx, uint8_t operation, uint16_t handle, uint8_t* value, uint16_t length);

/**
 ****************************************************************************************
 * @brief Prepare write command request.
 *
 * @param[in] idx        Connection index
 * @param[in] handle     Handle to write
 * @param[in] value      Pointer to the value data
 * @param[in] length     Value length
 * @param[in] offset     Write offset @see gatt_write_cmd
 * @return state in GATT_ATT_INFO_REQ_IND
 ****************************************************************************************
 */
void gatt_pre_write(uint8_t conidx, uint16_t handle, uint8_t* value, uint16_t length, uint16_t offset);

/**
 ****************************************************************************************
 * @brief Execute write characteristic request.
 *
 * @param[in] idx        Connection index
 * @param[in] execute    Cancel(0) / Execute(1) pending write operations
 * @return state in GATT_WRITE_REQ_IND
 ****************************************************************************************
 */
void gatt_exe_write(uint8_t conidx, bool execute);

/**
 ****************************************************************************************
 * @brief Read command.
 *
 * @param[in] idx        Connection index
 * @param[in] handle     Handle to read
 * @param[in] length     Data length
 * @return state in GATT_READ_IND
 ****************************************************************************************
 */
void gatt_read(uint8_t conidx, uint16_t handle, uint16_t length);

/**
 ****************************************************************************************
 * @brief Read command.
 *
 * @param[in] idx        Connection index
 * @param[in] handle     Handle to read
 * @param[in] length     Data length
 * @param[in] offset     Read offset @see gatt_read_simple
 * @return state in GATT_READ_IND
 ****************************************************************************************
 */
void gatt_read_long(uint8_t conidx, uint16_t handle, uint16_t length, uint16_t offset);

/**
 ****************************************************************************************
 * @brief Read Using UUID.
 *
 * @param[in] idx        Connection index
 * @param[in] ulen       UUID length
 * @param[in] uuid       Pointer to the uuid data
 * @param[in] shdl       Start handle
 * @param[in] ehdl       End handle
 * @return state in GATT_READ_IND
 ****************************************************************************************
 */
void gatt_read_by_uuid(uint8_t conidx, uint8_t ulen, uint8_t *uuid, uint16_t shdl, uint16_t ehdl);

/**
 ****************************************************************************************
 * @brief Read Multiple Values.
 *
 * @param[in] idx        Connection index
 * @param[in] nb         number of read @see gatt_read_cmd
 * @param[in] handles    Pointer to the handles data @see gatt_read_multiple
 * @return state in GATT_READ_IND
 ****************************************************************************************
 */
void gatt_read_by_multiple(uint8_t conidx, uint8_t nb, uint8_t* handles);

/**
 ****************************************************************************************
 * @brief Write Confirm.
 *
 * @param[in] idx        Connection index
 * @param[in] status     confirm status to peer. @see att_err
 * @param[in] handle     Handle to write confirm.
 ****************************************************************************************
 */
void gatt_write_cfm(uint8_t conidx, uint8_t status, uint16_t handle);

/**
 ****************************************************************************************
 * @brief Read Confirm.
 *
 * @param[in] idx        Connection index
 * @param[in] status     confirm status to peer. @see att_err
 * @param[in] handle     Handle to read confirm.
 * @param[in] length     Data length
 * @param[in] data       Pointer the confirm data to peer.
 ****************************************************************************************
 */
void gatt_read_cfm(uint8_t conidx, uint8_t status, uint16_t handle, uint16_t length, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief Info Confirm.
 *
 * @param[in] idx        Connection index
 * @param[in] status     confirm status to peer. @see att_err
 * @param[in] handle     Handle to info confirm.
 * @param[in] length     Data length
 ****************************************************************************************
 */
void gatt_info_cfm(uint8_t conidx, uint8_t status, uint16_t handle, uint16_t length);

/**
 ****************************************************************************************
 * @brief Send notification to peer.
 *
 * @param[in] conidx     Connection Index
 * @param[in] handle     Handle to NOTIFY.
 * @param[in] length     Data length
 * @param[in] data       Pointer the data to peer.
 ****************************************************************************************
 */
void gatt_ntf_send(uint8_t conidx, uint16_t handle, uint16_t length, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief Send indication to peer.
 *
 * @param[in] conidx     Connection Index
 * @param[in] handle     Handle to INDICATE.
 * @param[in] length     Data length
 * @param[in] data       Pointer the data to peer.
 ****************************************************************************************
 */
void gatt_ind_send(uint8_t conidx, uint16_t handle, uint16_t length, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief Event Confirm.
 *
 * @param[in] idx        Connection index
 * @param[in] handle     Handle to event confirm.
 ****************************************************************************************
 */
void gatt_evt_cfm(uint8_t conidx, uint16_t handle);


#endif // GATT_API_H_
