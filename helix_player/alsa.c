#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define PCM_DEVICE      "default"
// #define PCM_DEVICE      "hw:1,0"
#define BUFFER_FRAMES   2048                        // 缓冲区帧数 
#define BUFFER_MULTIPLE 6                           // 缓冲区倍数 

static snd_pcm_t *pcm_handle = NULL;

int alsa_device_open(unsigned int channels, unsigned int sample_rate, unsigned int format_bits)
{
    int rc;
    snd_pcm_hw_params_t *params;

    // 打开PCM设备
    if ((rc = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "ALSA open error: %s\n", snd_strerror(rc));
        return -1;
    }

    // 分配硬件参数对象
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    // 设置交错模式
    if ((rc = snd_pcm_hw_params_set_access(pcm_handle, params, 
            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "Access type error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置采样格式
    snd_pcm_format_t format;
    if (format_bits == 16)
        format = SND_PCM_FORMAT_S16_LE;
    else if (format_bits == 24)
        format = SND_PCM_FORMAT_S24_LE;
    else if (format_bits == 32)
        format = SND_PCM_FORMAT_S32_LE;
    else {
        fprintf(stderr, "Unsupported bit depth: %d\n", format_bits);
        goto err;
    }
    
    if ((rc = snd_pcm_hw_params_set_format(pcm_handle, params, format)) < 0) {
        fprintf(stderr, "Format error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置声道数
    if ((rc = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)) < 0) {
        fprintf(stderr, "Channels error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置采样率
    unsigned int rate = sample_rate;
    if ((rc = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0) {
        fprintf(stderr, "Rate error: %s\n", snd_strerror(rc));
        goto err;
    }
    if (rate != sample_rate) {
        fprintf(stderr, "Warning: Sample rate adjusted from %u to %u\n", 
               sample_rate, rate);
    }

    // 设置周期大小
    snd_pcm_uframes_t period_size = BUFFER_FRAMES;
    if ((rc = snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &period_size, 0)) < 0) {
        fprintf(stderr, "Period size error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置缓冲区大小
    snd_pcm_uframes_t buffer_size = period_size * BUFFER_MULTIPLE;
    if ((rc = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &buffer_size)) < 0) {
        fprintf(stderr, "Buffer size error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 打印实际缓冲区设置
    snd_pcm_hw_params_get_period_size(params, &period_size, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    printf("ALSA buffer: period=%u frames, buffer=%lu frames (%.1fms @ %uHz)\n",
          (uint32_t)period_size, buffer_size, 
          (float)buffer_size * 1000 / rate, rate);

    // 应用参数设置
    if ((rc = snd_pcm_hw_params(pcm_handle, params)) < 0) {
        fprintf(stderr, "Param apply error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 准备PCM设备
    if ((rc = snd_pcm_prepare(pcm_handle)) < 0) {
        fprintf(stderr, "PCM prepare error: %s\n", snd_strerror(rc));
        goto err;
    }

    printf("ALSA device ready for playback\n");
    return 0;

err:
    snd_pcm_close(pcm_handle);
    return -1;
}

int alsa_device_write(int16_t *pcm, size_t frames)
{
    // 验证输入参数
    if (pcm == NULL || frames == 0) {
        fprintf(stderr, "Invalid PCM data: %p, frames: %zu\n", pcm, frames);
        return -1;
    }

    // 获取当前ALSA参数用于验证
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_current(pcm_handle, params);
    
    unsigned int channels;
    snd_pcm_hw_params_get_channels(params, &channels);
    
    // 验证PCM数据大小
    size_t expected_size = frames * channels;
    if (expected_size > BUFFER_FRAMES * BUFFER_MULTIPLE * channels) {
        fprintf(stderr, "Write size %zu exceeds buffer capacity\n", expected_size);
        return -1;
    }

    // 写入音频数据
    snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, pcm, frames);
    
    if (written == -EPIPE) {  // Underrun处理
        fprintf(stderr, "Underrun occurred (frames: %zu, ch: %u)\n", frames, channels);
        snd_pcm_prepare(pcm_handle);
        // 重试写入
        written = snd_pcm_writei(pcm_handle, pcm, frames);
    } 
    
    if (written < 0) {
        fprintf(stderr, "Write error: %s (frames: %zu, ch: %u)\n", 
               snd_strerror(written), frames, channels);
        return -1;
    } else if ((size_t)written != frames) {
        fprintf(stderr, "Short write: %ld of %zu frames (%.1f%%)\n", 
              written, frames, (float)written/frames*100);
    }

    return written;
}

void alsa_device_close(void)
{
    if (pcm_handle != NULL) {
        // 确保所有数据都被播放完
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
}
