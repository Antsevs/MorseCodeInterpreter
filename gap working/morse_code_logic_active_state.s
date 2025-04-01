.global morse_code_main
.extern report_init
.extern read_gpio_pin
.extern send_morse_signal
.extern delay_ms

.section .text
morse_code_main:
    BL report_init           @ Initialize the Morse code interpreter

    MOV R4, #0               @ Previous button state (0: unpressed)
    MOV R5, #0               @ Press duration counter
    MOV R6, #0               @ Unpressed duration counter
    MOV R7, #0               @ Gap pending flag (0: no gap, 1: gap pending)

main_loop:
    BL read_gpio_pin         @ Read GPIO pin state (1: pressed, 0: unpressed)
    MOV R3, R0               @ Store current button state in R3

    CMP R4, R3               @ Compare current and previous state
    BEQ check_idle           @ If states are the same, check idle or gap detection

    CMP R3, #1               @ Check if button is pressed
    BEQ button_pressed       @ Handle button press

    CMP R4, #1               @ If button was previously pressed, handle release
    BEQ button_released

check_idle:
    CMP R4, #0               @ Check if button is idle
    BEQ check_gap            @ If unpressed, check gap

    B main_loop              @ Continue loop

button_pressed:
    CMP R7, #1               @ Check if a gap is pending
    BNE reset_press          @ If no gap pending, reset press tracking

    MOV R0, #3               @ Signal 3 represents a gap
    BL send_morse_signal     @ Notify C program of a gap
    MOV R7, #0               @ Clear gap pending flag
    B reset_press

reset_press:
    MOV R4, #1               @ Set state to pressed
    MOV R5, #0               @ Reset press duration counter
    MOV R6, #0               @ Reset gap duration counter
    B main_loop

button_released:
    CMP R5, #5               @ Compare press duration to 5 ticks (0.5s threshold)
    BLT send_dot             @ Short press (< 0.5s) is a dot
    BGE send_dash            @ Long press (>= 0.5s) is a dash

send_dot:
    MOV R0, #1               @ Signal 1 represents a dot
    BL send_morse_signal     @ Notify C program of a dot
    B reset_idle

send_dash:
    MOV R0, #2               @ Signal 2 represents a dash
    BL send_morse_signal     @ Notify C program of a dash
    B reset_idle

reset_idle:
    MOV R4, #0               @ Set state to unpressed
    MOV R5, #0               @ Reset press duration counter
    B main_loop

check_gap:
    CMP R6, #30              @ Compare gap duration to 30 ticks (3s threshold)
    BLT increment_gap        @ If unpressed < 3s, increment gap counter

    MOV R7, #1               @ Set gap pending flag
    MOV R6, #0               @ Reset gap duration counter
    B main_loop              @ Wait for next press to register the gap

increment_gap:
    ADD R6, R6, #1           @ Increment gap duration counter
    MOV R0, #100             @ Add 100ms delay between checks
    BL delay_ms
    B main_loop
