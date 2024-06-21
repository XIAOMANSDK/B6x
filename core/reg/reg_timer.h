#ifndef _REG_TIMER_H_
#define _REG_TIMER_H_

#include "reg_base.h"

#define CTMR_BASE 0x40021000
#define ATMR_BASE 0x40022000

typedef union   // 0x000 TIM_CR1
{
    struct
    {
        uint32_t CEN:        1; // bit0      --- Counter Enable
        uint32_t UDIS:       1; // bit1      --- Update Disable
        uint32_t URS:        1; // bit2      --- Update Request Source
        uint32_t OPM:        1; // bit3      --- One Pluse Mode
        uint32_t DIR:        1; // bit4      --- Direction
        uint32_t CMS:        2; // bit[6:5]  --- Center-aligned Mode Select
        uint32_t ARPE:       1; // bit7      --- Auto-Reload Preload Enable
        uint32_t CKD:        2; // bit[9:8]  --- Clock Division
        uint32_t CMPSEL:     2; // bit[11:10]--- Compare input Select (Only ATMR)
        uint32_t RES:       20; // bit[31:12]--- Reserved
    };
    uint32_t Word;
} TIMER_CR1_Typedef;

//================================
#define TIMER_CEN_POS                        0
#define TIMER_UDIS_POS                       1
#define TIMER_URS_POS                        2
#define TIMER_OPM_POS                        3
#define TIMER_DIR_POS                        4
#define TIMER_CMS_LSB                        5
#define TIMER_CMS_WIDTH                      2
#define TIMER_ARPE_POS                       7
#define TIMER_CKD_LSB                        8
#define TIMER_CKD_WIDTH                      2
#define TIMER_CMP_SEL_LSB                    10
#define TIMER_CMP_SEL_WIDTH                  2
//================================


typedef union   // 0x004 TIM_CR2
{
    struct
    {
        uint32_t CCPC:       1; // bit0      --- Capture/Compare Preload Control
        uint32_t RES0:       1; // bit1      --- Reserved
        uint32_t CCUS:       1; // bit2      --- Capture/Compare Control Update Select
        uint32_t CCDS:       1; // bit3      --- Capture/Compare DMA Select
        uint32_t MMS:        3; // bit[6:4]  --- Master Mode Select
        uint32_t TI1S:       1; // bit7      --- TI1 Select
        uint32_t OIS1:       1; // bit8      --- Output Idle State1(OC1 Output)
        uint32_t OIS1N:      1; // bit9      --- Output Idle State1(OC1N Output)
        uint32_t OIS2:       1; // bit10     --- Output Idle State2(OC2 Output)
        uint32_t OIS2N:      1; // bit11     --- Output Idle State2(OC2N Output)
        uint32_t OIS3:       1; // bit12     --- Output Idle State3(OC3 Output)
        uint32_t OIS3N:      1; // bit13     --- Output Idle State3(OC3N Output)
        uint32_t OIS4:       1; // bit14     --- Output Idle State4(OC4 Output)
        uint32_t RES1:      17; // bit[31:15]--- Reserved
    };
    uint32_t Word;
} TIMER_CR2_Typedef;


//================================
#define TIMER_CCPC_POS                       0
#define TIMER_CCUS_POS                       2
#define TIMER_CCDS_POS                       3
#define TIMER_MMS_LSB                        4
#define TIMER_MMS_WIDTH                      3
#define TIMER_TI1S_POS                       7
#define TIMER_OIS1_POS                       8
#define TIMER_OIS1N_POS                      9
#define TIMER_OIS2_POS                       10
#define TIMER_OIS2N_POS                      11
#define TIMER_OIS3_POS                       12
#define TIMER_OIS3N_POS                      13
#define TIMER_OIS4_POS                       14
//================================

