// i2c_lcd_dev.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#define I2C_LCD_NAME        "i2c_lcd"
#define I2C_LCD_MAJOR       231

/* ----- IOCTL ----- */
#define I2C_LCD_IOC_MAGIC   'L'
#define I2C_LCD_CLEAR       _IO(I2C_LCD_IOC_MAGIC, 0)
#define I2C_LCD_HOME        _IO(I2C_LCD_IOC_MAGIC, 1)
#define I2C_LCD_SETPOS      _IOW(I2C_LCD_IOC_MAGIC, 2, int) /* (row<<16)|col */

/* ----- Module params ----- */
static int bus  = 1;   module_param(bus,  int, 0444); MODULE_PARM_DESC(bus,  "I2C bus number");
static int addr = 0x27;module_param(addr, int, 0644); MODULE_PARM_DESC(addr, "I2C address (0x27 or 0x3F)");
static int cols = 16;  module_param(cols, int, 0644); MODULE_PARM_DESC(cols, "LCD columns");
static int rows = 2;   module_param(rows, int, 0644); MODULE_PARM_DESC(rows, "LCD rows");

/* 안정화 옵션 */
static int slow_mode = 0; module_param(slow_mode, int, 0644); MODULE_PARM_DESC(slow_mode, "1=extra delays");
static int debug     = 0; module_param(debug,     int, 0644); MODULE_PARM_DESC(debug,     "1=log i2c bytes");

/* ----- PCF8574 mapping (Layout B / YwRobot)
 * P4..P7: D4..D7 (상위 니블)
 * P3: Backlight (active-high)
 * P2: EN
 * P1: RW (always 0)
 * P0: RS
 */
#define BL_BIT  0x08  /* P3 */
#define EN_BIT  0x04  /* P2 */
#define RW_BIT  0x02  /* P1 (always 0) */
#define RS_BIT  0x01  /* P0 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
#define CLASS_CREATE(name) class_create(name)
#else
#define CLASS_CREATE(name) class_create(THIS_MODULE, name)
#endif

struct i2c_lcd_dev {
    struct i2c_client *client;
    struct cdev        cdev;
    struct class      *cls;
    struct device     *dev;
    int                cur_row;
    int                cur_col;
    u8                 bl_on;
};
static struct i2c_lcd_dev gdev;

/* ----- helpers ----- */
static inline void tiny_udelay(unsigned int us)
{
    if (slow_mode) udelay(us * 4);
    else           udelay(us);
}

/* 어떤 바이트든 BL 상태를 항상 반영(깜빡임 방지) */
static inline u8 with_bl(u8 data)
{
    return gdev.bl_on ? (data | BL_BIT) : (data & ~BL_BIT);
}

static int i2c_lcd_write_byte_raw(u8 data)
{
    if (debug) pr_info("i2c_lcd: write 0x%02x\n", data);
    return i2c_smbus_write_byte(gdev.client, data);
}

static int i2c_lcd_write_byte(u8 data)
{
    return i2c_lcd_write_byte_raw(with_bl(data));
}

static int i2c_lcd_pulse(u8 data)
{
    int ret;

    /* EN=1 */
    ret = i2c_lcd_write_byte(data | EN_BIT);
    if (ret < 0) return ret;
    tiny_udelay(slow_mode ? 5 : 1);

    /* EN=0 */
    ret = i2c_lcd_write_byte(data & ~EN_BIT);
    if (ret < 0) return ret;
    tiny_udelay(50);

    return 0;
}

/* Layout B: D4..D7 -> P4..P7 (상위 니블) */
static int i2c_lcd_send_nibble(u8 nibble, u8 rs)
{
    u8 data = (nibble & 0x0F) << 4;  /* P4..P7 <- D4..D7 */
    if (rs) data |= RS_BIT;          /* data: RS=1, command: RS=0 */
    /* RW는 항상 0 */
    return i2c_lcd_pulse(data);
}

