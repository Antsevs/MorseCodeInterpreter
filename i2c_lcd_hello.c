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

void wait_for_transfer() {
    // Wait for Transfer to Complete
    while (!(i2c[I2C_S / 4] & 0x02)) {
        // Wait until DONE bit (bit 1) is set in the status register
    }
    // Clear DONE bit by writing to status register
    i2c[I2C_S / 4] |= 0x02;
}

void send_command(unsigned char command) {
    // Set Data Length to 1
    i2c[I2C_DLEN / 4] = 1;

    // Write command to FIFO (RS bit = 0 for command)
    i2c[I2C_FIFO / 4] = command & 0xF0;  // High nibble

    // Start I2C Transfer
    i2c[I2C_C / 4] |= 0x8080;

    // Wait for Transfer to Complete
    wait_for_transfer();

    // Add delay for command execution
    delay_ms(5);

    // Send low nibble
    i2c[I2C_FIFO / 4] = (command << 4) & 0xF0;  // Low nibble

    // Start I2C Transfer
    i2c[I2C_C / 4] |= 0x8080;

    // Wait for Transfer to Complete
    wait_for_transfer();

    // Add delay for command execution
    delay_ms(5);
}

void send_data(unsigned char data) {
    // Set Data Length to 1
    i2c[I2C_DLEN / 4] = 1;

    // Write data to FIFO (RS bit = 1 for data)
    i2c[I2C_FIFO / 4] = (data & 0xF0) | 0x01;  // High nibble with RS = 1

    // Start I2C Transfer
    i2c[I2C_C / 4] |= 0x8080;

    // Wait for Transfer to Complete
    wait_for_transfer();

    // Add delay for data execution
    delay_ms(5);

    // Send low nibble
    i2c[I2C_FIFO / 4] = ((data << 4) & 0xF0) | 0x01;  // Low nibble with RS = 1

    // Start I2C Transfer
    i2c[I2C_C / 4] |= 0x8080;

    // Wait for Transfer to Complete
    wait_for_transfer();

    // Add delay for data execution
    delay_ms(5);
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

    // Send Display Control Command (0x0C) to turn on display, cursor off
    send_command(0x0C);

    // Send Function Set Command (0x28) for 2 lines, 5x8 dots
    send_command(0x28);

    // Send Entry Mode Set Command (0x06) for increment mode
    send_command(0x06);

    // Write "Hello" to the LCD
    char message[] = "Hello";
    for (int i = 0; i < 5; i++) {
        send_data(message[i]);
    }

    printf("Sent 'Hello' to LCD.\n");

    // Keep the program running to retain memory mapping
    while (1) {
        sleep(1);
    }

    // Cleanup (not reached in this loop)
    munmap(gpio_map, BLOCK_SIZE);
    munmap(i2c_map, BLOCK_SIZE);

    return 0;
}
