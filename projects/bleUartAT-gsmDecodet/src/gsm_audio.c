/**
 ****************************************************************************************
 *
 * @file gsm_audio.c
 *
 * @brief GSM audio playback interface implementation
 *
 * @details This file provides the GSM audio playback interface that combines:
 *          - GSM 6.10 decoder (from gsm-1.0-pl23 library)
 *          - Flash data reading (GSM frames stored at 0x18010000)
 *          - PWM speaker output (via speaker.c)
 *
 *          Data Format in Flash:
 *          [uint32_t length][GSM frame 0][GSM frame 1]...
 *          - Each GSM frame is 33 bytes
 *          - First 4 bytes contain total data length (little-endian)
 *          - Data length is followed by continuous GSM frames
 *
 *          Playback Process:
 *          1. Read GSM frame (33 bytes) from Flash
 *          2. Decode to PCM using gsm_decode() (160 samples, 320 bytes)
 *          3. Output PCM via PWM speaker (8kHz sample rate)
 *          4. Repeat until all frames are played
 *
 ****************************************************************************************
 */

#include "gsm_audio.h"
#include "gsm.h"
#include "cfg.h"
#include "drvs.h"
#include "dbg.h"
#include "speaker.h"

#if (GSM_DECODE_EN)

/*
 * DEFINES
 ****************************************************************************************
 */

// GSM frame size: 33 bytes per frame (GSM 06.10 standard)
#define GSM_FRAME_SIZE         33

// PCM output from GSM decoder: 160 samples per frame (320 bytes of 16-bit samples)
#define PCM_SAMPLES_PER_FRAME  160
#define PCM_BYTES_PER_FRAME    (PCM_SAMPLES_PER_FRAME * 2)

// Double buffering for smoother playback (ping-pong between two buffers)
#define DOUBLE_BUF            (1)

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

// GSM decoder state structure
static gsm gsm_decoder = NULL;

// Playback flag: 1 = actively playing, 0 = stopped
static volatile uint8_t is_playing = 0;

// Non-blocking playback state variables
static uint32_t nb_flash_addr;         // Current Flash address for reading
static uint32_t nb_total_frames;       // Total frames to play
static uint32_t nb_frame_count;        // Current frame count
static uint8_t nb_state;              // State machine state
static uint8_t nb_loop_play;          // Loop play flag: 0=once, 1=continuous
static uint8_t nb_decode_buf_idx;      // Decode buffer index (persisted across states)
static uint32_t nb_start_addr;         // Start address for loop playback
static uint32_t nb_data_size;          // Data size for loop playback

// Non-blocking playback states
enum {
    NB_STATE_IDLE = 0,
    NB_STATE_PREFILL1,     // Decode first frame to buffer[0]
    NB_STATE_PREFILL2,     // Decode second frame to buffer[1]
    NB_STATE_START_PLAY,    // Start DMA playback
    NB_STATE_DECODE_NEXT,   // Decode next frame to other buffer
    NB_STATE_WAIT_DMA,      // Wait for DMA completion
    NB_STATE_PLAY_NEXT,     // Start next buffer playback
    NB_STATE_WAIT_LAST,     // Wait for last frame to complete
    NB_STATE_COMPLETE       // Playback complete
};

#if (DOUBLE_BUF)
// Dual PCM buffers for double-buffering
static int16_t pcm_buffer[2][PCM_SAMPLES_PER_FRAME];
// Current buffer index (0 or 1) for ping-pong operation
static volatile uint8_t buf_idx = 0;
#else
// Single PCM buffer (no double buffering)
static int16_t pcm_buffer[PCM_SAMPLES_PER_FRAME];
#endif

/*
 * EXTERNAL FUNCTIONS
 ****************************************************************************************
 */

// External speaker functions from speaker.c
extern void pwmInit(void);
extern void speakerStop(void);

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Convert little-endian 16-bit value to CPU format
 * @note On Cortex-M0, little-endian is native, so this is a no-op
 ****************************************************************************************
 */
//static inline uint16_t le16_to_cpu(uint16_t val)
//{
//    return val;
//}

