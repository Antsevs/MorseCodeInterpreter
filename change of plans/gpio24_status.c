#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#define GPIO_BASE 0x3F200000  // Base address for GPIO (Raspberry Pi 3/4)
#define BLOCK_SIZE (4 * 1024)
#define GPIO_PIN 24  // GPIO pin to monitor

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

    printf("Monitoring GPIO 24 status. Press the button to see changes.\n");

    // Monitor GPIO 24 status
    while (1) {
        unsigned int level = *(gpio + 13);  // GPLEV0 register (offset 0x34 / 4 bytes)
        int status = (level & (1 << GPIO_PIN)) != 0;  // Check if GPIO 24 is high
        printf("GPIO 24 status: %s\n", status ? "PRESSED" : "NOT PRESSED");
        usleep(100000);  // Polling delay
    }

    // Cleanup
    munmap((void *)gpio, BLOCK_SIZE);
    close(mem_fd);
    return 0;
}
