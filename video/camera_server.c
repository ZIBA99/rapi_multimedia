#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <pthread.h> // 멀티 클라이언트 처리를 위한 스레드

#define FRAMEBUFFER_DEVICE  "/dev/fb0"
#define WIDTH               640 // 클라이언트가 보내는 영상의 폭
#define HEIGHT              480 // 클라이언트가 보내는 영상의 높이
#define SERVER_PORT         8080 // 클라이언트 연결을 기다리는 포트 번호

// 프레임버퍼 관련 전역 변수
static struct fb_var_screeninfo vinfo;
static uint16_t *fbp_global = NULL; // 프레임버퍼 포인터
static uint32_t fb_screensize = 0;  // 프레임버퍼 크기
static int fb_fd = -1;              // 프레임버퍼 파일 디스크립터

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

            // YUV to RGB conversion (ITU-R BT.601 standard for YCbCr)
            // Simplified integer arithmetic to avoid floating point
            int C1 = Y1 - 16;
            int C2 = Y2 - 16;
            int D = U - 128;
            int E = V - 128;

            int R1 = (298 * C1 + 409 * E + 128) >> 8;
            int G1 = (298 * C1 - 100 * D - 208 * E + 128) >> 8;
            int B1 = (298 * C1 + 516 * D + 128) >> 8;

            int R2 = (298 * C2 + 409 * E + 128) >> 8;
            int G2 = (298 * C2 - 100 * D - 208 * E + 128) >> 8;
            int B2 = (298 * C2 + 516 * D + 128) >> 8;

            // Clamp values to 0-255
            R1 = (R1 < 0) ? 0 : ((R1 > 255) ? 255 : R1);
            G1 = (G1 < 0) ? 0 : ((G1 > 255) ? 255 : G1);
            B1 = (B1 < 0) ? 0 : ((B1 > 255) ? 255 : B1);
            R2 = (R2 < 0) ? 0 : ((R2 > 255) ? 255 : R2);
            G2 = (G2 < 0) ? 0 : ((G2 > 255) ? 255 : G2);
            B2 = (B2 < 0) ? 0 : ((B2 > 255) ? 255 : B2);

            // RGB565 포맷으로 변환 (R: 5비트, G: 6비트, B: 5비트)
            uint16_t pixel1 = ((R1 & 0xF8) << 8) | ((G1 & 0xFC) << 3) | (B1 >> 3);
            uint16_t pixel2 = ((R2 & 0xF8) << 8) | ((G2 & 0xFC) << 3) | (B2 >> 3);

            // 프레임버퍼에 픽셀 쓰기
            // 화면 범위 벗어나는지 확인 (오프셋 적용 후)
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
    while (total_received < length) {
        ssize_t received = recv(socket, ptr + total_received, length - total_received, 0);
        if (received == -1) {
            perror("recv failed");
            return -1; // 에러 발생
        }
        if (received == 0) { // 연결 종료
            return 0;
        }
        total_received += received;
    }
    return total_received;
}

// 각 클라이언트 연결을 처리할 스레드 함수
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg); // 동적으로 할당된 소켓 디스크립터 메모리 해제

    size_t frame_size = WIDTH * HEIGHT * 2; // YUYV는 픽셀당 2바이트
    uint8_t *received_buffer = malloc(frame_size);
    if (!received_buffer) {
        perror("Failed to allocate received buffer for client");
        close(client_sock);
        return NULL;
    }

    printf("Handling client on socket %d\n", client_sock);

    while (1) {
        ssize_t bytes_received = recv_all(client_sock, received_buffer, frame_size);
        if (bytes_received <= 0) { // 0이면 연결 종료, -1이면 에러
            if (bytes_received == 0) {
                printf("Client on socket %d disconnected.\n", client_sock);
            } else {
                perror("Error receiving frame from client");
            }
            break;
        }

        // 수신된 YUYV 데이터를 프레임버퍼에 출력
        // 현재는 단일 클라이언트만 표시하도록 되어 있습니다.
        // 여러 클라이언트 영상을 동시에 표시하려면 이 부분을 수정해야 합니다.
        display_frame(fbp_global, received_buffer, WIDTH, HEIGHT);
        // printf("Received frame of size %zd bytes from socket %d\n", bytes_received, client_sock);
    }

    free(received_buffer);
    close(client_sock);
    return NULL;
}

int main() {
    // 1. 프레임버퍼 설정
    fb_fd = open(FRAMEBUFFER_DEVICE, O_RDWR);
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        return 1;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb_fd);
        return 1;
    }

    fb_screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp_global = mmap(0, fb_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
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

    // SO_REUSEADDR 옵션 설정 (서버 재시작 시 포트 충돌 방지)
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 모든 IP 주소로부터의 연결 허용
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind server socket");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    if (listen(server_sock, 10) == -1) { // 최대 10개의 대기 연결
        perror("Failed to listen on server socket");
        close(server_sock);
        munmap(fbp_global, fb_screensize);
        close(fb_fd);
        return 1;
    }

    printf("Server listening on port %d for incoming client connections...\n", SERVER_PORT);

    // 3. 클라이언트 연결 대기 및 처리
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("Failed to accept client connection");
            continue; // 다음 연결 대기
        }
        printf("Client connected from %s:%d (socket %d)\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_sock);

        // 새 클라이언트를 처리할 스레드 생성
        pthread_t tid;
        int *new_sock = malloc(sizeof(int)); // 스레드에 소켓 디스크립터 전달
        if (new_sock == NULL) {
            perror("Failed to allocate memory for new_sock");
            close(client_sock);
            continue;
        }
        *new_sock = client_sock;

        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) != 0) {
            perror("Failed to create thread for client");
            close(client_sock);
            free(new_sock);
            continue;
        }
        pthread_detach(tid); // 스레드가 종료될 때 자원 자동 해제
    }

    // 자원 해제 (루프가 무한히 돌기 때문에 실제로는 거의 실행되지 않음)
    munmap(fbp_global, fb_screensize);
    close(fb_fd);
    close(server_sock);

    return 0;
}

