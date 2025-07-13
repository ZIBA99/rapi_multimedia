#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

// readBmp 함수는 BMP 파일로부터 이미지 데이터를 읽어오는 역할을 한다고 가정합니다.
// #include "bmpHeader.h"

#define FBDEVFILE "/dev/fb0"

typedef unsigned char ubyte;

/* BMP 파일의 헤더를 분석해서 원하는 정보를 얻기 위한 함수 (선언) */
extern int readBmp(char *filename, ubyte **pData, int *cols, int *rows, int *color);

/* 24비트 RGB 값을 16비트(RGB565) 값으로 변환하는 함수 */
unsigned short makepixel(unsigned char r, unsigned char g, unsigned char b) {
    // R(5bit), G(6bit), B(5bit) 형식으로 변환
    return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

int main(int argc, char **argv)
{
    int fbfd;
    struct fb_var_screeninfo vinfo;
    unsigned short *pFbMap; // 16비트(2바이트) 단위로 프레임버퍼에 쓰기 위해 포인터 타입을 변경

    ubyte *pBmpData; // BMP 파일의 원본 24비트 이미지 데이터가 저장될 포인터
    int img_cols, img_rows, img_color;
    
    int x, y;

    if(argc != 2) {
        printf("Usage: ./%s <bmp_file>\n", argv[0]);
        return 0;
    }

    /* 1. 프레임버퍼 장치 파일 열기 */
    fbfd = open(FBDEVFILE, O_RDWR);
    if(fbfd < 0) {
        perror("Error: open(FBDEVFILE)");
        return -1;
    }

    /* 2. 프레임버퍼의 정보(해상도, 색상 비트 등) 가져오기 */
    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error: ioctl(FBIOGET_VSCREENINFO)");
        close(fbfd);
        return -1;
    }

    /* 3. 프레임버퍼가 16bpp가 맞는지 확인 */
    if(vinfo.bits_per_pixel != 16) {
        fprintf(stderr, "Error: 이 프로그램은 16bpp 환경에서만 동작합니다. (현재: %dbpp)\n", vinfo.bits_per_pixel);
        close(fbfd);
        return -1;
    }

    // 요청하신 해상도와 실제 프레임버퍼 해상도가 맞는지 확인 (선택 사항)
    if(vinfo.xres != 1280 || vinfo.yres != 800) {
        printf("Warning: 프레임버퍼의 해상도(%dx%d)가 목표 해상도(1200x800)와 다릅니다.\n", vinfo.xres, vinfo.yres);
    }

    /* 4. BMP 파일 읽기 */
    // readBmp 함수가 BMP 이미지 데이터를 pBmpData에 동적 할당하여 저장해준다고 가정
    if(readBmp(argv[1], &pBmpData, &img_cols, &img_rows, &img_color) < 0) {
        perror("Error: readBmp()");
        close(fbfd);
        return -1;
    }
    // 24비트 BMP가 아니면 처리 중단
    if (img_color != 24) {
        fprintf(stderr, "Error: 24비트 BMP 파일만 지원합니다.\n");
        free(pBmpData);
        close(fbfd);
        return -1;
    }

    /* 5. 프레임버퍼 메모리 매핑 */
    long fb_size = vinfo.xres * vinfo.yres * sizeof(unsigned short); // 16비트(2바이트) 크기로 계산
    pFbMap = (unsigned short *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if(pFbMap == MAP_FAILED) {
        perror("Error: mmap()");
        free(pBmpData);
        close(fbfd);
        return -1;
    }

    /* 6. BMP 데이터를 프레임버퍼에 쓰기 */
    for(y = 0; y < vinfo.yres; y++) {
        for(x = 0; x < vinfo.xres; x++) {
            // BMP 이미지 경계를 벗어나는 영역은 검은색으로 처리
            if(x < img_cols && y < img_rows) {
                // BMP 데이터는 상하가 뒤집혀 있고, BGR 순서로 저장되어 있음
                long bmp_pos = ((img_rows - 1 - y) * img_cols + x) * 3; // 24비트(3바이트)
                ubyte b = pBmpData[bmp_pos];
                ubyte g = pBmpData[bmp_pos + 1];
                ubyte r = pBmpData[bmp_pos + 2];

                // 24비트 색상값을 16비트 값으로 변환
                unsigned short pixel = makepixel(r, g, b);

                // 변환된 16비트 픽셀 값을 프레임버퍼의 해당 위치에 저장
                pFbMap[y * vinfo.xres + x] = pixel;
            } else {
                // 화면의 남는 공간을 검은색으로 채움
                pFbMap[y * vinfo.xres + x] = 0x0000; // 16비트 검은색
            }
        }
    }

    /* 7. 자원 해제 */
    munmap(pFbMap, fb_size);
    free(pBmpData);
    close(fbfd);

    return 0;
}
