# p238_ledkey
key로 led 제어하는 예제
## call_ledkey_app.c

### 0. 헤더 및 변수 선언
```c++
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_FILENAME "/dev/ledkey"

int main(int argc, char * argv[])
{
	int dev;
	char buff;
	int ret;
	char key;
	int key_data = 0;
	int key_data_old = 0;

	if(argc != 2)		//에러처리
	{
		printf("USAGE : %s ledval[0x00~0xff]\n", argv[0]);
		return 1;
	}
	key = strtoul(argv[1], NULL, 16);	//문자열을 unsigned long으로 변환


```
### 1. 디바이스 파일 open
```c++
    if(access(DEVICE_FILENAME,F_OK) != 0)   // 파일 없을 시
    {
        int ret = mknod("/dev/ledkey", S_IRWXU|S_IRWXG|S_IFCHR, (230<<8) | 0);
        if(ret < 0)
            perror("mknod()");
    }
	
	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY); 
	//dev = open(DEVICE_FILENAME, O_RDWR); 
	if(dev < 0)
	{
		perror("open");
		return 2;
	}
```

### 2. read & write
```c++
	do{
		usleep(100000);	//100msec
		ret = read(dev, &buff, sizeof(buff));
		if(ret < 0)		//에러 처리
		{
			perror("read()");
			return 3;
		}

		ret = write(dev, &key, sizeof(key));
		if(ret < 0)		//에러 처리
		{
			perror("write()");
			return 4;
		}

		key_data = (int)buff;

		if(key_data < 0)	//에러 처리
		{
			printf("key_data: %d\n", key_data);
			perror("syscall()");
		}
		if(key_data_old == 0x80)
			break;
```
read : 키 값 read  
write : led에 write  

### 3. O X 출력 처리
```c++
		if(key_data != key_data_old)
		{
			key_data_old = key_data;
			if(key_data)
			{
				key = key_data;
				printf("key_data : %d\n", key_data);

				printf("0:1:2:3:4:5:6:7\n");
				for(int i = 0; i < 8; i++)
				{
					if((key_data >> i) & 0x01)
						putchar('O');
					else
						putchar('X');
					if(i != 7)
						putchar(':');
					else
						putchar('\n');
				}
				putchar('\n');
			}
		}
	}while(1);	
```

### 4. 디바이스 파일 닫기
```c++
	ret = close(dev);
	printf("ret = %08X\n", ret);

	return 0;
}
```

## call_ledkey_dev.c
커널 공간에서 동작하며, 실제 하드웨어(GPIO)를 제어하는 역할

### module 관련 함수
1. pi에서 insmod 명령어로 모듈을 커널에 적재 하면  
-> ledkey_init( ) 함수 호출됨  
    (1) register_chrdev( ) : CALL_DEV_MAJOR (주번호 230)와 CALL_DEV_NAME ("ledkey_dev")으로 문자 디바이스 드라이버를 커널에 등록합니다. 이때 ledkey_fops 구조체를 함께 등록하여 open, read, write 등의 동작을 이 드라이버가 처리하도록 지정  
    (2) gpioLedInit( ) : LED와 연결된 GPIO 핀 8개를 요청( gpio_request )하고, 입력 모드( gpio_direction_output )로 설정  
    (3) gpioKeyInit( ) : Key와 연결된 GPIO 핀 8개를 요청( gpio_request )하고, 입력 모드( gpio_direction_output )로 설정  

2. pi에서 rmmod 명령어로 모듈을 커널에 적재 해제하면  
-> ledkey_exit( ) 함수 호출됨
    (1) gpioLedFree( ), gpioKeyFree( ) : init과정에서 할당받았던 모든 GPIO 자원을 해제
    (2) unregister_chrdev( ) : 커널에 등록했던 문자 디바이스 드라이버를 해제
```c++
//-----------------module-----------------------
struct file_operations ledkey_fops =
{
	.owner = THIS_MODULE,
	.open = ledkey_open,
	.read 	= ledkey_read,
	.write 	= ledkey_write,
	//.unlocked_ioctl = ledkey_ioctl,
	.release = ledkey_release,
};

static int ledkey_init(void)
{
	int result;
	printk("call ledkey_init \n");
	result = register_chrdev(CALL_DEV_MAJOR, CALL_DEV_NAME, &ledkey_fops);
	if(result < 0) return result;

    result = gpioLedInit();
    if(result < 0)
        return result;
	
    result = gpioKeyInit();
    if(result < 0)
        return result;

	return 0;
}
static void ledkey_exit(void)
{
	printk("call ledkey_exit \n");

    gpioLedFree();
    gpioKeyFree();

	unregister_chrdev(CALL_DEV_MAJOR, CALL_DEV_NAME);
}
module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("test module");
MODULE_LICENSE("Dual BSD/GPL");
```