typedef union   // 0x008 TIM_SMCR
{
    struct
    {
        uint32_t SMS:        3; // bit[2:0]  --- Slave Mode Select
                                // 000: Slave mode disabled - if CEN = ?1? then the prescaler is clocked directly by the internal clock.
                                // 001: Reserved
                                // 010: Reserved
                                // 011: Reserved
                                // 100: Reset Mode - Rising edge of the selected trigger input (TRGI) reinitializes the counter and generates an update of the registers.
                                // 101: Gated Mode - The counter clock is enabled when the trigger input (TRGI) is high. The counter stops (but is not reset) as soon as the trigger becomes low. Both start and stop of the counter are controlled.
                                // 110: Trigger Mode - The counter starts at a rising edge of the trigger TRGI (but it is not reset). Only the start of the counter is controlled.
                                // 111: Reserved
        uint32_t RES0:       1; // bit3      --- Reserved
        uint32_t TS:         3; // bit[6:4]  --- Trigger Select
        uint32_t MSM:        1; // bit7      --- Master/Slave Mode
        uint32_t ETF:        4; // bit[11:8] --- External Trigger Filter
        uint32_t ETPS:       2; // bit[13:12]--- External Trigger Prescaler
        uint32_t ETE:        1; // bit14     --- External Clock Enable
        uint32_t ETP:        1; // bit15     --- External Trigger Polarity
        uint32_t RES1:      16; // bit[31:16]--- Reserved
    };
    uint32_t Word;
} TIMER_SMCR_Typedef;


//================================
#define TIMER_SMS_LSB                        0
#define TIMER_SMS_WIDTH                      3
#define TIMER_TS_LSB                         4
#define TIMER_TS_WIDTH                       3
#define TIMER_MSM_POS                        7
#define TIMER_ETF_LSB                        8
#define TIMER_ETF_WIDTH                      4
#define TIMER_ETPS_LSB                       12
#define TIMER_ETPS_WIDTH                     2
#define TIMER_ETE_POS                        14
#define TIMER_ETP_POS                        15
//================================

typedef union   // 0x00C/0x010/0x014/0x018/0x01C/0x020 TIM_IER/IDR/IVS/RIF/IFM/ICR
{
    struct
    {
        uint32_t UI:         1; // bit0      --- Update Interrupt
        uint32_t CC1I:       1; // bit1      --- Capture/Compare 1 Interrupt
        uint32_t CC2I:       1; // bit2      --- Capture/Compare 2 Interrupt
        uint32_t CC3I:       1; // bit3      --- Capture/Compare 3 Interrupt
        uint32_t CC4I:       1; // bit4      --- Capture/Compare 4 Interrupt
        uint32_t COMI:       1; // bit5      --- COM Interrupt
        uint32_t TI:         1; // bit6      --- Trigger Interrupt
        uint32_t BI:         1; // bit7      --- Break Interrupt
        uint32_t RES0:       1; // bit8      --- Reserved
        uint32_t CC1OI:      1; // bit9      --- Capture/Compare 1 Overcapture Interrupt
        uint32_t CC2OI:      1; // bit10     --- Capture/Compare 2 Overcapture Interrupt
        uint32_t CC3OI:      1; // bit11     --- Capture/Compare 3 Overcapture Interrupt
        uint32_t CC4OI:      1; // bit12     --- Capture/Compare 4 Overcapture Interrupt
        uint32_t RES1:      19; // bit[31:13]--- Reserved
    };
    uint32_t Word;
} TIMER_INT_Typedef;


//================================
#define TIMER_UI_POS                         0
#define TIMER_CC1I_POS                       1
#define TIMER_CC2I_POS                       2
#define TIMER_CC3I_POS                       3
#define TIMER_CC4I_POS                       4
#define TIMER_COMI_POS                       5
#define TIMER_TI_POS                         6
#define TIMER_BI_POS                         7
#define TIMER_CC1OI_POS                      9
#define TIMER_CC2OI_POS                      10
#define TIMER_CC3OI_POS                      11
#define TIMER_CC4OI_POS                      12
//================================

