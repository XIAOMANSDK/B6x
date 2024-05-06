#ifndef _REG_DMA_H_
#define _REG_DMA_H_

#include "reg_base.h" 

//================================
//BLOCK DMA define 

#define DMA_BASE                               ((uint32_t)0x40026000) 
#define DMA_STATUS_ADDR_OFFSET                 0x000 
#define DMA_CFG_ADDR_OFFSET                    0x004 
#define DMA_CTRLBASE_POINTER_ADDR_OFFSET       0x008 
#define DMA_ALTCTRLBASE_POINTER_ADDR_OFFSET    0x00c 
#define DMA_WAIT_REQ_STA_ADDR_OFFSET           0x010 
#define DMA_SOFT_REQ_ADDR_OFFSET               0x014 
#define DMA_USEBURST_SET_ADDR_OFFSET           0x018 
#define DMA_USEBURST_CLR_ADDR_OFFSET           0x01c 
#define DMA_REQMSK_SET_ADDR_OFFSET             0x020 
#define DMA_REQMSK_CLR_ADDR_OFFSET             0x024 
#define DMA_CHNL_EN_SET_ADDR_OFFSET            0x028 
#define DMA_CHNL_EN_CLR_ADDR_OFFSET            0x02c 
#define DMA_PRIALT_SET_ADDR_OFFSET             0x030 
#define DMA_PRIALT_CLR_ADDR_OFFSET             0x034 
#define DMA_PRIORITY_SET_ADDR_OFFSET           0x038 
#define DMA_PRIORITY_CLR_ADDR_OFFSET           0x03c 
#define DMA_ERR_CLR_ADDR_OFFSET                0x04c 
#define DMA_INTEGR_CFG_ADDR_OFFSET             0xe00 
#define DMA_STALL_STA_ADDR_OFFSET              0xe08 
#define DMA_REQ_STA_ADDR_OFFSET                0xe10 
#define DMA_SREQ_STA_ADDR_OFFSET               0xe18 
#define DMA_DONE_SET_ADDR_OFFSET               0xe20 
#define DMA_DONE_CLR_ADDR_OFFSET               0xe24 
#define DMA_ACTIVE_SET_ADDR_OFFSET             0xe28 
#define DMA_ACTIVE_CLR_ADDR_OFFSET             0xe2c 
#define DMA_ERR_SET_ADDR_OFFSET                0xe48 

//================================
//BLOCK DMA reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t EN:                                 1; // bit0 ---
                                                        // 0: controller is disabled
                                                        // 1: controller is enabled
        uint32_t RSV_NOUSE1:                         3; // bit[3:1] --- Ignore me
        uint32_t STATE:                              4; // bit[7:4] --- current state of the control state machine 
        uint32_t RSV_NOUSE2:                         8; // bit[15:8]--- Ignore me
        uint32_t CHNLS_MINUS1:                       5; // bit[20:16]-- number of availiable dma channels minus one
                                                        // for example 00000 = controller configured to use 1 dma channel
        uint32_t RSV_NOUSE3:                         7; // bit[27:21]-- Ignore me
        uint32_t TEST_STATUS:                        4; // bit[31:28]--
                                                        // 0: controller does not include the integration test logic
                                                        // 1: controller include the integration test logic
                                                        // 2-0xf = undefined
    };
    uint32_t Word;
} DMA_STATUS_TypeDef; //0x000 


//================================
#define DMA_EN_POS                          0
#define DMA_STATE_LSB                       4
#define DMA_STATE_WIDTH                     4
#define DMA_CHNLS_MINUS1_LSB                16
#define DMA_CHNLS_MINUS1_WIDTH              5
#define DMA_TEST_STATUS_LSB                 28
#define DMA_TEST_STATUS_WIDTH               4
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t EN:                                 1; // bit0     --- enble the controller
        uint32_t RSV_NOUSE1:                         4; // bit[4:1] --- Ignore me
        uint32_t CHNL_PROT_CTRL:                     3; // bit[7:5] --- [5]cacheable, [6]bufferable, [7]priviledged
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} DMA_CFG_TypeDef; //0x004 


