/**
 ****************************************************************************************
 *
 * @file dma.h
 *
 * @brief Header file - DMA Driver
 *
 ****************************************************************************************
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <stdint.h>
#include <stdbool.h>
#include "reg_dmachcfg.h"

// warning:  #61-D: integer operation result is out of range
#pragma diag_suppress 61


/*
 * DEFINES
 ****************************************************************************************
 */

/// DMA Channel Index
enum dma_channel
{
    // Channel primary(CH0 ~ MAX-1)
    DMA_CH0            = 0,
    DMA_CH1,
    DMA_CH2,
    DMA_CH3,
    DMA_CH4,
    DMA_CH5,
    DMA_CH6,
    DMA_CH7,
    DMA_CH_MAX,
    
    // Channel alternate(CHx + MAX)
    DMA_CH_ALT         = DMA_CH_MAX, // 0x08
    DMA_CH0_ALT,
    DMA_CH1_ALT,
    DMA_CH2_ALT,
    DMA_CH3_ALT,
    DMA_CH4_ALT,
    DMA_CH5_ALT,
    DMA_CH6_ALT,
    DMA_CH7_ALT,
};

/// DMA Peripheral Index
enum dma_peripheral
{
    DMA_PID_UART1_RX   = 0,
    DMA_PID_UART1_TX   = 1,
    DMA_PID_UART2_RX   = 2,
    DMA_PID_UART2_TX   = 3,
    DMA_PID_SADC       = 4,
    DMA_PID_SPIM_TX    = 5,
    DMA_PID_SPIM_RX    = 6,
    DMA_PID_SPIS_TX    = 7,
    DMA_PID_SPIS_RX    = 8,
    DMA_PID_CTMR_CH1   = 9,
    DMA_PID_CTMR_CH2   = 10,
    DMA_PID_CTMR_CH3   = 11,
    DMA_PID_CTMR_CH4   = 12,
    DMA_PID_CTMR_TRIG  = 13,
    DMA_PID_CTMR_UP    = 14,
    DMA_PID_ATMR_CH1   = 15,
    DMA_PID_ATMR_CH2   = 16,
    DMA_PID_ATMR_CH3   = 17,
    DMA_PID_ATMR_CH4   = 18,
    DMA_PID_ATMR_TRIG  = 19,
    DMA_PID_ATMR_UP    = 20,
    DMA_PID_ATMR_COM   = 21,
    DMA_PID_BTMR_UP    = 22,
    DMA_PID_USB        = 23,
    DMA_PID_MDM_TX     = 24,
    DMA_PID_MDM_RX     = 25,    
    DMA_PID_MAX,
};

/// DMA Pointer of Peripheral Address
enum dma_pointer
{
    DMA_PTR_UART1_RX   = 0x40023000,  // ((uint32_t)&UART1->RBR),
    DMA_PTR_UART1_TX   = 0x40023004,  // ((uint32_t)&UART1->TBR),
    DMA_PTR_UART2_RX   = 0x40024000,  // ((uint32_t)&UART2->RBR), 
    DMA_PTR_UART2_TX   = 0x40024004,  // ((uint32_t)&UART2->TBR),
    DMA_PTR_SADC_AUX   = 0x40007014,  // ((uint32_t)&SADC->AUX_ST), 
    DMA_PTR_SADC_PCM   = 0x40007018,  // ((uint32_t)&SADC->PCM_DAT), 
    DMA_PTR_SPIM_RX    = 0x40004000,  // ((uint32_t)&SPIM->RX_DATA), 
    DMA_PTR_SPIM_TX    = 0x40004004,  // ((uint32_t)&SPIM->TX_DATA),
    DMA_PTR_SPIS_TX    = 0x4000500C,  // ((uint32_t)&SPIS->TX_DAT),
    DMA_PTR_SPIS_RX    = 0x40005010,  // ((uint32_t)&SPIS->RX_DAT),  
    DMA_PTR_CTMR_CH1   = 0x40021044,  // ((uint32_t)&CTMR->CCR1),
    DMA_PTR_CTMR_CH2   = 0x40021048,  // ((uint32_t)&CTMR->CCR2),   
    DMA_PTR_CTMR_CH3   = 0x4002104C,  // ((uint32_t)&CTMR->CCR3),
    DMA_PTR_CTMR_CH4   = 0x40021050,  // ((uint32_t)&CTMR->CCR4),   
    DMA_PTR_CTMR_UP    = 0x4002103C,  // ((uint32_t)&CTMR->ARR),    
    DMA_PTR_ATMR_CH1   = 0x40022044,  // ((uint32_t)&ATMR->CCR1), 
    DMA_PTR_ATMR_CH2   = 0x40022048,  // ((uint32_t)&ATMR->CCR2),   
    DMA_PTR_ATMR_CH3   = 0x4002204C,  // ((uint32_t)&ATMR->CCR3),
    DMA_PTR_ATMR_CH4   = 0x40022050,  // ((uint32_t)&ATMR->CCR4),   
    DMA_PTR_ATMR_UP    = 0x4002203C,  // ((uint32_t)&ATMR->ARR),    
    DMA_PTR_BTMR_UP    = 0x4002003C,  // ((uint32_t)&BTMR->ARR),
    DMA_PTR_MDM_TX     = 0x40009030,  // ((uint32_t)&MDM->EXT_TX_DAT),
    DMA_PTR_MDM_RX     = 0x40009034,  // ((uint32_t)&MDM->EXT_TX_DAT), 
};