typedef union   // 0x024 TIM_EGR
{
    struct
    {
        uint32_t UG:         1; // bit0     --- Update Generation
        uint32_t CC1G:       1; // bit1     --- Capture/Compare 1 Generation
        uint32_t CC2G:       1; // bit2     --- Capture/Compare 2 Generation
        uint32_t CC3G:       1; // bit3     --- Capture/Compare 3 Generation
        uint32_t CC4G:       1; // bit4     --- Capture/Compare 4 Generation
        uint32_t COMG:       1; // bit5     --- Capture/Compare Control Update Generation
        uint32_t TG:         1; // bit6     --- Trigger Generation
        uint32_t BG:         1; // bit7     --- Break Generation
        uint32_t RES:       24; // bit[31:8]--- Reserved
    };
    uint32_t Word;
} TIMER_EGR_Typedef;

//================================
#define TIMER_UG_POS                         0
#define TIMER_CC1G_POS                       1
#define TIMER_CC2G_POS                       2
#define TIMER_CC3G_POS                       3
#define TIMER_CC4G_POS                       4
#define TIMER_COMG_POS                       5
#define TIMER_TG_POS                         6
#define TIMER_BG_POS                         7
//================================

typedef union   // 0x028 TIM_CCMR1
{
    struct
    {
        uint32_t CC1S:       2; // bit[1:0]  --- Capture/Compare 1 Select
        uint32_t OC1FE:      1; // bit2      --- Output Compare 1 Fast Enable
        uint32_t OC1PE:      1; // bit3      --- Output Compare 1 Preload Enable
        uint32_t OC1M:       3; // bit[6:4]  --- Output Compare 1 Mode
        uint32_t OC1CE:      1; // bit7      --- Output Compare 1 Clear Enable
        uint32_t CC2S:       2; // bit[9:8]  --- Capture/Compare 2 Select
        uint32_t OC2FE:      1; // bit10     --- Output Compare 2 Fast Enable
        uint32_t OC2PE:      1; // bit11     --- Output Compare 2 Preload Enable
        uint32_t OC2M:       3; // bit[14:12]--- Output Compare 2 Mode
        uint32_t OC2CE:      1; // bit15     --- Output Compare 2 Clear Enable
        uint32_t RES:       16; // bit[31:16]--- Reserved
    }OUTPUT;
    
    struct
    {
        uint32_t CC1S:       2; // bit[1:0]  --- Capture/Compare 1 Select
        uint32_t IC1PSC:     2; // bit[3:2]  --- Input Capture 1 Prescaler
        uint32_t IC1F:       4; // bit[7:4]  --- Input Capture 1 Filter
        uint32_t CC2S:       2; // bit[9:8]  --- Capture/Compare 2 Select
        uint32_t IC2PSC:     2; // bit[11:10]--- Input Capture 2 Prescaler
        uint32_t IC2F:       4; // bit[15:12]--- Input Capture 2 Filter
        uint32_t RES:       16; // bit[31:16]--- Reserved
    }INPUT;
    uint32_t Word;
} TIMER_CCMR1_Typedef;


