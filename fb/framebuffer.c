#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

int main() {
    int fb_fd = open("/dev/fb0", O_RDWR);
    struct fb_var_screeninfo vinfo;
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    
    int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    unsigned char *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    
    // (x,y) 좌표에 픽셀 그리기
    int x = 100, y = 100;
    int location = (x + y * vinfo.xres) * (vinfo.bits_per_pixel / 8);
    
    // 빨간색 픽셀 설정 (RGB 형식 가정)
    fbp[location] = 0;     // Blue
    fbp[location + 1] = 0; // Green
    fbp[location + 2] = 255; // Red
    
    munmap(fbp, screensize);
    close(fb_fd);
    return 0;
}
