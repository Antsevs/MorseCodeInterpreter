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

void delay_us(int microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

void delay_ms(int milliseconds) {
    delay_us(milliseconds * 1000);
}

void wait_for_transfer() {
    // Wait for Transfer to Complete
    while (!(i2c[I2C_S / 4] & 0x02)) {
        // Wait until DONE bit (bit 1) is set in the status register
    }
    // Clear DONE bit by writing to status register
    i2c[I2C_S / 4] |= 0x02;
}

void send_nibble(unsigned char nibble, unsigned char rs) {
    // Set Data Length to 1
    i2c[I2C_DLEN / 4] = 1;

    // Write nibble to FIFO (backlight on and optionally RS bit set)
    i2c[I2C_FIFO / 4] = (nibble & 0xF0) | 0x08 | rs;  // RS = 0 for command, 1 for data

    // Start I2C Transfer
    i2c[I2C_C / 4] |= 0x8080;

    // Wait for Transfer to Complete
    wait_for_transfer();

    // Pulse Enable (E)
    i2c[I2C_FIFO / 4] = ((nibble & 0xF0) | 0x0C | rs);  // Set E bit high
    i2c[I2C_C / 4] |= 0x8080;
    wait_for_transfer();

    delay_us(100);  // Increase delay to ensure data is latched correctly

    i2c[I2C_FIFO / 4] = ((nibble & 0xF0) | 0x08 | rs);  // Set E bit low
    i2c[I2C_C / 4] |= 0x8080;
    wait_for_transfer();

    delay_us(100);  // Additional delay for nibble execution
}

void send_command(unsigned char command) {
    // Send the high nibble
    send_nibble(command & 0xF0, 0x00);

    // Send the low nibble
    send_nibble((command << 4) & 0xF0, 0x00);
}

void send_data(unsigned char data) {
    // Send the high nibble
    send_nibble(data & 0xF0, 0x01);

    // Send the low nibble
    send_nibble((data << 4) & 0xF0, 0x01);
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

    // Minimal I2C Initialization (Enable I2C)
    printf("Initializing I2C minimally (writing to control register)...\n");
    fflush(stdout);
    i2c[I2C_C / 4] = 0x8000;  // Write 0x8000 to enable the I2C peripheral
    wait_for_user();

    // Set I2C Slave Address to LCD address (0x27)
    printf("Setting I2C Slave Address to LCD (0x27)...\n");
    fflush(stdout);
    i2c[I2C_A / 4] = 0x27;
    wait_for_user();

    // 4-bit Initialization Sequence
    printf("Sending 4-bit initialization sequence...\n");
    fflush(stdout);

    // Step 1: Send 0x30 three times to ensure 8-bit mode
    send_nibble(0x30, 0x00);
    delay_ms(50);
    wait_for_user();
    
    send_nibble(0x30, 0x00);
    delay_ms(50);
    wait_for_user();
    
    send_nibble(0x30, 0x00);
    delay_ms(50);
    wait_for_user();

    // Step 2: Send 0x20 to set 4-bit mode
    send_nibble(0x20, 0x00);
    delay_ms(50);
    wait_for_user();

    // Step 3: Send Function Set Command (0x28) for 4-bit mode, 2 lines, 5x8 dots
    send_command(0x28);
    delay_ms(50);
    wait_for_user();

    // Step 4: Send Display ON/OFF Control Command (0x0C) to turn on display, cursor off, backlight on
    send_command(0x0C);
    delay_ms(50);
    wait_for_user();

    // Step 5: Send Entry Mode Set Command (0x06) to set cursor to increment
    send_command(0x06);
    delay_ms(50);
    wait_for_user();

    printf("Initialization Completed Successfully.\n");
    fflush(stdout);
    wait_for_user();

    // Write single character 'A' to the LCD, with backlight on
    printf("Writing 'A' to LCD...\n");
    fflush(stdout);
    send_data('A');
    delay_ms(50);
    wait_for_user();

    // Test sending clear display command
    printf("Sending clear display command...\n");
    fflush(stdout);
    send_command(0x01);
    delay_ms(50);
    wait_for_user();

    printf("Completed writing to LCD.\n");
    fflush(stdout);

    // Keep the program running to retain memory mapping
    while (1) {
        sleep(1);
    }

    // Cleanup (not reached in this loop)
    munmap(gpio_map, BLOCK_SIZE);
    munmap(i2c_map, BLOCK_SIZE);

    return 0;
}
