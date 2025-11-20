/**
 ****************************************************************************************
 *
 * @file usbd_dfu.c
 *
 * @brief Function of USB Device Firmware Upgrade Class (MSC)
 *
 ****************************************************************************************
 */

#include "usbd.h"
#include "usbd_dfu.h"

/* Operation Time, unit in ms */
#ifndef DFU_WRITE_TIME
#define DFU_WRITE_TIME              50
#endif

#ifndef DFU_ERASE_TIME
#define DFU_ERASE_TIME              50
#endif

#ifndef DFU_MANIFEST_TIME
#define DFU_MANIFEST_TIME           2
#endif

#ifndef DFU_DETACH_TIME
#define DFU_DETACH_TIME             0
#endif

/* DFU Device structure */
typedef struct usbd_dfu_tag
{
    /// data info
    uint8_t  xfer_buf[DFU_XFER_SIZE]; // align(4)

    uint32_t dnAddr;
    uint16_t dnLen;
    uint16_t dnBlk;
    uint8_t  dnCmd;

    /// status info @see struct dfu_status
    uint8_t  state;
    uint8_t  status;
} usbd_dfu_t;


/* global environment */
usbd_dfu_t dfu_env;

#define dfu_state_get()     dfu_env.state

__INLINE__ void dfu_state_set(uint8_t state)
{
    dfu_env.state  = state;
    dfu_env.status = DFU_STATUS_OK;
}

__INLINE__ void dfu_state_err(uint8_t error)
{
    dfu_env.state  = DFU_STATE_ERROR;
    dfu_env.status = error;
}

__INLINE__ void dfu_state_rst(void)
{
    dfu_env.state  = DFU_STATE_IDLE;
    dfu_env.status = DFU_STATUS_OK;
}


/*
 * Class Handler
 ****************************************************************************
 */

void usbd_dfu_init(void)
{
    //memset(&dfu_env, 0, sizeof(dfu_env));
    dfu_env.dnAddr = DFU_APP_ADDR;
    dfu_env.dnBlk = 0U;
    dfu_env.dnLen = 0U;
    dfu_env.dnCmd = DFU_CMD_UNKNOWN;

    dfu_state_rst();

    #if (USBD_ADVANCE_API)
    // set big buffer to receive data
    usbd_register_buffer(DFU_XFER_SIZE, dfu_env.xfer_buf);
    #endif
}

__USBIRQ uint8_t* usbd_ep0_big_buffer(uint16_t len)
{
    return (len <= DFU_XFER_SIZE) ? dfu_env.xfer_buf : NULL;
}

__WEAK uint8_t dfu_itf_read(uint32_t addr, uint8_t *buff, uint32_t len)
{
    USB_LOG_INFO("    RD(addr:0x%X,buff:%p,len:%d)\r\n", addr, buff, len);

    /* Return a valid address to avoid HardFault */
    memcpy(buff, (uint8_t *)addr, len);

    return DFU_STATUS_OK; //DFU_STATUS_ERR_ADDRESS
}

__WEAK uint8_t dfu_itf_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    USB_LOG_INFO("    WR(addr:0x%X,data:%p,len:%d)\r\n", addr, data, len);

    return DFU_STATUS_OK; //DFU_STATUS_ERR_WRITE
}

__WEAK uint8_t dfu_itf_erase(uint32_t addr)
{
    USB_LOG_INFO("    ER(addr:%X)\r\n", addr);

    return DFU_STATUS_OK; //DFU_STATUS_ERR_ERASE
}

__WEAK void dfu_itf_leave(uint16_t timeout)
{
    //NVIC_SystemReset();
}