/// DMA State Machine
enum dma_state
{
    DMA_STA_IDLE       = 0x0,  // Idle
    DMA_STA_RD_CTRL    = 0x1,  // Read control
    DMA_STA_RD_SP      = 0x2,  // Read src pointer
    DMA_STA_RD_DP      = 0x3,  // Read dst pointer
    DMA_STA_RD_SRC     = 0x4,  // Read src data
    DMA_STA_WR_DST     = 0x5,  // Write dst data
    DMA_STA_WAIT_CLR   = 0x6,  // Wait request clear
    DMA_STA_WR_CTRL    = 0x7,  // Write control
    DMA_STA_STALLED    = 0x8,  // Stalled
    DMA_STA_DONE       = 0x9,  // Done
    DMA_STA_PER_SGTX   = 0xA   // Peripheral Scatter-Gather
};

/// DMA cycle ctrl mode
enum cycle_ctrl
{
    CCM_STOP       = 0x0,
    CCM_BASIC      = 0x1,
    CCM_AUTO_REQ   = 0x2,
    CCM_PING_PONG  = 0x3,
    CCM_MEM_SG     = 0x4,
    CCM_MEM_SG_ALT = 0x5,
    CCM_PER_SG     = 0x6,
    CCM_PER_SG_ALT = 0x7,
};

/// DMA unit for data-size and addr-inc
enum unit_size
{
    IN_BYTE            = 0x0,
    IN_HALF            = 0x1,
    IN_WORD            = 0x2,
    IN_NONE            = 0x3,
};

/// DMA channel control opcode
enum chnl_ctrl
{
    CHNL_DIS,   // Disable
    CHNL_EN,    // Enable
    CHNL_DONE,  // Enable, Wait done
};

/// Bits field of DMA Transfers Configuration
enum dma_trans_bfs
{
    // DMA cycle ctrl mode - bit[2:0]
    DMA_TRANS_CCMODE_LSB   = 0,
    DMA_TRANS_CCMODE_MSK   = (0x07 << DMA_TRANS_CCMODE_LSB),
    // DMA next useburst - bit3
    DMA_TRANS_NEXTUB_POS   = 3,
    DMA_TRANS_NEXTUB_BIT   = (1 << DMA_TRANS_NEXTUB_POS),
    // DMA transfers cycles(1~1024)-1 - bit[13:4]
    DMA_TRANS_CYCLES_LSB   = 4,
    DMA_TRANS_CYCLES_MSK   = (0x3FF << DMA_TRANS_CYCLES_LSB),
    // DMA transfers can occur before rearbitrates - bit[17:14]
    DMA_TRANS_RPOWER_LSB   = 14,
    DMA_TRANS_RPOWER_MSK   = (0xF << DMA_TRANS_RPOWER_LSB),
    
    // DMA HPROT[3:1] when reads source data - bit[20:18]
    DMA_TRANS_SRCPROT_LSB  = 18,
    DMA_TRANS_SRCPROT_MSK  = (0x07 << DMA_TRANS_SRCPROT_LSB),
    // DMA HPROT[3:1] when writes destination data - bit[23:21]
    DMA_TRANS_DSTPROT_LSB  = 21,
    DMA_TRANS_DSTPROT_MSK  = (0x07 << DMA_TRANS_DSTPROT_LSB),
    
