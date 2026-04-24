/**
 ****************************************************************************************
 *
 * @file example_decode_custom_io.c
 *
 * @brief MP3 decode and play example using custom I/O callbacks
 *
 * @details
 * Demonstrates MP3 decoding from a memory buffer with custom seek/read callbacks,
 * then playing decoded PCM audio through a PWM speaker via DMA.
 *
 ****************************************************************************************
 */

#include <helix_mp3.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "drvs.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define SAMPLES_PER_FRAME       (576)
#define PCM_BUFFER_SIZE_SAMPLES (576)
#define PCM_BUFFER_SIZE_FRAMES  (PCM_BUFFER_SIZE_SAMPLES / SAMPLES_PER_FRAME)

/*
 * VARIABLES
 ****************************************************************************************
 */

extern void speakerPlay(int16_t *buff, uint16_t length);
extern void speakerStop(void);

extern const uint8_t *mp3_data;
extern uint32_t mp3_data_len;

static unsigned int mp3_offset;  ///< Current read position in MP3 data

static int16_t pcm_buffer[2][PCM_BUFFER_SIZE_SAMPLES] __attribute__((aligned(4)));

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Seek callback for custom I/O
 *
 * @param user_data Unused
 * @param offset    Byte offset from start of data
 *
 * @return 0 on success
 ****************************************************************************************
 */
static int seek_callback(void *user_data, int offset)
{
    (void)user_data;
    mp3_offset = offset;
    return 0;
}

/**
 ****************************************************************************************
 * @brief Read callback for custom I/O
 *
 * @param user_data Unused
 * @param buffer    Destination buffer
 * @param size      Number of bytes to read
 *
 * @return Number of bytes actually read
 ****************************************************************************************
 */
static size_t read_callback(void *user_data, void *buffer, size_t size)
{
    (void)user_data;

    if ((mp3_offset + size + 1) <= mp3_data_len)
    {
        memcpy(buffer, mp3_data + mp3_offset, size);
        mp3_offset += size;
        return size;
    }
    else
    {
        size_t remaining = mp3_data_len - mp3_offset;
        if (remaining > 0)
        {
            memcpy(buffer, mp3_data + mp3_offset, remaining);
        }
        mp3_offset = mp3_data_len;
        return remaining;
    }
}

/**
 ****************************************************************************************
 * @brief Decode MP3 data and play through speaker
 *
 * @return 0 on success, negative on error
 ****************************************************************************************
 */
int mp3_decoder(void)
{
    helix_mp3_t mp3;
    int err;

    /* Create I/O interface */
    helix_mp3_io_t io = {
        .seek = seek_callback,
        .read = read_callback,
        .user_data = NULL
    };

    do {
        /* Initialize decoder */
        err = helix_mp3_init(&mp3, &io);
        if (err) {
            printf("Failed to init decoder, error: %d\n", err);
            break;
        }

        /* Decode and play all frames */
        uint32_t frame_cnt = 0;
        while (1) {
            GPIO_DAT_SET(GPIO14);
            const size_t frames_read = helix_mp3_read_pcm_frames_s16(&mp3, pcm_buffer[frame_cnt % 2], 1);
            GPIO_DAT_CLR(GPIO14);

            if (frames_read == 0) {
                printf("Reached EOF!\n\r");
                break;
            }

            GPIO_DAT_SET(GPIO15);
            speakerPlay(pcm_buffer[frame_cnt % 2], PCM_BUFFER_SIZE_SAMPLES);
            GPIO_DAT_CLR(GPIO15);

            frame_cnt++;
        }

        const size_t frame_count = helix_mp3_get_pcm_frames_decoded(&mp3);
        const uint32_t sample_rate = helix_mp3_get_sample_rate(&mp3);
        const uint32_t bitrate = helix_mp3_get_bitrate(&mp3);
        printf("Done! Decoded %zu frames, sample rate: %" PRIu32 "Hz, bitrate: %" PRIu32 "kbps\n",
               frame_count, sample_rate, bitrate / 1000);
        speakerStop();
    } while (0);

    helix_mp3_deinit(&mp3);

    return err;
}
