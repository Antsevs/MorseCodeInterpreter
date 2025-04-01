// Morse Code Interpreter using ARM Assembly on Raspberry Pi
// Hardware: Raspberry Pi, LED Screen, Button, Breadboard, Connectors

// Setting up GPIOs
.equ GPIO_BASE, 0x3F200000  // GPIO base address (change according to Pi version)
.equ BUTTON_PIN_NUMBER, 17  // GPIO pin number for button
.equ SCREEN_PIN_NUMBER, 27  // GPIO pin number for LED screen
.equ GPIO_FSEL_OFFSET, 0x00 // GPIO function select offset
.equ GPIO_LEV_OFFSET, 0x34  // GPIO pin level offset
.equ SYST_CLO, 0x3F003004
.equ DEBOUNCE_DELAY, 10000  // Debounce delay in microseconds

.section .text
.global _start

_start:
    // Initialize GPIOs for button input and screen output
    LDR R0, =GPIO_BASE

    // Set GPIO pin for button (input mode)
    LDR R1, =1 << BUTTON_PIN_NUMBER
    STR R1, [R0, #GPIO_FSEL_OFFSET]  // Set pin as input

    // Set GPIO pins for LED screen (output mode)
    LDR R1, =1 << SCREEN_PIN_NUMBER
    STR R1, [R0, #GPIO_FSEL_OFFSET]  // Set pin as output

    // Main loop to read button input and interpret Morse code
loop:
    // Read button state
    LDR R1, [R0, #GPIO_LEV_OFFSET]   // Load button pin level
    TST R1, #1 << BUTTON_PIN_NUMBER  // Test button state
    BEQ button_not_pressed           // If button not pressed, skip

    // Button is pressed - record dot or dash based on press duration
    BL record_morse_signal

button_not_pressed:
    // Check for end of character (button released and pause detected)
    BL check_end_of_character

    // Display result if character is complete
    BL display_character

    // Repeat loop
    B loop

// Function to record a dot or dash based on button press duration
record_morse_signal:
    // Debounce the button press
    BL debounce_button

    // Record start time
    LDR R2, =SYST_CLO           // System timer lower 32 bits
    LDR R3, [R2]

wait_for_release:
    // Wait until button is released
    LDR R1, [R0, #GPIO_LEV_OFFSET]
    TST R1, #1 << BUTTON_PIN_NUMBER
    BNE wait_for_release

    // Record end time
    LDR R4, [R2]
    SUB R5, R4, R3              // Calculate press duration

    // Determine if dot or dash (threshold = 500ms)
    LDR R6, =500000             // Threshold for dot/dash in microseconds
    CMP R5, R6
    BLT record_dot
    B record_dash

record_dot:
    // Add dot to Morse sequence buffer
    LDR R7, =morse_buffer
    MOV R0, #48 // ASCII for '0'
    LDR R1, =morse_index
    LDR R2, [R1]
    STRB R0, [R7, R2]
    LDR R0, =morse_index
    LDR R1, [R0]
    ADD R1, R1, #1
    STR R1, [R0]
    BX LR

record_dash:
    // Add dash to Morse sequence buffer
    LDR R7, =morse_buffer
    MOV R0, #49 // ASCII for '1'
    LDR R1, =morse_index
    LDR R2, [R1]
    STRB R0, [R7, R2]
    LDR R1, =morse_index
    LDR R2, [R1]
    ADD R2, R2, #1
    STR R2, [R1]
    BX LR

// Function to debounce the button press
debounce_button:
    // Simple delay loop for debouncing
    LDR R1, =DEBOUNCE_DELAY

debounce_loop:
    SUBS R1, R1, #1
    BNE debounce_loop
    BX LR

// Function to check if a character has ended
check_end_of_character:
    // Record start time
    LDR R2, =SYST_CLO
    LDR R3, [R2]

wait_for_next_press:
    // Wait for next button press or timeout
    LDR R1, [R0, #GPIO_LEV_OFFSET]
    TST R1, #1 << BUTTON_PIN_NUMBER
    BNE character_not_ended

    // Check elapsed time
    LDR R4, [R2]
    SUB R5, R4, R3
    LDR R6, =1500000            // Threshold for character end in microseconds
    CMP R5, R6
    BLT wait_for_next_press

    // Character has ended
    LDR R0, =character_complete
    MOV R1, #1
    STR R1, [R0]
    BX LR

character_not_ended:
    LDR R0, =character_complete
    MOV R1, #0
    STR R1, [R0]
    BX LR

// Function to display the interpreted character on the LED screen
display_character:
    // Check if character is complete
    LDR R0, =character_complete
    LDR R1, [R0]
    CMP R1, #1
    BNE display_character_exit

    // Translate Morse sequence to character
    LDR R7, =morse_buffer
    LDR R8, =morse_table

translate_loop:
    // Compare Morse sequence with table entries
    LDRB R9, [R7]
    LDRB R10, [R8]
    CMP R9, R10
    BNE next_entry

    // If match found, send character to screen
    LDR R11, [R8, #4]           // Load corresponding ASCII character
    BL send_to_screen
    B display_character_exit

next_entry:
    ADD R8, R8, #5              // Move to next entry in table
    LDR R0, =morse_table_end
    CMP R8, R0
    BNE translate_loop

send_to_screen:
    // Send character to LED screen (implementation depends on screen type)
    // Placeholder for screen output logic
    BX LR

display_character_exit:
    // Clear Morse buffer and reset index
    LDR R7, =morse_buffer
    MOV R8, #0
    STRB R8, [R7]
    LDR R0, =morse_index
    MOV R1, #0
    STR R1, [R0]
    BX LR

.section .data
// Define Morse code table and any needed data here
morse_buffer: .space 10         // Buffer to store Morse code (dots and dashes)
morse_index: .word 0            // Index for Morse buffer
character_complete: .word 0     // Flag to indicate end of character

// Morse code table (each entry is 4 bytes for Morse + 1 byte for character)
morse_table:
    .asciz "01A"  // .- A
    .asciz "100B" // -... B
    .asciz "101C" // -.-. C
    // ... (add remaining Morse codes)
morse_table_end:
