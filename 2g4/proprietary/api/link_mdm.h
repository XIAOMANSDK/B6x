/**
 ****************************************************************************************
 *
 * @file link_mdm.h
 *
 * @brief Header file - Link MDM Test API
 *
 ****************************************************************************************
 */

#ifndef _LINK_MDM_H_
#define _LINK_MDM_H_

#include <stdint.h>
#include "link_param.h"
/*
 * DEFINES
 ****************************************************************************************
 */
typedef enum
{
    MDM_ROLE_RECEIVER = 0,
    MDM_ROLE_SENDER   = 1,
    MDM_ROLE_SCANNER  = 2,
} link_mdm_role_t;

typedef struct
{
    void *param;
    uint32_t flag;
    void (*user_proc)(void *, uint32_t);
} link_mdm_handler_t;


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

void link_mdm_init(link_mdm_role_t role);
void link_mdm_conf(link_conf_t *conf, link_mdm_handler_t *handler);
int link_mdm_listen(uint8_t *data, uint16_t limit);
int link_mdm_send(uint8_t *data, uint16_t length);
int link_mdm_scan(void);

void link_mdm_schedule(void);

#endif // _LINK_MDM_H_
