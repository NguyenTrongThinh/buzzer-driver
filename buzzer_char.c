/*
===============================================================================
Driver Name		:		buzzer
Author			:		THINHNGUYEN
License			:		GPL
Description		:		LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

#include "buzzer_char.h"

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
MODULE_VERSION("1.0.0");

int buzzer_major=0;

dev_t buzzer_device_num;

typedef struct privatedata {
	int nMinor;

	struct cdev cdev;
	struct device *buzzer_device;
	struct mutex lock;
	struct buzzer buzzerdev;
	wait_queue_head_t buzzer_wait_queue;
	int condition;

} buzzer_private;

buzzer_private *devices;
struct task_struct **task;
unsigned int numofdev = 0;
struct class *buzzer_class;



static int do_Work(void *param)
{
	pid_t PID = current->pid;
	buzzer_private *device = (buzzer_private*)param;
	unsigned int period;
	int repeat;
	while(true)
	{
		wait_event_interruptible(device->buzzer_wait_queue, device->condition == 1);
		if (kthread_should_stop())
			break;
		mutex_lock(&device->lock);
		device->condition = 0;
		period = 1000000/device->buzzerdev.freq;
		repeat = (int)device->buzzerdev.repeat;
		mutex_unlock(&device->lock);
		PDEBUG("period: %d, repeat: %d", period, repeat);
		while (repeat-- > 0)
		{
			buzzerdev_on(&device->buzzerdev);
			usleep_range(period, period);
			buzzerdev_off(&device->buzzerdev);
			usleep_range(period, period);
			PDEBUG("repeat %d", repeat);
		}
	}
	PINFO("Kernel thread stop, PID %d", PID);
	do_exit(0);
	return 0;
}

static int buzzer_open(struct inode *inode,struct file *filp)
{
	/* TODO Auto-generated Function */

	buzzer_private *priv = container_of(inode->i_cdev ,
									buzzer_private ,cdev);
	filp->private_data = priv;

	PINFO("In char driver open() function\n");

	return 0;
}					

static int buzzer_release(struct inode *inode,struct file *filp)
{
	/* TODO Auto-generated Function */

	buzzer_private *priv;

	priv=filp->private_data;

	PINFO("In char driver release() function\n");

	return 0;
}

static ssize_t buzzer_read(struct file *filp, 
	char __user *ubuff,size_t count,loff_t *offp)
{
	/* TODO Auto-generated Function */

	ssize_t n=count;

	return n;
}

static ssize_t buzzer_write(struct file *filp, 
	const char __user *ubuff, size_t count, loff_t *offp)
{
	ssize_t n  = count;

	return n;
}
static long buzzer_ioctl(struct file *filp, unsigned int ioctl_command, unsigned long arg)
{
	unsigned int freq = 0;
	unsigned int repeat = 0;
	buzzer_private *buzzer;
	buzzer = filp->private_data;

	switch ((char) ioctl_command)
	{
	case IOCTL_SET_FREQ:
		if (copy_from_user(&freq, (const void*) arg, sizeof(unsigned int)))
		{
			PDEBUG("in ioctl: Cannot copy from user");
			return -EFAULT;
		}

		mutex_lock(&buzzer->lock);
		buzzer->buzzerdev.freq = freq;
		mutex_unlock(&buzzer->lock);
		PDEBUG("Freq: %d", freq);
		break;
	case IOCTL_SET_REPEAT:
		if (copy_from_user(&repeat, (const void*) arg, sizeof(unsigned int)))
		{
			PDEBUG("in ioctl: Cannot copy from user");
			return -EFAULT;
		}
		mutex_lock(&buzzer->lock);
		buzzer->buzzerdev.repeat = repeat;
		mutex_unlock(&buzzer->lock);
		PDEBUG("Repeat: %d", repeat);
		break;
	default:
		buzzer->condition = 1;
		wake_up_interruptible(&buzzer->buzzer_wait_queue);
		break;
	}
	return 0;
}
static ssize_t freq_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int freq;
	buzzer_private *buzzer = dev_get_drvdata(dev);
	sscanf(buf, "%d", &freq);
	mutex_lock(&buzzer->lock);
	buzzer->buzzerdev.freq = freq;
	mutex_unlock(&buzzer->lock);
	return count;
}
static ssize_t freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	buzzer_private *buzzer = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%d\n", buzzer->buzzerdev.freq);
}
static DEVICE_ATTR_RW(freq);

static ssize_t repeat_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int repeat;
	buzzer_private *buzzer = dev_get_drvdata(dev);
	sscanf(buf, "%d", &repeat);
	mutex_lock(&buzzer->lock);
	buzzer->buzzerdev.repeat = repeat;
	mutex_unlock(&buzzer->lock);
	buzzer->condition = 1;
	wake_up_interruptible(&buzzer->buzzer_wait_queue);
	return count;
}

static ssize_t repeat_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	buzzer_private *buzzer = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%d\n", buzzer->buzzerdev.repeat);
}
static DEVICE_ATTR_RW(repeat);

static struct attribute *buzzer_device_attrs[] = {
	&dev_attr_freq.attr,
	&dev_attr_repeat.attr,
	NULL
};
ATTRIBUTE_GROUPS(buzzer_device);

