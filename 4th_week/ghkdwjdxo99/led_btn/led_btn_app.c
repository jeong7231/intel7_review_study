#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define GPIOCNT 4
#define DEVICE_FILENAME  "/dev/ledkey"

int main(int argc, char * argv[])
{
    int dev;
    char buff, buff_old = 0x0;
    int ret;
	char led;

	if(argc != 2)
	{
		printf("USAGE : %s ledval[0x0~0xf]\n",argv[0]);
		return 1;
	}
	led = strtoul(argv[1],NULL,16);

    if(access(DEVICE_FILENAME,F_OK) != 0)   //파일 없을시
    {
        int ret = mknod(DEVICE_FILENAME, S_IRWXU | S_IRWXG | S_IFCHR, (230 << 8) | 0);
        if(ret < 0)
            perror("mknod()");
    }

    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY);
    if( dev < 0 )
    {
		perror("open");
		return 2;
	}
	buff = led;
	
	while(1)
	{
		ret = write(dev, &led, sizeof(buff));
		// printf( "ret = %08X\n", ret );
		if(buff_old == 0x8){
			break;
		}

		ret = read(dev, &buff, sizeof(buff));              
		if(ret < 0)
		{
			perror("read()");
			return 3;
		}
		// printf( "ret = %08X, led = %#04x\n",ret, buff );

		if(buff != buff_old)
		{
			buff_old = buff;
			if(buff)
			{
				led = buff;
				puts("0:1:2:3");
				for(int i = 0; i < GPIOCNT; i++)
				{
					if(buff & (0x1 << i))
						putchar('O');
					else
						putchar('X');
					if(i != GPIOCNT - 1)
						putchar(':');
					else
						putchar('\n');
				}
				putchar('\n');
			}
		}
	}
	

	ret = close(dev);
    return 0;
}

