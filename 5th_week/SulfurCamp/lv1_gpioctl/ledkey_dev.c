#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <linux/gpio.h>
#include <linux/moduleparam.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DEBUG 1

#define LEDKEY_DEV_NAME   "ledkey"
#define LEDKEY_DEV_MAJOR  230

#define OFF 0
#define ON  1
#define GPIOCNT 1   // 단일 LED/버튼

// 라즈베리파이 BCM → 커널 GPIO 번호 (구 환경): 512 + BCM
// BCM17(LED)=512+17=529, BCM16(KEY)=512+16=528
static int gpioLed[GPIOCNT] = { 512 + 17 }; // 529
static int gpioKey[GPIOCNT] = { 512 + 16 }; // 528
static int irqKey[GPIOCNT];

static int openFlag = 0;
static int keyNum = 0; // 1이면 버튼 이벤트 도착
static DEFINE_MUTEX(keyMutex);
static DECLARE_WAIT_QUEUE_HEAD(waitQueueRead);

static irqreturn_t keyIsr(int irq, void *data)
{
    int i;
    for (i = 0; i < GPIOCNT; i++) {
        if (irq == irqKey[i]) {
            if (mutex_trylock(&keyMutex) != 0) {
                keyNum = 1; // 단일 키: 1로 고정
                mutex_unlock(&keyMutex);
                break;
            }
        }
    }
#if DEBUG
    printk("keyIsr() irq:%d, keyNum:%d\n", irq, keyNum);
#endif
    wake_up_interruptible(&waitQueueRead);
    return IRQ_HANDLED;
}

static int gpioLedInit(void)
{
    int ret;
    ret = gpio_request(gpioLed[0], "led0");
    if (ret < 0) {
        printk("Failed request gpio%d\n", gpioLed[0]);
        return ret;
    }
    ret = gpio_direction_output(gpioLed[0], OFF);
    if (ret < 0) {
        printk("Failed direction_output gpio%d\n", gpioLed[0]);
        return ret;
    }
    return 0;
}

static void gpioLedSet(long val)
{
    // val의 LSB만 사용 (0=OFF, 1=ON)
    gpio_set_value(gpioLed[0], (val & 0x01));
}

static void gpioLedFree(void)
{
    gpio_free(gpioLed[0]);
}

static int gpioKeyInit(void)
{
    int ret;
    ret = gpio_request(gpioKey[0], "key0");
    if (ret < 0) {
        printk("Failed request gpio%d\n", gpioKey[0]);
        return ret;
    }
    ret = gpio_direction_input(gpioKey[0]);
    if (ret < 0) {
        printk("Failed direction_input gpio%d\n", gpioKey[0]);
        return ret;
    }
    return 0;
}

static void gpioKeyFree(void)
{
    gpio_free(gpioKey[0]);
}

static int irqKeyInit(void)
{
    int ret;
    irqKey[0] = gpio_to_irq(gpioKey[0]);
    if (irqKey[0] < 0) {
        printk("Failed gpio_to_irq() gpio%d\n", gpioKey[0]);
        return irqKey[0];
    }
#if DEBUG
    printk("gpio_to_irq() gpio%d -> irq%d\n", gpioKey[0], irqKey[0]);
#endif
    return 0;
}

static void irqKeyFree(void)
{
    free_irq(irqKey[0], NULL);
}

static int ledkey_open(struct inode *inode, struct file *filp)
{
#if DEBUG
    printk("open: major=%d minor=%d\n", MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
#endif
    if (openFlag) return -EBUSY;
    openFlag = 1;

    if (!try_module_get(THIS_MODULE))
        return -ENODEV;

    return 0;
}

static ssize_t ledkey_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    char kbuf = 0;

    // 블로킹 read: 버튼 인터럽트 올 때까지 대기
    if (!(filp->f_flags & O_NONBLOCK)) {
        wait_event_interruptible(waitQueueRead, keyNum);
    }

    if (mutex_trylock(&keyMutex) != 0) {
        if (keyNum != 0) {
            kbuf = 1;    // 단일 버튼 이벤트 신호
            keyNum = 0;  // 소비
        }
        mutex_unlock(&keyMutex);
    }

    if (put_user(kbuf, buf) != 0)
        return -EFAULT;

#if DEBUG
    printk("read -> key:%d\n", kbuf);
#endif
    return 1; // 1바이트 반환
}

static ssize_t ledkey_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char kbuf = 0;
    if (get_user(kbuf, buf) != 0)
        return -EFAULT;

#if DEBUG
    printk("write -> led:%d\n", (kbuf & 1));
#endif
    gpioLedSet(kbuf);
    return 1;
}

static __poll_t ledkey_poll(struct file *filp, struct poll_table_struct *wait)
{
    __poll_t mask = 0;
    poll_wait(filp, &waitQueueRead, wait);
    if (keyNum > 0)
        mask |= POLLIN;
    return mask;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int ledkey_release(struct inode *inode, struct file *filp)
{
    module_put(THIS_MODULE);
    openFlag = 0;
    return 0;
}

static const struct file_operations ledkey_fops = {
    .open           = ledkey_open,
    .read           = ledkey_read,
    .write          = ledkey_write,
    .unlocked_ioctl = ledkey_ioctl,
    .poll           = ledkey_poll,
    .release        = ledkey_release,
};

static int __init ledkey_init(void)
{
    int ret;

    printk("ledkey_init\n");

    mutex_init(&keyMutex);

    ret = gpioLedInit();
    if (ret < 0) return ret;

    ret = gpioKeyInit();
    if (ret < 0) return ret;

    ret = irqKeyInit();
    if (ret < 0) return ret;

    // 풀업 버튼(눌렀을 때 GND)이라면 FOLLING이 ‘눌림’ 신호에 가깝다.
    ret = request_irq(irqKey[0], keyIsr, IRQF_TRIGGER_FALLING, "irqKey0", NULL);
    if (ret < 0) return ret;

    ret = register_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);
    if (ret < 0) return ret;

    return 0;
}

static void __exit ledkey_exit(void)
{
    unregister_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME);
    irqKeyFree();
    gpioLedFree();
    gpioKeyFree();
    mutex_destroy(&keyMutex);
    printk("ledkey_exit\n");
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("kcci + customized");
MODULE_DESCRIPTION("Single LED(KEY) toggle module for RPi (LED:BCM17, KEY:BCM16)");
