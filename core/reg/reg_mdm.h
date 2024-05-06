#ifndef _REG_MDM_H_
#define _REG_MDM_H_

#include "reg_base.h" 

//================================
//BLOCK MDM define 

#define MDM_BASE                               ((uint32_t)0x40009000) 
#define MDM_REG0_ADDR_OFFSET                   0x000 
#define MDM_REG1_ADDR_OFFSET                   0x004 
#define MDM_CRC_PRESET_ADDR_OFFSET             0x008 
#define MDM_ACCESS_REG_ADDR_OFFSET             0x00c 
#define MDM_DAT_CFG_ADDR_OFFSET                0x010 
#define MDM_EXT_CTRL_ADDR_OFFSET               0x014 
#define MDM_RXSYNC_WIN_ADDR_OFFSET             0x018 
#define MDM_SLOT_SET_ADDR_OFFSET               0x01c 
#define MDM_FINECNT_SET_ADDR_OFFSET            0x020 
#define MDM_SLOTCNT_SET_ADDR_OFFSET            0x024 
#define MDM_FINECNT_ADDR_OFFSET                0x028 
#define MDM_SLOTCNT_ADDR_OFFSET                0x02c 
#define MDM_EXT_TX_DAT_ADDR_OFFSET             0x030 
#define MDM_EXT_RX_DAT_ADDR_OFFSET             0x034 
#define MDM_EXT_ST_ADDR_OFFSET                 0x038 

//================================
//BLOCK MDM reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t RF_CHANNEL_NO:                      6; // bit[5:0]
        uint32_t WHITENING_ON:                       1; // bit6
        uint32_t CD1_EN:                             1; // bit7
        uint32_t ACC_REG_EN:                         1; // bit8
        uint32_t TX_INVERT:                          1; // bit9
        uint32_t RX_INVERT:                          1; // bit10
        uint32_t IQ_INVERT:                          1; // bit11
        uint32_t LI_INVERT:                          1; // bit12
        uint32_t LQ_INVERT:                          1; // bit13
        uint32_t ACC_INVERT:                         1; // bit14
        uint32_t FILT2_CLK_PH:                       1; // bit15 --- no use
        uint32_t TXCLK_PH:                           1; // bit16 --- use for RF CTRL sample the modem out TX_DATA
                                                        // 0: use the posedge clk sample
                                                        // 1: use the negedge clk sample
        uint32_t RSV_NOUSE1:                         1; // bit17
        uint32_t DEBUG_MODE:                         4; // bit[21:18] --- debug out select
        uint32_t TX_SCALE_EN:                        1; // bit22
        uint32_t TX_SCALE_COEF_1M:                   3; // bit[25:23] ---
                                                        // tx_scale_coef[2]==1 : tx_out = tx_in *( 1 + tx_scale_coef[1:0]/16) 
                                                        // tx_scale_coef[2]==0 : tx_out = tx_in *( 1 - tx_scale_coef[1:0]/16) 
        uint32_t TX_SCALE_COEF_2M:                   3; // bit[28:26] ---
                                                        // tx_scale_coef[2]==1 : tx_out = tx_in *( 1 + tx_scale_coef[1:0]/16) 
                                                        // tx_scale_coef[2]==0 : tx_out = tx_in *( 1 - tx_scale_coef[1:0]/16) 
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} MDM_REG0_TypeDef; //0x000 