//================================
#define TIMER_OUT_CC1S_LSB       0
#define TIMER_OUT_CC1S_WIDTH     2
#define TIMER_OUT_OC1FE_POS      2
#define TIMER_OUT_OC1PE_POS      3
#define TIMER_OUT_OC1M_LSB       4
#define TIMER_OUT_OC1M_WIDTH     3 
#define TIMER_OUT_OC1CE_POS      7
#define TIMER_OUT_CC2S_LSB       8
#define TIMER_OUT_CC2S_WIDTH     2
#define TIMER_OUT_OC2FE_POS      10
#define TIMER_OUT_OC2PE_POS      11
#define TIMER_OUT_OC2M_LSB       12
#define TIMER_OUT_OC2M_WIDTH     3
#define TIMER_OUT_OC2CE_POS      15
//================================
#define TIMER_IN_CC1S_LSB        0      
#define TIMER_IN_CC1S_WIDTH      2 
#define TIMER_IN_IC1PSC_LSB      2
#define TIMER_IN_IC1PSC_WIDTH    2
#define TIMER_IN_IC1F_LSB        4
#define TIMER_IN_IC1F_WIDTH      4
#define TIMER_IN_CC2S_LSB        8
#define TIMER_IN_CC2S_WIDTH      2
#define TIMER_IN_IC2PSC_LSB      10
#define TIMER_IN_IC2PSC_WIDTH    2
#define TIMER_IN_IC2F_LSB        12
#define TIMER_IN_IC2F_WIDTH      4
//================================


typedef union   // 0x02C TIM_CCMR2
{
    struct
    {
        uint32_t CC3S:       2; // bit[1:0]  --- Capture/Compare 3 Select
        uint32_t OC3FE:      1; // bit2      --- Output Compare 3 Fast Enable
        uint32_t OC3PE:      1; // bit3      --- Output Compare 3 Preload Enable
        uint32_t OC3M:       3; // bit[6:4]  --- Output Compare 3 Mode
        uint32_t OC3CE:      1; // bit7      --- Output Compare 3 Clear Enable
        uint32_t CC4S:       2; // bit[9:8]  --- Capture/Compare 4 Select
        uint32_t OC4FE:      1; // bit10     --- Output Compare 4 Fast Enable
        uint32_t OC4PE:      1; // bit11     --- Output Compare 4 Preload Enable
        uint32_t OC4M:       3; // bit[14:12]--- Output Compare 4 Mode
        uint32_t OC4CE:      1; // bit15     --- Output Compare 4 Clear Enable
        uint32_t RES:       16; // bit[31:16]--- Reserved
    }OUTPUT;
    
    struct
    {
        uint32_t CC3S:       2; // bit[1:0]  --- Capture/Compare 3 Select
        uint32_t IC3PSC:     2; // bit[3:2]  --- Input Capture 3 Prescaler
        uint32_t IC3F:       4; // bit[7:4]  --- Input Capture 3 Filter
        uint32_t CC4S:       2; // bit[9:8]  --- Capture/Compare 4 Select
        uint32_t IC4PSC:     2; // bit[11:10]--- Input Capture 4 Prescaler
        uint32_t IC4F:       4; // bit[15:12]--- Input Capture 4 Filter
        uint32_t RES:       16; // bit[31:16]--- Reserved
    }INPUT;
    uint32_t Word;
} TIMER_CCMR2_Typedef;


//================================
#define TIMER_OUT_CC3S_LSB       0
#define TIMER_OUT_CC3S_WIDTH     2
#define TIMER_OUT_OC3FE_POS      2
#define TIMER_OUT_OC3PE_POS      3
#define TIMER_OUT_OC3M_LSB       4
#define TIMER_OUT_OC3M_WIDTH     3 
#define TIMER_OUT_OC3CE_POS      7
#define TIMER_OUT_CC4S_LSB       8
#define TIMER_OUT_CC4S_WIDTH     2
#define TIMER_OUT_OC4FE_POS      10
#define TIMER_OUT_OC4PE_POS      11
#define TIMER_OUT_OC4M_LSB       12
#define TIMER_OUT_OC4M_WIDTH     3
#define TIMER_OUT_OC4CE_POS      15
//================================
#define TIMER_IN_CC3S_LSB        0      
#define TIMER_IN_CC3S_WIDTH      2 
#define TIMER_IN_IC3PSC_LSB      2
#define TIMER_IN_IC3PSC_WIDTH    2
#define TIMER_IN_IC3F_LSB        4
#define TIMER_IN_IC3F_WIDTH      4
#define TIMER_IN_CC4S_LSB        8
#define TIMER_IN_CC4S_WIDTH      2
#define TIMER_IN_IC4PSC_LSB      10
#define TIMER_IN_IC4PSC_WIDTH    2
#define TIMER_IN_IC4F_LSB        12
#define TIMER_IN_IC4F_WIDTH      4
//================================


