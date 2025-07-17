//chat_ght_code

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#define TCP_PORT 5100
#define MAX_BUFFER_SIZE (1024 * 1024)  // 최대 프레임 버퍼 크기

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    // 1. 소켓 생성
    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    // 2. 주소 재사용 옵션
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 모든 IP에서 접속 허용
    serv_addr.sin_port = htons(TCP_PORT);

    // 4. 바인드
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind()");
        close(serv_sock);
        exit(1);
    }

    // 5. 리슨
    if (listen(serv_sock, 1) < 0) {
        perror("listen()");
        close(serv_sock);
        exit(1);
    }

    printf("🔌 TCP 서버 실행 중... 포트 %d\n", TCP_PORT);

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock < 0) {
        perror("accept()");
        close(serv_sock);
        exit(1);
    }

    printf("✅ 클라이언트 연결됨: %s\n", inet_ntoa(clnt_addr.sin_addr));

    // 6. 프레임 수신 루프
    char *recv_buf = malloc(MAX_BUFFER_SIZE);
    if (!recv_buf) {
        perror("malloc()");
        close(clnt_sock);
        close(serv_sock);
        return 1;
    }

    while (1) {
        int frame_size = 0;
        int recv_len = 0;

        // (1) 프레임 크기 수신
        while ((recv_len = recv(clnt_sock, &frame_size, sizeof(frame_size), 0)) < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            } else {
                perror("recv() frame size");
                goto cleanup;
            }
        }
        if (recv_len == 0) {
            printf("클라이언트 연결 종료\n");
            break;
        }

        printf("📦 수신할 프레임 크기: %d 바이트\n", frame_size);
        if (frame_size > MAX_BUFFER_SIZE) {
            fprintf(stderr, "Frame size too large\n");
            break;
        }

        // (2) 프레임 데이터 수신
        int total_received = 0;
        while (total_received < frame_size) {
            int chunk = recv(clnt_sock, recv_buf + total_received, frame_size - total_received, 0);
            if (chunk < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(1000);
                    continue;
                } else {
                    perror("recv() frame data");
                    goto cleanup;
                }
            } else if (chunk == 0) {
                printf("클라이언트 연결 종료 (도중)\n");
                goto cleanup;
            }
            total_received += chunk;
        }

        printf("🖼️ 프레임 %d바이트 수신 완료\n", total_received);

        // (3) 수신 완료 응답 전송 (1:성공)
        int ok = 1;
        send(clnt_sock, &ok, sizeof(ok), 0);
    }

cleanup:
    free(recv_buf);
    close(clnt_sock);
    close(serv_sock);
    printf("서버 종료\n");
    return 0;
}


