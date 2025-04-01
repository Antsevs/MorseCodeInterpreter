.global morse_code_main
.extern report_init
.extern read_gpio_pin
.extern send_message_to_lcd
.extern delay_ms

.section .text
morse_code_main:
    BL report_init           @ Call C function to report successful initialization

    MOV R4, #0               @ Initialize R4 as the previous button state (0: not pressed)

loop_forever:
    BL read_gpio_pin         @ Call C function to read GPIO 17 state
    MOV R3, R0               @ Move the current button state into R3

    CMP R4, #0               @ Check if the previous state was not pressed (0)
    CMPNE R3, #1             @ Check if the current state is pressed (1)
    BEQ button_pressed       @ If previous state was 0 and current state is 1, button is pressed

    MOV R4, R3               @ Update previous button state with current state

    B loop_forever           @ Continue looping

button_pressed:
    BL send_message_to_lcd   @ Call C function to send a message to the I2C LCD module

    MOV R4, #1               @ Update previous button state to pressed (1)

    @ Wait until the button is released to avoid retriggering
wait_for_release:
    BL read_gpio_pin         @ Read GPIO pin state again
    CMP R0, #0               @ Check if the button is released (0)
    BNE wait_for_release     @ If not released, keep waiting

    @ Add debounce delay to prevent bouncing effects after release
    MOV R0, #200             @ Set delay value to 200 ms for debounce
    BL delay_ms              @ Call delay function

    B loop_forever           @ Return to main loop after waiting for release
