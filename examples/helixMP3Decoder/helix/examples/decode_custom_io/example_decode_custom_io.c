#include <helix_mp3.h>
#include <errno.h>
#include <inttypes.h>
//#include <unistd.h>
//#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "drvs.h"

#define SAMPLES_PER_FRAME (576)
#define PCM_BUFFER_SIZE_SAMPLES (576) 
#define PCM_BUFFER_SIZE_FRAMES (PCM_BUFFER_SIZE_SAMPLES / SAMPLES_PER_FRAME)

extern void speakerPlay(int16_t *buff, uint16_t length);
extern void speakerStop(void);

extern const uint8_t* mp3_data;
extern uint32_t mp3_data_len;
unsigned int mp3_offset = 0;

//16Khz 内插到64KHz PCM数据输出，供PWM使用
//线性内插（Linear Interpolation） + 一阶低通滤波器（LPF）
/* 滤波系数控制：Alpha 值越大，滤波越强（声音变闷），Alpha 越小，镜像噪声越多 */
/* 建议范围 4 到 8 */
#define FILTER_ALPHA_SHIFT 6 

typedef struct {
    int32_t last_out;
    int16_t last_sample;
} Resampler_t;

/**
 * @brief 一阶 IIR 低通滤波器 (定点实现)
 */
static inline int16_t simple_lpf(Resampler_t *r, int16_t input) {
    // 公式: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
    r->last_out += (input - (r->last_out >> 8)) * (256 >> (FILTER_ALPHA_SHIFT - 4));
    return (int16_t)(r->last_out >> 8);
}

/**
 * @brief 4倍升采样核心函数
 * @param input_sample 原始 16kHz 的一个采样点
 * @param output_buf   存放生成的 4 个 64kHz 采样点的数组
 */
static void resample_16to64_linear(Resampler_t *r, int16_t input_sample, int16_t *output_buf) {
    int16_t a = r->last_sample;
    int16_t b = input_sample;
    
    /* 线性内插公式: P(i) = a + (b - a) * i / 4 */
    /* 计算出 4 个点 */
    int16_t diff = b - a;
    
    // 点 0 (靠近 a)
    output_buf[0] = simple_lpf(r, a);
    // 点 1
    output_buf[1] = simple_lpf(r, a + (diff >> 2));
    // 点 2
    output_buf[2] = simple_lpf(r, a + (diff >> 1));
    // 点 3
    output_buf[3] = simple_lpf(r, a + (diff >> 2) + (diff >> 1));

    r->last_sample = input_sample;
}

static int seek_callback(void *user_data, int offset)
{
//    int fd = *(int *)user_data;
//    return (lseek(fd, offset, SEEK_SET) < 0);
    mp3_offset = offset;
    return 0;
}

static size_t read_callback(void *user_data, void *buffer, size_t size)
{
//    int fd = *(int *)user_data;
//    return read(fd, buffer, size);
    if((mp3_offset + size + 1) <= mp3_data_len)
    {
        memcpy(buffer, mp3_data + mp3_offset, size);
        mp3_offset += size;
        return size;
    }
    else    
    {
        memcpy(buffer, mp3_data + mp3_offset, mp3_data_len - mp3_offset -1);
        mp3_offset = mp3_data_len - 1;
        return mp3_data_len - mp3_offset;
    }
}
static int16_t pcm_buffer[2][PCM_BUFFER_SIZE_SAMPLES] __attribute__((aligned(4)));

int mp3_decoder(void)
{
    helix_mp3_t mp3;
    
    int err;
    int in_fd = -1;

    /* Create I/O interface */
    helix_mp3_io_t io = {
        .seek = seek_callback,
        .read = read_callback,
        .user_data = &in_fd
    };

    /* Open an input stream */
//    in_fd = open(input_path, O_RDONLY);
//    if (in_fd < 0) {
//        printf("Failed to open input file '%s', error: %d\n", input_path, errno);
//        return -EIO;
//    }

    do {
        /* Initialize decoder */
        err = helix_mp3_init(&mp3, &io);
        if (err) {
            printf("Failed to init decoder, error: %d\n", err);
            break;
        }

        /* Open output file */
//        out_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
//        if (out_fd < 0) {
//            printf("Failed to open output file '%s', error: %d\n", output_path, errno);
//            err = -EIO;
//            break;
//        }

//        printf("Decoding '%s' to '%s'...\n", input_path, output_path);

        /* Decode the whole file */
        uint32_t frame_cnt = 0;
        while (1) {
            GPIO_DAT_SET(GPIO14);
            const size_t frames_read = helix_mp3_read_pcm_frames_s16(&mp3, pcm_buffer[frame_cnt%2], 1);
            GPIO_DAT_CLR(GPIO14);
            if (frames_read == 0) {
                printf("Reached EOF!\n\r");
                break;
            }
            
            //printf("decoded frame %d\n\r", frame_cnt);
            /*
            for(uint16_t c = 0; c < PCM_BUFFER_SIZE_SAMPLES; c++)
            {
                printf("0x%04X, ", (uint16_t)pcm_buffer[frame_cnt%2][c]);
                if(c%12 == 11)
                    printf("\n\r");
            }
            */
            GPIO_DAT_SET(GPIO15);
            speakerPlay(pcm_buffer[frame_cnt%2], PCM_BUFFER_SIZE_SAMPLES);
            GPIO_DAT_CLR(GPIO15);
            frame_cnt++;            
        }

        const size_t frame_count = helix_mp3_get_pcm_frames_decoded(&mp3);
        const uint32_t sample_rate = helix_mp3_get_sample_rate(&mp3);
        const uint32_t bitrate = helix_mp3_get_bitrate(&mp3);
        printf("Done! Decoded %zu frames, last frame sample rate: %" PRIu32 "Hz, bitrate: %" PRIu32 "kbps\n", frame_count, sample_rate, bitrate / 1000);
        speakerStop();
    } while (0);

    /* Cleanup */
//    close(out_fd);
//    close(in_fd);
    helix_mp3_deinit(&mp3);

    return err;
}
