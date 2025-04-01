.global morse_code_main
.extern report_dot

.section .text
morse_code_main:
    LDR R0, =GPIO_BASE       @ Load GPIO base address
    ADD R1, R0, #0x34        @ Load GPLEV0 offset into R1 (used to read GPIO pin levels)

main_loop:
    LDR R2, [R1]             @ Load value from GPLEV0 register into R2
    MOV R3, #1               @ Prepare mask for GPIO 17
    LSL R3, R3, #17          @ Shift mask to the 17th bit position
    TST R2, R3               @ Test if bit 17 is set (button pressed)
    BNE button_pressed       @ If bit 17 is set, branch to button_pressed

    B main_loop              @ Repeat the loop if button is not pressed

button_pressed:
    BL report_dot            @ Call C function to report a dot
    B main_loop              @ Return to main loop