//================================
#define MDM_RF_CHANNEL_NO_LSB               0
#define MDM_RF_CHANNEL_NO_WIDTH             6
#define MDM_WHITENING_ON_POS                6
#define MDM_CD1_EN_POS                      7
#define MDM_ACC_REG_EN_POS                  8
#define MDM_TX_INVERT_POS                   9
#define MDM_RX_INVERT_POS                   10
#define MDM_IQ_INVERT_POS                   11
#define MDM_LI_INVERT_POS                   12
#define MDM_LQ_INVERT_POS                   13
#define MDM_ACC_INVERT_POS                  14
#define MDM_FILT2_CLK_PH_POS                15
#define MDM_TXCLK_PH_POS                    16
#define MDM_DEBUG_MODE_LSB                  18
#define MDM_DEBUG_MODE_WIDTH                4
#define MDM_TX_SCALE_EN_POS                 22
#define MDM_TX_SCALE_COEF_1M_LSB            23
#define MDM_TX_SCALE_COEF_1M_WIDTH          3
#define MDM_TX_SCALE_COEF_2M_LSB            26
#define MDM_TX_SCALE_COEF_2M_WIDTH          3
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t MIN_MAG_CONF:                      11; // bit[10:0]
        uint32_t RSV_NOUSE1:                         1; // bit11
        uint32_t DRIFT_COR_SET:                      2; // bit[13:12]
        uint32_t RSV_NOUSE2:                         2; // bit[15:14]
        uint32_t IF_SHIFT:                          13; // bit[28:16]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} MDM_REG1_TypeDef; //0x004 


//================================
#define MDM_MIN_MAG_CONF_LSB                0
#define MDM_MIN_MAG_CONF_WIDTH              11
#define MDM_DRIFT_COR_SET_LSB               12
#define MDM_DRIFT_COR_SET_WIDTH             2
#define MDM_IF_SHIFT_LSB                    16
#define MDM_IF_SHIFT_WIDTH                  13
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t MDM_DAT_SET:                        8; // bit[7:0]
        uint32_t MDM_DIRECT_SET:                     1; // bit8 ---
                                                        // 0: modem out use the gauss filter out 
                                                        // 1: modem out data is use mdm_dat_set[7:0]
        uint32_t RSV_END:                           23; // bit[31:9]
    };
    uint32_t Word;
} MDM_DAT_CFG_TypeDef; //0x010 


//================================
#define MDM_MDM_DAT_SET_LSB                 0
#define MDM_MDM_DAT_SET_WIDTH               8
#define MDM_MDM_DIRECT_SET_POS              8
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t MDM_EXT_EN:                         1; // bit0 ---
        uint32_t MDM_INT_EN:                         5; // bit[5:1] ---
                                                        // [0] mdm_tx_done interrupt en
                                                        // [1] mdm_sync_err interrupt en
                                                        // [2] mdm_slot_int interrupt en
                                                        // [3] mdm_slot_over interrupt en
                                                        // [4] mdm_sync_found interrupt en
        uint32_t MDM_SLOT_CNT_HOLD:                  1; // bit6
        uint32_t MDM_SLOT_UPLOAD:                    1; // bit7
        uint32_t MDM_SLOT_INT_CLR:                   1; // bit8
        uint32_t RSV_END:                           23; // bit[31:9]
    };
    uint32_t Word;
} MDM_EXT_CTRL_TypeDef; //0x014 


//================================
#define MDM_MDM_EXT_EN_POS                  0
#define MDM_MDM_INT_EN_LSB                  1
#define MDM_MDM_INT_EN_WIDTH                5
#define MDM_MDM_SLOT_CNT_HOLD_POS           6
#define MDM_MDM_SLOT_UPLOAD_POS             7
#define MDM_MDM_SLOT_INT_CLR_POS            8
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t MDM_SLOT_WIN:                      12; // bit[11:0] ---
                                                        // slot window : (mdm_slot_win + 1 ) us 
                                                        // max slot window : 4096 us
                                                        // default : 1ms
        uint32_t MDM_SLOT_OFF:                      12; // bit[23:12] ---
                                                        // slot interrput event offset
        uint32_t RSV_END:                            8; // bit[31:24]
    };
    uint32_t Word;
} MDM_SLOT_SET_TypeDef; //0x01c 


