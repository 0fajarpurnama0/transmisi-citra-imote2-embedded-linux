/*
 *   driver/tosmac/TOSMac.c
 *
 *   TOS_MAC Driver based on CC2420 radio chip
 *   It is compatible with TinyOS MAC layer.
 *
 *   Author: Gefan Zhang
 *   Last modified by Y.Cheng:11/April/2008 
 *             
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/poll.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/system.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/stargate2.h>
#include "../platx/pmic.h"

#include "SG2_CC2420.h"
#include "TOSMac.h"
#include "CC2420.h"
#include "CC2420Const.h"
#include "AM.h"
#include "byteorder.h"

/* module information */
MODULE_AUTHOR("Gefan Zhang <gefan@soe.ucsc.edu>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("TOS_MAC Driver based on CC2420");
MODULE_VERSION ("0.2");

//static int devno;
static int devno_result;
static int cdev_result;

static int tosmac_major = TOSMAC_MAJOR;
static int tosmac_minor = 0;
u16 local_addr = 99;
u16 TOS_LOCAL_ADDRESS = 99;

module_param(tosmac_major, int, S_IRUGO);
module_param(tosmac_minor, int, S_IRUGO);
module_param(local_addr, ushort, S_IRUGO);


struct tosmac_device *tosmac_dev;

//size of payload data
int TOSH_DATA_LENGTH;
// size of the full packet
int MSG_DATA_SIZE;
// size of header(including length byte) and payload data
int MSG_HEADER_DATA_SIZE;
// size of the whole TOSMSG
int TOSMSG_SIZE;


static ssize_t tosmac_read (struct file *file, char __user *buffer,
                            size_t length, loff_t *ppos);
static ssize_t tosmac_write (struct file *file, const char __user *buffer,
                             size_t length, loff_t *ppos);
static unsigned int tosmac_poll (struct file *file, struct poll_table_struct *wait);
static int tosmac_ioctl (struct inode *inode, struct file * file,
                         unsigned int cmd, unsigned long arg);
static int tosmac_open (struct inode *inode, struct file *file);
static int tosmac_release (struct inode *inode, struct file *file);

static struct file_operations tosmac_fops = {
        .owner          THIS_MODULE,
        .read           tosmac_read,
        .write          tosmac_write,
	.poll		tosmac_poll,
        .ioctl          tosmac_ioctl,
        .open           tosmac_open,
        .release        tosmac_release,
};

static int tosmac_open (struct inode *inode, struct file *filp)
{
    uint8_t ret;
    struct tosmac_device *dev;
    uint8_t is_non_blocking;

    dev = container_of(inode->i_cdev, struct tosmac_device, cdev);
    filp->private_data = dev;
    
    if(filp->f_flags & O_NONBLOCK)
	is_non_blocking = TRUE;
    else
	is_non_blocking = FALSE;
    
    down(&dev->sem);
    CC2420_start(is_non_blocking);    
    up(&dev->sem);

    return 0;
}

static unsigned int tosmac_poll (struct file *filp, struct poll_table_struct *wait)
{
    struct tosmac_device *dev = filp->private_data;
    unsigned int mask;
    
    mask = CC2420_poll(filp, wait);
    
    return mask;
}

static ssize_t tosmac_read (struct file *filp, char __user *buffer,
                            size_t count, loff_t *ppos)
{
    struct tosmac_device *dev = filp->private_data;
    int ret = 0;
    int i;
    
    if(CC2420_read( )){
	if(rx_packet.length != 255) {
	        count = ((count+TOSMSG_SIZE - TOSH_DATA_LENGTH)  < TOSMSG_SIZE) ? (count+(TOSMSG_SIZE - TOSH_DATA_LENGTH)) : TOSMSG_SIZE;//YC

//                printk("count in tosmac: %d, TOSMSG: %d\n",count,TOSMSG_SIZE);
         	if(copy_to_user(buffer, &rx_packet, count)) {
			ret = -EFAULT;
			goto out;
	 	}
         	ret = count;
	} else 
		ret = -EFAULT;
    }
    else {
//		printk(KERN_INFO "non_blocking read, no data in rxfifo\n");
		ret = -EAGAIN;
     	}
out:
    return ret;
}

static ssize_t tosmac_write (struct file *filp, const char __user *buffer,
                             size_t count, loff_t *ppos)
{
    struct tosmac_device *dev = filp->private_data;
    int i, ret = 0;
    TOS_Msg *packet_buf;
    int buf_length;
    int retval;
//    down(&dev->sem);

    packet_buf = kmalloc(sizeof(TOS_Msg), GFP_KERNEL);
    count = (count > TOSH_DATA_LENGTH) ? TOSH_DATA_LENGTH : count;
    buf_length = MSG_HEADER_SIZE + 1 + count;
    if (copy_from_user(packet_buf, buffer, buf_length)) {
        ret = -EFAULT;
        goto out;
    }   
    //reset length to count
    packet_buf->length = count;
	
    retval = CC2420_send(packet_buf);


    if(retval && (tx_buf_ptr->length != 255))
    	ret = count;
    else
	ret = -EFAULT;


    out:
    kfree(packet_buf);
//    up(&dev->sem);
    return ret;
}

static int tosmac_ioctl (struct inode *inode, struct file * file,
                         unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int err = 0;
    int channel, freq;
    int power;

    //arguments check
    if (_IOC_TYPE(cmd) != TOSMAC_IOCTL_MAGIC) return -ENOTTY;
    if (_IOC_DIR(cmd) & _IOC_READ)
	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
	err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

    // FIXME:  Most of this should be pushed to the lower layer so that other 
    // systems such as PSI can make use of it.  Only the LINUX specific
    // stuff should be here...

    switch(cmd) {

     case TOSMAC_IOSETADDR:
	TOS_LOCAL_ADDRESS = arg;
	CC2420_set_short_addr(TOS_LOCAL_ADDRESS);
	break;
     case TOSMAC_IOGETADDR:
	return TOS_LOCAL_ADDRESS;	
	break;
     case TOSMAC_IOAUTOACK:
	CC2420_enable_ack();
	break;
     case TOSMAC_IODISAUTOACK:
	CC2420_disable_ack();
	break;
     case TOSMAC_IOSETCHAN: 
	channel = arg;
	if(channel < 11) channel = 11;
	if(channel > 26) channel = 26;
	CC2420_tune_preset(channel);
	break;       
     case TOSMAC_IOGETCHAN:
	return CC2420_get_preset();
	break;
     case TOSMAC_IOSETFREQ: 
	freq = arg;
	if(freq < 2405) freq = 2405;
	if(freq > 2480) freq = 2480;
	CC2420_tune_manual(freq);
	break;       
     case TOSMAC_IOGETFREQ:
	return CC2420_get_frequency();
	break;
     case TOSMAC_IOSETMAXDATASIZE:
	TOSH_DATA_LENGTH = arg;
    	MSG_DATA_SIZE = 10 + TOSH_DATA_LENGTH + sizeof(uint16_t);
    	MSG_HEADER_DATA_SIZE = 10 + TOSH_DATA_LENGTH;
    	TOSMSG_SIZE = MSG_HEADER_DATA_SIZE + TOSMSG_FOOTER_SIZE;
	break;
     case TOSMAC_IOGETMAXDATASIZE:
	return TOSH_DATA_LENGTH;
	break;
     case TOSMAC_IOSETPOWER:
	power = arg;
	if (power > 31) power = 31;
	if (power < 3 ) power = 3;
	CC2420_set_rf_power((uint8_t)power);	
	break;
     case TOSMAC_IOGETPOWER:
	return CC2420_get_rf_power();
	break;
     default:
	ret = -ENOTTY;
    }

    return ret;
}

static int tosmac_release (struct inode *inode, struct file *filp)
{
    struct tosmac_device *dev;

    dev = container_of(inode->i_cdev, struct tosmac_device, cdev);
    filp->private_data = dev;
//    printk(KERN_INFO "close tosmac\n");
    CC2420_stop(dev);
    return 0;
}

static void __exit tosmac_exit(void)
{
   printk(KERN_INFO "TOSMAC driver is unloading...\n");
   if(tosmac_dev != NULL) {
	/*power off radio */
	CC2420_exit(tosmac_dev);
	if(!cdev_result)
            cdev_del(&(tosmac_dev->cdev));
	if(!devno_result)
            unregister_chrdev_region(tosmac_dev->devno, 1);
	kfree(tosmac_dev);
   }
   printk(KERN_INFO "Unloading is done.\n");
}

static int __init tosmac_init(void)
{
    int ret = 0;
    int devno;
    int devno_ret = -1;
    int cdev_ret = -1;

    TOSH_DATA_LENGTH = 28;
    MSG_DATA_SIZE = 10 + TOSH_DATA_LENGTH + sizeof(uint16_t);
    MSG_HEADER_DATA_SIZE = 10 + TOSH_DATA_LENGTH;
    TOSMSG_SIZE = MSG_HEADER_DATA_SIZE + TOSMSG_FOOTER_SIZE; 
    printk(KERN_INFO "TOSMAC driver is loading...\n");
   
    tosmac_dev = kmalloc(sizeof(struct tosmac_device), GFP_KERNEL);
    if (!tosmac_dev) {
	    printk(KERN_INFO "kmalloc error, failed to load driver\n");
            ret = -ENOMEM;
            goto fail;
    }
    memset(tosmac_dev, 0, sizeof(struct tosmac_device));
    init_MUTEX(&tosmac_dev->sem);

    devno = MKDEV(tosmac_major, tosmac_minor);
    devno_result = register_chrdev_region(devno, 1, "tosmac");
    if (devno_result < 0) {
        printk(KERN_ERR "Failed to get driver major num:%d\n", tosmac_major);
        goto fail;
    }
    tosmac_dev->devno = devno;
    init_waitqueue_head(&(tosmac_dev->rq));
    /* turn on radio */
    if(CC2420_init(tosmac_dev) == FAIL)
	goto fail;
    
    cdev_init(&tosmac_dev->cdev, &tosmac_fops);
    tosmac_dev->cdev.owner = THIS_MODULE;
    tosmac_dev->cdev.ops = &tosmac_fops;
    cdev_result = cdev_add(&tosmac_dev->cdev, devno, 1);
    if(cdev_result) {
        printk(KERN_ERR "Failed to register TOS-MAC driver\n");
        goto fail;
    }
    else
        printk(KERN_INFO "TOS-MAC driver installed\n");

   return ret;
   fail:
        tosmac_exit();
        return ret;
}

module_init(tosmac_init);
module_exit(tosmac_exit);
