
#define DRIVER_NAME "buzzer"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)
#include<linux/cdev.h>
#include<linux/circ_buf.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/init.h>
#include<linux/interrupt.h>
#include<linux/kdev_t.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<linux/semaphore.h>
#include<linux/slab.h>
#include<linux/types.h>
#include<linux/uaccess.h>
#include<linux/wait.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/string.h>
