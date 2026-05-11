/**
 ****************************************************************************************
 *
 * @file speaker.h
 *
 * @brief PWM speaker driver for GSM audio output
 *
 * @details This header file declares the API for the PWM speaker driver that
 *          provides audio output capability for GSM decoded PCM samples.
 *
 *          Hardware Configuration:
 *          - PWM+ Pin: PA07 (configurable via PA_ATMR_P in speaker.c)
 *          - PWM- Pin: PA11 (differential output)
 *          - DMA Channel: DMA_CH0
 *          - PWM Frequency: 64kHz carrier / 8 = 8kHz sample rate
 *
 ****************************************************************************************
 */

#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 ****************************************************************************************
 * @brief Initialize PWM speaker for audio output
 *
 * @details Initializes ATMR peripheral for PWM generation with DMA support.
 *          Sets up 64kHz PWM carrier frequency divided by 8 = 8kHz sample rate.
 ****************************************************************************************
 */
void pwmInit(void);

/**
 ****************************************************************************************
 * @brief Play audio samples via PWM speaker (blocking)
 *
 * @param buff Pointer to 16-bit PCM samples (signed)
 * @param samples Number of samples to play (typically 160 for GSM frame)
 *
 * @details This function blocks until the DMA transfer completes.
 *          Waits for previous DMA transfer to complete before starting new one.
 ****************************************************************************************
 */
void speakerPlay(int16_t *buff, uint16_t samples);

/**
 ****************************************************************************************
 * @brief Stop speaker output (mute)
 *
 * @details Sets PWM duty cycle to 50% (mute/center position)
 ****************************************************************************************
 */
void speakerStop(void);

/**
 ****************************************************************************************
 * @brief Check if speaker DMA transfer is complete
 *
 * @return 1 if DMA is done (ready for next transfer), 0 if busy
 *
 * @details Non-blocking check for DMA completion status.
 *          Used for ping-pong buffer implementation.
 ****************************************************************************************
 */
uint8_t speaker_is_ready(void);

/**
 ****************************************************************************************
 * @brief Non-blocking speaker play
 *
 * @param buff Pointer to 16-bit PCM samples (signed)
 * @param samples Number of samples to play
 *
 * @details This function assumes dma_done is already set (speaker_is_ready() returned true).
 *          Does NOT wait for previous transfer to complete.
 *          Used for ping-pong buffer implementation.
 *
 * @note  Call speaker_is_ready() first to ensure DMA is ready
 ****************************************************************************************
 */
void speakerPlay_nonblock(int16_t *buff, uint16_t samples);

#ifdef __cplusplus
}
#endif

#endif /* SPEAKER_H */
