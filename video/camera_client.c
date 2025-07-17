#include <stdio.h>              // 표준 입출력 함수 (printf, perror 등)
#include <stdlib.h>             // 메모리 할당, exit 등
#include <stdint.h>             // 고정 크기 정수형 사용 (uint8_t 등)
#include <string.h>             // memset 등 문자열/메모리 함수
#include <fcntl.h>              // 파일 제어 (open, O_RDWR 등)
#include <unistd.h>             // 시스템 호출 (read, write, close 등)
#include <sys/ioctl.h>          // 장치 제어 함수 ioctl()
#include <sys/socket.h>         // 소켓 프로그래밍
#include <netinet/in.h>         // sockaddr_in 구조체 정의 등
#include <arpa/inet.h>          // inet_pton() 등 IP 주소 변환
#include <linux/videodev2.h>    // V4L2(Video4Linux2) API

#define VIDEO_DEVICE        "/dev/video0"        // 사용할 비디오 장치 경로
#define WIDTH               640                  // 캡처할 영상의 가로 해상도
#define HEIGHT              480                  // 캡처할 영상의 세로 해상도
#define SERVER_PORT         8080                 // 서버가 수신 대기할 포트 번호
#define SERVER_IP           "192.168.137.2"      // 서버의 IP 주소 (정수의 라즈 베리 주소)


// 지정한 길이만큼 데이터를 소켓을 통해 모두 전송할 때까지 반복하는 함수
ssize_t send_all(int socket, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const uint8_t *ptr = (const uint8_t *)buffer;
    while (total_sent < length) {
        ssize_t sent = send(socket, ptr + total_sent, length - total_sent, 0);
        //socket에서 ptr + total_sent 주소값 위치에 전체데이터 길이- 포탈센트 만큼 넣는 것을 sent 변수 명으로 정의 한다.
        if (sent == -1) {
            perror("send failed");
            return -1; // 에러 발생
        }
        if (sent == 0) { // 연결 종료
            fprintf(stderr, "Server disconnected unexpectedly.\n");
            return 0;
        }
        total_sent += sent;
    }
    return total_sent;
    //전체 데이터 길이만큼 보내면 종료
}

int main() {
    // 1. 비디오 장치 열기 및 설정
    int video_fd = open(VIDEO_DEVICE, O_RDWR);
    if (video_fd == -1) {
        perror("Failed to open video device");
        return 1;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt)); //fmt 구조체 초기화
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 비디오 캡쳐 타입
    fmt.fmt.pix.width = WIDTH; // 해상도 너비
    fmt.fmt.pix.height = HEIGHT; // 해상도 높이
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // YUYV 포맷
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(video_fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("Failed to set video format");
        close(video_fd);
        return 1;
    }

    // 비디오 버퍼 할당 (YUYV 데이터는 WIDTH * HEIGHT * 2 바이트)
    uint8_t *video_buffer = malloc(fmt.fmt.pix.sizeimage);
    if (!video_buffer) { // 비디오 버퍼 할당 실패
        perror("Failed to allocate video buffer");
        close(video_fd);
        return 1;
    }

    // 2. TCP 클라이언트 소켓 설정
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) { // 클라언드 소켓 생성 실패
        perror("Failed to create client socket");
        free(video_buffer);
        close(video_fd);
        return 1;
    }

    struct sockaddr_in server_addr; // 서버 주소 정보를 담는 구조체 선언
    memset(&server_addr, 0, sizeof(server_addr)); // 구조체 전체를 0으로 초기화 (필수 - 쓰레기값 방지)
    server_addr.sin_family = AF_INET; // 주소 체계를 IPv4로 설정 (AF_INET: IPv4 인터넷 프로토콜)
    server_addr.sin_port = htons(SERVER_PORT); // 포트 번호를 네트워크 바이트 순서로 변환하여 설정
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address or address not supported");
        close(client_sock);
        free(video_buffer);
        close(video_fd);
        return 1;
    }

    printf("Connecting to server at %s:%d...\n", SERVER_IP, SERVER_PORT);
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) { // 서버 연결 실패
        perror("Failed to connect to server"); 
        close(client_sock);
        free(video_buffer);
        close(video_fd);
        return 1;
    }
    printf("Connected to server.\n");

    // 3. 메인 루프: 영상 캡처 후 서버로 프레임 전송
    while (1) { // 비디오 장치에서 한 프레임 캡처 (YUYV 포맷, fmt.fmt.pix.sizeimage 크기만큼)
        ssize_t bytes_read = read(video_fd, video_buffer, fmt.fmt.pix.sizeimage);
        if (bytes_read == -1) {
            perror("Failed to read frame from video device");
            break;
        }

        if (send_all(client_sock, video_buffer, bytes_read) == -1) { // 캡처한 프레임을 서버로 전송 (send_all은 전체 데이터가 전송될 때까지 반복)
            fprintf(stderr, "Error sending frame. Disconnecting.\n");
            break;
        }
        printf("Sent frame of size %zd bytes\n", bytes_read);
    }

    // 자원 해제
    close(client_sock); //클라이언트 소켓 닫기
    free(video_buffer); //비디오 버퍼 해제->
    close(video_fd); // 비디오 장치 닫기

    return 0;
}

