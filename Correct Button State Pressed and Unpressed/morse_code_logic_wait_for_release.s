.global morse_code_main
.extern report_init
.extern read_gpio_pin
.extern send_morse_signal
.extern delay_ms
.extern report_button_pressed
.extern report_button_unpressed

.section .text
morse_code_main:
    BL report_init           @ Initialize program

    MOV R4, #0               @ Previous button state (0: unpressed)
    MOV R5, #0               @ Counter for press duration

loop_forever:
    BL read_gpio_pin         @ Read GPIO pin state (1: pressed, 0: unpressed)
    MOV R3, R0               @ Current button state in R3

    CMP R4, R3               @ Compare current and previous state
    BEQ no_change            @ If no change, continue looping

    CMP R3, #1               @ Check if the button is pressed
    BEQ button_pressed       @ Handle button press

button_released:
    @ Button is released; determine duration
    CMP R5, #5               @ Compare duration counter to threshold
    BLT send_dot             @ Short press: send dot
    BGE send_dash            @ Long press: send dash

send_dot:
    MOV R0, #1               @ Signal 1 represents a dot
    BL send_morse_signal     @ Notify C program of a dot
    B reset_state

send_dash:
    MOV R0, #2               @ Signal 2 represents a dash
    BL send_morse_signal     @ Notify C program of a dash
    B reset_state

button_pressed:
    ADD R5, R5, #1           @ Increment press duration counter
    MOV R4, #1               @ Update previous button state to pressed
    BL report_button_pressed @ Log button pressed
    B loop_forever

reset_state:
    MOV R4, #0               @ Reset previous button state to unpressed
    MOV R5, #0               @ Reset press duration counter
    BL report_button_unpressed @ Log button released
    B loop_forever

no_change:
    BL delay_ms, #100        @ Delay for 100 ms to debounce
    B loop_forever
