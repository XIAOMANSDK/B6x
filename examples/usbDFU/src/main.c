/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief USB DFU (Device Firmware Upgrade) bootloader
 *
 * @details
 * Implements a USB DFU mode bootloader that runs from a dedicated flash region.
 *
 * Boot flow:
 * 1. Check DFU flag (AON backup register) or verify app flash is empty (0xFFFFFFFF)
 * 2. If DFU requested: switch sysclk to 48MHz, init USB, enter DFU mode
 * 3. If app present: jump to application at FLASH_CODE_BASE (0x18004000)
 *
 * DFU mode:
 * - Host sends firmware via USB DFU class (DFU_MODE protocol)
 * - Flash write is page-based (256 bytes), erase is sector-based (4KB)
 * - On DFU leave: restore sysclk, enable watchdog, jump to new app
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "dbg.h"

#include "usbd.h"
#include "usbd_dfu.h"

#if (DBG_DFU)
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define USBD_VID                    0x3839
#define USBD_PID                    0xDF11
#define USBD_MAX_POWER              100
#define USBD_LANGID_STRING          1033

#define DFU_FLAG                    BIT(7)
#define FLASH_CODE_BASE             (0x18004000)
#define IWDT_RST_TO                 (1)

/*!< count of interface descriptor */
enum intf_num {
    /* class interface */
    DFU_INTF_NUM                    = 0,

    /* total interface count */
    USB_CONFIG_INTF_CNT,

    /* start&end interface */
    USB_DFU_INTF_START              = DFU_INTF_NUM,
    USB_DFU_INTF_END                = DFU_INTF_NUM,
};

/*!< Length of Configure descriptor */
#define USB_DFU_CONFIG_SIZE         (9 + DFU_DESCRIPTOR_LEN)

#define USBD_DFU_MEDIA              "@Internal Flash /0x18000000/1*004Ka,63*004Kg"

/*!< USB device descriptor */
const uint8_t dfu_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USB_1_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x011a, 0x01),

    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_DFU_CONFIG_SIZE, USB_CONFIG_INTF_CNT,
            0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    DFU_DESCRIPTOR_INIT(0, DFU_PROTOCOL_DFUMODE, DFU_ATTR_CAPABLE, 0x04),

    /* Descriptor - String */
    // String0 - Language ID (Size:4)
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String1 - iManufacturer
    0x06,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('X'),
    WCHAR('M'),

    // String2 - iProduct
    0x08,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('D'),
    WCHAR('F'),
    WCHAR('U'),

    // String3 - iSerialNumber
    #if (SRNM_STR)
    0x16,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('2'),
    WCHAR('0'),
    WCHAR('2'),
    WCHAR('5'),
    WCHAR('0'),
    WCHAR('6'),
    WCHAR('1'),
    WCHAR('8'),
    WCHAR('0'),
    WCHAR('8'),
    #else
    0x02,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    #endif

    // String4 - FLASH descriptor
    0x4E,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('@'),
    WCHAR('I'),
    WCHAR('n'),
    WCHAR('t'),
    WCHAR(' '),
    WCHAR('F'),
    WCHAR('l'),
    WCHAR('a'),
    WCHAR('s'),
    WCHAR('h'),
    WCHAR(' '),
    WCHAR('/'),
    WCHAR('0'),
    WCHAR('x'),
    WCHAR('1'),
    WCHAR('8'),
    WCHAR('0'),
    WCHAR('0'),
    WCHAR('0'),
    WCHAR('0'),
    WCHAR('0'),
    WCHAR('0'),
    WCHAR('/'),
    WCHAR('0'),
    WCHAR('1'),
    WCHAR('*'),
    WCHAR('0'),
    WCHAR('4'),
    WCHAR('K'),
    WCHAR('a'),
    WCHAR(','),
    WCHAR('6'),
    WCHAR('3'),
    WCHAR('*'),
    WCHAR('0'),
    WCHAR('4'),
    WCHAR('K'),
    WCHAR('g'),

    #if (CONFIG_USB_HS)
    /* Descriptor - Device Qualifier (Size:10) */
    USB_QUALIFIER_INIT(0x01),
    #endif

    /* Descriptor - EOF */
    0x00
};


/*
 * Configuration
 ****************************************************************************
 */

/*!< table of class */
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(USB_DFU_INTF_START, USB_DFU_INTF_END, &usbd_dfu_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t dfu_configuration[] = {
    USBD_CONFIG_N(1, USB_CONFIG_INTF_CNT, class_tab)
};


/*
 * Handlers
 ****************************************************************************
 */

/**
 ****************************************************************************************
 * @brief USB event notification handler
 *
 * @param[in] event  Event type
 * @param[in] arg    Event parameter
 ****************************************************************************************
 */
__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    (void)arg;
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}


