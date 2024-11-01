#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>

// base address of vector processor
#define VECTOR_PROCESSOR_ADDR (0x49000000)

#define CSR_OFFSET (0x0)

int main(int argc, char *argv[])
{
	int fd;       /// file descriptor to phys mem
	void *pVecProc;  /// pointer to base address of vector processor
	unsigned page_size=sysconf(_SC_PAGESIZE);  /// get page size 

	//open device file representing physical memory 
	fd=open("/dev/mem",O_RDWR);
	if(fd<1) {
		perror(argv[0]);
		exit(-1);
	}

	pVecProc = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (VECTOR_PROCESSOR_ADDR & ~(page_size - 1)));
	if (pVecProc == MAP_FAILED) {
		perror("Failed to map vector processor");
		close(fd);
		exit(-1);
	}

	// initialize the CSR register
    volatile uint32_t *csr_reg = (volatile uint32_t *)(pVecProc + CSR_OFFSET);

    // Setting the value into CSR
    *csr_reg = 1;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Convert to nanoseconds
    long long old_qemu = tv.tv_sec * 1000000000LL + tv.tv_usec * 1000; // Convert seconds to ns and microseconds to ns

    // Read SystemC time
    volatile uint32_t *trace_reg = (volatile uint32_t *)(pVecProc + 0xC4);

    // Read time
    uint32_t curr_time = *trace_reg;
    printf("Current Time (SystemC): %u\n", curr_time);

    while (1) 
	{
        // Reading the value from CSR
        uint32_t val = *((volatile uint32_t *)(pVecProc + CSR_OFFSET));
        if (val == 0x0) 
		{
            gettimeofday(&tv, NULL);
            long long now_qemu = tv.tv_sec * 1000000000LL + tv.tv_usec * 1000; // Convert seconds to ns and microseconds to ns
            uint32_t now_sysc = *trace_reg;
            long long diff_time = now_qemu - old_qemu;
            uint32_t diff = now_sysc - curr_time;
            printf("Time took (QEMU): %lld\n", diff_time);
            printf("Time took (SystemC): %u\n", diff);
            break;
        }
    }


	return 0; 
}