typedef union   // 0x030 TIM_CCER
{
    struct
    {
        uint32_t CC1E:       1; //bit0      --- Capture/Compare 1 Output Enable
        uint32_t CC1P:       1; //bit1      --- Capture/Compare 1 Output Polarity
        uint32_t CC1NE:      1; //bit2      --- Capture/Compare 1 Complementary Output Enable
        uint32_t CC1NP:      1; //bit3      --- Capture/Compare 1 Complementary Output Polarity
        uint32_t CC2E:       1; //bit4      --- Capture/Compare 2 Output Enable
        uint32_t CC2P:       1; //bit5      --- Capture/Compare 2 Output Polarity
        uint32_t CC2NE:      1; //bit6      --- Capture/Compare 2 Complementary Output Enable
        uint32_t CC2NP:      1; //bit7      --- Capture/Compare 2 Complementary Output Polarity
        uint32_t CC3E:       1; //bit8      --- Capture/Compare 3 Output Enable
        uint32_t CC3P:       1; //bit9      --- Capture/Compare 3 Output Polarity
        uint32_t CC3NE:      1; //bit10     --- Capture/Compare 3 Complementary Output Enable
        uint32_t CC3NP:      1; //bit11     --- Capture/Compare 3 Complementary Output Polarity
        uint32_t CC4E:       1; //bit12     --- Capture/Compare 4 Output Enable
        uint32_t CC4P:       1; //bit13     --- Capture/Compare 4 Output Polarity
        uint32_t RES:       18; //bit[31:14]--- Reserved
    };
    uint32_t Word;
} TIMER_CCER_Typedef;

//================================
#define TIMER_CC1E_POS   0
#define TIMER_CC1P_POS   1
#define TIMER_CC1NE_POS  2
#define TIMER_CC1NP_POS  3
#define TIMER_CC2E_POS   4
#define TIMER_CC2P_POS   5
#define TIMER_CC2NE_POS  6   
#define TIMER_CC2NP_POS  7
#define TIMER_CC3E_POS   8
#define TIMER_CC3P_POS   9
#define TIMER_CC3NE_POS  10
#define TIMER_CC3NP_POS  11
#define TIMER_CC4E_POS   12  
#define TIMER_CC4P_POS   13
//================================

typedef union   // 0x054 TIM_BDTR
{
    struct
    {
        uint32_t DTG:        8; // bit[7:0]  --- Dead-Time Generator Setup
        uint32_t LOCK:       2; // bit[9:8]  --- Lock Configuration
        uint32_t OSSI:       1; // bit10     --- Off-state Selection for Idle Mode
        uint32_t OSSR:       1; // bit11     --- Off-state Selection for Run Mode
        uint32_t BKE:        1; // bit12     --- Break Enable
        uint32_t BKP:        1; // bit13     --- Break Polarity
        uint32_t AOE:        1; // bit14     --- Automatic Output Enable
        uint32_t MOE:        1; // bit15     --- Main Output Enable
        uint32_t RES:       16; // bit[31:16]--- Reserved
    };
    uint32_t Word;
} TIMER_BDTR_Typedef;


//================================
#define TIMER_DTG_LSB    0
#define TIMER_DTG_WIDTH  8
#define TIMER_LOCK_LSB   8
#define TIMER_LOCK_WIDTH 2
#define TIMER_OSSI_POS   10
#define TIMER_OSSR_POS   11
#define TIMER_BKE_POS    12
#define TIMER_BKP_POS    13
#define TIMER_AOE_POS    14
#define TIMER_MOE_POS    15
//===================================


