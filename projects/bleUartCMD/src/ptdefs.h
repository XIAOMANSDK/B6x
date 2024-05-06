#ifndef _PTDEFS_H_
#define _PTDEFS_H_

#include <stdint.h>

#ifndef BIT
#define BIT(pos)             (0x01UL << (pos))
#endif

#ifndef RD_32
#define RD_32(addr)          (*(volatile uint32_t *)(addr))
#define WR_32(addr,value)    (*(volatile uint32_t *)(addr)) = (value)
#endif

/// Version
#define VER_CHIPSET          0x0106
#define VER_BURNER           0x0106 //1.06
#define VER_BATCH            0x0106 //1.06


#define ADC_TO_CURR(adc, r)  ((((adc + 1) * 3300) >> 12) / r) // mA = mV/R, R=10
#define KHZ_TO_XTAL(val)     ((val * 1000) / 150) // KHz -> XtalOffset, 150=2400/16MHz 

#define IS_NONE(value)       (((value)== 0xFFFFFFFF) || ((value) == 0x00))
#define IS_HIBE(value, msb)  ((((value) >> 24) & 0xFF) == (msb))

//#define MACC_START_GETSET(cidx)   ((gBchFirm.macCFG.nstart) + (IS_NONE(gChnFirm.macConfig) ? 0 : (((((gChnFirm.macConfig) >> 8) & 0xFFFFFF) * ((gChnFirm.macConfig) & 0xFF)) * (cidx))))


/// PageSize
#define RAM_PAGE_SIZE        (0x100)
#define FSH_PAGE_SIZE        (0x100)
#define ADDR2PAGE(addr)      ((uint16_t)(((addr) >> 8) & 0xFFFF))
#define ADDR2PAGE_H(addr)    ((uint16_t)((((addr) + 255) >> 8) & 0xFFFF))

/// Address
#define CHIP_SDRV_ADDR       0x08000000

#define FSH_ADDR_CODE_INFO   0x18000000
#define FSH_ADDR_SDRV_INFO   0x18000010 // burner
#define FSH_ADDR_CODE_BASE   0x18001000
#define FSH_ADDR_SDRV_BASE   0x18005000

#define FSH_ADDR_FIRM_INFO   0x18008000
#define FSH_ADDR_MAC_INFO    0x18008100
#define FSH_ADDR_FIRM_BASE   0x18008200


/// Operations of ptTest
enum pt_opcode
{
    OP_NULL,
    
    // burner op
    OP_LOAD_SDRV,
    OP_ONLINE_MD,
    
    OP_BURN_CHIP,
    OP_TEST_CHIP,
    
    // batch op
    OP_SYNC_INFO,  
    
    OP_BURN_CHAN, /* Update */
    OP_BURN_FIRM, /* Download */
    
    OP_RUN_START,
    OP_RUN_STATE,
};

/// States of ptTest
enum pt_state
{
    PSTA_NULL              = 0x00,   // Initization
    
    PSTA_BUSY              = BIT(0), // 0:IDLE, 1:BUSY
    PSTA_OK                = BIT(1), // Result OK
    PSTA_FAIL              = BIT(2), // Result Fail
    PSTA_ERROR             = BIT(3), // Result Error

    PSTA_ONLINE            = BIT(4), // 0:OFF line, 1:ON line(PC/Batch)
    PSTA_MANUAL            = BIT(5), // 0:Auto Op, 1:Manual Op
    PSTA_SYNCED            = BIT(6), // 0:not, 1:sdrv synced
};

/// Errors of ptTest
enum pt_error
{
    PERR_NO_ERROR          = 0x00, // sucess
    PERR_XX_STATE          = 0x01, // invalid
    
    PERR_CHAN_SET,
    PERR_FIRM_CFG,
    
    PERR_BURN_FAIL,      //= 0x01,
//    PERR_NO_CODE ,     //= 0x02,
    PERR_RSP     ,       //= 0x03,
    PERR_TIME_OUT,       //= 0x04,
//    PERR_BOOT    ,     //= 0x05,
    PERR_MAC_END ,       //= 0x06,
    
    PERR_RUN_FAIL,
    
    PERR_TEST_FAIL,      //= 0x07,
    PERR_TEST_PWR ,      //= 0x07,
    PERR_TEST_XTAL,      //= 0x08,
    PERR_TEST_GPIO,      //= 0x09,
    PERR_TEST_RF  ,      //= 0x0A,

    PERR_UNKNOWN  ,      //= 0x0B,
};

