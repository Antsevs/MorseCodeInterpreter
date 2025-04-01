.section .text
.global _start

_start:

    @ I2C Base Address for Raspberry Pi 3
    .equ I2C_BASE, 0x3F804000

    @ I2C Register Offsets
    .equ I2C_C, I2C_BASE + 0x00  @ Control register
    .equ I2C_S, I2C_BASE + 0x04  @ Status register
    .equ I2C_DLEN, I2C_BASE + 0x08 @ Data length register
    .equ I2C_A, I2C_BASE + 0x0C  @ Slave address register
    .equ I2C_FIFO, I2C_BASE + 0x10 @ FIFO data register

    @ LCD I2C Address
    .equ LCD_ADDR, 0x27

    @ Set I2C Address and Data Length to Send One Byte
    LDR R0, =I2C_BASE
    MOV R1, #LCD_ADDR
    STR R1, [R0, #0x0C]           @ Set the slave address register
    MOV R1, #1
    STR R1, [R0, #0x08]           @ Set data length to 1

    @ Write a Command to the LCD FIFO
    MOV R1, #0x33                 @ Initialization byte for LCD
    STR R1, [R0, #0x10]           @ Write the byte to the FIFO

    @ Start I2C Transfer
    MOV R1, #0x8080               @ Start the I2C transfer
    STR R1, [R0, #0x00]           @ Write to the control register

wait_transfer:
    LDR R1, [R0, #0x04]           @ Read the status register
    TST R1, #0x2                  @ Check if the DONE bit is set
    BEQ wait_transfer             @ Wait if not done

success:
    B success                     @ If successful, loop infinitely

error:
    B error                       @ Loop here if there’s a segmentation fault
