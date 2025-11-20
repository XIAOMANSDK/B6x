/**
 ****************************************************************************************
 *
 * @file usbd_msc.c
 *
 * @brief Function of USB Mass Storage Class (MSC)
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "usbd.h"
#include "usbd_msc.h"


/* MSC Bulk-only Stage */
enum msc_stage {
    MSC_READ_CBW    = 0, /* Command Block Wrapper */
    MSC_DATA_OUT    = 1, /* Data Out Phase */
    MSC_DATA_IN     = 2, /* Data In Phase */
    MSC_SEND_CSW    = 3, /* Command Status Wrapper */
    MSC_WAIT_CSW    = 4, /* Command Status Wrapper */
};

/* Device data structure */
typedef struct usbd_msc_tag {
    // Bulk-Only Command Block Wrapper  - 31B
    struct CBW  cbw;
    // Bulk-Only State machine Stage    - 1B
    uint8_t     stage;
    // Bulk-Only Command Status Wrapper - 13B
    struct CSW  csw;

    uint8_t     out_ep;
    uint8_t     in_ep;
    bool        readonly;
    bool        popup;
    uint8_t     max_lun;
    // Sense Key|Additional Sense Code|Sense Qualifier: (sKey<<16)|(ASC<<8)|ASQ
    uint32_t    kcq;
    uint32_t    start_sector;
    uint32_t    nsectors;
    uint32_t    scsi_blk_size[CONFIG_USBD_MSC_MAX_LUN];
    uint32_t    scsi_blk_nbr[CONFIG_USBD_MSC_MAX_LUN];

    // Block aligned(4)
    uint16_t    block_length;
    uint16_t    block_offset;
    uint8_t     block_buffer[CONFIG_USBD_MSC_MAX_BUFSIZE];
} usbd_msc_t;


/* global environment */
usbd_msc_t msc_env;


void usbd_msc_init(const uint8_t out_ep, const uint8_t in_ep)
{
    memset((uint8_t *)&msc_env, 0, sizeof(struct usbd_msc_tag));
    msc_env.out_ep = out_ep;
    msc_env.in_ep  = in_ep;

    msc_env.max_lun = CONFIG_USBD_MSC_MAX_LUN - 1;

    for (uint8_t i = 0u; i <= msc_env.max_lun; i++) {
        usbd_msc_get_cap(i, &msc_env.scsi_blk_nbr[i], &msc_env.scsi_blk_size[i]);

        if (msc_env.scsi_blk_size[i] > CONFIG_USBD_MSC_MAX_BUFSIZE) {
            USB_LOG_ERR("msc block buffer overflow\r\n");
        }
    }
}

void usbd_msc_reset(void)
{
    USB_LOG_DBG("Start reading cbw\r\n");
    msc_env.stage = MSC_READ_CBW;
    msc_env.readonly = false;
}

void usbd_msc_set_readonly(bool readonly)
{
    msc_env.readonly = readonly;
}

bool usbd_msc_get_popup(void)
{
    USB_LOG_DBG("popup\r\n");
    return msc_env.popup;
}

uint8_t usbd_msc_class_handler(struct usb_setup_packet *setup, uint8_t **data, uint16_t *len)
{
    USB_LOG_DBG("MSC Class request: bRequest 0x%02x\r\n", setup->bRequest);

    switch (setup->bRequest) {
        case MSC_REQUEST_RESET:
            usbd_msc_reset();
            break;

        case MSC_REQUEST_GET_MAX_LUN:
            (*data)[0] = msc_env.max_lun;
            *len = 1;
            break;

        default:
            USB_LOG_WRN("Unhandled MSC Class bRequest 0x%02x\r\n", setup->bRequest);
            return USBD_FAIL;
    }

    return USBD_OK;
}

__WEAK void usbd_msc_get_cap(uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    //Pretend having so many buffer,not has actually.
    *block_num = 32;
    *block_size = CONFIG_USBD_MSC_MAX_BUFSIZE;
}

__WEAK bool usbd_msc_sector_read(uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    // todo: read data
    return false;
}

__WEAK bool usbd_msc_sector_write(uint8_t lun, uint32_t sector, const uint8_t *data, uint32_t length)
{
    // todo: save data
    return false;
}

