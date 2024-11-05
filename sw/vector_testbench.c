#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/time.h>

// base address of vector processor
#define VECTOR_PROCESSOR_ADDR (0x49000000)
#define CSR_OFFSET 0x0
#define VA_OFFSET 0x04
#define VB_OFFSET 0x44
#define VC_OFFSET 0x84

int main(int argc, char *argv[]) {
    int fd;
    void *pVecProc;
    unsigned page_size = sysconf(_SC_PAGESIZE);

    // Open device file for physical memory
    fd = open("/dev/mem", O_RDWR);
    if (fd < 1) {
        perror("Failed to open /dev/mem");
        exit(-1);
    }

    pVecProc = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (VECTOR_PROCESSOR_ADDR & ~(page_size - 1)));
    if (pVecProc == MAP_FAILED) {
        perror("Failed to map vector processor");
        close(fd);
        exit(-1);
    }

    // Map pointers to CSR and trace registers
    volatile uint32_t *csr_reg = (volatile uint32_t *)(pVecProc + CSR_OFFSET);
    volatile uint32_t *trace_reg = (volatile uint32_t *)(pVecProc + 0xC4);

    // Initialize vectors VA, VB, and clear VC in mapped memory
    uint32_t VA[16] = {1, 5, 3, 4, 1, 55, 3, 91, 12, 12, 5, 31, 88, 87, 3, 5};
    uint32_t VB[16] = {5, 1, 56, 8, 4, 35, 6, 2, 65, 12, 3, 56, 1, 6, 543, 3};

    volatile uint32_t *va_reg = (volatile uint32_t *)(pVecProc + VA_OFFSET);
    volatile uint32_t *vb_reg = (volatile uint32_t *)(pVecProc + VB_OFFSET);
    volatile uint32_t *vc_reg = (volatile uint32_t *)(pVecProc + VC_OFFSET);

    // Write VA and VB to vector processor registers
    printf("Writing values to VA:\n");
    for (int i = 0; i < 16; ++i) {
        va_reg[i] = VA[i];
        printf("%u ", VA[i]);
    }
    printf("\n");

    printf("Writing values to VB:\n");
    for (int i = 0; i < 16; ++i) {
        vb_reg[i] = VB[i];
        printf("%u ", VB[i]);
    }
    printf("\n");

    // Trigger the vector addition
    *csr_reg = 1;

    // Wait for completion (CSR becomes 0)
    while (*csr_reg != 0) {
        usleep(100);  // Polling delay
    }

    // Read and verify the result in VC
    int errors = 0;
    printf("VC result:\n");
    for (int i = 0; i < 16; ++i) {
        uint32_t expected = VA[i] + VB[i];
        uint32_t result = vc_reg[i];
        printf("VC[%d] = %u (Expected: %u)\n", i, result, expected);
        if (result != expected) {
            errors++;
        }
    }

    if (errors == 0) {
        printf("Vector addition test passed!\n");
    } else {
        printf("Vector addition test failed with %d errors.\n", errors);
    }

    munmap(pVecProc, page_size);
    close(fd);
    return 0;
}