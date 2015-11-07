obj-m += mmread.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	cc mmtest.c -g -o mmtest -lrt -lpthread
	mknod /dev/readdev c 251 1	
	insmod mmread.ko

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf mmtest /dev/readdev 
	rmmod mmread
