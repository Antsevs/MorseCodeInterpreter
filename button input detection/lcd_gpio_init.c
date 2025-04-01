#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#define PERIPHERAL_BASE 0x3F000000
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define I2C_BASE (PERIPHERAL_BASE + 0x804000)
#define BLOCK_SIZE (4 * 1024)

// GPIO Register Offsets
#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPLEV0 0x34

// I2C Register Offsets
#define I2C_C 0x00
#define I2C_S 0x04
#define I2C_DLEN 0x08
#define I2C_A 0x0C
#define I2C_FIFO 0x10

volatile unsigned int *gpio;
volatile unsigned int *i2c;

void delay_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void wait_for_user() {
    printf("Press Enter to continue...\n");
    fflush(stdout);
    getchar();
}

int main() {
    int mem_fd;
    void *gpio_map, *i2c_map;

    // Open /dev/mem to access physical memory
    printf("Opening /dev/mem to access physical memory...\n");
    fflush(stdout);
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("open");
        return -1;
    }

    // Map GPIO memory
    printf("Mapping GPIO memory...\n");
    fflush(stdout);
    gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
    if (gpio_map == MAP_FAILED) {
        perror("mmap GPIO");
        close(mem_fd);
        return -1;
    }

    // Map I2C memory
    printf("Mapping I2C memory...\n");
    fflush(stdout);
    i2c_map = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, I2C_BASE);
    if (i2c_map == MAP_FAILED) {
        perror("mmap I2C");
        munmap(gpio_map, BLOCK_SIZE);
        close(mem_fd);
        return -1;
    }

    // Close /dev/mem file descriptor after mapping
    printf("Closing /dev/mem file descriptor after mapping...\n");
    fflush(stdout);
    close(mem_fd);

    // Assign pointers to mapped regions
    gpio = (volatile unsigned int *)gpio_map;
    i2c = (volatile unsigned int *)i2c_map;

    // Configure GPIO pins for I2C (GPIO 2 and GPIO 3)
    printf("Configuring GPIO pins for I2C...\n");
    fflush(stdout);
    gpio[GPFSEL0 / 4] &= ~(0b111 << 6);  // Clear bits for GPIO 2
    gpio[GPFSEL0 / 4] |= (0b100 << 6);   // Set GPIO 2 to ALT0 (SDA)
    gpio[GPFSEL0 / 4] &= ~(0b111 << 9);  // Clear bits for GPIO 3
    gpio[GPFSEL0 / 4] |= (0b100 << 9);   // Set GPIO 3 to ALT0 (SCL)

    // Configure GPIO 17 as input (GPFSEL1 controls pins 10-19, GPIO 17 corresponds to bits 21-23)
    printf("Configuring GPIO pin 17 as input...\n");
    fflush(stdout);
    gpio[GPFSEL1 / 4] &= ~(0b111 << 21);  // Clear bits 21-23 to set GPIO 17 as input
    wait_for_user();

    // Verify GPIO 17 input by reading its value continuously
    printf("Starting to read GPIO 17 status...\n");
    fflush(stdout);

    unsigned int previous_state = 0;
    while (1) {
        unsigned int current_state = gpio[GPLEV0 / 4] & (1 << 17);  // Read GPLEV0 register (offset 0x34) to check GPIO 17
        if (current_state != previous_state) {
            if (current_state) {
                printf("Button Pressed (GPIO 17 is HIGH)\n");
            } else {
                printf("Button Released (GPIO 17 is LOW)\n");
            }
            fflush(stdout);
            previous_state = current_state;
        }
        delay_ms(100);  // 100 ms delay to reduce output frequency
    }

    // Cleanup
    munmap(gpio_map, BLOCK_SIZE);
    munmap(i2c_map, BLOCK_SIZE);

    return 0;  // Exit the program
}