static void usbd_msc_bot_abort(void)
{
    if ((msc_env.cbw.bmFlags == 0) && (msc_env.cbw.dDataLength != 0)) {
        usbd_ep_stall(msc_env.out_ep, true);
    }

    usbd_ep_stall(msc_env.in_ep, true);
    //usbd_ep_start_read(msc_env.out_ep, (uint8_t *)&msc_env.cbw, USB_SIZEOF_MSC_CBW);
}

static void usbd_msc_send_csw(uint8_t status)
{
    msc_env.csw.dSignature = MSC_CSW_Signature;
    msc_env.csw.bStatus = status;

    /* updating the State Machine , so that we wait CSW when this
     * transfer is complete, ie when we get a bulk in callback
     */
    USB_LOG_DBG("Send csw\r\n");
    msc_env.stage = MSC_WAIT_CSW;
    usbd_ep_write(msc_env.in_ep, sizeof(struct CSW), (uint8_t *)&msc_env.csw, NULL);
}

static void usbd_msc_send_info(uint8_t *buffer, uint8_t size)
{
    size = MIN(size, msc_env.cbw.dDataLength);

    /* updating the State Machine , so that we send CSW when this
     * transfer is complete, ie when we get a bulk in callback
     */
    msc_env.stage = MSC_SEND_CSW;
    usbd_ep_write(msc_env.in_ep, size, buffer, NULL);

    msc_env.csw.dDataResidue -= size;
    msc_env.csw.bStatus = CSW_STATUS_CMD_PASSED;
}

/**
 * @brief  SCSI_SetSenseData
 *         Load the last error code in the error list
 *
 * @param  KCQ: Sense Key-Code-Qualifier
 * @retval none
*/
static void SCSI_SetSenseData(uint32_t KCQ)
{
    msc_env.kcq = KCQ;
}

