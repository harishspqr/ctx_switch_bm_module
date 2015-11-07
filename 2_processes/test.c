#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define BUF_LEN 256
//#define DEBUG_MODE
static char buffer[BUF_LEN];
int iterations = 1000000;
int main()
{
	const pid_t pid = fork();
	struct timespec start,stop; 	
	unsigned long long diff = 0;
	unsigned long long switches = iterations * 2;
	float switch_time = 0.0;
	if( pid == 0) //child
	{
		int fd,ret,i;
		fd = open("/dev/readdev", O_RDWR);
		if(fd < 0)
		{
			printf("Oops! failed to open the device\n");
			return errno;
		}
		for (i = 0;i<iterations;i++)
		{
			ret = read(fd, buffer, BUF_LEN); 
			#ifdef DEBUG_MODE
			printf("Calling read from child\n");
			if(ret < 0) 
			{
				printf("Oops! Error reading from deivce\n"); 
				return errno;
			}
			#endif
		}
		close(fd);
	}
	else // Parent
	{	
		int fd,ret,i;
		char str[BUF_LEN];
		strcpy(str,"");
		fd = open("/dev/readdev", O_RDWR);
		if(fd < 0)
		{
			printf("Oops! failed to open the device\n");
			return errno;
		}
		if(clock_gettime( CLOCK_REALTIME, &start) == -1)
		{
			printf("Error in getting clock time\n");
			return -1;
		}
		for(i = 0;i<iterations;i++) // 2 context switches per iteration. child->parent then parent->child. 
		{
			ret = read(fd, buffer, BUF_LEN); 
			#ifdef DEBUG_MODE
			printf("calling read from parent\n");
			if (ret	< 0)
			{
				printf("Oops! failed to write to the device\n");
				return errno;
			}
			#endif
		}
			
		close(fd);
		if(clock_gettime( CLOCK_REALTIME, &stop) == -1)
		{
			printf("Error in getting clock time\n");
			return -1;
		}
		diff = (((stop.tv_sec - start.tv_sec) * 1000000000LLU) + (stop.tv_nsec - start.tv_nsec));
		switch_time = (diff/switches); 
		printf("Context switch time:%f\n", switch_time);	
	}
	return 0;
}