/// Test-step in order
enum test_step
{
    TEST_PWR,     
    TEST_GPIO,
    TEST_XTAL,          
    TEST_RF,
    
    TEST_MAX,
};

enum chns_mode
{
    CHNS_DOWNLOAD,
    CHNS_UPDATE
};

enum chns_state
{
    CHAN_FOUND             = 0x00,

    CHAN_START             = 0x01,
    CHAN_SUCCESS           = 0x02,

    CHAN_ERR_SYNC          = 0x03,    
    CHAN_ERR0_CODECRC      = 0x04,
    CHAN_ERR1_MAC          = 0x05,   
    CHAN_ERR2_MAC_START    = 0x06,
    CHAN_ERR3_MAC_CONFIG   = 0x07,   
    CHAN_ERR4_UNKNOWN      = 0x08,   
};

/// Structure
typedef struct
{
    uint32_t sdrvLen;
    uint32_t sdrvAddr;
} sdrvInfo_t;

//typedef struct
//{
//    // Base Info @see BOOT
//    uint32_t magicCode;
//    uint32_t codeLen;  
//    uint32_t codeAddr; 
//    uint32_t sramAddr; 
//    
//    // Extend Info   
//    uint32_t dataLen; 
//    uint32_t dataAddr;
//    uint32_t macOffset;  // dataAddr @see firmInfo_t
//    uint32_t macStart;
//    uint32_t macConfig; //1byte:increment 3byte£ºcount
//    uint32_t chipBaud;   // chipSet uart2 baud   
//    uint32_t firmCRC;
//    
//    uint32_t testCfg; //bit0:curr
//    uint32_t gpioMsk;
//    uint8_t workCurr;
//    uint8_t xtalVal;
//    uint8_t xtalPio;
//    uint8_t rfChan;    
//} firmInfo_t;

enum fw_bconf
{
    FW_BURN_OTP_POS        = 0,
    FW_BURN_CODE_POS       = 1,
    FW_BURN_DATA_POS       = 2,
    FW_BURN_MAC_POS        = 3,
    
    FW_BURN_BAUD_POS       = 4,

    FW_RESET_RUN_POS       = 6,
    FW_ERASE_ALL_POS       = 7,
};

enum fw_tconf
{
    FW_TEST_PWR_POS        = 0,
    FW_TEST_GPIO_POS       = 1,
    FW_TEST_XTAL_POS       = 2,
    FW_TEST_RF_POS         = 3,

    FW_TEST_CFG_MSK        = 0x0F,
};

#define FWCFG_EN(cfg)        (1 << FW_##cfg##_POS)

/// Firmware Configure, align32
typedef union
{
    struct
    {
        uint8_t  bConf;  // burn cfgBits
        uint8_t  tConf;  // test cfgBits @see test_step
        uint8_t  uRate;  // uart Rate*115200
        uint8_t  bChns;  // batch Channls
    };
    uint32_t Word;
} fwconf_t;

/// MAC Configure, align32
typedef struct
{
    uint32_t offset;
    uint32_t nstart;
    uint16_t ncount;
    uint16_t ndelta;
} maccfg_t;

/// Test Configure, align32
typedef struct
{
    uint32_t gpioMsk;
    uint8_t  pwrCurr;
    uint8_t  xtalVal;
    uint8_t  xtalPio;
    uint8_t  rfChanl;
} tstcfg_t;

typedef struct
{
    // Base Info @see BOOT
    uint32_t magicCode;
    uint32_t codeLen;  
    uint32_t codeAddr; 
    uint32_t sramAddr; 
    
    // Extend Info   
    uint32_t dataLen; 
    uint32_t dataAddr;
    
    uint32_t firmCRC;
    fwconf_t firmCFG;
    
    tstcfg_t testCFG;
    maccfg_t macCFG;
} firmInfo_t;

typedef struct
{
    uint16_t pwrVadc;
    uint16_t xtalCnt; 
    uint8_t  xtalCap;
    uint8_t  gpioRes;
    uint8_t  rfRxRec; 
    uint8_t  rfTxRec;
} testInfo_t;

typedef struct
{
    uint32_t cntCurr;
    uint32_t macCurr;
} macInfo_t;

typedef struct
{
    uint8_t state;
    uint8_t error;
    uint8_t opCode;
    uint8_t opStep;
} burner_t;

typedef struct 
{
    uint8_t state;
    uint8_t error;
    uint8_t opCode;
    uint8_t opStep;
    
	uint8_t gChans;
    uint8_t chsSet;
    uint8_t chsErr;
    uint8_t chsIdx;
} batch_t;


#endif // _PTDEFS_H_
