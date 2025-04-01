.global morse_code_main
.extern report_init
.extern read_gpio_pin
.extern send_message_to_lcd

.section .text
morse_code_main:
    BL report_init           @ Call C function to report successful initialization

loop_forever:
    BL read_gpio_pin         @ Call C function to read GPIO 17 state
    CMP R0, #1               @ Compare returned value with 1 (check if button is pressed)
    BEQ button_pressed       @ If button is pressed, branch to button_pressed

    B loop_forever           @ Continue looping if button is not pressed

button_pressed:
    BL send_message_to_lcd   @ Call C function to send a message to the I2C LCD module

    B loop_forever           @ Return to main loop after sending the message