static const struct file_operations buzzer_fops= {
	.owner				= THIS_MODULE,
	.open				= buzzer_open,
	.release			= buzzer_release,
	.read				= buzzer_read,
	.write				= buzzer_write,
	.unlocked_ioctl		= buzzer_ioctl,
};

static int buzzer_probe(struct platform_device *pdev)
{
	int i;
	int res = 0;
	unsigned int defaultfreq = 100, defaultrepeat = 20, activesate = ACTIVE_HIGHT;
	char threadName[100];
	const char *buzzergpios[5];
	const char *activestatus;
	struct device_node *np = pdev->dev.of_node;

	numofdev = of_property_read_string_array(np, "gpios", buzzergpios, 5);
		if (numofdev <= 0)
			return -ENODEV;
	if (of_property_read_u32(np, "freq", &defaultfreq))
		defaultfreq = 100;
	if (of_property_read_u32(np, "repeat", &defaultrepeat))
		defaultrepeat = 2;
	if (of_property_read_string(np, "active_status", &activestatus))
		activesate = ACTIVE_HIGHT;
	else
	{
		if (strcmp(activestatus, "low") == 0)
			activesate = ACTIVE_LOW;
		else
			activesate = ACTIVE_HIGHT;
	}
	res = alloc_chrdev_region(&buzzer_device_num,BUZZER_FIRST_MINOR,numofdev ,DRIVER_NAME);
	if(res) {
		PERR("register device no failed\n");
		return -1;
	}
	buzzer_major = MAJOR(buzzer_device_num);
	buzzer_class = class_create(THIS_MODULE , DRIVER_NAME);
	if(buzzer_class == NULL) {
		PERR("class creation failed\n");
		return -1;
	}
	devices = (buzzer_private*)kcalloc(numofdev, sizeof(buzzer_private), GFP_KERNEL);
	task = (struct task_struct**)kcalloc(numofdev, sizeof(struct task_struct*), GFP_KERNEL);

	for (i = 0; i<numofdev; i++)
	{
		buzzer_device_num= MKDEV(buzzer_major ,BUZZER_FIRST_MINOR+i);
		cdev_init(&devices[i].cdev , &buzzer_fops);
		cdev_add(&devices[i].cdev,buzzer_device_num,1);
		devices[i].buzzer_device = device_create_with_groups(buzzer_class, NULL, buzzer_device_num,
				NULL, buzzer_device_groups, "buzzer%d", MINOR(buzzer_device_num));
		if (IS_ERR(devices[i].buzzer_device))
		{
			PERR("Can't create device %d\n", buzzer_device_num);
		}

		devices[i].nMinor = BUZZER_FIRST_MINOR+i;
		if (kstrtoint(buzzergpios[i], 0, &res) == 0)
			devices[i].buzzerdev.gpio_pin = res;
		else
			PDEBUG("Cannot convert PIN");
		buzzerdev_set_freq(&devices[i].buzzerdev, defaultfreq);
		buzzerdev_set_repeat(&devices[i].buzzerdev, defaultrepeat);
		buzzerdev_set_active_state(&devices[i].buzzerdev, activesate);
		buzzerdev_init(&devices[i].buzzerdev);
		dev_set_drvdata(devices[i].buzzer_device, &devices[i]);
		memset(threadName, 0, sizeof(threadName));
		sprintf(threadName, "buzzer%d", buzzer_device_num);
		task[i] = kthread_run(do_Work, &devices[i], threadName);
		if (task[i])
		{
			init_waitqueue_head(&devices[i].buzzer_wait_queue);
			devices[i].condition = 0;

			PDEBUG("Thread %d created", buzzer_device_num);
		}
		else
		{
			PDEBUG("Thread %d not created", buzzer_device_num);
		}

	}

	PINFO("INIT\n");
	return 0;
}
static int buzzer_remove(struct platform_device *pdev)
{
	int i;

	PINFO("EXIT\n");

	for(i=0;i<numofdev;i++)
	{
		buzzer_device_num= MKDEV(buzzer_major ,BUZZER_FIRST_MINOR+i);
		buzzerdev_deinit(&devices[i].buzzerdev);
		if (task[i] != NULL)
		{
			devices[i].condition = 1;
			wake_up_interruptible(&devices[i].buzzer_wait_queue);
			kthread_stop(task[i]);
			task[i] = NULL;
			PINFO("Thread is %d about to stop", buzzer_device_num);
		}

		cdev_del(&devices[i].cdev);
		device_destroy(buzzer_class , buzzer_device_num);
	}
	class_destroy(buzzer_class);
	unregister_chrdev_region(buzzer_device_num ,BUZZER_N_MINORS);	
	kfree(devices);
	kfree(task);
	return 0;
}


static const struct of_device_id buzzer_dts[] = {
		{ .compatible = "gpio-buzzers", },
		{}
};
MODULE_DEVICE_TABLE(of, buzzer_dts);

static struct platform_driver buzzer_driver = {
		.driver = {
				.name = "buzzer",
				.owner = THIS_MODULE,
				.of_match_table = of_match_ptr(buzzer_dts),
		},
		.probe = buzzer_probe,
		.remove = buzzer_remove,
};

module_platform_driver(buzzer_driver);
