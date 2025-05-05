#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

// #define JOYSTICK_DEVICE "/dev/input/event1"
#define JOYSTICK_DEVICE "/dev/input/js1"

const char* get_button_name(int code)
{
    printf("code: %x\n", code);
    switch(code) {
        case 0x121: return "A按钮";
        case 0x122: return "B按钮";
        case 0x120: return "X按钮";
        case 0x123: return "Y按钮";
        case 0x124: return "左肩键";
        case 0x125: return "右肩键";
        case 0x128: return "选择键";
        case 0x129: return "开始键";
        case 0x12a: return "左摇杆按下";
        case 0x12b: return "右摇杆按下";
        default: return "未知按钮";
    }
}

int main(int argc, char **argv)
{
    const char* dev = JOYSTICK_DEVICE;
    int fd = open(dev, O_RDONLY);
    
    if (fd == -1) {
        perror("无法打开输入设备");
        return EXIT_FAILURE;
    }
    
    printf("成功打开游戏手柄设备: %s\n", dev);
    printf("按Ctrl+C退出程序\n\n");
    
    struct input_event ev;
    while (1) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == -1) {
            perror("读取输入事件失败");
            close(fd);
            return EXIT_FAILURE;
        }
        
        if (n != sizeof(ev)) {
            fprintf(stderr, "读取到不完整的事件\n");
            continue;
        }

        // 只处理特定类型的事件
        if (ev.type == EV_KEY) {
            printf("按键事件: %s %s\n", 
                    get_button_name(ev.code),
                    ev.value ? "按下" : "释放");
        }
        else if (ev.type == EV_ABS) {
            const char* axis_name = "未知轴";
            
            // 识别常见轴
            switch(ev.code) {
                case ABS_X: axis_name = "左摇杆X轴"; break;
                case ABS_Y: axis_name = "左摇杆Y轴"; break;
                case ABS_RX: axis_name = "右摇杆X轴"; break;
                case ABS_RY: axis_name = "右摇杆Y轴"; break;
                case ABS_HAT0X: axis_name = "方向键X轴"; break;
                case ABS_HAT0Y: axis_name = "方向键Y轴"; break;
            }
            
            printf("摇杆/轴事件: %s(0x%x), 值: %d\n", 
                   axis_name, ev.code, ev.value);
        }

    }
    
    close(fd);
    return EXIT_SUCCESS;
}