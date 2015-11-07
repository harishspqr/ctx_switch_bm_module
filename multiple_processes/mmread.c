#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include "ioctl_basic.h"
#include<linux/fs.h>
#include<asm/uaccess.h> 
#include<linux/sched.h>
#include<linux/wait.h>
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple kernel module to benchmark context switching time");
#define SUCCESS 0
static int major;
//#define DEBUG_ON
static DECLARE_WAIT_QUEUE_HEAD(bwq); 
static bool block = true;
static bool done = false;
struct task_struct *waiting_task = NULL;
struct task_struct *last_task = NULL;

/* Open the device- allow multiple process to open the device simultaneously  */
static int device_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
	return SUCCESS;
}

/* use IOCTL to syncronize processes in kernel mode */
static long device_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
	int ret = 1;
	printk(KERN_INFO "IOCTL invoked\n");
	switch(cmd) 
	{
		case IOCTL_BLOCK: /* once IOCTL_BLOCK is called, all process that call read will be blocked  */
		{
			block = true;
#ifdef DEBUG_ON
			printk(KERN_INFO "DEVICE blocked\n");
#endif
			ret = 0;
			done = false;
			break;
		}
		case IOCTL_UNBLOCK: /* until IOCTL_UBLOCK is called  */
		{
			block = false;
#ifdef DEBUG_ON
			printk(KERN_INFO "DEVICE unblocked\n");
#endif
			wake_up_interruptible(&bwq);	
			ret = 0;
			break;
		}
		case IOCTL_RELEASE: /* sets the done flag to true - used during clean up from user mode */
		{
			done = true;
			if(waiting_task)
			{
				wake_up_process(waiting_task);
				waiting_task = NULL;
			}
		}
	}
	return ret;
}	

static int device_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
#ifdef DEBUG_ON
	printk (KERN_INFO "device closed:%d\n", current->pid); 
#endif
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	int bytes_read = 0;
	if(block == true) /* if block is true all prcesses calling read would be added to wait queue bwq */
	{
		wait_event_interruptible(bwq, block == false); /* they would be unblocked once they are awakened by IOCTL_UNBLOCK */
	}
	if(waiting_task && waiting_task != current) /* If there is a waiting task and its not current, wake it up */
	{
#ifdef DEBUG_ON
		printk(KERN_INFO "%d wakes up %d\n", current->pid, waiting_task->pid);
#endif
		wake_up_process(waiting_task);
	}
	if(!done) /* until we are not done(decided by user mode code), sleep the current task to be awakened by some other task */
	{
		set_current_state(TASK_INTERRUPTIBLE);
		waiting_task = current;
#ifdef DEBUG_ON
		printk(KERN_INFO "Going to sleep %d\n", current->pid);
#endif
		schedule(); 	
	}
	else if(waiting_task) /* if we are done and if there is a sleeping task, wake it up and reset the done to false for next run */
	{
		wake_up_process(waiting_task);
#ifdef DEBUG_ON
		printk(KERN_INFO "Finally waking up %d\n",waiting_task->pid);
#endif
		waiting_task=NULL; /* reset everything to initial coniditions after we wake up the last task */
		done = false;
		block = true;
	}
	/*else
	{
		block = true;
	}*/
	return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t* off)
{
	return len;
}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl = device_ioctl
};

int init_module(void)
{
	major = register_chrdev(0, "readdev",&fops);
	if( major < 0)
	{
		printk(KERN_ALERT "Registering char device failed\n");
		return major;
	}
#ifdef DEBUG_ON
	printk(KERN_INFO "Registered the device:%d\n", major); 
#endif
	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(major, "readdev");
#ifdef DEBUG_ON
	printk(KERN_INFO "Cleaned up \n");
#endif
}
