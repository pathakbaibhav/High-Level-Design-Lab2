#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <inttypes.h>

// base address of vector processor
#define VECTOR_PROCESSOR_ADDR (0x49000000)
#define CSR_OFFSET 0x0
#define VA_OFFSET 0x04
#define VB_OFFSET 0x44
#define VC_OFFSET 0x84

void validate_result(volatile uint32_t *vc_reg, uint32_t *expected, const char *operation) {
    int errors = 0;
    printf("VC result after %s:\n", operation);
    for (int i = 0; i < 16; ++i) {
        uint32_t result = vc_reg[i];
        printf("VC[%d] = %u (Expected: %u)\n", i, result, expected[i]);
        if (result != expected[i]) {
            errors++;
        }
    }
    if (errors == 0) {
        printf("%s test passed!\n", operation);
    } else {
        printf("%s test failed with %d errors.\n", operation, errors);
    }
}

int main(int argc, char *argv[]) {
    int fd;
    void *pVecProc;
    unsigned page_size = sysconf(_SC_PAGESIZE);

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

    volatile uint32_t *csr_reg = (volatile uint32_t *)(pVecProc + CSR_OFFSET);
    volatile uint32_t *va_reg = (volatile uint32_t *)(pVecProc + VA_OFFSET);
    volatile uint32_t *vb_reg = (volatile uint32_t *)(pVecProc + VB_OFFSET);
    volatile uint32_t *vc_reg = (volatile uint32_t *)(pVecProc + VC_OFFSET);

    uint32_t VA[16] = {1, 5, 3, 4, 1, 55, 3, 91, 12, 12, 5, 31, 88, 87, 3, 5};
    uint32_t VB[16] = {5, 1, 56, 8, 4, 35, 6, 2, 65, 12, 3, 56, 1, 6, 543, 3};

    printf("Writing values to VA and VB:\n");
    for (int i = 0; i < 16; ++i) {
        va_reg[i] = VA[i];
        vb_reg[i] = VB[i];
        printf("VA[%d] = %u, VB[%d] = %u\n", i, VA[i], i, VB[i]);
    }

    // Add operation
    *csr_reg = 1;
    while (*csr_reg != 0) usleep(100);
    uint32_t expected_add[16];
    for (int i = 0; i < 16; ++i) expected_add[i] = VA[i] + VB[i];
    validate_result(vc_reg, expected_add, "Addition");

    // Subtract operation
    *csr_reg = 0x2;
    while (*csr_reg != 0) usleep(100);
    uint32_t expected_sub[16];
    for (int i = 0; i < 16; ++i) expected_sub[i] = VA[i] - VB[i];
    validate_result(vc_reg, expected_sub, "Subtraction");

    // Multiply operation
    *csr_reg = 0x3;
    while (*csr_reg != 0) usleep(100);
    uint32_t expected_mul[16];
    for (int i = 0; i < 16; ++i) expected_mul[i] = VA[i] * VB[i];
    validate_result(vc_reg, expected_mul, "Multiplication");

    // Division operation
    *csr_reg = 0x4;
    while (*csr_reg != 0) usleep(100);
    uint32_t expected_div[16];
    for (int i = 0; i < 16; ++i) expected_div[i] = VB[i] != 0 ? VA[i] / VB[i] : 0;
    validate_result(vc_reg, expected_div, "Division");

    // Multiply and Accumulate operation
    for (int i = 0; i < 16; ++i) vc_reg[i] = expected_add[i];
    *csr_reg = 0x5;
    while (*csr_reg != 0) usleep(100);
    uint32_t expected_mac[16];
    for (int i = 0; i < 16; ++i) expected_mac[i] = VA[i] * VB[i] + expected_add[i];
    validate_result(vc_reg, expected_mac, "Multiply and Accumulate");

    munmap(pVecProc, page_size);
    close(fd);
    return 0;
}