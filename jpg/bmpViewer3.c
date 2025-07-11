#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <fcntl.h>
#include <limits.h>                     /* USHRT_MAX 상수를 위해서 사용 */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "bmpHeader.h"

#define FBDEVFILE            "/dev/fb0"
#define NO_OF_COLOR    24/8
#define LIMIT_USHRT(n) (n>USHRT_MAX)?USHRT_MAX:(n<0)?0:n
#define LIMIT_UBYTE(n) (n>UCHAR_MAX)?UCHAR_MAX:(n<0)?0:n

extern int readBmp(char *filename, unsigned char **pData, int *cols, int *rows);

/* RGB565 형식으로 픽셀값 생성하는 함수 */
unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

int main(int argc, char **argv)
{
    int cols, rows;                  // 프레임버퍼 가로, 세로 크기
    int width, height;               // BMP 이미지 가로, 세로 크기
    unsigned char *pData;            // BMP 이미지 원본 데이터 포인터 (24비트)
    unsigned char r, g, b;           // 각 픽셀의 빨강, 초록, 파랑 값
    unsigned short *pBmpData, *pfbmap, pixel;  // 16비트 픽셀 데이터, 프레임버퍼 맵, 임시 픽셀값
    struct fb_var_screeninfo vinfo;  // 프레임버퍼 화면 정보 구조체
    int fbfd;                        // 프레임버퍼 디바이스 파일 디스크립터
    int x, y, k, t;                 // 반복문 변수 및 인덱스 변수

    // 실행 인자 개수가 2가 아니면 사용법 출력 후 종료
    if(argc != 2) {
        printf("\nUsage: ./%s xxx.bmp\n", argv[0]);
        return 0;
    }

    /* 프레임버퍼 디바이스 파일을 읽기/쓰기 모드로 연다 */
    fbfd = open(FBDEVFILE, O_RDWR);
    if(fbfd < 0) {
        perror("open( )");
        return -1;
    }

    /* 프레임버퍼의 현재 화면 정보를 ioctl 호출로 얻어온다 */
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("ioctl( ) : FBIOGET_VSCREENINFO");
        return -1;
    }

    cols = vinfo.xres;  // 프레임버퍼 가로 해상도 저장
    rows = vinfo.yres;  // 프레임버퍼 세로 해상도 저장

    /* BMP 데이터를 16비트로 변환하여 저장할 메모리 할당 */
    pBmpData = (unsigned short *)malloc(cols * rows * sizeof(unsigned short));
    if(pBmpData == NULL) {
        perror("malloc() : pBmpData");
        return -1;
    }

    /* BMP 원본 24비트 데이터를 저장할 메모리 할당 */
    pData = (unsigned char *)malloc(cols * rows * sizeof(unsigned char) * NO_OF_COLOR);
    if(pData == NULL) {
        perror("malloc() : pData");
        free(pBmpData);
        return -1;
    }

    /* 프레임버퍼 메모리를 프로세스 주소 공간에 매핑 */
    pfbmap = (unsigned short *)mmap(0, cols*rows*2, 
                PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
    if((unsigned)pfbmap == (unsigned)-1) {
        perror("mmap( )");
        free(pBmpData);
        free(pData);
        return -1;
    }

    /* BMP 파일에서 헤더 및 이미지 데이터를 읽어온다 */
    if(readBmp(argv[1], &pData, &width, &height) < 0) {
        perror("readBMP( )");
        munmap(pfbmap, cols*rows*2);
        free(pBmpData);
        free(pData);
        return -1;
    }

    printf("\nWidth : %d, Height : %d\n", width, height);

    /* 24 비트의 BMP 이미지 데이터를 16비트 이미지 데이터로 변경 */
    for(y = 0; y < height; y++) {
        // BMP는 아래서 위로 저장되어 있으므로 뒤집어서 읽기 위한 인덱스 계산
        k = (height-y-1)*width*NO_OF_COLOR;
        t = y*cols;

        for(x = 0; x < width; x++) {
            // BMP 데이터에서 B, G, R 값을 읽어온다
            b = LIMIT_UBYTE(pData[k+x*NO_OF_COLOR+0]);
            g = LIMIT_UBYTE(pData[k+x*NO_OF_COLOR+1]);
            r = LIMIT_UBYTE(pData[k+x*NO_OF_COLOR+2]);
            
            // 24비트 RGB를 16비트 RGB565로 변환
            pixel = LIMIT_USHRT(makepixel(r, g, b));
            
            // 변환된 픽셀을 16비트 BMP 데이터 배열에 저장
            pBmpData[t+x] = pixel; 
        }
    }

    /* 변환된 16비트 BMP 데이터를 프레임버퍼 메모리로 복사 */
    memcpy(pfbmap, pBmpData, cols*rows*2);

    /* 사용한 메모리 맵 해제 */
    munmap(pfbmap, cols*rows*2);
    
    /* 동적 할당한 메모리 해제 */
    free(pBmpData);
    free(pData);
    
    /* 프레임버퍼 디바이스 파일 닫기 */
    close(fbfd);

    return 0;
}