/*
 * DFU Flash Operations
 ****************************************************************************
 */

#ifndef __SRAMFN_LN
#define __SRAMFN_LN(name)   __attribute__((section("ram_func."#name)))
#endif

/// Disable cache around flash operations
#define FSHC_CACHE_DISABLE()        GLOBAL_INT_DISABLE()
#define FSHC_CACHE_RESTORE()        GLOBAL_INT_RESTORE()

#define PAGE_SIZE                   256
#define PAGE_SIZE_WLEN              (PAGE_SIZE / 4)
#define PAGE_NB(len)                (((len) + (PAGE_SIZE - 1)) / PAGE_SIZE)

/**
 ****************************************************************************************
 * @brief Write a flash page (runs from SRAM)
 *
 * @param[in] offset  Flash offset address
 * @param[in] data    Data to write (word-aligned)
 * @param[in] wlen    Number of uint32_t words
 ****************************************************************************************
 */
__SRAMFN_LN("dfu.write")
void dfu_page_write(uint32_t offset, uint32_t *data, uint32_t wlen)
{
    FSHC_CACHE_DISABLE();

    fshc_write(offset, data, wlen, FSH_CMD_WR);

    FSHC_CACHE_RESTORE();
}

/**
 ****************************************************************************************
 * @brief DFU write callback - program flash pages
 *
 * @param[in] addr  Target flash address
 * @param[in] data  Data buffer
 * @param[in] len   Data length in bytes
 *
 * @return DFU_STATUS_OK on success
 ****************************************************************************************
 */
uint8_t dfu_itf_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint32_t offset = addr & 0xFFFFFF;
    uint32_t nb_page = PAGE_NB(len);

    for (uint32_t page_idx = 0; page_idx < nb_page; ++page_idx)
    {
        dfu_page_write(offset + (page_idx * PAGE_SIZE), (uint32_t *)(data + (page_idx * PAGE_SIZE)), PAGE_SIZE_WLEN);
    }

    return DFU_STATUS_OK;
}

/**
 ****************************************************************************************
 * @brief DFU erase callback - erase a 4KB flash sector
 *
 * @param[in] addr  Target flash address (must be 4KB-aligned)
 *
 * @return DFU_STATUS_OK on success, DFU_STATUS_ERR_ERASE on failure
 ****************************************************************************************
 */
__SRAMFN_LN("dfu.erase")
uint8_t dfu_itf_erase(uint32_t addr)
{
    uint32_t offset = addr & 0xFFFFFF;

    if (offset)
    {
        if ((offset & 0x0FFF) == 0) // 4K sector aligned
        {
            FSHC_CACHE_DISABLE();
            fshc_erase(offset, FSH_CMD_ER_SECTOR);
            FSHC_CACHE_RESTORE();
            return DFU_STATUS_OK;
        }
    }
    else
    {
        // Mass erase not supported
    }

    return DFU_STATUS_ERR_ERASE;
}

/**
 ****************************************************************************************
 * @brief Jump to application at FLASH_CODE_BASE
 ****************************************************************************************
 */
static void appJump(void)
{
    AON->BACKUP1 &= ~DFU_FLAG;
    sysJumpTo(FLASH_CODE_BASE);
}

/**
 ****************************************************************************************
 * @brief DFU leave callback - reset and jump to application
 *
 * @param[in] timeout  Delay in ms before jumping
 ****************************************************************************************
 */
void dfu_itf_leave(uint16_t timeout)
{
    if (timeout > 1)
    {
        bootDelayMs(timeout);
    }

    // Restore sysclk and watchdog before jumping to app
    rcc_sysclk_set(SYS_CLK_16M);
    iwdt_conf(IWDT_RST_TO);

    appJump();
}


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize USB DFU device
 ****************************************************************************************
 */
static void usbdInit(void)
{
    rcc_usb_en();

    usbd_init();
    usbd_register(dfu_descriptor, dfu_configuration);

    usbd_dfu_init();

    NVIC_EnableIRQ(USB_IRQn);

    GLOBAL_INT_START();
}

/**
 ****************************************************************************************
 * @brief System initialization -- decide DFU mode or app jump
 ****************************************************************************************
 */
static void sysInit(void)
{
    uint32_t no_app_code = RD_32(0x18004000);

    if ((AON->BACKUP1 & DFU_FLAG) || (no_app_code == 0xFFFFFFFFU))
    {
        // Switch sysclk to 48MHz for USB
        rcc_sysclk_set(SYS_CLK_48M);
        iwdt_disable();

        usbdInit();
    }
    else
    {
        appJump();
    }
}

/**
 ****************************************************************************************
 * @brief DFU bootloader entry point
 ****************************************************************************************
 */
int __main(void)
{
    sysInit();

    while (1)
    {
        usbd_dfu_schedule();
    }
}
