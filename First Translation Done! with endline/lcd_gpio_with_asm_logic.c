#include <stdio.h>
#include <string.h>
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

// Morse code translation table
typedef struct {
    char *morse;
    char letter;
} MorseCode;

MorseCode morse_table[] = {
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, 
    {".", 'E'},    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'},
    {"..", 'I'},   {".---", 'J'}, {"-.-", 'K'},  {".-..", 'L'},
    {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},  {".--.", 'P'},
    {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'},
    {"-.--", 'Y'}, {"--..", 'Z'}, {NULL, '\0'}
};

// Morse code buffer
#define BUFFER_SIZE 64
char morse_buffer[BUFFER_SIZE];
int buffer_index = 0;

// English text buffer
#define TEXT_BUFFER_SIZE 256
char text_buffer[TEXT_BUFFER_SIZE];
int text_index = 0;

// Endline counter
int endline_counter = 0;

// Delay function in milliseconds
void delay_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Function to send strings to the I2C LCD
void lcd_send_string(const char *str) {
    int fd = open("/dev/i2c-1", O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        return;
    }

    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(fd);
        return;
    }

    while (*str) {
        char buf[2] = {0x80, *str++};  // 0x80 for sending a character
        if (write(fd, buf, 2) != 2) {
            perror("Failed to write to LCD");
            break;
        }
    }

    close(fd);
}

// Function to translate Morse code to English
char translate_morse_to_english(const char *morse) {
    for (int i = 0; morse_table[i].morse != NULL; i++) {
        if (strcmp(morse, morse_table[i].morse) == 0) {
            return morse_table[i].letter;
        }
    }
    return '?';  // Return '?' if Morse code is invalid
}

// Function to process Morse signals
void send_morse_signal(int signal) {
    if (signal == 1) {  // Dot
        printf("Dot (.) received.\n");
        morse_buffer[buffer_index++] = '.';
        endline_counter++;
        
        if (endline_counter >= 10) {  // Check for endline condition
            printf("Endline detected! Printing translated characters: %s\n", text_buffer);
            text_buffer[text_index] = '\0';  // Null-terminate the text buffer
            printf("%s\n", text_buffer);    // Print the translated text

            lcd_send_string(text_buffer);   // Send text buffer to the LCD

            text_index = 0;                 // Reset text buffer
            endline_counter = 0;            // Reset endline counter
        }
    } else if (signal == 2) {  // Dash
        printf("Dash (-) received.\n");
        morse_buffer[buffer_index++] = '-';
        endline_counter = 0;  // Reset endline counter on non-dot
    } else if (signal == 3) {  // Gap
        printf("Gap detected (translating to English).\n");
        morse_buffer[buffer_index] = '\0';  // Null-terminate the Morse code
        char translated = translate_morse_to_english(morse_buffer);
        printf("Translated: %c\n", translated);

        // Add translated character to text buffer
        if (text_index < TEXT_BUFFER_SIZE - 1) {
            text_buffer[text_index++] = translated;
        }

        buffer_index = 0;  // Reset Morse code buffer
        endline_counter = 0;  // Reset endline counter on gap
    }
    fflush(stdout);
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
