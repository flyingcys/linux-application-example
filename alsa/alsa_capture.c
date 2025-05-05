#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define PCM_DEVICE      "default"
#define SAMPLE_RATE     44100                       // 采样率（Hz）
#define CHANNELS        2                           // 声道数
#define FORMAT          SND_PCM_FORMAT_S16_LE       // 采样格式
#define BUFFER_FRAMES   1024                        // 缓冲区帧数
#define BUFFER_MULTIPLE 4                           // 缓冲区倍数     (生产环境推荐: 6)
#define FORMAT_BITS     16                          // 采样位数（支持16/24/32）

#define DURATION_SEC    10                          // 采集时长（秒）

static snd_pcm_t *capture_handle;

static int init_capture(void)
{
    int rc;
    snd_pcm_hw_params_t *params;
    
    // 打开PCM设备（采集模式）
    if ((rc = snd_pcm_open(&capture_handle, PCM_DEVICE, 
                         SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "ALSA open error: %s\n", snd_strerror(rc));
        return -1;
    }

    // 分配参数对象
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(capture_handle, params);

    // 设置交错模式[1,2](@ref)
    if ((rc = snd_pcm_hw_params_set_access(capture_handle, params,
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
    if ((rc = snd_pcm_hw_params_set_format(capture_handle, params, format)) < 0) {
        fprintf(stderr, "Format error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置声道数
    if ((rc = snd_pcm_hw_params_set_channels(capture_handle, params, CHANNELS)) < 0) {
        fprintf(stderr, "Channels error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置采样率
    unsigned int rate = SAMPLE_RATE;
    if ((rc = snd_pcm_hw_params_set_rate_near(capture_handle, params, &rate, 0)) < 0) {
        fprintf(stderr, "Rate error: %s\n", snd_strerror(rc));
        goto err;
    }

    // 设置缓冲区参数[1](@ref)
    /* 
    设置4倍周期大小的缓冲区（buffer_size = BUFFER_FRAMES *4），平衡延迟和抗干扰能力
    每周期采集BUFFER_FRAMES帧数据，防止缓冲区溢出
    */
    snd_pcm_uframes_t buffer_size = BUFFER_FRAMES * BUFFER_MULTIPLE; // 4倍周期大小
    snd_pcm_hw_params_set_buffer_size_near(capture_handle, params, &buffer_size);
    snd_pcm_uframes_t period_size = BUFFER_FRAMES;
    snd_pcm_hw_params_set_period_size_near(capture_handle, params, &period_size, 0);

    // 应用参数
    if ((rc = snd_pcm_hw_params(capture_handle, params)) < 0) {
        fprintf(stderr, "Param apply error: %s\n", snd_strerror(rc));
        goto err;
    }

    return 0;

err:
    snd_pcm_close(capture_handle);
    return -1;
}

static void capture_pcm(const char *filename)
{
    int fd, rc;
    char *buffer;
    const size_t frame_size = (FORMAT_BITS/8) * CHANNELS;
    const size_t buffer_size = BUFFER_FRAMES * frame_size;

    // 创建PCM文件
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("File create error");
        return;
    }

    // 分配缓冲区
    buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation error\n");
        close(fd);
        return;
    }

    // 采集循环
    unsigned int total_frames = SAMPLE_RATE * DURATION_SEC;
    while (total_frames > 0) {
        snd_pcm_sframes_t frames = snd_pcm_readi(capture_handle, buffer, BUFFER_FRAMES);
        
        if (frames == -EPIPE) {  // 溢出处理
            fprintf(stderr, "Overrun occurred\n");
            snd_pcm_prepare(capture_handle);
            continue;
        } else if (frames < 0) {
            fprintf(stderr, "Read error: %s\n", snd_strerror(frames));
            break;
        }

        // 写入文件
        write(fd, buffer, frames * frame_size);
        total_frames -= frames;
    }

    free(buffer);
    close(fd);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output.pcm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (init_capture() != 0) {
        return EXIT_FAILURE;
    }

    capture_pcm(argv[1]);
    
    // 清理资源
    snd_pcm_drain(capture_handle);
    snd_pcm_close(capture_handle);
    return EXIT_SUCCESS;
}