#include <stdio.h>
#include <sys/types.h>		//open需要的头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	        //write
#include <sys/types.h>
#include <sys/mman.h>		//mmap  内存映射相关函数库
#include <stdlib.h>	        //malloc free 动态内存申请和释放函数头文件
#include <string.h> 	
#include <linux/fb.h>
#include <sys/ioctl.h>

#define FB_DEVICE 			"/dev/fb0"

//32位的颜色，最高 8bit 为透明度，需设置为0xFF，否则无法正常显示颜色
#define BLACK 	0x00000000
#define WHITE 	0xFFFFFFFF
#define RED 	0xFFFF0000
#define GREEN 	0xFF00FF00
#define BLUE 	0xff99ffff

int fd;
unsigned int *fb_mem  = NULL;
struct fb_var_screeninfo var;
struct fb_fix_screeninfo fix;
unsigned int line_width;
unsigned int pixel_width;
unsigned int screen_size;

int main(int argc, char **argv)
{
	unsigned int i;
	int ret;

	fd = open(FB_DEVICE, O_RDWR);			//打开 framebuffer 设备
	if(fd == -1){
		perror("Open LCD");
		return -1;
	}
 
 	//获取屏幕的可变参数
	ioctl(fd, FBIOGET_VSCREENINFO, &var);

	//获取屏幕的固定参数
	ioctl(fd, FBIOGET_FSCREENINFO, &fix);
   
  	//打印分辨率
	printf("xres= %d, yres= %d \n", var.xres, var.yres);

 	//打印总字节数和每行的长度
	printf("line_length=%d, smem_len= %d \n", fix.line_length, fix.smem_len);
	printf("xpanstep=%d, ypanstep= %d \n" ,fix.xpanstep, fix.ypanstep);

	line_width  = var.xres * var.bits_per_pixel / 8;
	pixel_width = var.bits_per_pixel / 8;
	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;

    //获取显存，映射内存
    fb_mem = (unsigned int *)mmap(NULL, var.xres * var.yres * 4, PROT_READ |  PROT_WRITE, MAP_SHARED, fd, 0);   							  
	if (fb_mem == MAP_FAILED) {
		perror("Mmap LCD");
		return -1;	
	}

	memset(fb_mem, 0xff, var.xres * var.yres * 4);		//清屏
	sleep(1);
    
	//将屏幕全部设置成蓝色
	for(i = 0; i < var.xres * var.yres; i ++)
		fb_mem[i] = BLUE;

	sleep(2);

	memset(fb_mem, 0x00, var.xres*  var.yres * 4);		//清屏

    //将屏幕全部设置成红色
	for(i = 0; i < var.xres * var.yres; i ++)
        fb_mem[i] = RED;

    sleep(2);

    memset(fb_mem, 0x00, var.xres*  var.yres * 4);		//清屏
	
	munmap(fb_mem, var.xres * var.yres * 4);			//映射后的地址，通过mmap返回的值	
	close(fd); 			                        		//关闭fb0设备文件
	return 0;			
}