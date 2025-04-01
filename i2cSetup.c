#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PERIPHERAL_BASE 0x3F000000
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define I2C_BASE (PERIPHERAL_BASE + 0x804000)
#define BLOCK_SIZE (4 * 1024)

// GPIO Register Offsets
#define GPFSEL0 0x00
#define GPFSEL1 0x04

// I2C Register Offsets
#define I2C_C 0x00
#define I2C_S 0x04
#define I2C_DLEN 0x08
#define I2C_A 0x0C
#define I2C_FIFO 0x10
#define I2C_DIV 0x14

volatile unsigned int *gpio;
volatile unsigned int *i2c;

int main() {
    int mem_fd;
    void *gpio_map, *i2c_map;

    // Open /dev/mem to access physical memory
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("open");
        return -1;
    }

    // Map GPIO memory
    gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
    if (gpio_map == MAP_FAILED) {
        perror("mmap GPIO");
        close(mem_fd);
        return -1;
    }

    // Map I2C memory
    i2c_map = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, I2C_BASE);
    if (i2c_map == MAP_FAILED) {
        perror("mmap I2C");
        munmap(gpio_map, BLOCK_SIZE);
        close(mem_fd);
        return -1;
    }

    // Close /dev/mem file descriptor after mapping
    close(mem_fd);

    // Assign pointers to mapped regions
    gpio = (volatile unsigned int *)gpio_map;
    i2c = (volatile unsigned int *)i2c_map;

    // Configure GPIO pins for I2C (GPIO 2 and GPIO 3)
    gpio[GPFSEL0 / 4] &= ~(0b111 << 6);  // Clear bits for GPIO 2
    gpio[GPFSEL0 / 4] |= (0b100 << 6);   // Set GPIO 2 to ALT0 (SDA)
    gpio[GPFSEL0 / 4] &= ~(0b111 << 9);  // Clear bits for GPIO 3
    gpio[GPFSEL0 / 4] |= (0b100 << 9);   // Set GPIO 3 to ALT0 (SCL)

    // Initialize I2C
    printf("Initializing I2C...\n");

    // Disable I2C during setup
    i2c[I2C_C / 4] = 0x0000;

    // Set the clock divider to achieve 100 kHz (divider value is 0x30 for this example)
    i2c[I2C_DIV / 4] = 0x30;

    // Enable I2C and clear FIFO
    i2c[I2C_C / 4] = 0x8030;

    // Verify I2C control register value
    unsigned int i2c_control = i2c[I2C_C / 4];
    printf("I2C Control Register Value: 0x%08x\n", i2c_control);

    // Keep the program running to retain memory mapping
    while (1) {
        sleep(1);
    }

    // Cleanup (not reached in this loop)
    munmap(gpio_map, BLOCK_SIZE);
    munmap(i2c_map, BLOCK_SIZE);

    return 0;
}