### 1. 헤더 및 함수 선언
```c++
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

#define CALL_DEV_NAME "ledkey_dev"
#define CALL_DEV_MAJOR 230
#define GPIOCNT 8
#define LED_OFF 0
#define LED_ON 1

#define DEBUG 1

typedef struct {
	int num;
	//char name[10];
	char * pName;
} Student;

//int gpioLed[GPIOCNT] = {6,7,8,9,10,11,12,13};
static unsigned int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);
static unsigned int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};
static int gpioKeyInit(void);
static long gpioKeyGet(void);
static void gpioKeyFree(void);

```

### 2. LED관련 함수
```c++
//---------------function-------------------------------
static int gpioLedInit(void)
{
	int i;
	int ret=0 ;
	char gpioName[10];
	for(i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"led%d",i);
		ret=gpio_request(gpioLed[i],gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n",gpioLed[i]);
			return ret;
		}
	}
	for(i=0;i<GPIOCNT;i++)
	{
		ret = gpio_direction_output(gpioLed[i],LED_OFF);	
		if(ret < 0)
		{
			printk("Failed direction_output gpio%d error\n",gpioLed[i]);
			return ret;
		}
	}
	return ret;
}
static void gpioLedSet(long val)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_set_value(gpioLed[i],((val >> i) & 0x01));
	}			
}
static void gpioLedFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}
```

### 3. key 관련 함수
```c++
static int gpioKeyInit(void)
{
	int i;
	int ret=0 ;
	char gpioName[10];
	for(i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"key%d",i);
		ret=gpio_request(gpioKey[i],gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n",gpioKey[i]);
			return ret;
		}
	}
	for(i=0;i<GPIOCNT;i++)
	{
		ret = gpio_direction_input(gpioKey[i]);	
		if(ret < 0)
		{
			printk("Failed direction_output gpio%d error\n",gpioKey[i]);
			return ret;
		}
	}
	return ret;
}
static long gpioKeyGet(void)
{
	int i;
	long key=0;
	long ret;
	for(i=0;i<GPIOCNT;i++)
	{
		ret = gpio_get_value(gpioKey[i]);
		if (ret < 0)
			return ret;
		key |= ret << i;
//		key |= gpio_get_value(gpioKey[i]) << i;
	}			
	return key;
}
static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}
```

### 4. ledkey 관련 함수
```c++
//---------------------------------------------------
static int ledkey_open(struct inode *inode, struct file *filp)
{
	Student *pStudent;
	int num0 = MAJOR(inode -> i_rdev);
	int num1 = MINOR(inode -> i_rdev);
	printk("call open -> major : %d\n", num0);
	printk("call open -> minor : %d\n", num1);
//	try_module_get(THIS_MODULE);

	pStudent = (Student *)kmalloc(sizeof(Student), GFP_KERNEL);
	if(pStudent == NULL)
		return -ENOMEM;
	
	pStudent -> num = 1234;
	pStudent -> pName = "hong gildong";

	filp -> private_data = pStudent;
	return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char key;
	int ret;

	Student *pStudent = (Student *)filp->private_data;
#ifdef DEBUF
//	printk("call read-> buf : %08X, count : %08X\n", (unsigned int)buf, count);
	printk("num : %d, name : %s\n", pStudent->num, pStudent->pName);
#endif

	key = (char)gpioKeyGet();
	if(key < 0)
		return key;
#ifdef DEBUG
	printk(KERN_INFO "key = %#04x \n",key);
#endif
//	put_user(key, buf);
	ret = copy_to_user(buf, &key, count);
	if (ret < 0)		//반환값 처리
		return -EFAULT;

	return count;
}

static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
//	printk("call write -> buf : %08X, count : %08X\n", (unsigned int)buf, count);
	char val;
	get_user(val,buf);
#ifdef DEBUG
	printk(KERN_INFO "key_val = %#04x \n",val);
#endif
	gpioLedSet(val);

	return count;
}

static int ledkey_release(struct inode *inode, struct file *filp)
{
	printk("call release \n");
//	module_put(THIS_MODULE);
	kfree(filp->private_data);
	return 0;
}
```