    // DMA source data size - bit[25:24]
    DMA_TRANS_SRCSIZE_LSB  = 24,
    DMA_TRANS_SRCSIZE_MSK  = (0x03 << DMA_TRANS_SRCSIZE_LSB),
    // DMA control source address increment - bit[27:26]
    DMA_TRANS_SRCINC_LSB   = 26,
    DMA_TRANS_SRCINC_MSK   = (0x03 << DMA_TRANS_SRCINC_LSB),
    
    // DMA destination data size, must same with source - bit[29:28]
    DMA_TRANS_DSTSIZE_LSB  = 28,
    DMA_TRANS_DSTSIZE_MSK  = (0x03 << DMA_TRANS_DSTSIZE_LSB),
    // DMA control source address increment - bit[31:30]
    DMA_TRANS_DSTINC_LSB   = 30,
    DMA_TRANS_DSTINC_MSK   = (0x03 << DMA_TRANS_DSTINC_LSB),
};

/// DMA Transfer Configuration struct
typedef union
{
     struct
     {
        uint32_t  CYCLE_CTRL    : 3;   // bit[2:0]  --- Operating mode of the DMA cycle
        uint32_t  NEXT_USEBURST : 1;   // bit[3:3]  --- Next transfer use burst
        uint32_t  N_MINUS_1     : 10;  // bit[13:4] --- Total number of DMA transfers - 1
        uint32_t  R_POWER       : 4;   // bit[17:14]--- How many DMA transfers can occur before controller rearbitrates
        uint32_t  SRC_PROT_CTRL : 3;   // bit[20:18]--- Protect when controller reads the source data
        uint32_t  DST_PROT_CTRL : 3;   // bit[23:21]--- Protect when controller writes the destination data
        uint32_t  SRC_SIZE      : 2;   // bit[25:24]--- Size of the source data, 0-byte 1-halfword 2-word
        uint32_t  SRC_INC       : 2;   // bit[27:26]--- Source address increment
        uint32_t  DST_SIZE      : 2;   // bit[29:28]--- Destination data size, must same with src_size
        uint32_t  DST_INC       : 2;   // bit[31:30]--- Destination address increment
     };
     uint32_t Word;
} DMA_TRANS_CFG_Typedef;

/// DMA Channel control data element
typedef struct
{
    // Source data end pointer
    uint32_t                SRC_DATA_END_PTR;
    // Destination data end pointer
    uint32_t                DST_DATA_END_PTR;
    // Control data configuration
    DMA_TRANS_CFG_Typedef   TRANS_CFG_DATA;
    // Configuration reserved for reload
    DMA_TRANS_CFG_Typedef   TRANS_CFG_RESV;
} DMA_CHNL_CTRL_Typedef;

/// DMA Channel control data struct
typedef struct
{
    // Channel primary
    DMA_CHNL_CTRL_Typedef   PRI_CHNL[DMA_CH_MAX];
    // Channel alternate
    DMA_CHNL_CTRL_Typedef   ALT_CHNL[DMA_CH_MAX];
} DMA_CHNL_CTRL_STRUCT_Typedef;

/// Global DMA Channel control data base pointer
extern volatile DMA_CHNL_CTRL_STRUCT_Typedef dma_ctrl_base;


/*
 * MACROS
 ****************************************************************************************
 */

/// transfers cycle control mode
#define TRANS_CCMODE(m)      ((m) << DMA_TRANS_CCMODE_LSB)
/// transfers cycles(1~1024)-1
#define TRANS_CYCLES(c)      ((c) << DMA_TRANS_CYCLES_LSB)
/// transfers occur (before rearbitrates) = (1 << r), r>=10 means no arbitration occurs
#define TRANS_RPOWER(r)      ((r) << DMA_TRANS_RPOWER_LSB)

/// transfers state of HPROT[3:1]: b0-privileged, b1-bufferable, b2-cacheable
#define TRANS_SRCPROT(p)     ((p) << DMA_TRANS_SRCPROT_LSB)
#define TRANS_DSTPROT(p)     ((p) << DMA_TRANS_DSTPROT_LSB)

/// transfers unit size: 0-byte, 1-halfword, 2-word
#define TRANS_UNIT(size)     (((size) << DMA_TRANS_SRCSIZE_LSB) | ((size) << DMA_TRANS_DSTSIZE_LSB))
/// transfers address increment: 0-byte, 1-halfword, 2-word, 3-no increment(address remains)
#define TRANS_SRCINC(inc)    ((inc) << DMA_TRANS_SRCINC_LSB)
#define TRANS_DSTINC(inc)    ((inc) << DMA_TRANS_DSTINC_LSB)

