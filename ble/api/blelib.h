/**
 ****************************************************************************************
 *
 * @file blelib.h
 *
 * @brief BLE Lib Definitions.
 *
 ****************************************************************************************
 */

#ifndef _BLELIB_H_
#define _BLELIB_H_

/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// struct *_env_tag *_env
#define __VAR_ENV               __attribute__((section("le_env_mem_area"), zero_init))

/// define the BLE Task handler attribute for this compiler
#define __TASKFN                __attribute__((section("le_task_hdl_func")))

/// Lite Mode(ble6_lite.lib) or Multi Mode(ble6.lib)
#if (BLE_LITELIB)

/// Maximum number of simultaneous conntions - only 1 slave role, support ultra low power
#define BLE_CONNECTION_MAX      (1)
/// Maximum number of simultaneous activities(only 1 Advertising and 1 Connection)
#define BLE_ACTIVITY_MAX        (2)

/// Memory usage for BLE HW Exchange - at Retention Memory Area
#define BLE_EXCH_BASE           (0x20008000)
#define BLE_EXCH_SIZE           (0x0BAC)
#define BLE_EXCH_END            (BLE_EXCH_BASE + BLE_EXCH_SIZE)

/// Lite Heap usage for BLE Stack(MEM_ENV + MEM_MSG) - at Retention Memory Area
#define BLE_HEAP_BASE           (BLE_EXCH_END)
#define BLE_HEAP_ENV_SIZE       (0x600)
#define BLE_HEAP_MSG_SIZE       (0x600)
#define BLE_HEAP_SIZE           (BLE_HEAP_ENV_SIZE + BLE_HEAP_MSG_SIZE)
#define BLE_HEAP_END            (BLE_HEAP_BASE + BLE_HEAP_SIZE)

/// Memory Area for *ble6*.lib (+RW +ZI) - at Retention Memory Area
#define BLE_RWZI_BASE           (BLE_HEAP_END)
#define BLE_RWZI_END            (0x2000A000)
#define BLE_RWZI_SIZE           (BLE_RWZI_END - BLE_RWZI_BASE)

#else //!(BLE_LITELIB)
#if (BLE_LARGELIB)
///Maximum number of simultaneous conntions
#define BLE_CONNECTION_MAX      (6)
/// Maximum number of simultaneous activities(Advertising, Scanning, Initiating, Connection)
#define BLE_ACTIVITY_MAX        (8)

/// Memory usage for BLE HW Exchange - at Retention Memory Area
#define BLE_EXCH_BASE           (0x20008000)
#define BLE_EXCH_SIZE           (0x1590)

#else //(BLE_DFLTLIB)
///Maximum number of simultaneous conntions
#define BLE_CONNECTION_MAX      (3)
/// Maximum number of simultaneous activities(Advertising, Scanning, Initiating, Connection)
#define BLE_ACTIVITY_MAX        (4)

/// Memory usage for BLE HW Exchange - at Retention Memory Area
#define BLE_EXCH_BASE           (0x20008000)
#define BLE_EXCH_SIZE           (0x0FD8)
#endif //(BLE_DFLTLIB)
#define BLE_EXCH_END            (BLE_EXCH_BASE + BLE_EXCH_SIZE)

/// Multi Heap usage for BLE Stack(MEM_ENV + MEM_MSG) - exceed Retention Memory
#if !(BLE_HEAP_BASE)
#define BLE_HEAP_BASE           (0x20004E00)
#endif //(BLE_HEAP_BASE)
#if !(BLE_HEAP_ENV_SIZE)
#define BLE_HEAP_ENV_SIZE       (0xC00)
#endif //(BLE_HEAP_ENV_SIZE)
#if !(BLE_HEAP_MSG_SIZE)
#define BLE_HEAP_MSG_SIZE       (0x2000)
#endif //(BLE_HEAP_ENV_SIZE)
#define BLE_HEAP_SIZE           (BLE_HEAP_ENV_SIZE + BLE_HEAP_MSG_SIZE)

/// Memory Area for *ble6*.lib (+RW +ZI) - at Retention Memory Area
#define BLE_RWZI_BASE           (BLE_EXCH_END)
#define BLE_RWZI_END            (0x2000A000)
#define BLE_RWZI_SIZE           (BLE_RWZI_END - BLE_RWZI_BASE)

#endif //(BLE_LITELIB)

#endif /* _BLELIB_H_ */
