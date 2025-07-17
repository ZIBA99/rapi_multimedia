#include <stdio.h>              // 표준 입출력 함수
#include <stdlib.h>             // 동적 메모리 할당, exit 등
#include <stdint.h>             // 고정 크기 정수형(uint8_t, uint16_t 등)
#include <string.h>             // memset 등 문자열 관련 함수
#include <fcntl.h>              // open()을 위한 플래그 정의
#include <unistd.h>             // close(), read(), write() 등
#include <sys/ioctl.h>          // ioctl() 호출
#include <sys/mman.h>           // mmap(), munmap() 등 메모리 매핑
#include <sys/socket.h>         // 소켓 함수
#include <netinet/in.h>         // sockaddr_in 구조체
#include <arpa/inet.h>          // IP 주소 변환 (inet_ntoa 등)
#include <linux/fb.h>           // 프레임버퍼 관련 구조체 정의

#define FRAMEBUFFER_DEVICE  "/dev/fb0"   // 사용할 프레임버퍼 장치 파일
#define WIDTH               640          // 클라이언트 영상 너비
#define HEIGHT              480          // 클라이언트 영상 높이
#define SERVER_PORT         8080         // 서버가 수신 대기할 포트

// 프레임버퍼 관련 전역 변수
static struct fb_var_screeninfo vinfo;      // 화면 해상도 및 컬러 깊이 정보
static uint16_t *fbp_global = NULL;         // 프레임버퍼 매핑 주소 (전역)
static uint32_t fb_screensize = 0;          // 프레임버퍼 총 바이트 수
static int fb_fd = -1;                      // 프레임버퍼 파일 디스크립터

// YUYV 데이터를 RGB565로 변환하여 프레임버퍼에 출력하는 함수
void display_frame(uint16_t *fbp, uint8_t *data, int width, int height) {
    if (!fbp || !data) return;

    // 화면 중앙에 표시하기 위한 오프셋 계산
    int x_offset = (vinfo.xres - width) / 2;
    int y_offset = (vinfo.yres - height) / 2;

    // YUYV -> RGB565 변환 및 출력
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 2) {
            uint8_t Y1 = data[(y * width + x) * 2];
            uint8_t U = data[(y * width + x) * 2 + 1];
            uint8_t Y2 = data[(y * width + x + 1) * 2];
            uint8_t V = data[(y * width + x + 1) * 2 + 1];
            //위 4줄 내용은 교수님 코드 그대로 사용

            // YUV to RGB 변환 (ITU-R BT.601 표준) 약어 Rec.601표준
            // 소수점 값을 버리기 위해 소수 첫 번째 자리에서 반올림 
            int C1 = Y1 - 16;
            int C2 = Y2 - 16;
            int D = U - 128;  // U - 128 쓰기 불편해서 사용
            int E = V - 128;  // V - 128 쓰기 불편해서 사용

            //YCC 기준으로 RGB 변환
            int R1 = (298 * C1 + 409 * E + 128) >> 8; // 298.082를 298로 사용, 408.583를 409로 사용
            int G1 = (298 * C1 - 100 * D - 208 * E + 128) >> 8; // 100.886을 100으로 사용, 208.524를 208로 사용
            int B1 = (298 * C1 + 516 * D + 128) >> 8; // 516.412를 516으로 사용 

            int R2 = (298 * C2 + 409 * E + 128) >> 8; 
            int G2 = (298 * C2 - 100 * D - 208 * E + 128) >> 8; 
            int B2 = (298 * C2 + 516 * D + 128) >> 8;

            // 0~255 범위로 제한
            R1 = (R1 < 0) ? 0 : ((R1 > 255) ? 255 : R1);
            G1 = (G1 < 0) ? 0 : ((G1 > 255) ? 255 : G1);
            B1 = (B1 < 0) ? 0 : ((B1 > 255) ? 255 : B1);
            R2 = (R2 < 0) ? 0 : ((R2 > 255) ? 255 : R2);
            G2 = (G2 < 0) ? 0 : ((G2 > 255) ? 255 : G2);
            B2 = (B2 < 0) ? 0 : ((B2 > 255) ? 255 : B2);

            // RGB565 포맷으로 변환 즉, 2byte 크기의 데이터로 변환
            uint16_t pixel1 = ((R1 & 0xF8) << 8) | ((G1 & 0xFC) << 3) | (B1 >> 3);
            uint16_t pixel2 = ((R2 & 0xF8) << 8) | ((G2 & 0xFC) << 3) | (B2 >> 3);

            // 프레임버퍼에 픽셀 쓰기 (오프셋 적용)
            // 내가 설정한 640*480 화면 크기에 프레임 버퍼 출력
            int fb_x1 = x + x_offset;
            int fb_x2 = x + x_offset + 1;
            int fb_y = y + y_offset;

            if (fb_y >= 0 && fb_y < vinfo.yres) {
                if (fb_x1 >= 0 && fb_x1 < vinfo.xres) {
                    fbp[fb_y * vinfo.xres + fb_x1] = pixel1;
                }
                if (fb_x2 >= 0 && fb_x2 < vinfo.xres) {
                    fbp[fb_y * vinfo.xres + fb_x2] = pixel2;
                }
            }
        }
    }
}

