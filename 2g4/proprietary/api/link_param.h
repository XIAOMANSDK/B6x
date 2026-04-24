/**
 ****************************************************************************************
 *
 * @file link_param.h
 *
 * @brief Header file - Link Layer Parameters Definition
 *
 ****************************************************************************************
 */

#ifndef _LINK_PARAM_H_
#define _LINK_PARAM_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

#define LINK_MAX_CHN           (8)
#define LINK_INVALID_CHN       (0xFF)
#define LINK_MAX_CONN          (0x07)
#define LINK_TX_POS(slot_n)    ((slot_n) & 0x03)
#define LINK_PACKET_MAX_LENGTH (32)

// Link RF rate value
enum link_rf_rate
{
    LINK_1Mbps = 0x0,
    LINK_2Mbps = 0x1,
};

// Link phy configuration
typedef struct
{
    // Channels list
    uint8_t chn_list[LINK_MAX_CHN];
    // RF rate
    uint8_t rf_rate;
    // Acccess code
    uint32_t access_code;
    // Slot window
    uint16_t slot_win;
    // Rx power up time
    uint16_t rx_pwrup;
    // Tx power up time
    uint16_t tx_pwrup;
    // Rx path delay
    uint16_t rx_pathdly;
    // Tx path delay
    uint16_t tx_pathdly;
} link_conf_t;

// Link service feature value
enum
{
    LINK_TOS_FEATURE_NO_TRAFFIC = 0,
    LINK_TOS_FEATURE_BEST_EFFORT,
    LINK_TOS_FEATURE_GUARANTEED
};

// Link service parameters
typedef struct
{
    // Service type (user defined)
    uint8_t type    : 4;
    // Service feature
    uint8_t feature : 2;
    // Preferred exclusive slots
  uint8_t slots     : 2;
} link_tos_t;

// Link connection slot assignment
typedef struct
{
    // Shared slots map
    uint8_t preemptive : 4;
    // Exclusive slots map
    uint8_t exclusive  : 4;
} link_assignment_t;

enum
{
    LINK_LOG_LEVEL_NONE = 0,
    LINK_LOG_LEVEL_ERROR,
    LINK_LOG_LEVEL_WARN,
    LINK_LOG_LEVEL_INFO,
    LINK_LOG_LEVEL_DEBUG,
    LINK_LOG_LEVEL_ALL
};

enum rf_channel
{
    RF_CHNL_2402 = 37,
    RF_CHNL_2404 = 0,
    RF_CHNL_2406 = 1,
    RF_CHNL_2408 = 2,
    RF_CHNL_2410 = 3,
    RF_CHNL_2412 = 4,
    RF_CHNL_2414 = 5,
    RF_CHNL_2416 = 6,
    RF_CHNL_2418 = 7,
    RF_CHNL_2420 = 8,
    RF_CHNL_2422 = 9,
    RF_CHNL_2424 = 10,
    RF_CHNL_2426 = 38,
    RF_CHNL_2428 = 11,
    RF_CHNL_2430 = 12,
    RF_CHNL_2432 = 13,
    RF_CHNL_2434 = 14,
    RF_CHNL_2436 = 15,
    RF_CHNL_2438 = 16,
    RF_CHNL_2440 = 17,
    RF_CHNL_2442 = 18,
    RF_CHNL_2444 = 19,
    RF_CHNL_2446 = 20,
    RF_CHNL_2448 = 21,
    RF_CHNL_2450 = 22,
    RF_CHNL_2452 = 23,
    RF_CHNL_2454 = 24,
    RF_CHNL_2456 = 25,
    RF_CHNL_2458 = 26,
    RF_CHNL_2460 = 27,
    RF_CHNL_2462 = 28,
    RF_CHNL_2464 = 29,
    RF_CHNL_2466 = 30,
    RF_CHNL_2468 = 31,
    RF_CHNL_2470 = 32,
    RF_CHNL_2472 = 33,
    RF_CHNL_2474 = 34,
    RF_CHNL_2476 = 35,
    RF_CHNL_2478 = 36,
    RF_CHNL_2480 = 39,

    RF_CHNL_INVALID = 0xFF,
};

#endif //_LINK_PARAM_H_