static int i2c_lcd_send(u8 val, u8 rs)
{
    int ret;

    ret = i2c_lcd_send_nibble((val >> 4) & 0x0F, rs);
    if (ret < 0) return ret;

    ret = i2c_lcd_send_nibble(val & 0x0F, rs);
    if (ret < 0) return ret;

    if (!rs) {
        if (val == 0x01 || val == 0x02) mdelay(2);   /* clear/home */
        else                             tiny_udelay(50);
    } else {
        tiny_udelay(50);
    }
    return 0;
}

static int lcd_cmd(u8 cmd)  { return i2c_lcd_send(cmd, 0); }
static int lcd_data(u8 dat) { return i2c_lcd_send(dat, 1); }

static int lcd_set_cursor(int row, int col)
{
    static const u8 row_addr[] = { 0x00, 0x40, 0x14, 0x54 };

    if (row < 0) row = 0;
    if (row >= rows) row = rows - 1;
    if (col < 0) col = 0;
    if (col >= cols) col = cols - 1;

    gdev.cur_row = row;
    gdev.cur_col = col;

    return lcd_cmd(0x80 | (row_addr[row] + col));
}

static int lcd_clear(void)
{
    int ret = lcd_cmd(0x01);
    if (ret < 0) return ret;
    gdev.cur_row = 0;
    gdev.cur_col = 0;
    mdelay(2);
    return 0;
}

static int lcd_home(void)
{
    int ret = lcd_cmd(0x02);
    if (ret < 0) return ret;
    gdev.cur_row = 0;
    gdev.cur_col = 0;
    mdelay(2);
    return 0;
}

static int lcd_puts(const char *s, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n') {
            int nr = gdev.cur_row + 1;
            if (nr >= rows) nr = 0;
            lcd_set_cursor(nr, 0);
            continue;
        }
        lcd_data((u8)c);
        gdev.cur_col++;
        if (gdev.cur_col >= cols) {
            gdev.cur_col = 0;
            gdev.cur_row++;
            if (gdev.cur_row >= rows) gdev.cur_row = 0;
            lcd_set_cursor(gdev.cur_row, gdev.cur_col);
        }
    }
    return 0;
}

/* ----- file ops ----- */
static int i2c_lcd_open   (struct inode *inode, struct file *filp){ return 0; }
static int i2c_lcd_release(struct inode *inode, struct file *filp){ return 0; }

static ssize_t i2c_lcd_write(struct file *filp,
                             const char __user *buf,
                             size_t len,
                             loff_t *ppos)
{
    char kbuf[256];
    size_t n = (len > sizeof(kbuf) - 1) ? (sizeof(kbuf) - 1) : len;
    if (copy_from_user(kbuf, buf, n)) return -EFAULT;
    kbuf[n] = '\0';
    lcd_puts(kbuf, n);
    return len;
}

static long i2c_lcd_ioctl(struct file *filp,
                          unsigned int cmd,
                          unsigned long arg)
{
    switch (cmd) {
    case I2C_LCD_CLEAR:  return lcd_clear();
    case I2C_LCD_HOME:   return lcd_home();
    case I2C_LCD_SETPOS: {
        int packed = (int)arg;
        int row    = (packed >> 16) & 0xFFFF;
        int col    = (packed      ) & 0xFFFF;
        return lcd_set_cursor(row, col);
    }
    default: return -ENOTTY;
    }
}

static const struct file_operations i2c_lcd_fops = {
    .owner          = THIS_MODULE,
    .open           = i2c_lcd_open,
    .release        = i2c_lcd_release,
    .write          = i2c_lcd_write,
    .unlocked_ioctl = i2c_lcd_ioctl,
};