/**
 ****************************************************************************************
 * @brief Convert little-endian 32-bit value to CPU format
 * @note On Cortex-M0, little-endian is native, so this is a no-op
 ****************************************************************************************
 */
//static inline uint32_t le32_to_cpu(uint32_t val)
//{
//    return val;
//}

/**
 ****************************************************************************************
 * @brief Initialize GSM audio system
 *
 * @details Creates GSM decoder instance and initializes PWM speaker output.
 *          Must be called once before using any other GSM audio functions.
 *
 *          Initializes:
 *          - GSM 6.10 decoder state structure
 *          - PWM timer for audio output (via pwmInit())
 *
 * @note If decoder creation fails, error is logged but function returns
 ****************************************************************************************
 */
void gsm_audio_init(void)
{
    // Create GSM decoder instance
    gsm_decoder = gsm_create();
    if (gsm_decoder == NULL)
    {
        debug("GSM: Failed to create decoder\r\n");
        return;
    }

    debug("GSM: Decoder created at 0x%08X\r\n", (uint32_t)gsm_decoder);

    // Initialize PWM speaker output
    pwmInit();

    debug("GSM: Audio system initialized\r\n");
}



/**
 * @brief Check if audio playback is active
 * @return 1 if playing, 0 if stopped
 */
uint8_t gsm_audio_is_playing(void)
{
    return is_playing;
}

/**
 * @brief Stop current playback
 */
void gsm_audio_stop(void)
{
    if (is_playing)
    {
        is_playing = 0;
        nb_state = NB_STATE_IDLE;
        speakerStop();
        debug("GSM: Playback stopped\r\n");
    }
}

/**
 ****************************************************************************************
 * @brief Start non-blocking playback from memory with known size
 *
 * @param data_addr Memory address containing raw GSM frames (no length prefix)
 * @param data_size Total size of GSM data in bytes
 * @return 0 on success, -1 on error
 *
 * @details Similar to gsm_play_start() but takes data size directly.
 *          Used for playing from embedded data arrays (gsm_audio_data).
 ****************************************************************************************
 */
int gsm_play_start_memory(uint32_t data_addr, uint32_t data_size, int loop_play)
{
    if (gsm_decoder == NULL)
    {
        debug("GSM: Decoder not initialized\r\n");
        return -1;
    }

    if (is_playing)
    {
        debug("GSM: Already playing\r\n");
        return -1;
    }

    if (data_size == 0)
    {
        debug("GSM: Invalid data size\r\n");
        return -1;
    }

    // Initialize state machine
    nb_total_frames = data_size / GSM_FRAME_SIZE;
    nb_frame_count = 0;
    nb_flash_addr = data_addr;  // No length prefix to skip
    nb_state = NB_STATE_PREFILL1;

    nb_loop_play = loop_play;
    nb_start_addr = data_addr;    // Save for loop playback
    nb_data_size = data_size;      // Save for loop playback

    is_playing = 1;
    debug("GSM: Non-blocking play started, %d frames from 0x%08X (loop=%d)\r\n",
          nb_total_frames, data_addr, loop_play);

    return 0;
}

/**
 ****************************************************************************************
 * @brief Poll function for non-blocking playback
 *
 * @return 0 if still playing, 1 if playback completed
 *
 * @details Call this function repeatedly in main loop.
 *          Processes one state per call to avoid blocking.
 ****************************************************************************************
 */
