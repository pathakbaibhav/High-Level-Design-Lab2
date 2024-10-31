#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

/// base address of debug device (physical)
#define SYSTEMC_DEVICE_ADDR (0x48000000)

#define MESSAGE_OFFSET (0x04)

int main(int argc, char *argv[])
{
	int fd;       /// file descriptor to phys mem
	void *pDev;   /// pointer to base address of device (mapped into user's virtual mem)
	unsigned page_size=sysconf(_SC_PAGESIZE);  /// get page size 

	//open device file representing physical memory 
	fd=open("/dev/mem",O_RDWR);
	if(fd<1) {
		perror(argv[0]);
		exit(-1);
	}


	/// get a pointer in process' virtual memory that points to the physcial address of the device
	pDev = mmap(NULL,page_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(SYSTEMC_DEVICE_ADDR & ~(page_size-1)));
	
	//Message
	const char *msg = "Cosimulation Works";

	unsigned char *msg_reg = (volatile unsigned char *)(pDev + MESSAGE_OFFSET);

	// printf("Reading debugdev timer every 200ms\n");
	for(int i = 0; i < 10; i++) 
	{
		*msg_reg = msg[i];
		usleep(100000);
		// read the base register of the debug device (offset 0)
		//uint64_t val = *((unsigned int*) (pDev+ 0));
		//printf("TIMER = %llu\n", val);
	}

	*msg_reg = *(unsigned char *)"\n";
	printf("Message sent to debug device: %s\n", msg);
		
	return 0; 
}