/// transfers to read Peripheral(as SRC)
#define TRANS_PER_RD(ccm, len, size, inc) \
            (TRANS_CCMODE(ccm) | TRANS_CYCLES((len)-1) | TRANS_UNIT(size) | TRANS_SRCINC(IN_NONE) | TRANS_DSTINC(inc))

/// transfers to write Peripheral(as DST)
#define TRANS_PER_WR(ccm, len, size, inc) \
            (TRANS_CCMODE(ccm) | TRANS_CYCLES((len)-1) | TRANS_UNIT(size) | TRANS_SRCINC(inc) | TRANS_DSTINC(IN_NONE))

/// DMA Peripheral Configure
// UARTx RX/TX
#define DMA_UARTx_RX_INIT(chidx, x)     dma_chnl_init(chidx, DMA_PID_UART##x##_RX)
#define DMA_UARTx_RX_CONF(chidx, x, buff, len, ccm)  \
            dma_chnl_conf(chidx, DMA_PTR_UART##x##_RX, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_BYTE, IN_BYTE))

#define DMA_UARTx_TX_INIT(chidx, x)     dma_chnl_init(chidx, DMA_PID_UART##x##_TX)
#define DMA_UARTx_TX_CONF(chidx, x, buff, len, ccm)  \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_UART##x##_TX, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

// Common Timer CHx
#define DMA_CTMR_CHx_INIT(chidx, x)    dma_chnl_init(chidx, DMA_PID_CTMR_CH##x)
#define DMA_CTMR_CHx_CONF(chidx, x, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_CTMR_CH##x, TRANS_PER_WR(ccm, len, IN_WORD, IN_WORD))
#define DMA_CTMR_IN_CHx_CONF(chidx, x, buff, len, ccm) \
            dma_chnl_conf(chidx, DMA_PTR_CTMR_CH##x, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_WORD, IN_WORD))
            
// Advance Timer CHx
#define DMA_ATMR_CHx_INIT(chidx, x)    dma_chnl_init(chidx, DMA_PID_ATMR_CH##x)
#define DMA_ATMR_CHx_CONF(chidx, x, buff, len, ccm) \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_ATMR_CH##x, TRANS_PER_WR(ccm, len, IN_WORD, IN_WORD))

// SPI Master Role
#define DMA_SPIM_TX_INIT(chidx)         dma_chnl_init(chidx, DMA_PID_SPIM_TX)
#define DMA_SPIM_TX_CONF(chidx, buff, len, ccm)      \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_SPIM_TX, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

#define DMA_SPIM_RX_INIT(chidx)         dma_chnl_init(chidx, DMA_PID_SPIM_RX)
#define DMA_SPIM_RX_CONF(chidx, buff, len, ccm)      \
            dma_chnl_conf(chidx, DMA_PTR_SPIM_RX, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_BYTE, IN_BYTE))

// SPI Slave Role
#define DMA_SPIS_TX_INIT(chidx)         dma_chnl_init(chidx, DMA_PID_SPIS_TX)
#define DMA_SPIS_TX_CONF(chidx, buff, len, ccm)      \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_SPIS_TX, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))

#define DMA_SPIS_RX_INIT(chidx)         dma_chnl_init(chidx, DMA_PID_SPIS_RX)
#define DMA_SPIS_RX_CONF(chidx, buff, len, ccm)      \
            dma_chnl_conf(chidx, DMA_PTR_SPIS_RX, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_BYTE, IN_BYTE))

// SADC Aux or PCM
#define DMA_SADC_INIT(chidx)            dma_chnl_init(chidx, DMA_PID_SADC)
#define DMA_SADC_AUX_CONF(chidx, buff, len, ccm)     \
            dma_chnl_conf(chidx, DMA_PTR_SADC_AUX, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_HALF, IN_HALF))
#define DMA_SADC_PCM_CONF(chidx, buff, len, ccm)     \
            dma_chnl_conf(chidx, DMA_PTR_SADC_PCM, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_HALF, IN_HALF))

// MDM Ext Data
#define DMA_MDM_TX_INIT(chidx)          dma_chnl_init(chidx, DMA_PID_MDM_TX)
#define DMA_MDM_TX_CONF(chidx, buff, len, ccm)       \
            dma_chnl_conf(chidx, (uint32_t)&(buff)[(len)-1], DMA_PTR_MDM_TX, TRANS_PER_WR(ccm, len, IN_BYTE, IN_BYTE))
            
