#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mp3dec.h"

// ALSA functions declaration
int alsa_device_open(unsigned int channels, unsigned int sample_rate, unsigned int format_bits);
int alsa_device_write(int16_t *pcm, size_t frames);
void alsa_device_close(void);

static HMP3Decoder hMP3Decoder;
static MP3FrameInfo mp3FrameInfo;
short pcm[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];

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

    hMP3Decoder = MP3InitDecoder();
    if (!hMP3Decoder) {
        printf("Failed to allocate MP3 decoder\n");
        free(data);
        return -1;
    }

    // 解码一帧
    uint8_t *data_ptr = data;
    int data_size = size;
    while (data_size > 0) {
        /* find start of next MP3 frame - assume EOF if no sync found */
        int offset = MP3FindSyncWord(data_ptr, data_size);
        if (offset < 0) {
            printf("No sync word found\n");
            goto error;
        }

        data_ptr += offset;
        data_size -= offset;
        
        int err = MP3Decode(hMP3Decoder, &data_ptr, &data_size, pcm, 0);
        if (err) {
            switch (err) {
                case ERR_MP3_INDATA_UNDERFLOW:
                    printf("MP3 decoder: INDATA_UNDERFLOW\n");
                    break;
                case ERR_MP3_MAINDATA_UNDERFLOW:
                    printf("MP3 decoder: MAINDATA_UNDERFLOW\n");
                    break;
                case ERR_MP3_FREE_BITRATE_SYNC:
                    printf("MP3 decoder: FREE_BITRATE_SYNC\n");
                    break;
                default:
                    printf("MP3 decoder: error %d\n", err);
                    break;
            }
        } else {
            MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);

            //打印 MP3 信息
            if (init == 0) {
                printf(" \r\n Bitrate       %dKbps", mp3FrameInfo.bitrate/1000);
                printf(" \r\n Samprate      %dHz",   mp3FrameInfo.samprate);
                printf(" \r\n BitsPerSample %db",    mp3FrameInfo.bitsPerSample);
                printf(" \r\n nChans        %d",     mp3FrameInfo.nChans);
                printf(" \r\n Layer         %d",     mp3FrameInfo.layer);
                printf(" \r\n Version       %d",     mp3FrameInfo.version);
                printf(" \r\n OutputSamps   %d",     mp3FrameInfo.outputSamps);
                printf("\r\n");
                        
                if (mp3FrameInfo.nChans == 0 || mp3FrameInfo.samprate == 0) {
                    printf("Invalid MP3 format: channels=%d, sample_rate=%d\n", mp3FrameInfo.nChans, mp3FrameInfo.samprate);
                    goto error;
                }
            
                if (alsa_device_open(mp3FrameInfo.nChans, mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample) < 0) {
                    printf("Failed to open ALSA device\n");
                    goto error;
                }
                init = 1;
            }


            if (mp3FrameInfo.outputSamps == 0) {
                printf("Invalid MP3 format: outputSamps=%d\n", mp3FrameInfo.outputSamps);
                goto error;
            }

            printf("Decoded frame %ld/%zu (%.1f%%)\r", data_ptr - data, size, (float)(data_ptr - data)/size*100);
            fflush(stdout);

            int frames = mp3FrameInfo.outputSamps / mp3FrameInfo.nChans;

            // 写入音频数据
            int write_result = alsa_device_write(pcm, frames);
            if (write_result < 0) {
                printf("ALSA write failed: %d (frames: %d, ch: %d, rate: %d)\n", 
                      write_result, frames, mp3FrameInfo.nChans, mp3FrameInfo.samprate);
                goto error;
            }

        }

    }

    MP3FreeDecoder(hMP3Decoder);
    free(data);
    alsa_device_close();

    return 0;

error:
    MP3FreeDecoder(hMP3Decoder);
    free(data);
    alsa_device_close();
    return -1;
}