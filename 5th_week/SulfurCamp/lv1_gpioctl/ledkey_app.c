#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define DEVICE_FILENAME "/dev/ledkey"

int main(int argc, char *argv[])
{
    int dev;
    char buff;
    int ret;
    int led_state; // 0=OFF, 1=ON
    struct pollfd Events[2];
    char line[80];

    if (argc != 2) {
        printf("Usage : %s <init_led_state: 0|1>\n", argv[0]);
        return 1;
    }

    // /dev 노드 없으면 생성
    if (access(DEVICE_FILENAME, F_OK) != 0) {
        int r = mknod(DEVICE_FILENAME, S_IRWXU | S_IRWXG | S_IFCHR, (230 << 8) | 0);
        if (r < 0) {
            perror("mknod");
            return 2;
        }
    }

    // 초기 LED 상태 파싱
    led_state = atoi(argv[1]);
    if (!(led_state == 0 || led_state == 1)) {
        printf("Usage : %s <init_led_state: 0|1>\n", argv[0]);
        return 2;
    }

    // 디바이스 오픈(블로킹)
    dev = open(DEVICE_FILENAME, O_RDWR);
    if (dev < 0) {
        perror("open");
        return 2;
    }

    // 초기 LED 상태 설정
    buff = (char)(led_state & 1);
    if (write(dev, &buff, sizeof(buff)) != sizeof(buff)) {
        perror("write init");
        close(dev);
        return 2;
    }
    printf("초기 LED 상태: %s\n", led_state ? "ON" : "OFF");
    printf("q + Enter 로 종료. 0 또는 1 입력 시 강제로 LED 상태 설정.\n");

    // poll 설정: stdin, device
    memset(Events, 0, sizeof(Events));
    Events[0].fd = fileno(stdin);
    Events[0].events = POLLIN;
    Events[1].fd = dev;
    Events[1].events = POLLIN;

    while (1) {
        ret = poll(Events, 2, -1); // 무한 대기
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // STDIN
        if (Events[0].revents & POLLIN) {
            if (!fgets(line, sizeof(line), stdin)) continue;
            if (!strcmp(line, "q\n")) break;

            // 강제 LED 설정: "0\n" 또는 "1\n"
            if (!strcmp(line, "0\n") || !strcmp(line, "1\n")) {
                led_state = (line[0] == '1') ? 1 : 0;
                buff = (char)led_state;
                if (write(dev, &buff, sizeof(buff)) != sizeof(buff))
                    perror("write force");
                printf("수동설정 → LED: %s\n", led_state ? "ON" : "OFF");
            } else {
                // 기타 입력은 무시
                printf("입력 무시: q(종료), 0, 1 만 유효\n");
            }
        }

        // 디바이스(버튼 이벤트)
        if (Events[1].revents & POLLIN) {
            char k;
            ret = read(dev, &k, sizeof(k)); // k==1 이면 "눌림 이벤트"
            if (ret == sizeof(k) && k == 1) {
                // 토글
                led_state ^= 1;
                buff = (char)led_state;
                if (write(dev, &buff, sizeof(buff)) != sizeof(buff))
                    perror("write toggle");

                printf("버튼눌림! → LED: %s\n", led_state ? "ON" : "OFF");
            }
        }
    }

    close(dev);
    return 0;
}
