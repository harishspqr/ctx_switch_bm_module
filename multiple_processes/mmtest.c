#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include "ioctl_basic.h"
#define BUF_LEN 256
//#define DEBUG_MODE
static char buffer[BUF_LEN];
int iterations = 10000;
int volatile started = 0;
static int running_children = 0;
int main(int argc, char** argv)
{
	//fprintf(stderr, "hi:");
	pid_t pid;
	struct timespec start,stop; 	
	unsigned long long diff = 0;
	float switch_time = 0.0;
	int n = 0,k,i=0;
	if(argc < 2)
	{
		printf("Missing argument - No of processes\n");
		return;
	}
	n = atoi(argv[1]);
	unsigned long long switches = iterations * n +  (2*n) ;/* when the child finishes , there is a switch to parent and then back to child */
	// start blocking the device 
	int fd = open("/dev/readdev", O_RDWR);
	int ioret = ioctl(fd, IOCTL_BLOCK); /* block the device until all children are created */
	
	for (k = 0;k<n;k++) /* create n children & call read, iteration times with all children */
	{
		pid = fork();
		if( pid == 0) 
		{
			int ret,i,cur_pid = getpid();
			fd = open("/dev/readdev", O_RDWR);
			if(fd < 0)
			{
				fprintf(stderr, "Oops! failed to open the device\n");
				return errno;
			}
			for (i = 0;i<iterations;i++)
			{
#ifdef DEBUG_MODE
				fprintf(stderr, "Calling read from child:%d %d\n",getpid(), i);
#endif
				ret = read(fd, buffer, BUF_LEN); 
#ifdef DEBUG_MODE
				if(ret < 0) 
				{
					fprintf(stderr, "Oops! Error reading from/writing to deivce\n"); 
					return errno;
				}
#endif
			}
			close(fd);
			exit(0);
		}
	}
#ifdef DEBUG_MODE
	fprintf(stderr, "Created all children\n");	
#endif
	if(clock_gettime( CLOCK_REALTIME, &start) == -1)
	{
		fprintf(stderr, "Error in getting clock time\n");
		return -1;
	}
	ioret = ioctl(fd, IOCTL_UNBLOCK); /* After all children are created, unblock the device */
	i = 1;
	while(i < n) /* Wait for n-1 children to complete execution */ 
	{
		wait(NULL);	
		i++;
	}
#ifdef DEBUG_MODE
	fprintf(stderr, "Parent: calling IOCTL_RELEASE\n");
#endif
	ioctl(fd, IOCTL_RELEASE); /* if there is only 1 child left, release the child and dont block it any more */
	if(clock_gettime( CLOCK_REALTIME, &stop) == -1)
	{
		fprintf(stderr, "Error in getting clock time\n");
		return -1;
	}
	diff = (((stop.tv_sec - start.tv_sec) * 1000000000LLU) + (stop.tv_nsec - start.tv_nsec));
	switch_time = (diff/(switches)); 
	wait(NULL); /* wait for the last child to complete though */
	fprintf(stderr, "Context switch time:%f\n", switch_time);
	return 0;
}
