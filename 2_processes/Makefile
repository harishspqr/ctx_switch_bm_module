#This was compiled against kernel version 4.2
obj-m += read.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	cc test.c -g -o test -lrt -lpthread
#device id generated was 251 in my linux
	mknod /dev/readdev c 251 1	
	insmod read.ko

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf test /dev/readdev 
	rmmod read
