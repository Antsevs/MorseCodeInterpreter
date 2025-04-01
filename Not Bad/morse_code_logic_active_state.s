.global morse_code_main
.extern report_init
.extern read_gpio_pin
.extern send_morse_signal
.extern delay_ms
.extern report_button_pressed
.extern report_button_unpressed

.section .text
morse_code_main:
    BL report_init           @ Initialize the Morse code interpreter

    MOV R4, #0               @ Previous button state (0: unpressed)
    MOV R5, #0               @ Counter for press duration

loop_forever:
    BL read_gpio_pin         @ Read GPIO pin state (1: pressed, 0: unpressed)
    MOV R3, R0               @ Store current button state in R3

    CMP R4, R3               @ Compare current and previous state
    BEQ continue_press       @ If states are the same, continue counting if pressed

    CMP R3, #1               @ Check if the button is pressed
    BEQ button_pressed       @ If pressed, branch to button_pressed

button_released:
    @ Button released: determine duration
    CMP R5, #5               @ Compare duration counter to threshold (5 ticks = ~0.5 seconds)
    BLT send_dot             @ Short press (< 5 ticks) is a dot
    BGE send_dash            @ Long press (>= 5 ticks) is a dash

send_dot:
    MOV R0, #1               @ Signal 1 represents a dot
    BL send_morse_signal     @ Notify C program of a dot
    B reset_state

send_dash:
    MOV R0, #2               @ Signal 2 represents a dash
    BL send_morse_signal     @ Notify C program of a dash
    B reset_state

button_pressed:
    MOV R4, #1               @ Update previous button state to pressed
    MOV R5, #0               @ Reset duration counter (start counting)
    BL report_button_unpressed @ Log "Button Unpressed" instead of "Pressed"
    @ Add debounce delay
    MOV R0, #100             @ 100ms delay to debounce the press
    BL delay_ms
    B loop_forever           @ Continue looping

continue_press:
    CMP R3, #1               @ Check if button is still pressed
    BNE reset_state          @ If not pressed, reset
    ADD R5, R5, #1           @ Increment press duration counter

    @ Delay for ~100ms between checks
    MOV R0, #100             @ Load 100 into R0 as the delay value
    BL delay_ms              @ Call delay function
    B loop_forever           @ Loop back to continue counting duration

reset_state:
    MOV R4, #0               @ Reset previous button state to unpressed
    MOV R5, #0               @ Reset press duration counter
    BL report_button_pressed @ Log "Button Pressed" instead of "Unpressed"
    @ Add debounce delay
    MOV R0, #100             @ 100ms delay to debounce the release
    BL delay_ms
    B loop_forever           @ Continue looping
