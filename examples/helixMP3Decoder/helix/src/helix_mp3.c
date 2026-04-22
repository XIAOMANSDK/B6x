/**
 ****************************************************************************************
 *
 * @file helix_mp3.c
 *
 * @brief Helix MP3 decoder wrapper with custom I/O support
 *
 * @details
 * Provides a streaming MP3 decoder interface using the Helix MP3 library.
 * Supports custom I/O callbacks for reading from memory, flash, or file.
 * Handles ID3v2 tag skipping, sync word detection, and frame decoding.
 *
 ****************************************************************************************
 */

#include "helix_mp3.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * DEFINES
 ****************************************************************************************
 */

#define HELIX_MP3_MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define HELIX_MP3_SAMPLES_PER_FRAME  (576)

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Skip ID3v2 tag at the beginning of MP3 data
 *
 * @param mp3 Decoder context
 *
 * @return Tag size on success, 0 if no tag found, -1 on error
 ****************************************************************************************
 */
static int helix_mp3_skip_id3v2_tag(helix_mp3_t *mp3)
{
    const size_t id3v2_frame_header_size = 10;
    const size_t id3v2_frame_offset = 0;
    const size_t id3v2_frame_magic_string_length = 3;
    const char *id3v2_frame_magic_string = "ID3";

    uint8_t frame_buffer[id3v2_frame_header_size];

    if (mp3->io->seek(mp3->io->user_data, id3v2_frame_offset) != 0) {
        return -1;
    }
    if (mp3->io->read(mp3->io->user_data, frame_buffer, id3v2_frame_header_size) != id3v2_frame_header_size) {
        return -1;
    }

    if (strncmp((const char *)frame_buffer, id3v2_frame_magic_string, id3v2_frame_magic_string_length) != 0) {
        mp3->io->seek(mp3->io->user_data, id3v2_frame_offset);
        return 0;
    }

    const size_t id3v2_tag_total_size = (((frame_buffer[6] & 0x7F) << 21) | ((frame_buffer[7] & 0x7F) << 14) |
                                        ((frame_buffer[8] & 0x7F) << 7) | ((frame_buffer[9] & 0x7F) << 0)) +
                                        id3v2_frame_header_size;

    if (mp3->io->seek(mp3->io->user_data, id3v2_frame_offset + id3v2_tag_total_size) != 0) {
        return -1;
    }
    return id3v2_tag_total_size;
}

/**
 ****************************************************************************************
 * @brief Refill MP3 input buffer from I/O source
 *
 * @param mp3 Decoder context
 *
 * @return Number of bytes read
 ****************************************************************************************
 */
static size_t helix_mp3_fill_mp3_buffer(helix_mp3_t *mp3)
{
    memmove(&mp3->mp3_buffer[0], mp3->mp3_read_ptr, mp3->mp3_buffer_bytes_left);

    const size_t bytes_to_read = HELIX_MP3_DATA_CHUNK_SIZE - mp3->mp3_buffer_bytes_left;
    const size_t bytes_read = mp3->io->read(mp3->io->user_data,
                                            &mp3->mp3_buffer[mp3->mp3_buffer_bytes_left],
                                            sizeof(*mp3->mp3_buffer) * bytes_to_read);

    if (bytes_read < bytes_to_read) {
        memset(&mp3->mp3_buffer[mp3->mp3_buffer_bytes_left + bytes_read], 0,
               bytes_to_read - bytes_read);
    }

    return bytes_read;
}

/**
 ****************************************************************************************
 * @brief Decode next MP3 frame
 *
 * @param mp3 Decoder context
 *
 * @return Number of PCM samples decoded, 0 on error or EOF
 ****************************************************************************************
 */
static size_t helix_mp3_decode_next_frame(helix_mp3_t *mp3)
{
    size_t pcm_samples_read;

    while (1) {
        if (mp3->mp3_buffer_bytes_left < HELIX_MP3_MIN_DATA_CHUNK_SIZE) {
            const size_t bytes_read = helix_mp3_fill_mp3_buffer(mp3);
            mp3->mp3_buffer_bytes_left += bytes_read;
            mp3->mp3_read_ptr = &mp3->mp3_buffer[0];
        }

        const int offset = MP3FindSyncWord(mp3->mp3_read_ptr, mp3->mp3_buffer_bytes_left);
        if (offset < 0) {
            pcm_samples_read = 0;
            break;
        }
        mp3->mp3_read_ptr += offset;
        mp3->mp3_buffer_bytes_left -= offset;

        const int err = MP3Decode(mp3->dec, &mp3->mp3_read_ptr, &mp3->mp3_buffer_bytes_left,
                                  mp3->pcm_buffer, 0);

        if (err == ERR_MP3_NONE) {
            MP3FrameInfo frame_info;
            MP3GetLastFrameInfo(mp3->dec, &frame_info);

            mp3->current_sample_rate = frame_info.samprate;
            mp3->current_bitrate = frame_info.bitrate;
            mp3->pcm_samples_left = frame_info.outputSamps;

            pcm_samples_read = mp3->pcm_samples_left;
            break;
        }
        else if (err == ERR_MP3_MAINDATA_UNDERFLOW) {
            continue;
        }
        else {
            pcm_samples_read = 0;
            break;
        }
    }

    return pcm_samples_read;
}