typedef union   // 0x058 TIM_DMAEN
{
    struct
    {
        uint32_t UDE:        1; // bit0     --- Update DMA Request Enable
        uint32_t CC1DE:      1; // bit1     --- Capture/Compare 1 DMA Request Enable
        uint32_t CC2DE:      1; // bit2     --- Capture/Compare 2 DMA Request Enable
        uint32_t CC3DE:      1; // bit3     --- Capture/Compare 3 DMA Request Enable
        uint32_t CC4DE:      1; // bit4     --- Capture/Compare 4 DMA Request Enable
        uint32_t COMDE:      1; // bit5     --- COM DMA Request Enable
        uint32_t TDE:        1; // bit6     --- Trigger DMA Request Enable
        uint32_t RES:       25; // bit[31:7]--- Reserved
    };
    uint32_t Word;
} TIMER_DMAEN_Typedef;


//================================
#define TIMER_UDE_POS                        0
#define TIMER_CC1DE_POS                      1
#define TIMER_CC2DE_POS                      2
#define TIMER_CC3DE_POS                      3
#define TIMER_CC4DE_POS                      4
#define TIMER_COMDE_POS                      5
#define TIMER_TDE_POS                        6
//================================

typedef struct
{
    __IO TIMER_CR1_Typedef   CR1  ; // 0x000 --- Control register 1
    __IO TIMER_CR2_Typedef   CR2  ; // 0x004 --- Control register 2
    __IO TIMER_SMCR_Typedef  SMCR ; // 0x008 --- Slave mode control register
    __O  TIMER_INT_Typedef   IER  ; // 0x00C --- Interrupt Enable register
    __O  TIMER_INT_Typedef   IDR  ; // 0x010 --- Interrupt Disable register
    __I  TIMER_INT_Typedef   IVS  ; // 0x014 --- Interrupt Valid status register
    __I  TIMER_INT_Typedef   RIF  ; // 0x018 --- Interrupt Raw interrupt Flag
    __I  TIMER_INT_Typedef   IFM  ; // 0x01C --- Interrupt Masked interrupt Flag
    __O  TIMER_INT_Typedef   ICR  ; // 0x020 --- Interrupt Clear status register
    __IO TIMER_EGR_Typedef   EGR  ; // 0x024 --- Event generation register
    union {
        __IO uint32_t        CCMR[2];
      struct {
        __IO TIMER_CCMR1_Typedef CCMR1; // 0x028 Capture/compare mode register 1
        __IO TIMER_CCMR2_Typedef CCMR2; // 0x02C Capture/compare mode register 2
      };
    };
    __IO TIMER_CCER_Typedef  CCER ; // 0x030 --- Capture/compare enable register
    __IO uint32_t            CNT  ; // 0x034 --- Timer Counter
    __IO uint32_t            PSC  ; // 0x038 --- Prescaler
    __IO uint32_t            ARR  ; // 0x03C --- Auto-reload register
    __IO uint32_t            RCR  ; // 0x040 --- Repetition counter register
    union {
        __IO uint32_t        CCR[4];
      struct {
        __IO uint32_t        CCR1 ; // 0x044 --- Capture/compare register 1
        __IO uint32_t        CCR2 ; // 0x048 --- Capture/compare register 2
        __IO uint32_t        CCR3 ; // 0x04C --- Capture/compare register 3
        __IO uint32_t        CCR4 ; // 0x050 --- Capture/compare register 4
      };
    };
    __IO TIMER_BDTR_Typedef  BDTR ; // 0x054 --- Break and dead-time register
    __IO TIMER_DMAEN_Typedef DMAEN; // 0x058 --- DMA trigger event enable
} TIMER_TypeDef;


#define CTMR  ((TIMER_TypeDef *) CTMR_BASE) 
#define ATMR  ((TIMER_TypeDef *) ATMR_BASE) 

#endif // _REG_TIMER_H_