// 모든 데이터를 받을 때까지 반복하여 수신하는 헬퍼 함수
ssize_t recv_all(int socket, void *buffer, size_t length) {
    size_t total_received = 0;
    uint8_t *ptr = (uint8_t *)buffer;
    while (total_received < length) { // 받은 데이터 길이가 전체 데이터 길이보다 작을 때까지 반복   
        ssize_t received = recv(socket, ptr + total_received, length - total_received, 0);
        // 소켓에서 ptr + total_received 주소값 위치에 전체데이터 길이- 포탈센트 만큼 넣는 것을 received 변수 명으로 정의 한다.
        if (received == -1) {
            perror("recv failed");
            return -1;
        }
        if (received == 0) { // 연결 종료
            return 0;
        }
        total_received += received; // 받은 데이터 길이만큼 증가    
    }
    return total_received; // 받은 데이터 길이만큼 증가 
}

int main() {
    // 1. 프레임버퍼 설정
    fb_fd = open(FRAMEBUFFER_DEVICE, O_RDWR); // FRAMEBUFFER_DEVICE 경로의 디스크립트를 읽기, 쓰기 활성화
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        return 1;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) { // 프레임버퍼 정보 읽기
        perror("Error reading variable screen info");
        close(fb_fd);
        return 1;
    }

    fb_screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8; // 프레임버퍼 크기 계산
    fbp_global = mmap(0, fb_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0); // 프레임버퍼 매핑
    if ((intptr_t)fbp_global == -1) {
        perror("Error mapping framebuffer device to memory");
        close(fb_fd);
        return 1;
    }
    printf("Framebuffer initialized: %dx%d, %d bpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    // 2. TCP 서버 소켓 설정
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Failed to create server socket");
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    // SO_REUSEADDR 옵션 설정
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) { // 소켓 옵션 설정
        perror("setsockopt failed");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    struct sockaddr_in server_addr; // 서버 주소 구조체
    memset(&server_addr, 0, sizeof(server_addr)); // 서버 주소 구조체 초기화
    server_addr.sin_family = AF_INET; // 주소 체계 설정
    server_addr.sin_addr.s_addr = INADDR_ANY; // 모든 인터페이스에서 수신 가능
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        // 바인딩 실패 시 에러 메시지 출력
        perror("Failed to bind server socket");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    if (listen(server_sock, 1) == -1) { // 단일 연결 대기
        perror("Failed to listen on server socket");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    printf("Server listening on port %d for incoming client connection...\n", SERVER_PORT);

    // 3. 단일 클라이언트 연결 처리
    struct sockaddr_in client_addr; // 클라이언트 주소 정보를 저장할 구조체
    socklen_t client_addr_len = sizeof(client_addr); // 클라이언트 주소 구조체 크기 설정
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
     // 클라이언트 연결 수락 (accept는 블로킹 함수로, 연결이 들어올 때까지 대기함)
    if (client_sock == -1) {
        perror("Failed to accept client connection");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }
    printf("Client connected from %s:%d (socket %d)\n", 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_sock); 

    size_t frame_size = WIDTH * HEIGHT * 2; // YUYV 2바이트/pixel
    uint8_t *received_buffer = malloc(frame_size);     // 프레임 데이터를 저장할 버퍼를 동적 할당
    if (!received_buffer) {
        perror("Failed to allocate received buffer");
        close(client_sock);                      // 클라이언트 소켓 닫기
        close(server_sock);                      // 서버 소켓 닫기
        munmap(fbp_global, fb_screensize);       // 프레임버퍼 메모리 매핑 해제
        close(fb_fd);                            // 프레임버퍼 파일 디스크립터 닫기
        return 1;
    }

    // 4. 프레임 수신 및 표시 루프
    while (1) {
        ssize_t bytes_received = recv_all(client_sock, received_buffer, frame_size);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected.\n");
            } else {
                perror("Error receiving frame from client");
            }
            break;
        }

        display_frame(fbp_global, received_buffer, WIDTH, HEIGHT);
    }

    free(received_buffer);  // 프레임 버퍼 수신 버퍼 메모리 해제
    close(client_sock); // 클라이언트 소켓 종료 (클라이언트 연결 닫기)
    close(server_sock);  // 서버 소켓 종료 (새 클라이언트 수신 중단)
    munmap(fbp_global, fb_screensize); // 프레임버퍼 메모리 매핑 해제
    close(fb_fd);  // 프레임버퍼 파일 디스크립터 닫기

    return 0;
}