/*
 * Default file I/O callbacks
 ****************************************************************************************
 */

static int helix_mp3_seek(void *user_data, int offset)
{
    return fseek((FILE *)user_data, offset, SEEK_SET);
}

static size_t helix_mp3_read(void *user_data, void *buffer, size_t size)
{
    return fread(buffer, sizeof(uint8_t), size, (FILE *)user_data);
}

static helix_mp3_io_t default_io =
{
    .seek = helix_mp3_seek,
    .read = helix_mp3_read
};

/*
 * Public API
 ****************************************************************************************
 */

int helix_mp3_init(helix_mp3_t *mp3, const helix_mp3_io_t *io)
{
    if ((mp3 == NULL) || (io == NULL)) {
        return -EINVAL;
    }

    memset(mp3, 0, sizeof(*mp3));
    mp3->io = io;

    int err = 0;
    do {
        mp3->dec = MP3InitDecoder();
        if (mp3->dec == NULL) {
            err = -ENOMEM;
            break;
        }

        mp3->mp3_buffer = malloc(HELIX_MP3_DATA_CHUNK_SIZE);
        if (mp3->mp3_buffer == NULL) {
            err = -ENOMEM;
            break;
        }
        mp3->pcm_buffer = malloc(HELIX_MP3_MAX_SAMPLES_PER_FRAME * sizeof(*mp3->pcm_buffer));
        if (mp3->pcm_buffer == NULL) {
            err = -ENOMEM;
            break;
        }

        if (helix_mp3_skip_id3v2_tag(mp3) < 0) {
            err = -1;
            break;
        }

        if (helix_mp3_decode_next_frame(mp3) == 0) {
            err = -2;
            break;
        }
    } while (0);

    if (err) {
        free(mp3->pcm_buffer);
        free(mp3->mp3_buffer);
        MP3FreeDecoder(mp3->dec);
    }
    return err;
}


int helix_mp3_init_file(helix_mp3_t *mp3, const char *path)
{
    FILE *fd = fopen(path, "rb");
    if (fd == NULL) {
       return -3;
    }
    default_io.user_data = fd;

    const int err = helix_mp3_init(mp3, &default_io);
    if (err) {
        fclose(fd);
        return err;
    }
    return 0;
}


int helix_mp3_deinit(helix_mp3_t *mp3)
{
    if (mp3 == NULL) {
        return -EINVAL;
    }

    if (mp3->io->read == default_io.read) {
        fclose((FILE *)mp3->io->user_data);
    }
    free(mp3->pcm_buffer);
    free(mp3->mp3_buffer);
    MP3FreeDecoder(mp3->dec);

    return 0;
}

uint32_t helix_mp3_get_sample_rate(helix_mp3_t *mp3)
{
    if (mp3 == NULL) {
        return 0;
    }
    return mp3->current_sample_rate;
}

uint32_t helix_mp3_get_bitrate(helix_mp3_t *mp3)
{
    if (mp3 == NULL) {
        return 0;
    }
    return mp3->current_bitrate;
}

size_t helix_mp3_get_pcm_frames_decoded(helix_mp3_t *mp3)
{
    if (mp3 == NULL) {
        return 0;
    }
    return mp3->current_pcm_frame;
}

/**
 ****************************************************************************************
 * @brief Read decoded PCM frames as signed 16-bit samples
 *
 * @details
 * Reads requested number of PCM frames from the MP3 stream. Each frame contains
 * HELIX_MP3_SAMPLES_PER_FRAME (576) samples. PCM data is converted from signed 16-bit
 * to unsigned 10-bit range for PWM speaker output: (sample + 0x8000) >> 6.
 *
 * @param mp3            Decoder context
 * @param buffer         Output PCM buffer (signed 16-bit)
 * @param frames_to_read Number of frames to decode
 *
 * @return Number of frames actually read
 ****************************************************************************************
 */
size_t helix_mp3_read_pcm_frames_s16(helix_mp3_t *mp3, int16_t *buffer, size_t frames_to_read)
{
    if ((mp3 == NULL) || (buffer == NULL) || (frames_to_read == 0)) {
        return 0;
    }

    size_t samples_to_read = frames_to_read * HELIX_MP3_SAMPLES_PER_FRAME;
    size_t samples_read = 0;

    while (1) {
        const size_t samples_to_consume = HELIX_MP3_MIN(mp3->pcm_samples_left, samples_to_read);

        /* Convert signed 16-bit PCM to unsigned 10-bit for PWM output */
        for (uint16_t i = 0; i < samples_to_consume; i++)
        {
            buffer[samples_read + i] =
                (mp3->pcm_buffer[HELIX_MP3_MAX_SAMPLES_PER_FRAME - mp3->pcm_samples_left + i]
                 + 0x8000) >> 6;
        }

        mp3->current_pcm_frame += (samples_to_consume / HELIX_MP3_SAMPLES_PER_FRAME);
        mp3->pcm_samples_left -= samples_to_consume;
        samples_read += samples_to_consume;
        samples_to_read -= samples_to_consume;

        if (mp3->pcm_samples_left == 0) {
            if (helix_mp3_decode_next_frame(mp3) == 0) {
                break;
            }
        }

        if (samples_to_read == 0) {
            break;
        }
    }

    return (samples_read / HELIX_MP3_SAMPLES_PER_FRAME);
}