void usbd_dfu_schedule(void)
{
    if (dfu_state_get() == DFU_STATE_DNLOAD_BUSY)
    {
        uint8_t status = DFU_STATUS_OK;

        switch (dfu_env.dnCmd)
        {
            case DFU_CMD_SE_ERASE:
            {
                /* Decode the required address */
                uint32_t er_addr = (dfu_env.dnLen == 5U) ? dfu_env.dnAddr : 0U;
                /*!< Erase, 0 means mass-erase */
                status = dfu_itf_erase(er_addr);
                USB_LOG_INFO("Erase start addr=0x%X, status=%d\r\n", dfu_env.dnAddr, status);
            } break;

            case DFU_CMD_WRITE:
            {
                /* Decode the required address */
                uint32_t wr_addr = dfu_env.dnAddr + ((dfu_env.dnBlk - 2U) * DFU_XFER_SIZE);
                /* Perform the write operation */
                status = dfu_itf_write(wr_addr, dfu_env.xfer_buf, dfu_env.dnLen);
                USB_LOG_INFO("Write start addr=0x%X len=%d, status=%d\r\n", wr_addr, dfu_env.dnLen, status);
            } break;

            case DFU_CMD_SE_SETADDR:
            {
                /* check DFU_STATUS_ERR_ADDRESS */
            } break;

            default:
            {
                status = DFU_STATUS_ERR_UNKNOWN;
            } break;
        }

        //dfu_env.dnCmd = DFU_CMD_UNKNOWN;
        /* Update the state machine */
        if (status == DFU_STATUS_OK)
        {
            dfu_state_set(DFU_STATE_DNLOAD_IDLE);
        }
        else
        {
            dfu_state_err(status);
        }
    }
    else if (dfu_state_get() == DFU_STATE_MANIFEST)
    {
        /* Generate system reset to allow jumping to the user code */
        dfu_itf_leave(DFU_MANIFEST_TIME);
    }
}


/*
 * Class Handler
 ****************************************************************************
 */

static void dfu_detach_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    if (dfu_state_get() != DFU_STATE_DNLOAD_BUSY)
    {
        /* Update the state machine */
        dfu_state_rst();

        // wait timeout, then SystemReset
        dfu_itf_leave(setup->wValue);
    }
}

static void dfu_upload_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    /* the request length and block number */
    uint16_t upBlk = setup->wValue;
    uint16_t upLen = MIN(setup->wLength, DFU_XFER_SIZE);

    /* Data setup request */
    if (upLen > 0U)
    {
        if ((dfu_state_get() == DFU_STATE_IDLE) || (dfu_state_get() == DFU_STATE_UPLOAD_IDLE))
        {
            uint32_t rd_addr = 0;
            if (upBlk == 0U)
            {
                /* Here pack user/customize infos to DFU get */
                if (upLen >= DFU_INFO_SIZE)
                {
                    upLen = DFU_INFO_SIZE;
                    rd_addr = DFU_INFO_ADDR;
                }
            }
            #if (DFU_ENB_UPLOAD)
            else if (upBlk > 1U)
            {
                /* Return the physical address where data are stored */
                rd_addr = dfu_env.dnAddr + ((upBlk - 2U) * DFU_XFER_SIZE);
            }
            #endif //(DFU_ENB_UPLOAD)

            if (rd_addr)
            {
                dfu_itf_read(rd_addr, dfu_env.xfer_buf, upLen);

                /* Send the xfer data over EP0 */
                *data = dfu_env.xfer_buf;
                *len = upLen;
                /* Update the state machine */
                dfu_state_set(DFU_STATE_UPLOAD_IDLE);
            }
            else /* unsupported block_num */
            {
                *len = 0;
                /* Call the error management function (command will be NAKed */
                USB_LOG_ERR("Dfu_upload unsupported block_num\r\n");
                dfu_state_err(DFU_STATUS_ERR_STALLEDPKT);
            }
        }
        /* Unsupported state */
        else
        {
            *len = 0;
            /* Call the error management function (command will be NAKed */
            USB_LOG_ERR("Dfu_request_upload unsupported state\r\n");
        }
    }
    /* No Data setup request */
    else
    {
        dfu_state_set(DFU_STATE_IDLE);
    }
}

