// Simple Program to Toggle LED to Simulate Displaying 'anthony'
// Hardware: Raspberry Pi, LED Screen, Button, Breadboard, Connectors

// Setting up GPIOs
.equ GPIO_BASE, 0x3F200000  // GPIO base address (change according to Pi version)
.equ SCREEN_PIN_NUMBER, 27  // GPIO pin number for LED screen
.equ GPIO_FSEL_OFFSET, 0x00 // GPIO function select offset
.equ GPIO_SET_OFFSET, 0x1C  // GPIO pin output set offset
.equ GPIO_CLR_OFFSET, 0x28  // GPIO pin output clear offset

.section .text
.global _start

_start:
    // Initialize GPIO for LED screen output
    LDR R0, =GPIO_BASE
    LDR R1, =1 << SCREEN_PIN_NUMBER
    STR R1, [R0, #GPIO_FSEL_OFFSET]  // Set pin as output

    // Toggle the LED to simulate displaying 'anthony'
    LDR R2, =6  // Number of characters in 'anthony'

loop_display:
    // Set pin high
    LDR R0, =GPIO_BASE
    LDR R1, =1 << SCREEN_PIN_NUMBER
    STR R1, [R0, #GPIO_SET_OFFSET]
    BL delay

    // Set pin low
    STR R1, [R0, #GPIO_CLR_OFFSET]
    BL delay

    // Decrement counter and repeat
    SUBS R2, R2, #1
    BNE loop_display

loop:
    // Infinite loop to keep the program running
    B loop

// Simple delay function
delay:
    MOV R3, #65535  // Reduced delay value to fit within valid range  // Arbitrary delay value
wait:
    SUBS R3, R3, #1
    BNE wait
    BX LR

.section .data
