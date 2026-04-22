/**
 ****************************************************************************************
 *
 * @file link.h
 *
 * @brief Header file - Link Layer Definition
 *
 ****************************************************************************************
 */

#ifndef _LINK_H_
#define _LINK_H_

#include <stdint.h>
#include "link_param.h"
#include "crc.h"
/*
 * DEFINES
 ****************************************************************************************
 */

#define LINK_PACKET_CRC_LENGTH (CRC_LEN)

#define LINK_PACKET_LENGTH(packet) ((packet->len) + sizeof(link_packet_t) + LINK_PACKET_CRC_LENGTH)

enum
{
    LINK_PACKET_DIR_TO_MASTER = 0,
    LINK_PACKET_DIR_TO_SLAVE  = 1,
};

enum
{
    LINK_PACKET_TYPE_CONN = 0,
    LINK_PACKET_TYPE_DATA = 1,
};

typedef struct
{
    uint8_t type : 1;
    uint8_t seq  : 1;
    uint8_t pos  : 2;
    uint8_t cid  : 3;
    uint8_t dir  : 1;
    uint8_t len  : 8;
} link_packet_t;

typedef struct
{
    uint16_t slot : 4;
    uint16_t fine : 12;
} link_slot_offset_t;

typedef struct
{
    uint8_t    address[4];
    link_tos_t tos;
} link_packet_payload_req_t;

typedef struct
{
    link_assignment_t  assign;
    link_slot_offset_t offset;
    uint8_t            address[4];
    uint8_t            cid;
} link_packet_payload_rsp_t;

typedef struct
{
    link_assignment_t  assign;
    link_slot_offset_t offset;
} link_packet_payload_ack_t;

#endif