//================================
#define MDM_MDM_SLOT_WIN_LSB                0
#define MDM_MDM_SLOT_WIN_WIDTH              12
#define MDM_MDM_SLOT_OFF_LSB                12
#define MDM_MDM_SLOT_OFF_WIDTH              12
//================================

typedef union //0x038 
{
    struct
    {
        uint32_t MDM_TX_DONE:                        1; // bit0 ---
                                                        // when is 1
                                                        // TX fifo is empty and last bit is sent out
                                                        // cleared when tx_en is 0
        uint32_t MDM_SYNC_ERR:                       1; // bit1 ---
                                                        // when RX sync over time, 
                                                        // mdm_sync_err is 1
                                                        // cleared by rx_en is 0
        uint32_t MDM_SLOT_INT:                       1; // bit2 ---
                                                        // cleared by mdm_slot_int_clr
        uint32_t MDM_SLOT_OVER:                      1; // bit3 ---
                                                        // cleared by mdm_slot_int_clr
        uint32_t MDM_SYNC_FOUND:                     1; // bit4 ---
                                                        // cleared by rx_en is 0
        uint32_t FIFO_EMPTY:                         1; // bit5
        uint32_t FIFO_FULL:                          1; // bit6
        uint32_t FIFO_NUM:                           3; // bit[9:7]
        uint32_t MDM_SYNC_TIME:                     12; // bit[21:10] ---
                                                        // only valid when mdm_sync_found is 1
                                                        // cleared when RX_EN is 0
        uint32_t RSV_END:                           10; // bit[31:22]
    };
    uint32_t Word;
} MDM_EXT_ST_TypeDef; //0x038 


//================================
#define MDM_MDM_TX_DONE_POS                 0
#define MDM_MDM_SYNC_ERR_POS                1
#define MDM_MDM_SLOT_INT_POS                2
#define MDM_MDM_SLOT_OVER_POS               3
#define MDM_MDM_SYNC_FOUND_POS              4
#define MDM_FIFO_EMPTY_POS                  5
#define MDM_FIFO_FULL_POS                   6
#define MDM_FIFO_NUM_LSB                    7
#define MDM_FIFO_NUM_WIDTH                  3
#define MDM_MDM_SYNC_TIME_LSB               10
#define MDM_MDM_SYNC_TIME_WIDTH             12
//================================

//================================
//BLOCK MDM top struct define 
typedef struct
{
    __IO  MDM_REG0_TypeDef                       REG0                ; // 0x000,  
    __IO  MDM_REG1_TypeDef                       REG1                ; // 0x004,  
    __IO  uint32_t                               CRC_PRESET          ; // 0x008,  
    __IO  uint32_t                               ACCESS_REG          ; // 0x00c,  
    __IO  MDM_DAT_CFG_TypeDef                    DAT_CFG             ; // 0x010,  
    __IO  MDM_EXT_CTRL_TypeDef                   EXT_CTRL            ; // 0x014,  
    __IO  uint32_t                               RXSYNC_WIN          ; // 0x018, 
                                                                       // ext rx sync window :
                                                                       // (mdm_ext_rxsync_win + 1) us
                                                                       // max sync time : 4096 us
                                                                       // default is 2ms 
    __IO  MDM_SLOT_SET_TypeDef                   SLOT_SET            ; // 0x01c,  
    __IO  uint32_t                               FINECNT_SET         ; // 0x020,  
    __IO  uint32_t                               SLOTCNT_SET         ; // 0x024,  
    __I   uint32_t                               FINECNT             ; // 0x028,  
    __I   uint32_t                               SLOTCNT             ; // 0x02c,  
    __O   uint32_t                               EXT_TX_DAT          ; // 0x030,  
    __I   uint32_t                               EXT_RX_DAT          ; // 0x034,  
    __I   MDM_EXT_ST_TypeDef                     EXT_ST              ; // 0x038,  
} MDM_TypeDef;

#define MDM  (( MDM_TypeDef  *)     MDM_BASE)

#endif
