#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define GPIO_BASE 0x3F200000  // Base address for GPIO (Raspberry Pi 3/4)
#define BLOCK_SIZE (4 * 1024)
#define GPIO_PIN 24  // GPIO pin for the button
#define I2C_ADDR 0x27  // I2C address for the LCD

// Function to check if the button is pressed
int is_button_pressed(volatile unsigned int *gpio) {
    unsigned int level = *(gpio + 13);  // GPLEV0 register (offset 0x34 / 4 bytes)
    return (level & (1 << GPIO_PIN)) == 0;  // Reversed logic: pressed if LOW (0)
}

// Function to send a command to the LCD
void lcd_send_command(int fd, uint8_t command, uint8_t backlight) {
    uint8_t data[4];
    data[0] = (command & 0xF0) | backlight | 0x04; // Upper nibble with EN=1
    data[1] = (command & 0xF0) | backlight;       // Upper nibble with EN=0
    data[2] = ((command << 4) & 0xF0) | backlight | 0x04; // Lower nibble with EN=1
    data[3] = ((command << 4) & 0xF0) | backlight;       // Lower nibble with EN=0
    write(fd, data, 4);
    usleep(2000);  // Command processing delay
}

// Function to send a character to the LCD
void lcd_send_char(int fd, char c, uint8_t backlight) {
    uint8_t data[4];
    data[0] = (c & 0xF0) | backlight | 0x05; // Upper nibble with RS=1, EN=1
    data[1] = (c & 0xF0) | backlight | 0x01; // Upper nibble with RS=1, EN=0
    data[2] = ((c << 4) & 0xF0) | backlight | 0x05; // Lower nibble with RS=1, EN=1
    data[3] = ((c << 4) & 0xF0) | backlight | 0x01; // Lower nibble with RS=1, EN=0
    write(fd, data, 4);
    usleep(43);  // Character processing delay
}

// Function to send a string to the LCD
void lcd_send_text(int fd, const char *text, uint8_t backlight) {
    while (*text) {
        lcd_send_char(fd, *text++, backlight);
    }
}

// Function to initialize the LCD
void lcd_init(int fd, uint8_t backlight) {
    lcd_send_command(fd, 0x33, backlight);  // Initialize to 8-bit mode
    lcd_send_command(fd, 0x32, backlight);  // Switch to 4-bit mode
    lcd_send_command(fd, 0x28, backlight);  // Function Set: 4-bit, 2-line, 5x8 dots
    lcd_send_command(fd, 0x0C, backlight);  // Display ON, Cursor OFF
    lcd_send_command(fd, 0x06, backlight);  // Entry Mode: Increment, No Shift
    lcd_send_command(fd, 0x01, backlight);  // Clear Display
    usleep(2000);  // Wait for display to clear
}

// Function to display a loading animation
void display_loading_animation(int fd, const char *message, uint8_t backlight) {
    lcd_send_command(fd, 0x01, backlight);  // Clear the display
    lcd_send_text(fd, message, backlight); // Display the static message
    lcd_send_command(fd, 0xC0, backlight); // Move to the second line

    for (int i = 0; i < 16; i++) {
        lcd_send_command(fd, 0xC0, backlight); // Move to start of second line
        for (int j = 0; j < i; j++) {
            lcd_send_char(fd, ' ', backlight); // Fill spaces before the dot
        }
        lcd_send_char(fd, '.', backlight);  // Display the moving dot
        usleep(200000);  // Wait 200ms
    }

    usleep(500000);  // Small delay before clearing the display
}

// Function to display "Sevarino Morse Machine"
void display_startup_message(int fd) {
    display_loading_animation(fd, "Initializing...", 0x08);  // Loading animation
    lcd_send_command(fd, 0x01, 0x08);  // Clear the display
    lcd_send_text(fd, "Sevarino Morse", 0x08);  // First line
    lcd_send_command(fd, 0xC0, 0x08);           // Move to second line
    lcd_send_text(fd, "Machine", 0x08);         // Second line
}

// Function to display "Powering Down"
void display_shutdown_message(int fd) {
    lcd_send_command(fd, 0x01, 0x08);  // Clear display
    lcd_send_text(fd, "Powering Down", 0x08);
    display_loading_animation(fd, "Shutting Down...", 0x08);  // Loading animation
    lcd_send_command(fd, 0x08, 0x00);  // Turn off display and backlight
}

// Function to start the other programs
void start_programs() {
    printf("Starting programs...\n");
    system("sudo ./gpio_morse_interpreter &");  // Start Morse code interpreter
    system("sudo ./lcd_file_reader &");         // Start LCD file reader
}

// Function to stop the other programs
void stop_programs() {
    printf("Stopping programs...\n");
    system("sudo pkill -f gpio_morse_interpreter");
    system("sudo pkill -f lcd_file_reader");
}

int main() {
    int mem_fd;
    volatile unsigned int *gpio;

    // Open /dev/gpiomem for GPIO access
    mem_fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Failed to open /dev/gpiomem");
        return -1;
    }

    // Map GPIO memory
    gpio = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
    if (gpio == MAP_FAILED) {
        perror("Failed to map GPIO memory");
        close(mem_fd);
        return -1;
    }

    // Set GPIO 24 as input
    *(gpio + (GPIO_PIN / 10)) &= ~(7 << ((GPIO_PIN % 10) * 3));  // Clear FSEL bits for GPIO 24

    // Open I2C device for LCD
    int lcd_fd = open("/dev/i2c-1", O_RDWR);
    if (lcd_fd < 0) {
        perror("Failed to open I2C device");
        munmap((void *)gpio, BLOCK_SIZE);
        close(mem_fd);
        return -1;
    }

    // Set I2C slave address
    if (ioctl(lcd_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(lcd_fd);
        munmap((void *)gpio, BLOCK_SIZE);
        close(mem_fd);
        return -1;
    }

    int running = 0;  // State flag: 0 = programs off, 1 = programs running
    int prev_state = 1;  // Previous button state (1 = not pressed, 0 = pressed)

    printf("Monitoring GPIO 24. Press the button to toggle programs.\n");

    // Monitor GPIO 24
    while (1) {
        int curr_state = is_button_pressed(gpio);

        if (!curr_state && prev_state) {  // Button transition: HIGH -> LOW
            if (running) {
                display_shutdown_message(lcd_fd);
                stop_programs();
                running = 0;  // Update state
            } else {
                display_startup_message(lcd_fd);
                sleep(4);  // Wait for 4 seconds
                start_programs();
                running = 1;  // Update state
            }

            // Debounce delay
            usleep(500000);
        }

        prev_state = curr_state;  // Update previous state
        usleep(100000);  // Polling delay
    }

    // Cleanup
    close(lcd_fd);
    munmap((void *)gpio, BLOCK_SIZE);
    close(mem_fd);
    return 0;
}