static void dfu_dnload_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    if ((dfu_state_get() == DFU_STATE_IDLE) || (dfu_state_get() == DFU_STATE_DNLOAD_IDLE))
    {
        /* Update the global length and block number */
        dfu_env.dnBlk = setup->wValue;
        dfu_env.dnLen = setup->wLength;

        if (dfu_env.dnLen > 0U)
        {
            dfu_env.dnCmd = DFU_CMD_UNKNOWN;
            /* Decode the Special Command -> 1cmd + [4addr] */
            if (dfu_env.dnBlk == 0U)
            {
                if (dfu_env.dnLen == DFU_INFO_SIZE)
                {
                    dfu_env.dnCmd  = DFU_CMD_WRITE;
                    dfu_env.dnAddr = DFU_INFO_ADDR;
                }
                else if (dfu_env.dnLen == 5U)
                {
                    dfu_env.dnCmd  = (*data)[0]; // DFU_CMD_SE_SETADDR DFU_CMD_SE_ERASE
                    dfu_env.dnAddr = ((uint32_t)(*data)[1] << 0) | ((uint32_t)(*data)[2] << 8) 
                                    | ((uint32_t)(*data)[3] << 16) | ((uint32_t)(*data)[4] << 24);
                }
                else if (dfu_env.dnLen == 1U)
                {
                    dfu_env.dnCmd  = (*data)[0]; //DFU_CMD_SE_ERASE
                }
            }
            /* Regular Download Command */
            else if (dfu_env.dnBlk > 1U)
            {
                dfu_env.dnCmd = DFU_CMD_WRITE;
                /*!< Data has received complete */
                if (*data != dfu_env.xfer_buf)
                {
                    memcpy(dfu_env.xfer_buf, *data, dfu_env.dnLen);
                }
            }

            /* Update the state machine */
            dfu_state_set(DFU_STATE_DNLOAD_SYNC);
            /*!< Set flag = 1 Write the firmware to the flash in the next dfu_getstatus_handler */
        }
        /* 0 Data DNLOAD request */
        else
        {
            /* End of DNLOAD operation*/
            dfu_env.dnCmd = DFU_CMD_ENDED;
            dfu_state_set(DFU_STATE_MANIFEST_SYNC);
        }
    }
    else
    {
        /* Call the error management function (command will be NAKed */
        USB_LOG_ERR("Dfu_request_dnload unsupported state %d\r\n", dfu_state_get());
    }
}

static uint16_t dfu_status_pack(uint8_t *data, uint32_t timeout)
{
    data[0] = dfu_env.status;
    data[1] = (timeout >> 0) & 0xFF;
    data[2] = (timeout >> 8) & 0xFF;
    data[3] = (timeout >> 16) & 0xFF;
    data[4] = dfu_env.state;
    data[5] = 0; // iString
    return 6;
}

static void dfu_getstatus_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    uint32_t timeout = 0;

    switch (dfu_state_get())
    {
        case DFU_STATE_DNLOAD_SYNC:
        {
            if (dfu_env.dnCmd == DFU_CMD_WRITE)
            {
                timeout = DFU_WRITE_TIME;
            }
            else if (dfu_env.dnCmd == DFU_CMD_SE_ERASE)
            {
                timeout = DFU_ERASE_TIME;
            }

            dfu_state_set(DFU_STATE_DNLOAD_BUSY);
        } break;

        case DFU_STATE_MANIFEST_SYNC:
        {
            timeout = DFU_MANIFEST_TIME;
            if (DFU_ATTR_CAPABLE & DFU_ATTR_WILL_DETACH)
            {
                dfu_state_set(DFU_STATE_MANIFEST);
            }
            else
            {
                dfu_state_set(DFU_STATE_MANIFEST_WAIT_RESET);
            }
        } break;

        default:
            break;
    }

    /* Send the status data over EP0 */
    *len = dfu_status_pack(*data, timeout);
}

uint8_t usbd_dfu_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    USB_LOG_INFO("DFU Class request: bRequest 0x%02x\r\n", setup->bRequest);

    switch (setup->bRequest)
    {
        case DFU_REQUEST_DETACH:
        {
            dfu_detach_handler(setup, data, len);
        } break;

        case DFU_REQUEST_DNLOAD:
        {
            dfu_dnload_handler(setup, data, len);
        } break;

        case DFU_REQUEST_UPLOAD:
        {
            dfu_upload_handler(setup, data, len);
        } break;

        case DFU_REQUEST_GETSTATUS:
        {
            dfu_getstatus_handler(setup, data, len);
        } break;

        case DFU_REQUEST_CLRSTATUS:
        {
            /* Only clear dfuERROR state */
            if (dfu_state_get() == DFU_STATE_ERROR)
            {
                dfu_state_rst();
            }
            else
            {
                /* Reject -> STALL */
                return USBD_FAIL;
            }
        } break;

        case DFU_REQUEST_GETSTATE:
        {
            /* Return the current state of the DFU interface */
            (*data)[0] = dfu_state_get();
            *len = 1;
        } break;

        case DFU_REQUEST_ABORT:
        {
            if ((dfu_state_get() != DFU_STATE_IDLE) && (dfu_state_get() != DFU_STATE_ERROR))
            {
                dfu_state_rst();

                // Todo: add other clean
            }
        } break;

        default:
        {
            USB_LOG_WRN("Unhandled DFU Class bRequest 0x%02x\r\n", setup->bRequest);
            return USBD_FAIL;
        }
    }

    return USBD_OK;
}
