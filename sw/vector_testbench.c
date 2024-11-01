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

// #define ERROR_ADDR_OFFSET (0xf0)

// #define STOP_SIM_OFFSET (0x08)

// base address of vector processor
#define VECTOR_PROCESSOR_ADDR (0x49000000)

int main(int argc, char *argv[])
{
	int fd;       /// file descriptor to phys mem
	void *pDev;   /// pointer to base address of device (mapped into user's virtual mem)
	void *pVecProc;  /// pointer to base address of vector processor
	unsigned page_size=sysconf(_SC_PAGESIZE);  /// get page size 

	//open device file representing physical memory 
	fd=open("/dev/mem",O_RDWR);
	if(fd<1) {
		perror(argv[0]);
		exit(-1);
	}

	/// get a pointer in process' virtual memory that points to the physcial address of the device
	pDev = mmap(NULL,page_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(SYSTEMC_DEVICE_ADDR & ~(page_size-1)));

	pVecProc = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (VECTOR_PROCESSOR_ADDR & ~(page_size - 1)));
	if (pVecProc == MAP_FAILED) {
		perror("Failed to map vector processor");
		close(fd);
		exit(-1);
	}
	
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

	// *msg_reg = *(unsigned char *)"\n";
	// printf("Message sent to debug device: %s\n", msg);

	//Issue 2: Task 3.2
	// int val = *((volatile int *)(pDev + ERROR_ADDR_OFFSET));
	// printf("Value read from address 0x%x: %d\n", ERROR_ADDR_OFFSET, val);

	//Issue 2: Task 3.3
	// Timer reading
    // uint64_t timer_val = *((uint64_t *)(pDev));
    // printf("Final TIMER value read: %" PRIu64 "\n", timer_val);

    // Stop simulation
    // *(volatile uint32_t *)(pDev + STOP_SIM_OFFSET) = 1;  // Writing to 0x8 to stop the simulation
    // printf("Simulation stop command sent.\n");

	// Issue 3: Task 2.2
	// Write and verify CSR value in vector processor
	// uint32_t write_val = 07;
	// *((volatile uint32_t *)(pVecProc + 0x0)) = write_val;  // Write to CSR
	
	// uint32_t read_val = *((volatile uint32_t *)(pVecProc + 0x0));  // Read back from CSR
	// printf("CSR written value: %u, CSR read value: %u\n", write_val, read_val);

	// Issue 4: Task 2.1
	uint32_t write_val = 1;


	return 0; 
}