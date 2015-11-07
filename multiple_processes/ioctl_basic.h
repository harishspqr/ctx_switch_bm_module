#include <linux/ioctl.h>
#define IOC_MAGIC 'k' 
#define IOCTL_BLOCK _IO(IOC_MAGIC,0)
#define IOCTL_UNBLOCK _IO(IOC_MAGIC,1)
#define IOCTL_RELEASE _IO(IOC_MAGIC,2)
