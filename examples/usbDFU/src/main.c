/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
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
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define USBD_VID                    0x3839//0x0483//0xffff
#define USBD_PID                    0xDF11//0xfff0
#define USBD_MAX_POWER              100
#define USBD_LANGID_STRING          1033

#define DFU_FLAG                    BIT(15)
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

    DFU_DESCRIPTOR_INIT(0, DFU_PROTOCOL_DFUMODE, DFU_ATTR_CAPABLE, 0x04/*FLASH_DESC_iSTR*/),

    /* Descriptor - String */
    // String0 - Language ID (Size:4)
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String1 - iManufacturer
    #if (0)
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('X'),                 /* wcChar0 */
    WCHAR('M'),                 /* wcChar1 */
    WCHAR('-'),                 /* wcChar2 */
    WCHAR('U'),                 /* wcChar3 */
    WCHAR('S'),                 /* wcChar4 */
    WCHAR('B'),                 /* wcChar5 */
    WCHAR('D'),                 /* wcChar6 */
    #else
    0x06,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('X'),                 /* wcChar0 */
    WCHAR('M'),                 /* wcChar1 */
    #endif

    // String2 - iProduct
    #if (0)
    0x12,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('D'),                 /* wcChar0 */
    WCHAR('F'),                 /* wcChar1 */
    WCHAR('U'),                 /* wcChar2 */
    WCHAR(' '),                 /* wcChar3 */
    WCHAR('D'),                 /* wcChar4 */
    WCHAR('E'),                 /* wcChar5 */
    WCHAR('M'),                 /* wcChar6 */
    WCHAR('O'),                 /* wcChar7 */
    #else
    0x08,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('D'),                 /* wcChar0 */
    WCHAR('F'),                 /* wcChar1 */
    WCHAR('U'),                 /* wcChar2 */
    #endif

    // String3 - iSerialNumber
    #if (SRNM_STR)
    0x16,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('2'),                 /* wcChar0 */
    WCHAR('0'),                 /* wcChar1 */
    WCHAR('2'),                 /* wcChar2 */
    WCHAR('5'),                 /* wcChar3 */
    WCHAR('0'),                 /* wcChar4 */
    WCHAR('6'),                 /* wcChar5 */
    WCHAR('1'),                 /* wcChar6 */
    WCHAR('8'),                 /* wcChar7 */
    WCHAR('0'),                 /* wcChar8 */
    WCHAR('8'),                 /* wcChar9 */
    #else
    0x02,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    #endif

    // String4 FLASH descriptor
    0x4E,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('@'),                 /* wcChar0 */
    WCHAR('I'),                 /* wcChar1 */
    WCHAR('n'),                 /* wcChar2 */
    WCHAR('t'),                 /* wcChar3 */
    WCHAR(' '),                 /* wcChar9 */
    WCHAR('F'),                 /* wcChar10 */
    WCHAR('l'),                 /* wcChar11*/
    WCHAR('a'),                 /* wcChar12 */
    WCHAR('s'),                 /* wcChar13 */
    WCHAR('h'),                 /* wcChar14 */
    WCHAR(' '),                 /* wcChar15 */
    WCHAR('/'),                 /* wcChar16 */
    WCHAR('0'),                 /* wcChar17 */
    WCHAR('x'),                 /* wcChar18 */
    WCHAR('1'),                 /* wcChar19 */
    WCHAR('8'),                 /* wcChar20 */
    WCHAR('0'),                 /* wcChar21*/
    WCHAR('0'),                 /* wcChar22 */
    WCHAR('0'),                 /* wcChar23 */
    WCHAR('0'),                 /* wcChar24 */
    WCHAR('0'),                 /* wcChar25 */
    WCHAR('0'),                 /* wcChar26 */
    WCHAR('/'),                 /* wcChar27 */
    WCHAR('0'),                 /* wcChar28 */
    WCHAR('1'),                 /* wcChar29 */
    WCHAR('*'),                 /* wcChar30 */
    WCHAR('0'),                 /* wcChar32 */
    WCHAR('4'),                 /* wcChar33 */
    WCHAR('K'),                 /* wcChar34 */
    WCHAR('a'),                 /* wcChar35 */
    WCHAR(','),                 /* wcChar36 */
    WCHAR('6'),                 /* wcChar38 */
    WCHAR('3'),                 /* wcChar39 */
    WCHAR('*'),                 /* wcChar40 */
    WCHAR('0'),                 /* wcChar42 */
    WCHAR('4'),                 /* wcChar43 */
    WCHAR('K'),                 /* wcChar44 */
    WCHAR('g'),                 /* wcChar45 */

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

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
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
 * Test Functions
 ****************************************************************************
 */

