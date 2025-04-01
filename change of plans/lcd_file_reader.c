#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>  // For uint8_t
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define I2C_ADDR 0x27  // I2C address for the LCD
#define BACKLIGHT 0x08 // Control bit for backlight

// Function declarations
void lcd_send_command(int fd, uint8_t command);
void lcd_send_char(int fd, char c);
void lcd_send_text(int fd, const char *text);
void lcd_init(int fd);

// Function to send a command to the LCD
void lcd_send_command(int fd, uint8_t command) {
    uint8_t data[4];
    data[0] = (command & 0xF0) | BACKLIGHT | 0x04; // Upper nibble with EN=1
    data[1] = (command & 0xF0) | BACKLIGHT;       // Upper nibble with EN=0
    data[2] = ((command << 4) & 0xF0) | BACKLIGHT | 0x04; // Lower nibble with EN=1
    data[3] = ((command << 4) & 0xF0) | BACKLIGHT;       // Lower nibble with EN=0
    if (write(fd, data, 4) != 4) {
        perror("Failed to send command to LCD");
    }
    usleep(2000); // Delay to allow command processing
}

// Function to send a single character to the LCD
void lcd_send_char(int fd, char c) {
    uint8_t data[4];
    data[0] = (c & 0xF0) | BACKLIGHT | 0x05; // Upper nibble with RS=1, EN=1
    data[1] = (c & 0xF0) | BACKLIGHT | 0x01; // Upper nibble with RS=1, EN=0
    data[2] = ((c << 4) & 0xF0) | BACKLIGHT | 0x05; // Lower nibble with RS=1, EN=1
    data[3] = ((c << 4) & 0xF0) | BACKLIGHT | 0x01; // Lower nibble with RS=1, EN=0
    if (write(fd, data, 4) != 4) {
        perror("Failed to send character to LCD");
    }
    usleep(43); // Delay to allow character processing
}

// Function to send a string to the LCD
void lcd_send_text(int fd, const char *text) {
    while (*text) {
        lcd_send_char(fd, *text++);
    }
}

// Function to initialize the LCD
void lcd_init(int fd) {
    // Initialization sequence as per the provided instruction set
    lcd_send_command(fd, 0x33);  // Initialize to 8-bit mode
    lcd_send_command(fd, 0x32);  // Switch to 4-bit mode
    lcd_send_command(fd, 0x28);  // Function Set: 4-bit, 2-line, 5x8 dots
    lcd_send_command(fd, 0x0C);  // Display ON, Cursor OFF
    lcd_send_command(fd, 0x06);  // Entry Mode: Increment, No Shift
    lcd_send_command(fd, 0x01);  // Clear Display
    usleep(2000);                // Wait for display to clear
}

// Function to read and display file content on the LCD
void read_and_display_file(const char *filename, int lcd_fd) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    lcd_send_command(lcd_fd, 0x01);  // Clear the LCD before displaying

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        lcd_send_text(lcd_fd, line);  // Display the line on the LCD
        usleep(2000000);              // Wait 2 seconds before clearing
    }

    fclose(file);

    // Optionally, clear the file after reading
    file = fopen(filename, "w");
    if (file) fclose(file);
}

int main() {
    const char *filename = "morse_output.txt";

    // Open I2C device
    int lcd_fd = open("/dev/i2c-1", O_RDWR);
    if (lcd_fd < 0) {
        perror("Failed to open I2C device");
        return -1;
    }

    // Set I2C slave address
    if (ioctl(lcd_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(lcd_fd);
        return -1;
    }

    // Initialize the LCD
    lcd_init(lcd_fd);

    // Continuously check and display file content
    while (1) {
        read_and_display_file(filename, lcd_fd);
        usleep(500000);  // Check the file every 0.5 seconds
    }

    close(lcd_fd);
    return 0;
}
