#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include<linux/gpio.h>


#define DEVICE_NAME "three_gpio"

#define GPIO_CON 0x56000050
#define GPIO_DAT 0x56000054

unsigned long gpio_config;
unsigned long gpio_data;

//static unsigned long led_table [] = {
//	S3C2410_GPF(0),
//	S3C2410_GPF(2),
//	S3C2410_GPF(4),
	//S3C2410_GPB(8),
//};

//static unsigned int led_cfg_table [] = {
//	S3C2410_GPB5_OUTPUT,
//	S3C2410_GPB6_OUTPUT,
//	S3C2410_GPB7_OUTPUT,
//	S3C2410_GPB8_OUTPUt,
//};

int gpio_open(struct inode*inode,struct file *filp)
{
	unsigned long tmp;
	gpio_config = (unsigned long)ioremap(GPIO_CON,0x8);
	gpio_data =gpio_config+0x4;
	tmp = readl(gpio_config);
	tmp = (tmp&~(0x0333U))|(0x0111U);
	writel(tmp,gpio_config);
	tmp = readl(gpio_data);
	tmp = tmp|0x15;
	writel(tmp,gpio_data);
	tmp = readl(gpio_data);
	return 0;	
}


static int sbc2440_leds_ioctl(
	struct inode *inode, 
	struct file *file, 
	unsigned int cmd, 
	unsigned long arg)
{
	unsigned long tmp;
	switch(cmd) {
	case 0:
		//tmp = readl(gpio_data);
		//tmp =(tmp&0xea)|;
		//writel(tmp,gpio_data);
		//break;
	case 1:	
		tmp = readl(gpio_data);
		tmp =(tmp&0xea)|(arg&0x15);
		writel(tmp,gpio_data);
		break;
	//s3c2410_gpio_setpin(led_table[arg], cmd);
	default:
		return -EINVAL;
	}
}

static struct file_operations dev_fops = {
	.owner	=	THIS_MODULE,
	.ioctl	=	sbc2440_leds_ioctl,
	.open   = 	gpio_open,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init dev_init(void)
{
	int ret;

	//s3c2410_gpio_cfgpin(led_table[0], S3C2410_GPIO_OUTPUT);
	//s3c2410_gpio_setpin(led_table[0], 0);
	

	ret = misc_register(&misc);

	printk (DEVICE_NAME"\tinitialized\n");

	return ret;
}

static void __exit dev_exit(void)
{
	iounmap((void *)gpio_config);
	misc_deregister(&misc);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");
