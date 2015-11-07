#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<asm/uaccess.h> 
#include<linux/sched.h>
#include<linux/wait.h>
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple kernel module to benchmark context switching time - using parent and a child process");
#define SUCCESS 0
#define DEBUG_MODE 
static int major;
struct task_struct *waiting_task = NULL;
static int device_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
	return SUCCESS;
}
	
static int device_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	if(waiting_task)
	{
		wake_up_process(waiting_task);
		waiting_task = NULL;
	}
#ifdef DEBUG_MODE
	printk (KERN_INFO "device closed\n"); 
#endif
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	int bytes_read = 0;
	if(waiting_task)
	{
#ifdef DEBUG_MODE
		printk(KERN_INFO "%d wake up %d\n",current->pid, waiting_task->pid);
#endif
		wake_up_process(waiting_task);
	}
	waiting_task = current;
	current->state = TASK_INTERRUPTIBLE;
	schedule(); 
	return bytes_read;
}


static struct file_operations fops = {
	.read = device_read,
	.open = device_open,
	.release = device_release
};

int init_module(void)
{
	major = register_chrdev(0, "readdev",&fops);
	if( major < 0)
	{
		printk(KERN_ALERT "Registering char device failed\n");
		return major;
	}
#ifdef DEBUG_MODE
	printk(KERN_INFO "Registered the device:%d\n", major); 
#endif
	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(major, "readdev");
#ifdef DEBUG_MODE
	printk(KERN_INFO "Cleaned up \n");
#endif
}
