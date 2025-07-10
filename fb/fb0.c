#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>  // mmap을 위한 헤더 추가
#include <string.h>    // memset을 위한 헤더 추가

#define FBDEVICE "/dev/fb0"

int main(int argc, char **argv)
{
    int fbfd;
    struct fb_var_screeninfo vinfo;
    unsigned short *fbp = NULL;
    int screensize = 0;

    // 프레임 버퍼 디바이스 열기
    fbfd = open(FBDEVICE, O_RDWR);
    if(fbfd < 0) {
        perror("Error: cannot open framebuffer device");
        return -1;
    }

    // 프레임 버퍼 정보 가져오기
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error reading variable information");
        close(fbfd);
        return -1;
    }

    // 화면 크기 계산 (바이트 단위)
    screensize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);
    
    // 프레임 버퍼를 메모리에 매핑
    fbp = (unsigned short *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        close(fbfd);
        return -1;
    }
    
    printf("프레임 버퍼 매핑 성공: %p, 화면 크기: %d 바이트\n", fbp, screensize);
    
    // 전체 화면을 검은색(0)으로 채우기
    memset(fbp, 0, screensize);
    
    printf("화면을 검은색으로 채웠습니다.\n");
    
    // 메모리 매핑 해제
    munmap(fbp, screensize);
    close(fbfd);
    
    return 0;
}