static bool SCSI_processRead(void)
{
    uint32_t transfer_len;

    transfer_len = MIN(msc_env.nsectors * msc_env.scsi_blk_size[msc_env.cbw.bLUN], CONFIG_USBD_MSC_MAX_BUFSIZE);

    USB_LOG_DBG("read lba:%d, len:%d\r\n", msc_env.start_sector, transfer_len);
    if (!usbd_msc_sector_read(msc_env.cbw.bLUN, msc_env.start_sector, msc_env.block_buffer, transfer_len)) {
        SCSI_SetSenseData(SCSI_KCQHE_UREINRESERVEDAREA);
        return false;
    }

    msc_env.start_sector     += (transfer_len / msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    msc_env.nsectors         -= (transfer_len / msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    msc_env.csw.dDataResidue -= transfer_len;

    if (msc_env.nsectors == 0) {
        msc_env.stage = MSC_SEND_CSW;
    }

    //usbd_ep_start_write(msc_env.in_ep, msc_env.block_buffer, transfer_len);
    msc_env.block_length = transfer_len;
    msc_env.block_offset = MIN(transfer_len, MSC_BULK_EP_MPS);
    usbd_ep_write(msc_env.in_ep, msc_env.block_offset, &msc_env.block_buffer[0], NULL);
    return true;
}

static bool SCSI_processWrite(uint32_t nbytes)
{
    uint32_t data_len = 0;
    USB_LOG_DBG("write lba:%d\r\n", msc_env.start_sector);

    if (!usbd_msc_sector_write(msc_env.cbw.bLUN, msc_env.start_sector, msc_env.block_buffer, nbytes)) {
        SCSI_SetSenseData(SCSI_KCQHE_WRITEFAULT);
        return false;
    }

    msc_env.start_sector += (nbytes / msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    msc_env.nsectors -= (nbytes / msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    msc_env.csw.dDataResidue -= nbytes;
    USB_LOG_DBG("%X, nb:%d, %d, %d, %d", msc_env.cbw.CB[0], nbytes, msc_env.start_sector, msc_env.nsectors, msc_env.csw.dDataResidue);
    if (msc_env.nsectors == 0) {
        usbd_msc_send_csw(CSW_STATUS_CMD_PASSED);
    } else {
        data_len = msc_env.nsectors * msc_env.scsi_blk_size[msc_env.cbw.bLUN];
        msc_env.block_length = MIN(data_len, CONFIG_USBD_MSC_MAX_BUFSIZE);
        msc_env.block_offset = 0;
        //data_len = MIN(msc_env.nsectors * msc_env.scsi_blk_size[msc_env.cbw.bLUN], CONFIG_USBD_MSC_MAX_BUFSIZE);
        //usbd_ep_start_read(msc_env.out_ep, msc_env.block_buffer, data_len);
    }

    return true;
}

/**
 * @brief SCSI Command list
 *
 */
static bool SCSI_testUnitReady(uint8_t **data, uint32_t *len)
{
    if (msc_env.cbw.dDataLength != 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }
    *data = NULL;
    *len = 0;
    return true;
}

static bool SCSI_startStopUnit(uint8_t **data, uint32_t *len)
{
    if (msc_env.cbw.dDataLength != 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    if ((msc_env.cbw.CB[4] & 0x3U) == 0x1U) /* START=1 */
    {
        //SCSI_MEDIUM_UNLOCKED;
    } else if ((msc_env.cbw.CB[4] & 0x3U) == 0x2U) /* START=0 and LOEJ Load Eject=1 */
    {
        //SCSI_MEDIUM_EJECTED;
        msc_env.popup = true;
    } else if ((msc_env.cbw.CB[4] & 0x3U) == 0x3U) /* START=1 and LOEJ Load Eject=1 */
    {
        //SCSI_MEDIUM_UNLOCKED;
    } else {
    }

    //*data = NULL;
    *len = 0;
    return true;
}

static bool SCSI_preventAllowMediaRemoval(uint8_t **data, uint32_t *len)
{
    if (msc_env.cbw.dDataLength != 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }
    if (msc_env.cbw.CB[4] == 0U) {
        //SCSI_MEDIUM_UNLOCKED;
    } else {
        //SCSI_MEDIUM_LOCKED;
    }
    //*data = NULL;
    *len = 0;
    return true;
}

static bool SCSI_requestSense(uint8_t **data, uint32_t *len)
{
    uint8_t data_len = SCSIRESP_FIXEDSENSEDATA_SIZEOF;
    if (msc_env.cbw.dDataLength == 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    if (msc_env.cbw.CB[4] < SCSIRESP_FIXEDSENSEDATA_SIZEOF) {
        data_len = msc_env.cbw.CB[4];
    }

    uint8_t request_sense[SCSIRESP_FIXEDSENSEDATA_SIZEOF] = {
        0x70,
        0x00,
        0x00, /* Sense Key */
        0x00,
        0x00,
        0x00,
        0x00,
        SCSIRESP_FIXEDSENSEDATA_SIZEOF - 8,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, /* Additional Sense Code */
        0x00, /* Additional Sense Request */
        0x00,
        0x00,
        0x00,
        0x00,
    };

    request_sense[2]  = (uint8_t)(msc_env.kcq >> 16); /* sKey Sense key */
    request_sense[12] = (uint8_t)(msc_env.kcq >> 8);  /* ASC  Additional Sense Code */
    request_sense[13] = (uint8_t)(msc_env.kcq);       /* ASQ  Additional Sense Qualifier */

    memcpy(*data, (uint8_t *)request_sense, data_len);
    *len = data_len;
    return true;
}

static bool SCSI_inquiry(uint8_t **data, uint32_t *len)
{
    uint8_t data_len = SCSIRESP_INQUIRY_SIZEOF;

    uint8_t inquiry00[6] = {
        0x00,
        0x00,
        0x00,
        (0x06 - 4U),
        0x00,
        0x80
    };

    /* USB Mass storage VPD Page 0x80 Inquiry Data for Unit Serial Number */
    uint8_t inquiry80[8] = {
        0x00,
        0x80,
        0x00,
        0x08,
        0x20, /* Put Product Serial number */
        0x20,
        0x20,
        0x20
    };

    uint8_t inquiry[SCSIRESP_INQUIRY_SIZEOF] = {
        /* 36 */

        /* LUN 0 */
        0x00,
        0x80,
        0x02,
        0x02,
        (SCSIRESP_INQUIRY_SIZEOF - 5),
        0x00,
        0x00,
        0x00,
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* Product      : 16 Bytes */
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ' /* Version      : 4 Bytes */
    };

    memcpy(&inquiry[8], CONFIG_USBD_MSC_MANUF_STR, strlen(CONFIG_USBD_MSC_MANUF_STR));
    memcpy(&inquiry[16], CONFIG_USBD_MSC_PRODUCT_STR, strlen(CONFIG_USBD_MSC_PRODUCT_STR));
    memcpy(&inquiry[32], CONFIG_USBD_MSC_VERSION_STR, strlen(CONFIG_USBD_MSC_VERSION_STR));

    if (msc_env.cbw.dDataLength == 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    if ((msc_env.cbw.CB[1] & 0x01U) != 0U) { /* Evpd is set */
        if (msc_env.cbw.CB[2] == 0U) {       /* Request for Supported Vital Product Data Pages*/
            data_len = 0x06;
            memcpy(*data, (uint8_t *)inquiry00, data_len);
        } else if (msc_env.cbw.CB[2] == 0x80U) { /* Request for VPD page 0x80 Unit Serial Number */
            data_len = 0x08;
            memcpy(*data, (uint8_t *)inquiry80, data_len);
        } else { /* Request Not supported */
            SCSI_SetSenseData(SCSI_KCQIR_INVALIDFIELDINCBA);
            return false;
        }
    } else {
        if (msc_env.cbw.CB[4] < SCSIRESP_INQUIRY_SIZEOF) {
            data_len = msc_env.cbw.CB[4];
        }
        memcpy(*data, (uint8_t *)inquiry, data_len);
    }

    *len = data_len;
    return true;
}

static bool SCSI_modeSense6(uint8_t **data, uint32_t *len)
{
    uint8_t data_len = 4;
    if (msc_env.cbw.dDataLength == 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }
    if (msc_env.cbw.CB[4] < SCSIRESP_MODEPARAMETERHDR6_SIZEOF) {
        data_len = msc_env.cbw.CB[4];
    }

    uint8_t sense6[SCSIRESP_MODEPARAMETERHDR6_SIZEOF] = { 0x03, 0x00, 0x00, 0x00 };

    if (msc_env.readonly) {
        sense6[2] = 0x80;
    }
    memcpy(*data, (uint8_t *)sense6, data_len);
    *len = data_len;
    return true;
}

static bool SCSI_modeSense10(uint8_t **data, uint32_t *len)
{
    uint8_t data_len = 27;
    if (msc_env.cbw.dDataLength == 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    if (msc_env.cbw.CB[8] < 27) {
        data_len = msc_env.cbw.CB[8];
    }

    uint8_t sense10[27] = {
        0x00,
        0x26,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x08,
        0x12,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
    };

    memcpy(*data, (uint8_t *)sense10, data_len);
    *len = data_len;
    return true;
}

static bool SCSI_readFormatCapacity(uint8_t **data, uint32_t *len)
{
    if (msc_env.cbw.dDataLength == 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }
    uint8_t format_capacity[SCSIRESP_READFORMATCAPACITIES_SIZEOF] = {
        0x00,
        0x00,
        0x00,
        0x08, /* Capacity List Length */
        (uint8_t)((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] >> 24) & 0xff),
        (uint8_t)((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] >> 16) & 0xff),
        (uint8_t)((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] >> 8) & 0xff),
        (uint8_t)((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] >> 0) & 0xff),

        0x02, /* Descriptor Code: Formatted Media */
        0x00,
        (uint8_t)((msc_env.scsi_blk_size[msc_env.cbw.bLUN] >> 8) & 0xff),
        (uint8_t)((msc_env.scsi_blk_size[msc_env.cbw.bLUN] >> 0) & 0xff),
    };

    memcpy(*data, (uint8_t *)format_capacity, SCSIRESP_READFORMATCAPACITIES_SIZEOF);
    *len = SCSIRESP_READFORMATCAPACITIES_SIZEOF;

    USB_LOG_DBG("FormatCapacity: %X,%X\r\n", msc_env.scsi_blk_nbr[msc_env.cbw.bLUN], msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    return true;
}

static bool SCSI_readCapacity10(uint8_t **data, uint32_t *len)
{
    if (msc_env.cbw.dDataLength == 0U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    USB_LOG_WRN("Capacity10: %X,%X\r\n", msc_env.scsi_blk_nbr[msc_env.cbw.bLUN], msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    //uint32_t block_nb = msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] - 1;
    //SET_BE32(*data, block_nb);
    //SET_BE32(*data+4, msc_env.scsi_blk_size[msc_env.cbw.bLUN]);
    uint8_t capacity10[SCSIRESP_READCAPACITY10_SIZEOF] = {
        (uint8_t)(((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] - 1) >> 24) & 0xff),
        (uint8_t)(((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] - 1) >> 16) & 0xff),
        (uint8_t)(((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] - 1) >> 8) & 0xff),
        (uint8_t)(((msc_env.scsi_blk_nbr[msc_env.cbw.bLUN] - 1) >> 0) & 0xff),

        (uint8_t)((msc_env.scsi_blk_size[msc_env.cbw.bLUN] >> 24) & 0xff),
        (uint8_t)((msc_env.scsi_blk_size[msc_env.cbw.bLUN] >> 16) & 0xff),
        (uint8_t)((msc_env.scsi_blk_size[msc_env.cbw.bLUN] >> 8) & 0xff),
        (uint8_t)((msc_env.scsi_blk_size[msc_env.cbw.bLUN] >> 0) & 0xff),
    };

    memcpy(*data, (uint8_t *)capacity10, SCSIRESP_READCAPACITY10_SIZEOF);
    *len = SCSIRESP_READCAPACITY10_SIZEOF;
    return true;
}

static bool SCSI_read(uint8_t cmd)
{
    if (((msc_env.cbw.bmFlags & 0x80U) != 0x80U) || (msc_env.cbw.dDataLength == 0U)) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    /* Logical Block Address of First Block */
    msc_env.start_sector = GET_BE32(&msc_env.cbw.CB[2]);

    /* Number of Blocks to transfer */
    msc_env.nsectors = (cmd == SCSI_CMD_READ10) ? GET_BE16(&msc_env.cbw.CB[7]) : GET_BE32(&msc_env.cbw.CB[6]);

    USB_LOG_DBG("SCSI_read - lba:%d, nsr:%d, nbr:%d, lun:%d\r\n", msc_env.start_sector, msc_env.nsectors, msc_env.scsi_blk_nbr[msc_env.cbw.bLUN], msc_env.cbw.bLUN);
    if ((msc_env.start_sector + msc_env.nsectors) > msc_env.scsi_blk_nbr[msc_env.cbw.bLUN]) {
        SCSI_SetSenseData(SCSI_KCQIR_LBAOUTOFRANGE);
        USB_LOG_ERR("LBA out of range\r\n");
        return false;
    }

    uint32_t data_len = msc_env.nsectors * msc_env.scsi_blk_size[msc_env.cbw.bLUN];
    if (msc_env.cbw.dDataLength != data_len) {
        USB_LOG_ERR("scsi_blk_len does not match with dDataLength\r\n");
        return false;
    }

    msc_env.stage = MSC_DATA_IN;
    return SCSI_processRead();
}

static bool SCSI_write(uint8_t cmd)
{
    if (((msc_env.cbw.bmFlags & 0x80U) != 0x00U) || (msc_env.cbw.dDataLength == 0U)) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    /* Logical Block Address of First Block */
    msc_env.start_sector = GET_BE32(&msc_env.cbw.CB[2]);

    /* Number of Blocks to transfer */
    msc_env.nsectors = (cmd == SCSI_CMD_WRITE10) ? GET_BE16(&msc_env.cbw.CB[7]) : GET_BE32(&msc_env.cbw.CB[6]);

    if ((msc_env.start_sector + msc_env.nsectors) > msc_env.scsi_blk_nbr[msc_env.cbw.bLUN]) {
        USB_LOG_ERR("LBA out of range\r\n");
        return false;
    }

    uint32_t data_len = msc_env.nsectors * msc_env.scsi_blk_size[msc_env.cbw.bLUN];
    
    USB_LOG_DBG("SCSI_write - dlen:%d, cbw.dlen:%d\r\n", data_len, msc_env.cbw.dDataLength);
    if (msc_env.cbw.dDataLength != data_len) {
        return false;
    }

    //data_len = MIN(data_len, CONFIG_USBD_MSC_MAX_BUFSIZE);
    //usbd_ep_start_read(msc_env.out_ep, msc_env.block_buffer, data_len);
    msc_env.stage = MSC_DATA_OUT;
    msc_env.block_length = MIN(data_len, CONFIG_USBD_MSC_MAX_BUFSIZE);
    msc_env.block_offset = 0;
    return true;
}

/* do not use verify to reduce code size */
#if 0
static bool SCSI_verify(uint8_t cmd)
{
    if ((msc_env.cbw.CB[1] & 0x02U) == 0x00U) {
        return true;
    }

    if (((msc_env.cbw.bmFlags & 0x80U) != 0x00U) || (msc_env.cbw.dDataLength == 0U)) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    if ((msc_env.cbw.CB[1] & 0x02U) == 0x02U) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDFIELDINCBA);
        return false; /* Error, Verify Mode Not supported*/
    }

    /* Logical Block Address of First Block */
    msc_env.start_sector = GET_BE32(&msc_env.cbw.CB[2]);
    USB_LOG_DBG("lba: 0x%x\r\n", msc_env.start_sector);

    /* Number of Blocks to transfer */
    msc_env.nsectors = (cmd == SCSI_CMD_VERIFY10) ? GET_BE16(&msc_env.cbw.CB[7]) : GET_BE32(&msc_env.cbw.CB[6]);
    USB_LOG_DBG("num (block) : 0x%x\r\n", msc_env.nsectors);

    if ((msc_env.start_sector + msc_env.nsectors) > msc_env.scsi_blk_nbr[msc_env.cbw.bLUN]) {
        USB_LOG_ERR("LBA out of range\r\n");
        return false;
    }

    uint32_t data_len = msc_env.nsectors * msc_env.scsi_blk_size[msc_env.cbw.bLUN];
    if (msc_env.cbw.dDataLength != data_len) {
        return false;
    }

    msc_env.stage = MSC_DATA_OUT;
    return true;
}
#endif

static bool SCSI_CBWDecode(uint32_t nbytes)
{
    uint8_t *buf2send = msc_env.block_buffer;
    uint32_t len2send = 0;
    bool ret = false;

    if (nbytes != sizeof(struct CBW)) {
        USB_LOG_ERR("size != sizeof(cbw)\r\n");
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    }

    //if (msc_env.cbw.CB[0] > SCSI_CMD_TESTUNITREADY)
    //{
    //    USB_LOG_WRN("nb:%d, 0x%02X\r\n", nbytes, msc_env.cbw.CB[0]);
    //    debugHex((uint8_t *)&msc_env.cbw, nbytes);
    //}

    msc_env.csw.dTag = msc_env.cbw.dTag;
    msc_env.csw.dDataResidue = msc_env.cbw.dDataLength;

    if ((msc_env.cbw.dSignature != MSC_CBW_Signature) || (msc_env.cbw.bCBLength < 1) || (msc_env.cbw.bCBLength > 16)) {
        SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
        return false;
    } else {
        switch (msc_env.cbw.CB[0]) {
            case SCSI_CMD_TESTUNITREADY:
                ret = SCSI_testUnitReady(&buf2send, &len2send);
                break;
            case SCSI_CMD_REQUESTSENSE:
                ret = SCSI_requestSense(&buf2send, &len2send);
                break;
            case SCSI_CMD_INQUIRY:
                ret = SCSI_inquiry(&buf2send, &len2send);
                break;
            case SCSI_CMD_STARTSTOPUNIT:
                ret = SCSI_startStopUnit(&buf2send, &len2send);
                break;
            case SCSI_CMD_PREVENTMEDIAREMOVAL:
                ret = SCSI_preventAllowMediaRemoval(&buf2send, &len2send);
                break;
            case SCSI_CMD_MODESENSE6:
                ret = SCSI_modeSense6(&buf2send, &len2send);
                break;
            case SCSI_CMD_MODESENSE10:
                ret = SCSI_modeSense10(&buf2send, &len2send);
                break;
            case SCSI_CMD_READFORMATCAPACITIES:
                ret = SCSI_readFormatCapacity(&buf2send, &len2send);
                break;
            case SCSI_CMD_READCAPACITY10:
                ret = SCSI_readCapacity10(&buf2send, &len2send);
                break;
            case SCSI_CMD_READ10:
            case SCSI_CMD_READ12:
                ret = SCSI_read(msc_env.cbw.CB[0]);
                break;
            case SCSI_CMD_WRITE10:
            case SCSI_CMD_WRITE12:
                ret = SCSI_write(msc_env.cbw.CB[0]);
                break;
            case SCSI_CMD_VERIFY10:
            case SCSI_CMD_VERIFY12:
                //ret = SCSI_verify(msc_env.cbw.CB[0]);
                ret = false;
                break;

            default:
                SCSI_SetSenseData(SCSI_KCQIR_INVALIDCOMMAND);
                USB_LOG_ERR("unsupported cmd:0x%02x\r\n", msc_env.cbw.CB[0]);
                ret = false;
                break;
        }
    }
    if (ret) {
        if (msc_env.stage == MSC_READ_CBW) {
            if (len2send) {
                //USB_LOG_WRN("Send info len:%d\r\n", len2send);
                //debugHex(buf2send, len2send);
                usbd_msc_send_info(buf2send, len2send);
            } else {
                usbd_msc_send_csw(CSW_STATUS_CMD_PASSED);
            }
        }
    }
    return ret;
}

void usbd_msc_bulk_out_handler(uint8_t ep)
{
    uint16_t nbytes;

    switch (msc_env.stage) {
        case MSC_READ_CBW:
            nbytes = usbd_ep_read(ep, MSC_BULK_EP_MPS, (uint8_t *)&msc_env.cbw);
            if (SCSI_CBWDecode(nbytes) == false) {
                USB_LOG_ERR("Command:0x%02x decode err\r\n", msc_env.cbw.CB[0]);
                usbd_msc_bot_abort();
                return;
            }
            break;
        case MSC_DATA_OUT:
            nbytes = usbd_ep_read(ep, MSC_BULK_EP_MPS, &msc_env.block_buffer[msc_env.block_offset]);
            msc_env.block_offset += nbytes;
            USB_LOG_DBG("nb:%d, block:%X, %X", nbytes,msc_env.block_offset, msc_env.block_length);
            if (msc_env.block_offset >= msc_env.block_length)
            {
                switch (msc_env.cbw.CB[0]) {
                    case SCSI_CMD_WRITE10:
                    case SCSI_CMD_WRITE12:
                        #if (0)
                        if (SCSI_processWrite(nbytes) == false) {
                            usbd_msc_send_csw(CSW_STATUS_CMD_FAILED); /* send fail status to host,and the host will retry*/
                        }
                        #else
                        if (SCSI_processWrite(msc_env.block_offset) == false) {
                            usbd_msc_send_csw(CSW_STATUS_CMD_FAILED); /* send fail status to host,and the host will retry*/
                        }
                        #endif
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
}

void usbd_msc_bulk_in_handler(uint8_t ep)
{
    if (msc_env.block_offset < msc_env.block_length)
    {
        uint16_t trans_len = MIN(msc_env.block_length-msc_env.block_offset, MSC_BULK_EP_MPS);
        usbd_ep_write(msc_env.in_ep, trans_len, &msc_env.block_buffer[msc_env.block_offset], NULL);
        msc_env.block_offset += trans_len;
        return;
    }

    switch (msc_env.stage) {
        case MSC_DATA_IN:
            switch (msc_env.cbw.CB[0]) {
                case SCSI_CMD_READ10:
                case SCSI_CMD_READ12:
                    if (SCSI_processRead() == false) {
                        usbd_msc_send_csw(CSW_STATUS_CMD_FAILED); /* send fail status to host,and the host will retry*/
                        return;
                    }
                    break;
                default:
                    break;
            }
            break;
        /*the device has to send a CSW*/
        case MSC_SEND_CSW:
            usbd_msc_send_csw(CSW_STATUS_CMD_PASSED);
            break;

        /*the host has received the CSW*/
        case MSC_WAIT_CSW:
            msc_env.stage = MSC_READ_CBW;
            USB_LOG_DBG("Start reading cbw\r\n");
            //usbd_ep_start_read(msc_env.out_ep, (uint8_t *)&msc_env.cbw, USB_SIZEOF_MSC_CBW);
            break;

        default:
            break;
    }
}
