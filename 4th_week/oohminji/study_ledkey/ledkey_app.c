#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_FILENAME  "/dev/ledkey"

int main(int argc, char * argv[])
{
    int dev;
    unsigned char buff = 0;
    unsigned char buff_old = 0;
    int ret;

    if(argc != 2)
    {
        printf("USAGE : %s ledval[0x00~0x07]\n", argv[0]);
        return 1;
    }

    unsigned char key = strtoul(argv[1], NULL, 16);

    if(access(DEVICE_FILENAME, F_OK) != 0)   // 파일 없으면 생성
    {
        int ret = mknod(DEVICE_FILENAME, S_IRWXU | S_IRWXG | S_IFCHR, (231 << 8) | 0);
        if(ret < 0) {
            perror("mknod()");
            return 2;
        }
    }

    dev = open(DEVICE_FILENAME, O_RDWR|O_NDELAY);
    if(dev < 0)
    {
        perror("open");
        return 3;
    }

    do {
        ret = read(dev, &buff, sizeof(buff));
        if(ret < 0)
        {
            perror("read()");
            break;
        }
        //buff = ~buff;

        if(buff_old != buff)
        {
            buff_old = buff;
            if(buff)
            {
                unsigned char led_val = buff & 0x07; // 하위 3비트만 LED에 반영
                ret = write(dev, &led_val, sizeof(led_val));

                puts("0:1:2");
                for(int i=0;i<3;i++)
                {
                    if((buff >> i) & 0x01) 
                        putchar('O');
                    else
                        putchar('X');
                    if(i != 2 )
                        putchar(':');
                    else
                        putchar('\n');
                }
                printf("changed buff: 0x%02X (%u)\n", buff, buff);
                putchar('\n');
            }
        }

        if(buff_old == 0x01) // 예: KEY3 눌리면 종료 (원래 0x80 대신)
            break;

    } while(1);

    close(dev);
    return 0;
}
