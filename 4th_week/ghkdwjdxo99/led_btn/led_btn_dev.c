#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/slab.h>
#include <linux/gpio.h>

#define GPIOCNT 	4
#define LED_OFF		0
#define LED_ON		1
#define CALL_DEV_NAME	"ledkey_dev"
#define CALL_DEV_MAJOR	 230

int gpioLedInit(void);
void gpioLedSet(long);
void gpioLedFree(void);

int gpioKeyInit(void);
long gpioKeyGet(void);
void gpioKeyFree(void);

unsigned int gpioLed[GPIOCNT] = {518, 519, 520, 521};
unsigned int gpioKey[GPIOCNT] = {528, 529, 530, 531};

int gpioLedInit()
{
	int i;
	int ret = 0;
	char gpioName[10];

	for(i = 0; i < GPIOCNT; i++)
	{
		sprintf(gpioName, "led%d", i);
		ret = gpio_request(gpioLed[i], gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n", gpioLed[i]);
			return ret;
		}
	}

	for(i = 0; i < GPIOCNT; i++)
	{
		ret = gpio_direction_output(gpioLed[i], LED_OFF);
		if(ret < 0)
		{
			printk("Failed direction_output gpio%d error\n", gpioLed[i]);
			return ret;
		}
	}
	return ret;
}
void gpioLedSet(long val)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		// gpio_set_value(gpioLed[i], (val & (0x01 << i)));
		gpio_set_value(gpioLed[i], ((val >> i) & 0x01));
	}
}
void gpioLedFree()
{
	for(int i = 0; i < GPIOCNT; i++)
		{
			gpio_free(gpioLed[i]);
		}
}




/* ****************************** */





int gpioKeyInit()
{
	// gpio_request(), gpio_direction_input();
	int ret = 0;
	char gpioName[10];

	for(int i = 0; i < GPIOCNT; i++)
	{
		sprintf(gpioName, "key%d", i);
		ret = gpio_request(gpioKey[i], gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n", gpioKey[i]);
			return ret;
		}
	}

	for(int i = 0; i < GPIOCNT; i++)
	{
		ret = gpio_direction_input(gpioKey[i]);
		if(ret < 0)
		{
			printk("Failed gpio_direction_input gpio%d error\n", gpioKey[i]);
			return ret;
		}
	}
	return ret;
}
long gpioKeyGet()
{
	long ret = 0;
	for(int i = 0; i < GPIOCNT; i++)
	{
		ret += (gpio_get_value(gpioKey[i]) << i);
		// printk("gpio_get_value(gpioKey[i]) << i : %d\n", gpio_get_value(gpioKey[i]) << i);
		// printk("ret : %ld\n", ret);
	}
	return ret;
}
void gpioKeyFree()
{
	for(int i = 0; i < GPIOCNT; i++)
		{
			gpio_free(gpioKey[i]);
		}
}

/* ***************************************** */

static int ledkey_open(struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );
	try_module_get(THIS_MODULE);

    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char key;

	key = (char)gpioKeyGet();
    printk(KERN_INFO "key = %#04x\n",key);

	if(key < 0)
		return key;
	put_user(key, buf);
	
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    printk( "call write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
	char key;
	get_user(key, buf);
	gpioLedSet((long)key);

    return count;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );
    return 0;
}

/* ***************************************** */
struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,
    .open     = ledkey_open,
    .read     = ledkey_read,
    .write    = ledkey_write,
    .release  = ledkey_release
};

static int led_init(void)
{
    printk(KERN_INFO "Hello, led\n");

    long val = 0x0;
    int ret = 0;

    ret = register_chrdev( CALL_DEV_MAJOR, CALL_DEV_NAME, &ledkey_fops);
    if(ret < 0)
        return ret;
    
	ret = gpioLedInit();
	if(ret < 0)
		return ret;

    ret = gpioKeyInit();
	if(ret < 0)
		return ret;

    val = gpioKeyGet();
	printk("val 1 : %ld\n", val);
    gpioLedSet(val);

    return 0;
}

static void led_exit(void)
{
    printk(KERN_INFO "Goodbye, led\n");

    long val = 0x00;

    val = gpioKeyGet();
	// val = 0x1;
	printk("val 2 : %ld\n", val);
    gpioLedSet(val);

    gpioLedFree();
    gpioKeyFree();

    unregister_chrdev( CALL_DEV_MAJOR, CALL_DEV_NAME );
}


/* ****************************** */

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("HJT");
MODULE_DESCRIPTION("test moudle");
MODULE_LICENSE("Dual BSD/GPL");