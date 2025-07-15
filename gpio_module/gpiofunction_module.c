#include <linux/fs.h>           /* open(), read(), write(), close() 커널 함수 */
#include <linux/cdev.h>         /* 문자 디바이스 */
#include <linux/module.h>
#include <linux/io.h>           /* ioremap(), iounmap() 커널 함수 */
#include <linux/uaccess.h>      /* copy_to_user(), copy_from_user() 커널 함수 */
#include <linux/gpio.h>

MODULE_LICENSE("GPL");

#define GPIO_LED 18

static char msg[BLOCK_SIZE] = {0};    /* write() 함수에서 읽은 데이터 저장 */

static int gpio_open(struct inode *, struct file *);
/* ~ 중간 표시 생략 ~ */

struct cdev gpio_cdev;

int init_module(void)
{
    dev_t devno;
    unsigned int count;
    //static void *map;             /* 함수 내에서 사용하지 않으므로 삭제 */
    int err;

    /* ~ 중간 표시 생략 ~ */

    printk("'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
    printk("'chmod 666 /dev/%s'\n", GPIO_DEVICE);

    /* GPIO 사용을 요청한다. */
    gpio_request(GPIO_LED, "LED");
    gpio_direction_output(GPIO_LED, 0);

    return 0;
}

void cleanup_module(void)
{
    dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);

    unregister_chrdev_region(devno, 1);

    cdev_del(&gpio_cdev);

    /* 더 이상 사용이 필요 없는 경우 관련 자원을 해제한다. */
    gpio_free(GPIO_LED);
    gpio_direction_output(GPIO_LED, 0);

    module_put(THIS_MODULE);

    printk(KERN_INFO "Good-bye module!\n");
}

/* ~ 중간 표시 생략 ~ */

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    short count;
    memset(msg, 0, BLOCK_SIZE);
    count = copy_from_user(msg, buff, len);

    /* LED를 설정한다. */
    gpio_set_value(GPIO_LED, (!strcmp(msg, "0")) ? 0 : 1);

    printk("GPIO Device(%d) write : %s(%d)\n",
           MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, len);

    return count;
}
