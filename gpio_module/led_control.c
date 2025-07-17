#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> // open
#include <unistd.h> // close, write
#include <string.h> // strlen

int main(int argc, char *argv[]) {
    int fd;
    char *led_state;

    if (argc != 2) {
        printf("사용법: %s [0|1]\n", argv[0]);
        printf("  0: LED 끄기\n");
        printf("  1: LED 켜기\n");
        return 1;
    }

    led_state = argv[1]; // 명령줄 인수를 LED 상태로 사용

    // /dev/gpioled 디바이스 파일 열기
    fd = open("/dev/gpioled", O_WRONLY);
    if (fd < 0) {
        perror("Error opening /dev/gpioled");
        return 1;
    }

    // LED 상태에 따라 "0" 또는 "1" 문자열을 모듈에 write
    if (strcmp(led_state, "0") == 0) {
        write(fd, "0", strlen("0"));
        printf("LED를 껐습니다.\n");
    } else {
        write(fd, "1", strlen("1")); // "0"이 아니면 모두 켜짐
        printf("LED를 켰습니다.\n");
    }

    // 디바이스 파일 닫기
    close(fd);

    return 0;
}
