
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ALSA functions declaration
int alsa_device_open(unsigned int channels, unsigned int sample_rate);
int alsa_device_write(int16_t *pcm, size_t frames);
void alsa_device_close(void);

static mp3dec_t mp3d;

int main(int argc, char **argv)
{
    int init = 0;

    if (argc < 2) {
        printf("Usage: %s <mp3 file>\n", argv[0]);
        return -1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        printf("Failed to open file %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);

    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *data = malloc(size);
    if (!data) {
        printf("Failed to allocate %zu bytes\n", size);
        fclose(file);
        return -1;
    }

    memset(data, 0, size);

    size_t read = fread(data, 1, size, file);
    if (read != size) {
        printf("Failed to read file %s\n", argv[1]);
        fclose(file);
        free(data);
        return -1;
    }

    fclose(file);

    mp3dec_init(&mp3d);

    mp3dec_frame_info_t info;
    mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

    // 解码一帧
    int offset = 0;
    while (offset < size) {
        int sample = mp3dec_decode_frame(&mp3d, data + offset, size - offset, pcm, &info);
        if (sample < 0) {
            printf("Decoding error: %d\n", sample);
            free(data);
            alsa_device_close();
            return -1;
        }

        if (init == 0) {
            printf("Frame layer: %d Channels: %d Frame Hz: %d Bitrate: %d\n", info.layer, info.channels, info.hz, info.bitrate_kbps);
        
            if (info.channels == 0 || info.hz == 0) {
                printf("Invalid MP3 format: channels=%d, sample_rate=%d\n", info.channels, info.hz);
                free(data);
                return -1;
            }
        
            if (alsa_device_open(info.channels, info.hz) < 0) {
                printf("Failed to open ALSA device\n");
                free(data);
                return -1;
            }
            init = 1;
        }
        
        if (info.frame_bytes <= 0) {
            printf("Invalid frame bytes: %d\n", info.frame_bytes);
            free(data);
            alsa_device_close();
            return -1;
        }
        offset += info.frame_bytes;

        printf("Decoded frame %d/%zu (%.1f%%)\r", offset, size, (float)offset/size*100);
        fflush(stdout);

        if (sample > 0) {
            // 验证PCM数据格式
            if (info.channels < 1 || info.channels > 2) {
                printf("Unsupported channel count: %d\n", info.channels);
                free(data);
                alsa_device_close();
                return -1;
            }

            // 计算正确的帧数(每个样本包含所有声道数据)
            int frames = sample;  // 样本数 = 帧数
            
            // 验证PCM数据
            if (frames * info.channels > MINIMP3_MAX_SAMPLES_PER_FRAME) {
                printf("PCM data overflow: %d samples > buffer size %d\n",
                      frames * info.channels, MINIMP3_MAX_SAMPLES_PER_FRAME);
                free(data);
                alsa_device_close();
                return -1;
            }

            // 写入音频数据
            int write_result = alsa_device_write(pcm, frames);
            
            if (write_result < 0) {
                printf("ALSA write failed: %d (frames: %d, ch: %d, rate: %d)\n", 
                      write_result, frames, info.channels, info.hz);
                free(data);
                alsa_device_close();
                return -1;
            }
            
            // // 调试信息
            // printf("Write %d frames (%d samples, %d channels, %dHz) - offset: %d/%zu (%.1f%%)\n", 
            //       frames, sample, info.channels, info.hz, 
            //       offset, size, (float)offset/size*100);
        } else if (sample == 0) {
            printf("Warning: Empty frame at offset %d/%zu (%.1f%%)\n", 
                  offset, size, (float)offset/size*100);
        }
    }

    free(data);
    alsa_device_close();

    return 0;
}