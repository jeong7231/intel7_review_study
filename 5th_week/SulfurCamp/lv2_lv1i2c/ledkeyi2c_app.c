// unified_app.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEV_LEDKEY "/dev/ledkey"
#define DEV_LCD    "/dev/i2c_lcd"

// LCD ioctl (i2c_lcd_dev.c와 일치해야 함)
#define I2C_LCD_IOC_MAGIC  'L'
#define I2C_LCD_CLEAR      _IO(I2C_LCD_IOC_MAGIC, 0)
#define I2C_LCD_HOME       _IO(I2C_LCD_IOC_MAGIC, 1)
#define I2C_LCD_SETPOS     _IOW(I2C_LCD_IOC_MAGIC, 2, int) // (row<<16)|col

static int ensure_node(const char* path, int major)
{
    if (access(path, F_OK) == 0) return 0;
    mode_t m = S_IRWXU | S_IRWXG | S_IFCHR;
    dev_t d = (major << 8) | 0;
    if (mknod(path, m, d) < 0) { perror("mknod"); return -1; }
    chmod(path, 0666);
    return 0;
}

int main(int argc, char** argv)
{
    int led_init = 0; // 초기 LED 상태(0/1)
    if (argc == 2) led_init = atoi(argv[1]) ? 1 : 0;

    // (udev 미사용 가정) /dev 노드 보조 생성
    // ledkey major=230, i2c_lcd major=231 (드라이버와 맞춰야 함)
    ensure_node(DEV_LEDKEY, 230);
    ensure_node(DEV_LCD,    231);

    int fd_ledkey = open(DEV_LEDKEY, O_RDWR); // 블록 모드 - poll로 이벤트 대기
    if (fd_ledkey < 0) { perror("open /dev/ledkey"); return 1; }

    int fd_lcd = open(DEV_LCD, O_WRONLY);
    if (fd_lcd < 0) {
        perror("open /dev/i2c_lcd"); // LCD 없이도 계속 동작하고 싶으면 그냥 경고만
    } else {
        ioctl(fd_lcd, I2C_LCD_CLEAR, 0);
        ioctl(fd_lcd, I2C_LCD_HOME, 0);
        write(fd_lcd, "LEDKEY + I2CLCD", 18);
        ioctl(fd_lcd, I2C_LCD_SETPOS, (1<<16)|0); // row=1,col=0
        write(fd_lcd, "Ready...", 8);
    }

    // LED 초기 상태 적용 (기존 ledkey 규약: write 1바이트)
    unsigned char led_state = (unsigned char)led_init;
    if (write(fd_ledkey, &led_state, 1) != 1) perror("write led init");
    printf("[INIT] LED=%s\n", led_state ? "ON":"OFF");

    struct pollfd pfd = { .fd = fd_ledkey, .events = POLLIN };
    while (1) {
        int ret = poll(&pfd, 1, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (pfd.revents & POLLIN) {
            unsigned char key;
            int r = read(fd_ledkey, &key, 1);
            if (r == 1) {
                // 버튼 이벤트 수신: LED 토글
                led_state ^= 1;
                if (write(fd_ledkey, &led_state, 1) != 1) perror("write led");

                char line[64];
                snprintf(line, sizeof(line), "press! LED=%s", led_state ? "ON":"OFF");
                printf("%s\n", line);

                if (fd_lcd >= 0) {
                    ioctl(fd_lcd, I2C_LCD_CLEAR, 0);
                    ioctl(fd_lcd, I2C_LCD_HOME, 0);
                    write(fd_lcd, line, strlen(line));
                    ioctl(fd_lcd, I2C_LCD_SETPOS, (1<<16)|0);
                    write(fd_lcd, "Press key", 19);
                }
            }
        }
    }

    if (fd_lcd >= 0) close(fd_lcd);
    close(fd_ledkey);
    return 0;
}