//================================
#define DMA_EN_POS                          0
#define DMA_CHNL_PROT_CTRL_LSB              5
#define DMA_CHNL_PROT_CTRL_WIDTH            3
//================================

typedef union //0x008 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         8; // bit[7:0] --- Ignore me
        uint32_t CTRL_BASE_PTR:                     24; // bit[31:8]--- pointer to the base address of the primary data structure
    };
    uint32_t Word;
} DMA_CTRLBASE_POINTER_TypeDef; //0x008 


//================================
#define DMA_CTRL_BASE_PTR_LSB               8
#define DMA_CTRL_BASE_PTR_WIDTH             24
//================================

//================================
//BLOCK DMA top struct define 
typedef struct
{
    __I   DMA_STATUS_TypeDef                     STATUS              ; // 0x000,  
    __O   DMA_CFG_TypeDef                        CFG                 ; // 0x004,  
    __IO  DMA_CTRLBASE_POINTER_TypeDef           CTRLBASE_POINTER    ; // 0x008,  
    __I   uint32_t                               ALTCTRLBASE_POINTER ; // 0x00c,  
    __I   uint32_t                               WAIT_REQ_STA        ; // 0x010, 
                                                                       // channel 0 wait on request status  
    __O   uint32_t                               SOFT_REQ            ; // 0x014, 
                                                                       // channel 0 generate a software dma request 
    __IO  uint32_t                               USEBURST_SET        ; // 0x018, 
                                                                       // disable dma_sreq[0] generating dma requests 
    __IO  uint32_t                               USEBURST_CLR        ; // 0x01c, 
                                                                       // enable dma_sreq[0] dma requests 
    __IO  uint32_t                               REQMSK_SET          ; // 0x020, 
                                                                       // disable channel[0] generating dma requests 
    __IO  uint32_t                               REQMSK_CLR          ; // 0x024, 
                                                                       // enable channel[0] generating dma requests 
    __IO  uint32_t                               CHNL_EN_SET         ; // 0x028, 
                                                                       // enable channel[0]
    __IO  uint32_t                               CHNL_EN_CLR         ; // 0x02c, 
                                                                       // disable channel[0] 
    __IO  uint32_t                               PRIALT_SET          ; // 0x030, 
                                                                       // config channel[] to use the alternate data structure 
    __IO  uint32_t                               PRIALT_CLR          ; // 0x034, 
                                                                       // config channel[0] to use the primary data structure 
    __IO  uint32_t                               PRIORITY_SET        ; // 0x038, 
                                                                       // config channel[0] priority 
    __IO  uint32_t                               PRIORITY_CLR        ; // 0x03c, 
                                                                       // clr channel[0] priority 
    __I   uint32_t                               RSV0[3]             ;
    __IO  uint32_t                               ERR_CLR             ; // 0x04c,  
    __I   uint32_t                               RSV1[876]           ;
    __IO  uint32_t                               INTEGR_CFG          ; // 0xe00,  
    __I   uint32_t                               RSV2[1]             ;
    __I   uint32_t                               STALL_STA           ; // 0xe08,  
    __I   uint32_t                               RSV3[1]             ;
    __I   uint32_t                               REQ_STA             ; // 0xe10, 
                                                                       // return status of dma_req[0] 
    __I   uint32_t                               RSV4[1]             ;
    __I   uint32_t                               SREQ_STA            ; // 0xe18, 
                                                                       // return status of dma_sreq[0] 
    __I   uint32_t                               RSV5[1]             ;
    __IO  uint32_t                               DONE_SET            ; // 0xe20, 
                                                                       // enable to assert dma_done[0] signals 
    __O   uint32_t                               DONE_CLR            ; // 0xe24, 
                                                                       // enable to deassert dma_done[0] signals 
    __IO  uint32_t                               ACTIVE_SET          ; // 0xe28, 
                                                                       // enable to assert dma_active[0] signals 
    __O   uint32_t                               ACTIVE_CLR          ; // 0xe2c, 
                                                                       // enable to desassert dma_active[0] signals 
    __I   uint32_t                               RSV6[6]             ;
    __IO  uint32_t                               ERR_SET             ; // 0xe48,  
} DMA_TypeDef;


#define DMA  (( DMA_TypeDef  *)     DMA_BASE)

#endif
