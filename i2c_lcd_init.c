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

// I2C Register Offsets
#define I2C_C 0x00
#define I2C_S 0x04
#define I2C_DLEN 0x08
#define I2C_A 0x0C
#define I2C_FIFO 0x10

volatile unsigned int *gpio;
volatile unsigned int *i2c;

void wait_for_transfer() {
    // Wait for Transfer to Complete
    while (!(i2c[I2C_S / 4] & 0x02)) {
        // Wait until DONE bit (bit 1) is set in the status register
    }
    // Clear DONE bit by writing to status register
    i2c[I2C_S / 4] |= 0x02;
}

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

    // Minimal I2C Initialization (Enable I2C)
    printf("Initializing I2C minimally (writing to control register)...\n");
    i2c[I2C_C / 4] = 0x8000;  // Write 0x8000 to enable the I2C peripheral

    // Set I2C Slave Address to LCD address (0x27)
    i2c[I2C_A / 4] = 0x27;

    // Send LCD Initialization Sequence
    unsigned char init_sequence[] = {
        0x03,  // Function set (Interface initialization)
        0x02,  // Function set (4-bit interface)
        0x28,  // Function set (2 lines, 5x8 dots)
        0x0C,  // Display control (Display ON, cursor OFF)
        0x06,  // Entry mode set (Increment cursor)
        0x01   // Clear display
    };

    for (int i = 0; i < sizeof(init_sequence); i++) {
        // Set Data Length to 1 for each command
        i2c[I2C_DLEN / 4] = 1;

        // Write command to FIFO
        i2c[I2C_FIFO / 4] = init_sequence[i];

        // Start I2C Transfer
        i2c[I2C_C / 4] |= 0x8080;

        // Wait for Transfer to Complete
        wait_for_transfer();
    }

    printf("LCD Initialization Completed Successfully.\n");

    // Keep the program running to retain memory mapping
    while (1) {
        sleep(1);
    }

    // Cleanup (not reached in this loop)
    munmap(gpio_map, BLOCK_SIZE);
    munmap(i2c_map, BLOCK_SIZE);

    return 0;
}
