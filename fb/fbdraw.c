#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#define FBDEVICE "/dev/fb0"

typedef unsigned char ubyte;
struct fb_var_screeninfo vinfo;	/* 프레임 버퍼 정보 처리를 위한 구조체 */

unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

static int drawpoint(int fd, int x, int y, unsigned short color)
{

    /* 색상 출력을 위한 위치 계산 : offset  = (X의_위치+Y의_위치x해상도의_넓이)x2  */
    // printf("%d, %d\n",x,y);
    int offset = (x + y*vinfo.xres)*2;
    lseek(fd, offset, SEEK_SET);
    write(fd, &color, 2);
    return 0;
}



static void drawline(int fd, int start_x, int end_x , int y, ubyte r, ubyte g, ubyte b)
{
	ubyte a = 0xFF;
	for(int x = start_x; x<end_x; x++){
		drawpoint(fd, x, y, makepixel(r,g,b));
	}
}

static void drawcircle(int fd, int center_x, int center_y, int radius, ubyte r, ubyte g, ubyte b){
    int x = radius, y=0;
    int radiusError = 1-x;

    while(x>=y){
        drawpoint(fd,  x+center_x,  y+center_y, makepixel(r, g, b));
        drawpoint(fd,  y+center_x,  x+center_y, makepixel(r, g, b));
        drawpoint(fd, -x+center_x,  y+center_y, makepixel(r, g, b));
        drawpoint(fd, -y+center_x,  x+center_y, makepixel(r, g, b));
        drawpoint(fd, -x+center_x, -y+center_y, makepixel(r, g, b));
        drawpoint(fd, -y+center_x, -x+center_y, makepixel(r, g, b));
        drawpoint(fd,  x+center_x, -y+center_y, makepixel(r, g, b));
        drawpoint(fd,  y+center_x, -x+center_y, makepixel(r, g, b));

        y++;
        if(radiusError <0){
            radiusError += 2*y +1;
        }else{
            x--;
            radiusError += 2*(y-x+1);
        }
    }
    
}

static void drawface(int fd, int start_x, int start_y, int end_x, int end_y, ubyte r, ubyte g, ubyte b){
    ubyte a = 0xFF;

    if(end_x == 0) end_x = vinfo.xres;
    if(end_y == 0) end_y = vinfo.yres;


    for(int y= start_y; y < end_y; y++){
        drawline(fd, start_x, end_x, y, r,g,b);
    }
}

// static void drawline(int fd, int start_x, int end_x , int y, ubyte r, ubyte g, ubyte b);
int main(int argc, char **argv)
{
    int fbfd, status, offset;

    fbfd = open(FBDEVICE, O_RDWR); 	/* 사용할 프레임 버퍼 디바이스를 연다. */
    if(fbfd < 0) {
        perror("Error: cannot open framebuffer device");
        return -1;
    }

    /* 현재 프레임 버퍼에 대한 화면 정보를 얻어온다. */
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error reading fixed information");
        return -1;
    }
#define RUN_LEV 2
#if RUN_LEV==1
    drawpoint(fbfd, 50, 50, makepixel(255, 0, 0));            /*  Red 점을 출력 */
    drawpoint(fbfd, 100, 100, makepixel(0, 255, 0));        	/*  Green 점을 출력 */
    drawpoint(fbfd, 150, 150, makepixel(0, 0, 255));        	/*  Blue 점을 출력 */
#elif RUN_LEV==2
    drawline(fbfd,100,1000,200, 255,0,0);
    drawcircle(fbfd, 200, 200, 100, 255, 0, 255);
    int s_x = 0;
    int s_y = 0;
    int w = 1280/3;
    int h = 800;
    drawface(fbfd, s_x, s_y, s_x+w, s_y+h,0,0,255);
    s_x += w;
    drawface(fbfd, s_x, s_y, s_x+w, s_y+h,255,255,255);
    s_x += w;
    drawface(fbfd, s_x, s_y, s_x+w, s_y+h,255,0,0);
#endif
    close(fbfd); 		/* 사용이 끝난 프레임 버퍼 디바이스를 닫는다. */

    return 0;
}
