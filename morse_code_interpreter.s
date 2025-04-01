.global morse_code_main
.extern report_dot
.extern report_dash

.section .text
morse_code_main:
    LDR R0, =GPIO_BASE       @ Load GPIO base address
    ADD R1, R0, #0x34        @ Load GPLEV0 offset into R1 (used to read GPIO pin levels)

main_loop:
    BL check_button          @ Call function to check button press
    B main_loop              @ Repeat the loop indefinitely

check_button:
    LDR R2, [R1]             @ Load value from GPLEV0 register into R2
    MOV R3, #1               @ Prepare mask for GPIO 17
    LSL R3, R3, #17          @ Shift mask to the 17th bit position
    TST R2, R3               @ Test if bit 17 is set (button pressed)
    BNE button_pressed       @ If bit 17 is set, branch to button_pressed

    @ Button not pressed, return
    BX LR

button_pressed:
    BL delay_50ms            @ Wait 50ms to debounce the button press

    @ Check if button is still pressed (debouncing)
    LDR R2, [R1]             @ Load value from GPLEV0 register again
    TST R2, R3               @ Test if bit 17 is still set
    BEQ button_released      @ If not set, branch to button_released

    @ Button is still pressed, start counting press duration
    MOV R4, #0               @ Initialize counter

count_press:
    BL delay_50ms            @ Wait 50ms
    ADD R4, R4, #1           @ Increment counter
    LDR R2, [R1]             @ Read GPLEV0 register again
    TST R2, R3               @ Test if bit 17 is still set
    BNE count_press          @ If button is still pressed, continue counting

    @ Button released, check duration
button_released:
    CMP R4, #3               @ Compare counter to threshold (3 * 50ms = 150ms)
    BLS short_press          @ If counter <= 3, it's a short press (dot)
    B long_press             @ Otherwise, it's a long press (dash)

short_press:
    BL report_dot            @ Call C function to report a dot
    BX LR                    @ Return

long_press:
    BL report_dash           @ Call C function to report a dash
    BX LR                    @ Return

delay_50ms:
    LDR R7, =500000          @ Load 50 ms delay value (500000 microseconds) into R7
delay_loop:
    SUBS R7, R7, #1          @ Decrement R7
    BNE delay_loop           @ If not zero, continue loop
    BX LR                    @ Return from delay

.section .bss
    .comm morse_buffer, 1    @ Reserve 1 byte for Morse buffer

.section .data
GPIO_BASE:
    .word 0x3F200000         @ Base address for GPIO
