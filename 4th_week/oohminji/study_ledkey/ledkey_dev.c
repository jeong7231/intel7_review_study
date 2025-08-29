//call_ledkey_dev.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/slab.h>
#include <linux/gpio.h>

#define GPIOCNT 3
#define LED_OFF 0
#define LED_ON 1
#define CALL_DEV_NAME            "ledkey_dev"
#define CALL_DEV_MAJOR            231      

#define DEBUG 1

typedef struct {
	int num;
//	char name[10];
	char * pName;
} Student;

//int gpioLed[GPIOCNT] = {6,7,8,9,10,11,12,13};;
static unsigned int gpioLed[GPIOCNT] = {518, 519, 520};
static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);
static unsigned int gpioKey[GPIOCNT] = {528, 529, 530};
static int gpioKeyInit(void);
static long gpioKeyGet(void);
static void gpioKeyFree(void);

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
//		key |= gpio_get_value(gpioKey[i]) << i;

		ret = gpio_get_value(gpioKey[i]);
		if(ret < 0)
			return ret;
		key |= ret << i;

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
static int ledkey_open(struct inode *inode, struct file *filp)
{
	Student *pStudent;
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );
	try_module_get(THIS_MODULE);

	pStudent = (Student *)kmalloc(sizeof(Student),GFP_KERNEL);
	if(pStudent == NULL)
		return -ENOMEM;

	pStudent->num=1234;
	pStudent->pName = "hong gildong";

	filp->private_data = pStudent;
    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char key;
//	int ret;	
//	char keyBuff[8];	

	Student *pStudent = (Student *)filp->private_data;
#ifdef DEBUG
//    printk( "call read -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
	printk("num : %d, name : %s\n", pStudent->num, pStudent->pName);
#endif

	key = (char)gpioKeyGet();
#ifdef DEBUG
	printk(KERN_INFO "key = %#04x\n",key);
#endif
	if(key < 0)
		return key;
	put_user(key, buf);
	 

/*	for(int i=0;i<count;i++)
		put_user(keyBuff[i],buf++);
*/
//	ret = copy_to_user(buf, &key, count);
//	if(ret < 0)
//		return ret;

//	copy_to_user(buf, keyBuff, count);

	
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char led;

	printk( "call write -> buf : %08X, count : %08X \n", (unsigned int)buf, count );
 	if (get_user(led, buf) != 0) {
		printk(KERN_ERR "get_user failed\n");
		return -EFAULT;
	}
	gpioLedSet(led);
	return 0x43;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk( "call ioctl -> cmd : %08X, arg : %08X \n", cmd, (unsigned int)arg );
    return 0x53;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );
	module_put(THIS_MODULE);
	kfree(filp->private_data);
    return 0;
}

struct file_operations ledkey_fops =
{
    .owner    = THIS_MODULE,
    .open     = ledkey_open,     
    .read     = ledkey_read,     
    .write    = ledkey_write,    
	.unlocked_ioctl = ledkey_ioctl,
    .release  = ledkey_release,  
};

static int ledkey_init(void)
{
    int result;

    printk( "call ledkey_init \n" );    

    result = register_chrdev( CALL_DEV_MAJOR, CALL_DEV_NAME, &ledkey_fops);
    if (result < 0) return result;

    result = gpioLedInit();    //gpio_request(), gpiot_direction_output();
    if(result < 0)
        return result;

    result = gpioKeyInit();
    if(result < 0)
        return result;

    return 0;
}

static void ledkey_exit(void)
{
    printk( "call ledkey_exit \n" );    
	gpioLedFree();          //gpio_free();
    gpioKeyFree();          //gpio_free();

    unregister_chrdev( CALL_DEV_MAJOR, CALL_DEV_NAME );
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("test moudle");
MODULE_LICENSE("Dual BSD/GPL");
