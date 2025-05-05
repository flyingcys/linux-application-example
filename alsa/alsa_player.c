
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define PCM_DEVICE      "default"
#define SAMPLE_RATE     44100                   // 采样率（Hz）
#define CHANNELS        2                          // 声道数
#define FORMAT          SND_PCM_FORMAT_S16_LE        // 采样格式
#define BUFFER_FRAMES   1024                 // 缓冲区帧数
#define BUFFER_MULTIPLE 4                   // 缓冲区倍数     (生产环境推荐: 6)
#define FORMAT_BITS    16                   // 采样位数（支持16/24/32）

static snd_pcm_t *pcm_handle;
static snd_pcm_uframes_t period_size;

int init_alsa(void)
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
    #if (FORMAT_BITS == 16)
    format = SND_PCM_FORMAT_S16_LE;
    #elif (FORMAT_BITS == 24)
    format = SND_PCM_FORMAT_S24_LE;
    #elif (FORMAT_BITS == 32)
    format = SND_PCM_FORMAT_S32_LE;
    #else
    fprintf(stderr, "Unsupported bit depth: %d\n", FORMAT_BITS);
    goto err;
    #endif
    
    if ((rc = snd_pcm_hw_params_set_format(pcm_handle, params, format)) < 0) {
        fprintf(stderr, "Format error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置声道数
    if ((rc = snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS)) < 0) {
        fprintf(stderr, "Channels error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置采样率
    unsigned int rate = SAMPLE_RATE;
    if ((rc = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0) {
        fprintf(stderr, "Rate error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置周期大小(每次传输的数据块大小)
    period_size = BUFFER_FRAMES;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &period_size, 0);

    // 设置缓冲区大小变量
    snd_pcm_uframes_t buffer_size = period_size * BUFFER_MULTIPLE;
    snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, (snd_pcm_uframes_t*)&buffer_size);

    // 应用参数设置
    if ((rc = snd_pcm_hw_params(pcm_handle, params)) < 0) {
        fprintf(stderr, "Param apply error: %s\n", snd_strerror(rc));
        goto err;
    }

    return 0;

err:
    snd_pcm_close(pcm_handle);
    return -1;
}

/*
*/
void play_pcm(const char *filename)
{
    int fd, rc;
    char *buffer;
    const size_t frame_size = (FORMAT_BITS / 8) * CHANNELS;
    const size_t buffer_size = period_size * frame_size;

    // 打开PCM文件
    if ((fd = open(filename, O_RDONLY)) == -1) {
        perror("File open error");
        return;
    }

    // 分配缓冲区
    buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation error\n");
        close(fd);
        return;
    }

    // 播放循环
    while (1) {
        ssize_t read_size = read(fd, buffer, buffer_size);
        if (read_size <= 0) break;

        // 写入音频数据
        snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, buffer, 
                                     read_size / frame_size);
        
        if (frames == -EPIPE) {  // Underrun处理
            fprintf(stderr, "Underrun occurred\n");
            snd_pcm_prepare(pcm_handle);
        } else if (frames < 0) {
            fprintf(stderr, "Write error: %s\n", snd_strerror(frames));
            break;
        }
    }

    free(buffer);
    close(fd);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pcm_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (init_alsa() != 0) {
        return EXIT_FAILURE;
    }

    play_pcm(argv[1]);
    
    // 清理资源
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    return EXIT_SUCCESS;
}