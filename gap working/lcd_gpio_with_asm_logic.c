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

// Function to send Morse signals to the terminal or LCD
void send_morse_signal(int signal) {
    if (signal == 1) {
        printf("Dot (.) received.\n");
    } else if (signal == 2) {
        printf("Dash (-) received.\n");
    } else if (signal == 3) {
        printf("Gap detected (new letter).\n");
    }
    fflush(stdout);
}

// Other functions remain unchanged...
void report_button_pressed() {
    printf("Button Pressed\n");
    fflush(stdout);
}

void report_button_unpressed() {
    printf("Button Unpressed\n");
    fflush(stdout);
}

extern void morse_code_main();  // Declaration of the assembly function

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

    // Hand control to the assembly code
    printf("Handing control over to Morse code interpreter in assembly...\n");
    fflush(stdout);
    morse_code_main();  // Call the assembly function

    // Cleanup
    munmap(gpio_map, BLOCK_SIZE);

    return 0;  // Exit the program
}