int gsm_play_poll(void)
{
    if (!is_playing || nb_state == NB_STATE_IDLE)
    {
        return 1;  // Not playing
    }

#if (DOUBLE_BUF)
    switch (nb_state)
    {
        case NB_STATE_PREFILL1:
            // Decode first frame to buffer[0]
            //debug("GSM: Decoding frame %d at 0x%08X\r\n", nb_frame_count, nb_flash_addr);
            gsm_decode(gsm_decoder, (gsm_byte *)nb_flash_addr, (gsm_signal *)pcm_buffer[0]);
            nb_frame_count++;
            nb_flash_addr += GSM_FRAME_SIZE;

            if (nb_frame_count >= nb_total_frames)
            {
                // Only one frame total
                nb_state = NB_STATE_START_PLAY;
            }
            else
            {
                nb_state = NB_STATE_PREFILL2;
            }
            break;

        case NB_STATE_PREFILL2:
            // Decode second frame to buffer[1]
            //debug("GSM: Decoding frame %d at 0x%08X\r\n", nb_frame_count, nb_flash_addr);
            gsm_decode(gsm_decoder, (gsm_byte *)nb_flash_addr, (gsm_signal *)pcm_buffer[1]);

            nb_frame_count++;
            nb_flash_addr += GSM_FRAME_SIZE;

            buf_idx = 0;  // Will play buffer[0] first
            nb_state = NB_STATE_START_PLAY;
            break;

        case NB_STATE_START_PLAY:
            // Start DMA playback with current buffer
            //debug("GSM: Play pcm_buffer %d \r\n", buf_idx);
            speakerPlay_nonblock(pcm_buffer[buf_idx], PCM_SAMPLES_PER_FRAME);
            if (nb_frame_count >= nb_total_frames)
            {
                nb_state = NB_STATE_WAIT_LAST;
            }
            else
            {
                nb_state = NB_STATE_DECODE_NEXT;
            }
            break;

        case NB_STATE_DECODE_NEXT:
            // Decode next frame to OTHER buffer (not currently playing)
            nb_decode_buf_idx = buf_idx ^ 1;  // Toggle 0<->1
            //debug("GSM: Decoding frame %d at 0x%08X to index %d\r\n", nb_frame_count, nb_flash_addr, nb_decode_buf_idx);
            gsm_decode(gsm_decoder, (gsm_byte *)nb_flash_addr, (gsm_signal *)pcm_buffer[nb_decode_buf_idx]);
            nb_frame_count++;
            nb_flash_addr += GSM_FRAME_SIZE;

            nb_state = NB_STATE_WAIT_DMA;
            break;

        case NB_STATE_WAIT_DMA:
            // Wait for DMA to complete (check and return if not ready)
            if (!speaker_is_ready())
            {
                return 0;  // Still waiting, continue next poll
            }

            nb_state = NB_STATE_PLAY_NEXT;
            break;

        case NB_STATE_PLAY_NEXT:
            // Switch to next buffer
            buf_idx = nb_decode_buf_idx;
            //debug("GSM: Play pcm_buffer %d \r\n", buf_idx);
            speakerPlay_nonblock(pcm_buffer[buf_idx], PCM_SAMPLES_PER_FRAME);

            if (nb_frame_count >= nb_total_frames)
            {
                nb_state = NB_STATE_WAIT_LAST;
            }
            else
            {
                nb_state = NB_STATE_DECODE_NEXT;
            }
            break;

        case NB_STATE_WAIT_LAST:
            // Wait for last frame to complete
            if (!speaker_is_ready())
            {
                return 0;  // Still waiting
            }
            nb_state = NB_STATE_COMPLETE;
            break;

        case NB_STATE_COMPLETE:
            // Check if loop playback is enabled
            if (nb_loop_play)
            {
                // Restart playback from beginning
                //debug("GSM: Restarting playback (loop mode)\r\n");

                // Reinitialize state machine
                nb_total_frames = nb_data_size / GSM_FRAME_SIZE;
                nb_frame_count = 0;
                nb_flash_addr = nb_start_addr;
                nb_state = NB_STATE_PREFILL1;

                // Stay in playing mode, return 0
                return 0;
            }
            else
            {
                // Single playback completed, stop
                speakerStop();
                is_playing = 0;
                debug("GSM: Playback completed, %d frames played\r\n", nb_frame_count);
                return 1;  // Complete
            }

        default:
            break;
    }

    return 0;  // Still playing
#else
    // Single buffer mode (blocking implementation)
    // For simplicity, single buffer mode is not supported in non-blocking mode
    return 1;
#endif
}

#endif /* GSM_DECODE_EN */