#define DMA_MDM_RX_INIT(chidx)          dma_chnl_init(chidx, DMA_PID_MDM_RX)
#define DMA_MDM_RX_CONF(chidx, buff, len, ccm)       \
            dma_chnl_conf(chidx, DMA_PTR_MDM_RX, (uint32_t)&(buff)[(len)-1], TRANS_PER_RD(ccm, len, IN_BYTE, IN_BYTE))

// chidx: @see enum dma_channel(Note:only primary channle enum dma_channel)
// Enable DMA Channle Interrupt.
#define DMACHNL_INT_EN(chidx)       ( DMACHCFG->IEFR0 |= (0x01UL << (chidx)) )

// Disable DMA Channle Interrupt.
#define DMACHNL_INT_DIS(chidx)      ( DMACHCFG->IEFR0 &= ~(0x01UL << (chidx)) )

// Clear DMA Channle Interrupt Flag.
#define DMACHNL_INT_CLR(chidx)      ( DMACHCFG->ICFR0 = (0x01UL << (chidx)) )

// Get DMA Channle Interrupt Flag.
#define DMACHNL_INT_GET(chidx)      ( (DMACHCFG->IFLAG0 >> chidx) & 0x01UL)

// Get All DMA Channle Interrupt Flag.
#define DMACHNL_INT_GET_ALL()       ( DMACHCFG->IFLAG0 )

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
 
/**
 ****************************************************************************************
 * @brief Init DMA Module.
 * 
 ****************************************************************************************
 */
void dma_init(void);

/**
 ****************************************************************************************
 * @brief Deinit DMA Module.
 *
 ****************************************************************************************
 */
void dma_deinit(void);

/**
 ****************************************************************************************
 * @brief Init DMA Channel.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *only primary part*
 * @param[in] chsel  Selected peripheral @see enum dma_peripheral
 *
 ****************************************************************************************
 */
void dma_chnl_init(uint8_t chidx, uint8_t chsel);

/**
 ****************************************************************************************
 * @brief Deinit DMA Channel.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *only primary part*
 *
 ****************************************************************************************
 */
void dma_chnl_deinit(uint8_t chidx);

/**
 ****************************************************************************************
 * @brief Configure DMA Channel Control, then enable.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *incl alternate part*
 * @param[in] src_ep Source data end pointer address, eg. (uint32_t)&buff[len-1]
 * @param[in] dst_ep Destination data end pointer address
 * @param[in] trans  Transfer control value @see enum dma_trans_bfs or DMA_TRANS_CFG_Typedef
 *
 ****************************************************************************************
 */
void dma_chnl_conf(uint8_t chidx, uint32_t src_ep, uint32_t dst_ep, uint32_t trans);

/**
 ****************************************************************************************
 * @brief Reload tansfer control value that configured via dma_chnl_conf(), continue run.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *only primary part*
 *
 * @return 'true' means auto turned to alt-channel *only ping-pong mode*
 ****************************************************************************************
 */
bool dma_chnl_reload(uint8_t chidx);

/**
 ****************************************************************************************
 * @brief Read DMA Channel remain cycles.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *incl alternate part*
 *
 * @return Cycles of wait transfer, 0 means done
 ****************************************************************************************
 */
uint16_t dma_chnl_remain(uint8_t chidx);

/**
 ****************************************************************************************
 * @brief Read DMA Channel CCM_PING_PONG Mode remain cycles.
 *
 * @param[in]  chidx  Channel index @see enum dma_channel, *incl alternate part*
 * @param[out] len    remain cycles of wait transfer.
 *
 * @return true is alt-channel(pong), false is primary-channel(ping).
 ****************************************************************************************
 */
bool dma_chnl_remain_pingpong(uint8_t chidx, uint16_t *len);

/**
 ****************************************************************************************
 * @brief Enable or disable DMA Channel after configured.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *only primary part*
 * @param[in] ctrl   Control opcode @see enum chnl_ctrl
 *
 ****************************************************************************************
 */
void dma_chnl_ctrl(uint8_t chidx, uint8_t ctrl);

/**
 ****************************************************************************************
 * @brief Read DMA Channel done state, auto clear if be done.
 *
 * @param[in] chidx  Channel index @see enum dma_channel, *only primary part*
 *
 * @return done state, true means done
 ****************************************************************************************
 */
bool dma_chnl_done(uint8_t chidx);

#endif// _DMA_H_
