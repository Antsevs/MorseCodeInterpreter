#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define GPIO_BASE 0x200000  // GPIO base address for /dev/gpiomem
#define BLOCK_SIZE (4 * 1024)  // Block size for GPIO
#define I2C_ADDR 0x27  // I2C address for the LCD

// GPIO Register Offsets
#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPLEV0 0x34

volatile unsigned int *gpio;

// Delay function in milliseconds
void delay_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Function to be called by the assembly code to report successful entry
void report_init() {
    printf("Entered Morse code interpreter in assembly successfully.\n");
    fflush(stdout);
}

// Function to read the state of GPIO 17 (button press)
unsigned int read_gpio_pin() {
    unsigned int gpio_value = gpio[GPLEV0 / 4];  // Read the GPIO pin level register
    unsigned int pin_value = (gpio_value >> 17) & 0x1;  // Extract GPIO 17 value
    return pin_value;  // Return 1 if pressed, 0 otherwise
}

// Function to send data to the I2C LCD in 4-bit mode
void lcd_send_data(int i2c_fd, char data, char mode) {
    // Send higher nibble
    char high_nibble = (data & 0xF0) | mode | 0x08;  // Include backlight bit (0x08)
    write(i2c_fd, &high_nibble, 1);
    delay_ms(1);
    high_nibble |= 0x04;  // Enable pulse
    write(i2c_fd, &high_nibble, 1);
    delay_ms(1);
    high_nibble &= ~0x04;  // Disable pulse
    write(i2c_fd, &high_nibble, 1);
    delay_ms(1);

    // Send lower nibble
    char low_nibble = ((data << 4) & 0xF0) | mode | 0x08;  // Include backlight bit (0x08)
    write(i2c_fd, &low_nibble, 1);
    delay_ms(1);
    low_nibble |= 0x04;  // Enable pulse
    write(i2c_fd, &low_nibble, 1);
    delay_ms(1);
    low_nibble &= ~0x04;  // Disable pulse
    write(i2c_fd, &low_nibble, 1);
    delay_ms(1);
}

// Function to send a message to the I2C LCD module
void send_message_to_lcd() {
    int i2c_fd;
    const char *device = "/dev/i2c-1";
    char message[] = "Button Pressed";

    // Open the I2C device
    i2c_fd = open(device, O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open I2C device");
        return;
    }

    // Set the I2C slave address
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(i2c_fd);
        return;
    }

    // Send initialization sequence for 4-bit mode
    lcd_send_data(i2c_fd, 0x33, 0x00);  // Initialization
    lcd_send_data(i2c_fd, 0x32, 0x00);  // Set to 4-bit mode
    lcd_send_data(i2c_fd, 0x28, 0x00);  // Function set: 2-line, 4-bit mode
    lcd_send_data(i2c_fd, 0x0C, 0x00);  // Display ON, Cursor OFF
    lcd_send_data(i2c_fd, 0x06, 0x00);  // Entry mode set

    // Clear the display before writing new data
    lcd_send_data(i2c_fd, 0x01, 0x00);
    delay_ms(2);

    // Write message to the LCD
    for (int i = 0; i < sizeof(message) - 1; i++) {
        lcd_send_data(i2c_fd, message[i], 0x01);  // Send data with RS = 1 for characters
    }

    // Close the I2C device
    close(i2c_fd);
}

extern void morse_code_main();  // Declaration of assembly function
extern void delay_ms(int milliseconds);  // Declare delay function for assembly use

int main() {
    int mem_fd;
    void *gpio_map;

    // Open /dev/gpiomem to access GPIO physical memory
    printf("Opening /dev/gpiomem to access GPIO memory...\n");
    fflush(stdout);
    mem_fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
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

    // Close /dev/gpiomem file descriptor after mapping
    printf("Closing /dev/gpiomem file descriptor after mapping...\n");
    fflush(stdout);
    close(mem_fd);

    // Assign pointers to mapped regions
    gpio = (volatile unsigned int *)gpio_map;

    // Configure GPIO 17 as input (GPFSEL1 controls pins 10-19, GPIO 17 corresponds to bits 21-23)
    printf("Configuring GPIO pin 17 as input...\n");
    fflush(stdout);
    gpio[GPFSEL1 / 4] &= ~(0b111 << 21);  // Clear bits 21-23 to set GPIO 17 as input

    // Test GPIO read from GPLEV0
    printf("Testing GPIO read from GPLEV0...\n");
    fflush(stdout);
    unsigned int gpio_value = gpio[GPLEV0 / 4];  // Read the GPIO pin level register
    printf("GPLEV0 Value: 0x%08X\n", gpio_value);
    fflush(stdout);

    // Hand control to the assembly code
    printf("Handing control over to Morse code interpreter in assembly...\n");
    fflush(stdout);
    morse_code_main();  // Call the assembly function

    // Cleanup
    munmap(gpio_map, BLOCK_SIZE);

    return 0;  // Exit the program
}
