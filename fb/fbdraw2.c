#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>  // mmap을 위한 헤더 추가

#define FBDEVICE "/dev/fb0"

typedef unsigned char ubyte;
struct fb_var_screeninfo vinfo; /* 프레임 버퍼 정보 처리를 위한 구조체 */
unsigned short *fbp = NULL;     /* 프레임 버퍼를 가리키는 포인터 */
int screensize = 0;             /* 화면 크기를 저장할 변수 */

unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

static int drawpoint(int x, int y, unsigned short color)
{
    /* 색상 출력을 위한 위치 계산 */
    // printf("%d, %d\n",x,y);
    if (x >= 0 && x < vinfo.xres && y >= 0 && y < vinfo.yres) {
        *(fbp + x + y * vinfo.xres) = color;
    }
    return 0;
}

static void drawline(int start_x, int end_x, int y, ubyte r, ubyte g, ubyte b)
{
    ubyte a = 0xFF;
    for(int x = start_x; x < end_x; x++) {
        drawpoint(x, y, makepixel(r,g,b));
    }
}

static void drawcircle(int center_x, int center_y, int radius, ubyte r, ubyte g, ubyte b) {
    int x = radius, y = 0;
    int radiusError = 1-x;

    while(x >= y) {
        drawpoint(x+center_x, y+center_y, makepixel(r, g, b));
        drawpoint(y+center_x, x+center_y, makepixel(r, g, b));
        drawpoint(-x+center_x, y+center_y, makepixel(r, g, b));
        drawpoint(-y+center_x, x+center_y, makepixel(r, g, b));
        drawpoint(-x+center_x, -y+center_y, makepixel(r, g, b));
        drawpoint(-y+center_x, -x+center_y, makepixel(r, g, b));
        drawpoint(x+center_x, -y+center_y, makepixel(r, g, b));
        drawpoint(y+center_x, -x+center_y, makepixel(r, g, b));

        y++;
        if(radiusError < 0) {
            radiusError += 2*y + 1;
        } else {
            x--;
            radiusError += 2*(y-x+1);
        }
    }
}

static void drawface(int start_x, int start_y, int end_x, int end_y, ubyte r, ubyte g, ubyte b) {
    ubyte a = 0xFF;

    if(end_x == 0) end_x = vinfo.xres;
    if(end_y == 0) end_y = vinfo.yres;

    for(int y = start_y; y < end_y; y++) {
        drawline(start_x, end_x, y, r, g, b);
    }
}

int main(int argc, char **argv)
{
    int fbfd;

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

    // 화면 크기 계산 (16비트 색상 기준)
    screensize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);
    
    // 프레임 버퍼를 메모리에 매핑
    fbp = (unsigned short *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        close(fbfd);
        return -1;
    }
    
    printf("프레임 버퍼 매핑 성공: %p, 화면 크기: %d 바이트\n", fbp, screensize);
    
#define RUN_LEV 2
#if RUN_LEV==1
    drawpoint(50, 50, makepixel(255, 0, 0));            /*  Red 점을 출력 */
    drawpoint(100, 100, makepixel(0, 255, 0));          /*  Green 점을 출력 */
    drawpoint(150, 150, makepixel(0, 0, 255));          /*  Blue 점을 출력 */
#elif RUN_LEV==2
    drawline(100, 1000, 200, 255, 0, 0);
    drawcircle(200, 200, 100, 255, 0, 255);
    int s_x = 0;
    int s_y = 0;
    int w = vinfo.xres/3;
    int h = vinfo.yres;
    drawface(s_x, s_y, s_x+w, s_y+h, 0, 0, 255);
    s_x += w;
    drawface(s_x, s_y, s_x+w, s_y+h, 255, 255, 255);
    s_x += w;
    drawface(s_x, s_y, s_x+w, s_y+h, 255, 0, 0);
#endif

    // 메모리 매핑 해제
    munmap(fbp, screensize);
    close(fbfd);                /* 사용이 끝난 프레임 버퍼 디바이스를 닫는다. */

    return 0;
}
