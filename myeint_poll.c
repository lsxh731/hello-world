#include <linux/input.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/input/mt.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/fs.h> 
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <mach/gpio-exynos4.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/mutex.h>

#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/compat.h>

#define DRIVER_NAME "myeint1"

#define AD7606_BUSY_PIN	   EXYNOS4_GPX0(1)

struct fasync_struct *pwm1_fasync;//定义fasync_struct结构
int pt_count; 
struct mutex irq_lock;
static DECLARE_WAIT_QUEUE_HEAD(eint1_waitq);
unsigned int readable;
 

static irqreturn_t eint1_interrupt(int irq,void *dev_id)
{
	mutex_lock(&irq_lock);
	pt_count++;
	readable = 1;
	if(pt_count > 40000)
	{
	//	printk("#####receive a interrupt 1!#####\n");
		pt_count = 0;
	}
	kill_fasync(&pwm1_fasync, SIGIO, POLL_IN);
	mutex_unlock(&irq_lock);
	return IRQ_RETVAL(IRQ_HANDLED);
}

static int myeint1_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
    poll_wait(file, &eint1_waitq, wait); 
    if (readable)
	{
		readable = 0;
		mask |= POLLIN | POLLRDNORM;
	}
    return mask;
}

static int myeint1_fasync(int fd, struct file * file, int on)
{
    int err;
 //   printk("fansync_helper\n");
    err = fasync_helper(fd, file, on, &pwm1_fasync);  //利用fasync_helper初始化pwm1_fasync
    if (err < 0)
        return err;
    return 0;
}


ssize_t myeint1_write(struct file *f, const char __user *buf, size_t t, loff_t *len){
	

}
static const struct file_operations myeint1_fops =
{
	.owner   	=   THIS_MODULE,
	.write		=   myeint1_write,
	.fasync     =   myeint1_fasync,
	.poll		=   myeint1_poll,
};
 
struct miscdevice myeint1_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "myeint1",
	.fops = &myeint1_fops,
};

static int myeint1_probe(struct platform_device *pdev)
{
	
	
	int ret,irq;
	pt_count = 0;
	readable = 0;
	mutex_init(&irq_lock);
	ret = gpio_request(EXYNOS4_GPX0(1), "EINT1");
    if (ret) {
            printk("request GPIO %d for EINT1 failed, ret = %d\n",EXYNOS4_GPX0(1), ret);
            return ret;
    }
	s3c_gpio_cfgpin(EXYNOS4_GPX0(1), S3C_GPIO_INPUT);
    s3c_gpio_setpull(EXYNOS4_GPX0(1), S3C_GPIO_PULL_NONE);
	gpio_free(EXYNOS4_GPX0(1));    

	irq = gpio_to_irq(EXYNOS4_GPX0(1));
	ret = misc_register(&myeint1_dev);
	
	request_irq(irq,eint1_interrupt,IRQ_TYPE_EDGE_FALLING,"my_eint1",pdev);
	return 0;
	
}

static int myeint1_remove (struct platform_device *pdev)
{
	printk("myeint1_ctl remove:power off!\n");
	misc_deregister(&myeint1_dev);
	free_irq(IRQ_EINT(1),pdev);
	return 0;
}

static int myeint1_suspend (struct platform_device *pdev, pm_message_t state)
{
	printk("myeint1_ctl suspend:power off!\n");
	return 0;
}

static int myeint1_resume (struct platform_device *pdev)
{
	printk("myeint1_ctl resume:power on!\n");

	return 0;
}

static struct platform_driver myeint1_driver = {
	.probe = myeint1_probe,
	.remove = myeint1_remove,
	.suspend = myeint1_suspend,
	.resume = myeint1_resume,
	.driver = {
		.name = "myeint1",
		.owner = THIS_MODULE,
	},
};

static int __init myeint1_init(void) {
	return platform_driver_register(&myeint1_driver);
}

static void __exit myeint1_exit(void) {
	platform_driver_unregister(&myeint1_driver);
} 
 
 
 
module_init(myeint1_init);  
module_exit(myeint1_exit);
MODULE_AUTHOR("hrtimer<hrtimer@hrtimer.cc>");
MODULE_LICENSE("GPL");