/* ----- LCD init sequence ----- */
static int lcd_init_sequence(void)
{
    int ret;

    gdev.bl_on = 1;

    /* 데이터/EN 토글 없이 BL만 먼저 래치해서 항상 켜진 상태 보장 */
    i2c_lcd_write_byte_raw(BL_BIT);
    mdelay(10);

    mdelay(50);

    /* 4-bit init */
    i2c_lcd_send_nibble(0x03, 0);
    mdelay(5);
    i2c_lcd_send_nibble(0x03, 0);
    tiny_udelay(150);
    i2c_lcd_send_nibble(0x03, 0);
    tiny_udelay(150);
    i2c_lcd_send_nibble(0x02, 0);
    tiny_udelay(50);

    /* function set: 4-bit, 2-line, 5x8 */
    ret = lcd_cmd(0x28); if (ret < 0) return -EIO;
    /* display off */
    ret = lcd_cmd(0x08); if (ret < 0) return -EIO;
    /* clear */
    ret = lcd_clear();   if (ret < 0) return -EIO;
    /* entry mode: I/D=1, S=0 */
    ret = lcd_cmd(0x06); if (ret < 0) return -EIO;
    /* display on, cursor off, blink off */
    ret = lcd_cmd(0x0C); if (ret < 0) return -EIO;

    return lcd_set_cursor(0, 0);
}

/* ----- module init/exit ----- */
static int __init i2c_lcd_init(void)
{
    int r;
    dev_t dev = MKDEV(I2C_LCD_MAJOR, 0);

    r = register_chrdev_region(dev, 1, I2C_LCD_NAME);
    if (r < 0) return r;

    cdev_init(&gdev.cdev, &i2c_lcd_fops);
    gdev.cdev.owner = THIS_MODULE;

    r = cdev_add(&gdev.cdev, dev, 1);
    if (r < 0) { unregister_chrdev_region(dev, 1); return r; }

    gdev.cls = CLASS_CREATE(I2C_LCD_NAME);
    if (IS_ERR(gdev.cls)) {
        r = PTR_ERR(gdev.cls);
        cdev_del(&gdev.cdev);
        unregister_chrdev_region(dev, 1);
        return r;
    }

    gdev.dev = device_create(gdev.cls, NULL, dev, NULL, I2C_LCD_NAME);
    if (IS_ERR(gdev.dev)) {
        r = PTR_ERR(gdev.dev);
        class_destroy(gdev.cls);
        cdev_del(&gdev.cdev);
        unregister_chrdev_region(dev, 1);
        return r;
    }

    /* I2C client */
    {
        struct i2c_adapter   *adap = i2c_get_adapter(bus);
        struct i2c_board_info info = { I2C_BOARD_INFO("pcf8574-lcd", addr) };

        if (!adap) { r = -ENODEV; goto err_device; }

        gdev.client = i2c_new_client_device(adap, &info);
        i2c_put_adapter(adap);

        if (IS_ERR(gdev.client)) {
            r = PTR_ERR(gdev.client);
            goto err_device;
        }
    }

    r = lcd_init_sequence();
    if (r < 0) {
        pr_err("i2c_lcd: init sequence failed %d\n", r);
        i2c_unregister_device(gdev.client);
        goto err_device;
    }

    pr_info("i2c_lcd: ready (bus=%d addr=0x%02x %dx%d slow=%d)\n",
            bus, addr, cols, rows, slow_mode);
    return 0;

err_device:
    device_destroy(gdev.cls, dev);
    class_destroy(gdev.cls);
    cdev_del(&gdev.cdev);
    unregister_chrdev_region(dev, 1);
    return r;
}

static void __exit i2c_lcd_exit(void)
{
    dev_t dev = MKDEV(I2C_LCD_MAJOR, 0);

    if (gdev.client) i2c_unregister_device(gdev.client);
    device_destroy(gdev.cls, dev);
    class_destroy(gdev.cls);
    cdev_del(&gdev.cdev);
    unregister_chrdev_region(dev, 1);

    pr_info("i2c_lcd: removed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("camp sulfur");
MODULE_DESCRIPTION("PCF8574 I2C LCD char driver (/dev/i2c_lcd) - Layout B + BL lock");
module_init(i2c_lcd_init);
module_exit(i2c_lcd_exit);
