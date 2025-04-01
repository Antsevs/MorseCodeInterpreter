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
    MOV R8, #0               @ Dash pending flag (0: no dash, 1: dash pending)

main_loop:
    BL read_gpio_pin         @ Read GPIO pin state (1: pressed, 0: unpressed)
    MOV R3, R0               @ Store current button state in R3

    CMP R4, R3               @ Compare current and previous state
    BEQ check_idle           @ If states are the same, check idle or gap detection

    CMP R3, #1               @ Check if the button is pressed
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
    MOV R8, #0               @ Reset dash pending flag
    B track_press

track_press:
    BL read_gpio_pin         @ Read GPIO pin state
    CMP R0, #0               @ Check if button is released
    BEQ button_released      @ If released, determine press type

    CMP R5, #10              @ Check if duration exceeds 0.5s (10 ticks at 50ms)
    BLT increment_press      @ If duration < 0.5s, continue incrementing

    MOV R8, #1               @ Set dash pending flag (duration >= 0.5s)
    B track_press            @ Continue tracking press

increment_press:
    ADD R5, R5, #1           @ Increment press duration counter
    MOV R0, #50              @ Add 50ms delay between checks (reduced from 100ms)
    BL delay_ms
    B track_press

button_released:
    CMP R8, #1               @ Check if a dash was pending
    BEQ register_dash        @ If yes, register a dash

    B register_dot           @ Otherwise, register a dot

register_dot:
    MOV R0, #1               @ Signal 1 represents a dot
    BL send_morse_signal     @ Notify C program of a dot
    B reset_idle

register_dash:
    MOV R0, #2               @ Signal 2 represents a dash
    BL send_morse_signal     @ Notify C program of a dash
    B reset_idle

reset_idle:
    MOV R4, #0               @ Set state to unpressed
    MOV R5, #0               @ Reset press duration counter
    MOV R8, #0               @ Reset dash pending flag
    B main_loop

check_gap:
    CMP R6, #40              @ Compare gap duration to 40 ticks (2s at 50ms)
    BLT increment_gap        @ If unpressed < 2s, increment gap counter

    MOV R7, #1               @ Set gap pending flag
    MOV R6, #0               @ Reset gap duration counter
    B main_loop              @ Wait for next button press to register the gap

increment_gap:
    ADD R6, R6, #1           @ Increment gap duration counter
    MOV R0, #50              @ Add 50ms delay between checks (reduced from 100ms)
    BL delay_ms
    B main_loop
