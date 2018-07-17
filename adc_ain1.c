#include<linux/kernel.h> 
#include<linux/module.h> 
#include<linux/init.h> 
#include<linux/wait.h>
#include<linux/interrupt.h> 
#include<linux/fs.h> 
#include<linux/clk.h> 
#include<linux/miscdevice.h>
#include<asm/io.h> 
#include<asm/irq.h> 
//#include<asm/arch/regs-adc.h> /* S3C2410_ADCCON */
#include<plat/regs-adc.h>/*S3C2410_ADCCON*/
#include<linux/sched.h>/*TASK_INTERRUPTIBLE*/
#include<asm/uaccess.h> /*copy_to_user*/

#define DEVICE_NAME "myadc"

#define S3C2410_ADCDAT2 
 
static struct clk *adc_clock;
 
static void __iomem *base_addr;
 

static wait_queue_head_t adc_waitqueue;
 
DECLARE_MUTEX(adc_lock);
EXPORT_SYMBOL(adc_lock);
 

static volatile int is_read_ok = 0;
 
static volatile int adc_data;
 
static int adc_open(struct inode *inode, struct file *file);
static ssize_t adc_read(struct file *filp, char *buffer, size_t count, loff_t *ppos);
static int adc_close(struct inode *inode, struct file *filp);
 

static struct file_operations adc_fops =
{
    .owner   = THIS_MODULE,
    .open    = adc_open,
    .read    = adc_read,   
    .release = adc_close,
};
 

static struct miscdevice adc_miscdev =
{
    .minor  = MISC_DYNAMIC_MINOR, 
    .name   = DEVICE_NAME,        
    .fops   = &adc_fops,         
};
 

static irqreturn_t adc_irq(int irq, void *dev_id)
{
    
    if(!is_read_ok)
    {
       
       adc_data = readl(base_addr +S3C2410_ADCDAT0) & 0x3ff;//
       
        is_read_ok = 1;
       wake_up_interruptible(&adc_waitqueue);
    }
 
    return IRQ_RETVAL(IRQ_HANDLED);
}
 
/*ADC¿¿¿¿¿¿¿¿IRQ_ADC¿¿¿¿¿¿*/
static int adc_open(struct inode *inode, struct file *file)
{
    int ret;
 
    /* ¿¿IRQ_ADC¿¿¿¿¿¿¿¿¿¿¿¿¿¿IRQF_SHARED¿¿¿¿¿¿¿¿¿¿¿NULL¿¿¿¿*/
    ret = request_irq(IRQ_ADC, adc_irq,IRQF_SHARED, DEVICE_NAME, (void *)1);
    if (ret)
    {
        printk(KERN_ERR "Could notallocate ts IRQ_ADC !\n");
        return -EBUSY;
    }
 
    return 0;
}
 
/*¿¿ADC¿¿¿¿¿¿¿¿AD¿¿*/
static void adc_run(void)
{
    volatile unsigned int adccon;
   
    /**/
    adccon = (1 << 14) | (20<< 6)|(1<<3);
    //adccon = (1 << 14) | (32 << 6)|(1<<3);
    //adccon = (1 << 14) | (32 << 6)|(2<<3);
    //adccon = (1 << 14) | (32 << 6)|(3<<3);
    writel(adccon, base_addr + S3C2410_ADCCON);
   
    /* ¿[0]=1¿¿¿¿ADC¿¿¿¿¿¿¿¿¿¿¿¿ADC¿¿¿¿¿¿0*/
    adccon = readl(base_addr + S3C2410_ADCCON)| (1 << 0);
    writel(adccon, base_addr + S3C2410_ADCCON);
}
 
/**/
static ssize_t adc_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
    int err;
 
    /*
     * 
     */
    down_interruptible(&adc_lock);
 
    /*adc_irq*/
    adc_run();
 
    /**/
    wait_event_interruptible(adc_waitqueue,is_read_ok);
 
    /**/
    is_read_ok = 0;
 
    /*adc_data*/
    err = copy_to_user(buff, (char*)&adc_data, min(sizeof(adc_data),count));
 
    /*adc_lock*/
    up(&adc_lock);
 
    return err ? -EFAULT : sizeof(adc_data);
}
 
/*ADC¿¿¿¿¿¿*/
static int adc_close(struct inode *inode, struct file *filp)
{
    /*¿¿¿¿*/
    free_irq(IRQ_ADC, (void *)1);   
    return 0;
}
 
static int __init adc_init(void)
{
    int ret;
 
    /*arch/arm/mach-s3c2410/clock.c¿¿¿¿¿¿¿¿PCLK */
    adc_clock = clk_get(NULL, "adc");
    if (!adc_clock)
    {
        printk(KERN_ERR "failed to get adcclock source\n");
        return -ENOENT;
    }
 
    /**/
    clk_enable(adc_clock);
 
    /* ioremap
     * S3C2410_PA_ADC=ADCCON
     */
    //base_addr = ioremap(S3C2410_PA_ADC, 0x1c);
    base_addr = ioremap(S3C2410_PA_ADC, 0x20);
    if (base_addr == NULL)
    {
        printk(KERN_ERR "Failed to remapregister block\n");
        return -ENOMEM;
        goto fail1;
    }
   
    /**/
    init_waitqueue_head(&adc_waitqueue);
 
    
    ret = misc_register(&adc_miscdev);
    if (ret)
    {
        printk(KERN_ERR "Failed toregister miscdev\n");
        goto fail2;
    }
 
    printk(DEVICE_NAME "initialized!\n");
 
    return 0;
   
fail2:
    iounmap(base_addr);
fail1:
    clk_disable(adc_clock);
    clk_put(adc_clock);
 
    return ret;
}
 
static void __exit adc_exit(void)
{
    /**/
    iounmap(base_addr);
 
   
    if (adc_clock)            
    {
        clk_disable(adc_clock);
        clk_put(adc_clock);
        adc_clock = NULL;
    }
 
    
    misc_deregister(&adc_miscdev);
}
 
module_init(adc_init);
module_exit(adc_exit);
 
MODULE_AUTHOR("JasonWu");
MODULE_DESCRIPTION("Mini2440ADC Misc Device Driver");
MODULE_VERSION("MINI2440ADC 1.0");
MODULE_LICENSE("GPL");
