/**
 ****************************************************************************************
 *
 * @file gsm_audio.h
 *
 * @brief GSM audio playback interface header file
 *
 * @details This module provides GSM 6.10 audio playback functionality with both
 *          blocking and non-blocking modes. It uses a ping-pong buffer
 *          to enable smooth audio playback while keeping CPU available for
 *          other tasks (e.g., BLE stack processing).
 *
 *          Data Format in Flash:
 *          [uint32_t length][GSM frame 0][GSM frame 1]...
 *          - Each GSM frame is 33 bytes
 *          - First 4 bytes contain total data length (little-endian)
 *          - Data length is followed by continuous GSM frames
 *
 *          For embedded data (gsm_audio_data), frames are stored without
 *          length prefix (raw GSM frames only).
 *
 *          Non-Blocking Usage:
 *          1. Call gsm_play_start_memory() to start
 *          2. Call gsm_play_poll() repeatedly in main loop
 *          3. Function returns 0 while playing, 1 when complete
 *          4. Loop continues until gsm_audio_stop() is called
 *
 ****************************************************************************************
 */

#ifndef GSM_AUDIO_H
#define GSM_AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PUBLIC API FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize GSM audio system
 *
 * @details Creates GSM decoder instance and initializes PWM speaker output.
 *          Must be called once before using any other GSM audio functions.
 *
 * @note  Call once at startup before using playback functions
 ****************************************************************************************
 */
void gsm_audio_init(void);

/**
 ****************************************************************************************
 * @brief Start non-blocking playback from memory with known size
 *
 * @param data_addr    Memory address containing raw GSM frames (no length prefix)
 * @param data_size    Total size of GSM data in bytes
 * @param loop_play    0 = play once, 1 = loop continuously (default: 0)
 * @return              0 on success, -1 on error
 *
 * @details Non-blocking playback initialization for embedded audio data.
 *          Suitable for use with gsm_audio_data[] which has no length prefix.
 *
 *          This function initializes the playback state machine and returns
 *          immediately. Call gsm_play_poll() repeatedly in main loop to
 *          advance playback.
 *
 *          State machine processes one step per poll call, allowing
 *          CPU to handle other tasks (BLE, UART, etc.) between steps.
 *
 * Example:
 * @code
 *   // Start non-blocking playback from embedded data, play once
 *   extern uint8_t gsm_audio_data[];
 *   gsm_play_start_memory((uint32_t)gsm_audio_data, 3696, 0);
 *
 *   // In main loop:
 *   while (1) {
 *       ble_schedule();
 *       if (gsm_play_poll()) {
 *           // Playback complete
 *       }
 *   }
 * @endcode
 ****************************************************************************************
 */
int gsm_play_start_memory(uint32_t data_addr, uint32_t data_size, int loop_play);

/**
 ****************************************************************************************
 * @brief Start non-blocking playback from Flash address
 *
 * @param flash_addr   Flash address containing GSM data (first 4 bytes = length)
 * @param loop_play    0 = play once, 1 = loop continuously (default: 0)
 * @return              0 on success, -1 on error
 *
 * @details Non-blocking playback initialization for Flash data with length prefix.
 *          This function initializes the playback state machine and returns
 *          immediately. Call gsm_play_poll() repeatedly in main loop to
 *          advance playback.
 *
 *          Flash data format: [uint32_t length][GSM frames...]
 *
 * Example:
 * @code
 *   // Start non-blocking playback from Flash, loop mode
 *   gsm_play_start(0x18020000, 1);
 *
 *   // In main loop:
 *   while (1) {
 *       ble_schedule();
 *       if (gsm_play_poll()) {
 *           // Auto-restart if loop_play=1
 *       }
 *   }
 * @endcode
 ****************************************************************************************
 */
int gsm_play_start(uint32_t flash_addr, int loop_play);

/**
 ****************************************************************************************
 * @brief Poll function for non-blocking playback state machine
 *
 * @return 0 if still playing, 1 if playback completed
 *
 * @details Advances the playback state machine by one step.
 *          Call this function repeatedly in main loop.
 *
 *          State machine flow:
 *          - PREFILL: Decode first frames to fill ping-pong buffers
 *          - START_PLAY: Begin DMA playback
 *          - DECODE_NEXT: Decode next frame to idle buffer
 *          - WAIT_DMA: Check if DMA transfer complete
 *          - PLAY_NEXT: Start DMA for next buffer
 *          - COMPLETE: Playback finished
 *
 *          Each state executes quickly (< 1ms), leaving CPU time
 *          for BLE stack and other tasks.
 *
 * @note  Only valid after calling gsm_play_start() or gsm_play_start_memory()
 * @note  Returns 1 when playback completes (call start function again to restart)
 ****************************************************************************************
 */
int gsm_play_poll(void);

/**
 ****************************************************************************************
 * @brief Check if audio playback is active
 *
 * @return 1 if playing, 0 if stopped
 *
 * @details Query current playback status without blocking.
 *          Useful for UI indicators or conditional logic.
 ****************************************************************************************
 */
uint8_t gsm_audio_is_playing(void);

/**
 ****************************************************************************************
 * @brief Stop current playback immediately
 *
 * @details Stops audio playback and resets state machine to idle.
 *          Can be called from interrupt context or main loop.
 *          Use AT+GSMSTOP command to stop from user input.
 ****************************************************************************************
 */
void gsm_audio_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* GSM_AUDIO_H */
