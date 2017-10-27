/*
===============================================================================
Driver Name		:		buzzer
Author			:		THINHNGUYEN
License			:		GPL
Description		:		LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

#include "buzzer_chr.h"

#include "buzzer_dev.h"

#define BUZZER_N_MINORS 1
#define BUZZER_FIRST_MINOR 0
#define BUZZER_BUFF_SIZE 1024
#define LOCK 0
#define UNLOCK 1
#define BUZZER_READ_BUFF_SIZE 1024
#define BUZZER_WRITE_BUFF_SIZE 1024

#define GPIO_PIN				203

MODULE_LICENSE("GPL");
MODULE_AUTHOR("THINHNGUYEN");

int buzzer_major=0;

dev_t buzzer_device_num;

typedef struct privatedata {
	int nMinor;

	struct cdev cdev;
	struct mutex lock;
	struct buzzer buzzerdev;
	struct task_struct *task;

} buzzer_private;

buzzer_private devices[BUZZER_N_MINORS];

int buzzer_thread(void *data)
{
	struct task_struct *task;
	struct buzzer *dev = (struct buzzer*) data;
	struct sched_param PARAM = { .sched_priority = MAX_RT_PRIO };
	unsigned int freq = dev->freq;
	unsigned int period = 0;
	static int toggle = 0;
	task = current;
	PARAM.sched_priority = MAX_RT_PRIO / 2;
	sched_setscheduler(task, SCHED_FIFO, &PARAM);

	while(true)
	{
		if (kthread_should_stop())
			break;
		toggle = (toggle + 1)%2;
		freq = dev->freq;
		period = 1000000 / freq;
		if (toggle && dev->repeat)
			buzzerdev_on(dev);
		else if (!toggle && dev->repeat)
			buzzerdev_off(dev);
		usleep_range(period, period);
	}
	return 0;
}

static int buzzer_open(struct inode *inode,struct file *filp)
{
	char name[3];
	buzzer_private *priv = container_of(inode->i_cdev ,
									buzzer_private ,cdev);
	filp->private_data = priv;
	PINFO("Initializing kernel thread...");
	memset(name, 0, sizeof(name));
	sprintf("%d", name, &priv->nMinor);
	priv->task = kthread_run(buzzer_thread, (void *) &priv->buzzerdev, name);

	PINFO("In char driver open() function\n");

	return 0;
}					

static int buzzer_release(struct inode *inode,struct file *filp)
{
	buzzer_private *priv;

	priv=filp->private_data;
	buzzerdev_off(&priv->buzzerdev);
	PINFO("In char driver release() function\n");
	PINFO("Exiting kernel thread...");
	kthread_stop(priv->task);
	return 0;
}

static long buzzer_ioctl(struct file *filp, unsigned int ioctl_command, unsigned long arg)
{
	buzzer_private *priv;
	priv = filp->private_data;
	unsigned int repeat;
	switch ((char) ioctl_command)
	{
	case IOCTL_START_BUZZER:
		mutex_lock(&priv->lock);
		if (copy_from_user(&repeat, (const void*) arg, sizeof(repeat)))
		{
			PDEBUG("Failed to copy from user buffer");
			return -EFAULT;
		}
		priv->buzzerdev.repeat = repeat;
		mutex_unlock(&priv->lock);
		break;
	case IOCTL_STOP_BUZZER:
		mutex_lock(&priv->lock);
		priv->buzzerdev.repeat = 0;
		mutex_unlock(&priv->lock);
		break;
	default:
		PDEBUG("fuck user");
		return -ENOTTY;
	}
	return 0;
}

static ssize_t buzzer_read(struct file *filp, 
	char __user *ubuff,size_t count,loff_t *offp)
{
	buzzer_private *priv;
	unsigned int freq = 0;
	size_t n;
	priv = filp->private_data;
	freq = priv->buzzerdev.freq;
	n = sizeof(unsigned int);

	n = min(n, count);

	mutex_lock(&priv->lock);

	if (copy_to_user(ubuff, &freq, n))
	{
		mutex_unlock(&priv->lock);
		return -EFAULT;
	}
	mutex_unlock(&priv->lock);
	PINFO("In char driver read() function\n");
	return n;
}
static ssize_t buzzer_write(struct file *filp, 
	const char __user *ubuff, size_t count, loff_t *offp)
{
	size_t n=0;
	unsigned int freq;
	buzzer_private *priv;

	priv = filp->private_data;

	n = sizeof(unsigned char);
	n = min(n, count);
	mutex_lock(&priv->lock);
	if (copy_from_user(&freq, ubuff, n))
	{
		mutex_unlock(&priv->lock);
		return -EFAULT;
	}
	priv->buzzerdev.freq = freq;
	mutex_unlock(&priv->lock);
	PINFO("In char driver write() function\n");

	return n;
}

static const struct file_operations buzzer_fops= {
	.owner				= THIS_MODULE,
	.open				= buzzer_open,
	.release			= buzzer_release,
	.read				= buzzer_read,
	.write				= buzzer_write,
	.unlocked_ioctl		= buzzer_ioctl,
};

static int __init buzzer_init(void)
{
	int i;
	int res;

	res = alloc_chrdev_region(&buzzer_device_num,BUZZER_FIRST_MINOR,BUZZER_N_MINORS ,DRIVER_NAME);
	if(res) {
		PERR("register device no failed\n");
		return -1;
	}
	buzzer_major = MAJOR(buzzer_device_num);

	for(i=0;i<BUZZER_N_MINORS;i++) {
		buzzer_device_num= MKDEV(buzzer_major ,BUZZER_FIRST_MINOR+i);
		cdev_init(&devices[i].cdev , &buzzer_fops);
		cdev_add(&devices[i].cdev,buzzer_device_num,1);

		devices[i].nMinor = BUZZER_FIRST_MINOR+i;
		devices[i].buzzerdev.gpio_pin = GPIO_PIN; //Will get in dev tree van gan gia tri tai day
		devices[i].buzzerdev.repeat = 0;
		buzzerdev_init(&devices[i].buzzerdev);
		buzzerdev_set_freq(&devices[i].buzzerdev, 2);
	}


	PINFO("INIT\n");

	return 0;
}

static void __exit buzzer_exit(void)
{	
	int i;

	PINFO("EXIT\n");

	for(i=0;i<BUZZER_N_MINORS;i++) {
		buzzer_device_num= MKDEV(buzzer_major ,BUZZER_FIRST_MINOR+i);

		buzzerdev_deinit(&devices[i].buzzerdev);

		cdev_del(&devices[i].cdev);

	}

	unregister_chrdev_region(buzzer_device_num ,BUZZER_N_MINORS);	

}

module_init(buzzer_init);
module_exit(buzzer_exit);

