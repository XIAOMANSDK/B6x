/**
 * @file ntag.h
 * @brief NTAG I2C plus
 */

#ifndef NTAG_H
#define NTAG_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NTAG_OK = 0,
    NTAG_ERR_I2C_START,
    NTAG_ERR_I2C_WRITE, 
    NTAG_ERR_I2C_READ,
    NTAG_ERR_INVALID_PARAM,
    NTAG_ERR_VERIFY_FAILED,
} ntag_status_t;

ntag_status_t ntag_init(void);

void ntag_deinit(void);

/*
 prefix X-DK://
 Normal state: 1T2O0NQ9KSPV35I1X91WIOUV13N789GUTV3TO108QYGLUDM3B6F3HCNOFZW7LODH
 Invalid state: 2I5OPGCVANLS 
 */
ntag_status_t ntag_write_xdk(const void *xdk, uint8_t xdk_len);

#endif