#ifndef __SRAMFN_LN
#define __SRAMFN_LN(name)   __attribute__((section("ram_func."#name)))
#endif

/* Wait cache idle, thene disable-flush cache */
//#undef FSHC_CACHE_DISABLE
//#define FSHC_CACHE_DISABLE()                        \
//do {                                                \
//    GLOBAL_INT_DISABLE();                           \
//    while (SYSCFG->ACC_CCR_BUSY);                   \
//    uint32_t reg_val = (CACHE->CCR.Word);           \
//    CACHE->CCR.Word  = 0;                           \
//    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);

///* Restore cache config */
//#undef FSHC_CACHE_RESTORE
//#define FSHC_CACHE_RESTORE()                        \
//    CACHE->CCR.Word = reg_val;                      \
//    GLOBAL_INT_RESTORE();                           \
//} while(0)
#define FSHC_CACHE_DISABLE()        GLOBAL_INT_DISABLE()
#define FSHC_CACHE_RESTORE()        GLOBAL_INT_RESTORE()

#define PAGE_SIZE                   256
#define PAGE_SIZE_WLEN              (PAGE_SIZE/4)
#define PAGE_NB(len)                (((len) + (PAGE_SIZE - 1)) / PAGE_SIZE)

__SRAMFN_LN("dfu.write")
void dfu_page_write(uint32_t offset, uint32_t *data, uint32_t wlen)
{
    //DEBUG("offset:%X, len:%d", offset, wlen);

    FSHC_CACHE_DISABLE();

    fshc_write(offset, data, wlen, FSH_CMD_WR);

    FSHC_CACHE_RESTORE();
}

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

__SRAMFN_LN("dfu.erase")
uint8_t dfu_itf_erase(uint32_t addr)
{
    uint32_t offset = addr & 0xFFFFFF;

    if (offset)
    {
        if ((offset & 0x0FFF) == 0) //4K sector aligned
        {
            FSHC_CACHE_DISABLE();
            fshc_erase(offset, FSH_CMD_ER_SECTOR);
            FSHC_CACHE_RESTORE();
            return DFU_STATUS_OK;
        }
    }
    else
    {
        // todo mass-erase
    }

    return DFU_STATUS_ERR_ERASE;
}

static void appJump(void)
{
    AON->BACKUP0 &= ~DFU_FLAG;
    sysJumpTo(FLASH_CODE_BASE);
}

//uint8_t gSysClk;

void dfu_itf_leave(uint16_t timeout)
{
    if (timeout > 1)
    {
        bootDelayMs(timeout);
    }

    // restore sysclk and watchdog
    rcc_sysclk_set(SYS_CLK_16M); //(gSysClk);
    iwdt_conf(IWDT_RST_TO);

    appJump();
}


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void usbdInit(void)
{
    // enable USB clk and iopad
    rcc_usb_en();

    usbd_init();
    usbd_register(dfu_descriptor, dfu_configuration);

    usbd_dfu_init();

    NVIC_EnableIRQ(USB_IRQn);
    
    // Global Interrupt Enable
    GLOBAL_INT_START();
}

static void sysInit(void)
{
    uint32_t no_app_code = RD_32(0x18004000);

    if ((AON->BACKUP0 & DFU_FLAG) || (no_app_code == 0xFFFFFFFFU))
    {
        // record current, then switch syclk to 48M for USB
        //gSysClk = rcc_sysclk_get();
        rcc_sysclk_set(SYS_CLK_48M);
        iwdt_disable();
        
        usbdInit();
    }
    else
    {
        appJump();
    }
}

int __main(void)
{
    sysInit();

    while (1)
    {
        usbd_dfu_schedule();
    }
}
