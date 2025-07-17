#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <vector>

#include <opencv2/opencv.hpp>

#define TCP_PORT 5100
#define WIDTH 640
#define HEIGHT 480
#define MAX_BUFFER_SIZE (1024 * 1024)

using namespace std;
using namespace cv;

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock < 0) {
        perror("socket()");
        return 1;
    }

    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(TCP_PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind()");
        close(serv_sock);
        return 1;
    }

    if (listen(serv_sock, 1) < 0) {
        perror("listen()");
        close(serv_sock);
        return 1;
    }

    printf("🔌 TCP 서버 실행 중... 포트 %d\n", TCP_PORT);

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock < 0) {
        perror("accept()");
        close(serv_sock);
        return 1;
    }

    printf("✅ 클라이언트 연결됨: %s\n", inet_ntoa(clnt_addr.sin_addr));

    vector<unsigned char> recv_buf(MAX_BUFFER_SIZE);

    while (true) {
        int frame_size = 0;
        int recv_len = recv(clnt_sock, &frame_size, sizeof(frame_size), 0);
        if (recv_len <= 0) break;

        if (frame_size > MAX_BUFFER_SIZE) {
            fprintf(stderr, "Frame too large\n");
            break;
        }

        int total_received = 0;
        while (total_received < frame_size) {
            int chunk = recv(clnt_sock, recv_buf.data() + total_received, frame_size - total_received, 0);
            if (chunk <= 0) {
                perror("recv() data");
                goto cleanup;
            }
            total_received += chunk;
        }

        // YUYV → BGR 변환
        Mat yuyv(HEIGHT, WIDTH, CV_8UC2, recv_buf.data());
        Mat bgr;
        cvtColor(yuyv, bgr, COLOR_YUV2BGR_YUYV);

        // 실시간 화면 표시
        imshow("📷 수신된 프레임", bgr);
        if (waitKey(1) == 27) break;  // ESC 키 누르면 종료

        // 응답 전송
        int ok = 1;
        send(clnt_sock, &ok, sizeof(ok), 0);
    }

cleanup:
    close(clnt_sock);
    close(serv_sock);
    destroyAllWindows();
    return 0;
}

