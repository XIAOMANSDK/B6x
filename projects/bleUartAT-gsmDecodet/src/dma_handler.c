/**
 ****************************************************************************************
 *
 * @file dma_handler.c
 *
 * @brief Unified DMA interrupt handler for UART and Speaker (PWM)
 *
 * @details This file provides a single DMAC_IRQHandler that handles DMA completion
 *          interrupts for both UART1 RX (DMA_CH7) and Speaker PWM output (DMA_CH0).
 *          This resolves the "multiply defined" linker error when both uart1Rb.c and
 *          speaker.c define their own DMAC_IRQHandler.
 *
 *          UART1 DMA uses DMA_CH7 for circular buffer RX
 *          Speaker PWM uses DMA_CH0 for audio output
 *
 * @note  The uart1Rb.c in this project has been modified to use __WEAK attribute
 *        for its DMAC_IRQHandler, allowing this unified handler to override it.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "cfg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// DMA Channel definitions
#define UART1_DMA_CHAN      DMA_CH7    // UART1 RX DMA channel (used by uart1Rb.c)
#define SPEAKER_DMA_CHNL     DMA_CH0    // Speaker PWM DMA channel (used by speaker.c)

/*
 * EXTERNAL VARIABLES
 ****************************************************************************************
 */

// External DMA done flags from uart1Rb.c
extern volatile bool pong;

// External DMA done flag from speaker.c (no longer used directly, kept for compatibility)
extern volatile uint8_t dma_done;

// External function declarations
#if (CFG_UART_DMA)
extern void uart1_dma_rx_done(void);
#endif

#if (GSM_DECODE_EN)
// Speaker DMA callback function (from speaker.c)
extern void speaker_dma_handler(void);
#endif

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Unified DMA Interrupt Handler
 *
 * @details This function handles DMA completion interrupts for multiple DMA channels.
 *          It checks the IFLAG0 register to determine which channel triggered
 *          the interrupt and dispatches to the appropriate handler:
 *          - DMA_CH7: UART1 RX (uart1Rb.c)
 *          - DMA_CH0: Speaker PWM (speaker.c)
 *
 *          The interrupt flags are cleared, then the appropriate handler is called,
 *          and finally the interrupt is re-enabled.
 ****************************************************************************************
 */
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;  ///< Get DMA interrupt flag register

    // Clear interrupt flags for all channels we handle
    DMACHCFG->ICFR0 = iflag;

#if (CFG_UART_DMA)
    // Handle UART1 DMA Channel (CH7)
    if (iflag & (1UL << UART1_DMA_CHAN))
    {
        uart1_dma_rx_done();
    }
#endif

#if (GSM_DECODE_EN)
    // Handle Speaker PWM DMA Channel (CH0)
    if (iflag & (1UL << SPEAKER_DMA_CHNL))
    {
        speaker_dma_handler();  // Call the callback in speaker.c
    }
#endif

    // Re-enable interrupts
    DMACHCFG->IEFR0 |= iflag;
}
