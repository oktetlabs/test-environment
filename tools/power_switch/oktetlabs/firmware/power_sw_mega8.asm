;   ATmega16 controller
.include "m8def.inc"

;   Definitions
;

;   Device type and properties
.equ    POWER_SW_TYPE       =   '1'     ; Power switch with single
                                        ; byte command (no command
                                        ; parser).
.equ    POWER_SW_N_LINES    =   16      ; Power switch with sixteen
                                        ; servicable lines
.equ    RESET_SUPPORT       =   1       ; Command
                                        ; 'reset line X' supported

;   Device signature bytes
.equ    SIGN_BYTE_1     =   POWER_SW_TYPE
.equ    SIGN_BYTE_2     =   (1<<6) + (RESET_SUPPORT<<5) + POWER_SW_N_LINES
.equ    SIGN_BYTE_3     =   '0'

;   ASCII symbols
.equ    CR_SYMBOL           =   0x0d    ; Provokes command execution
                                        ; when received.
.equ    SPACE_SYMBOL        =   ' '     ; To detect received byte
                                        ; be special ASCII symbol or not.
.equ    NACK_SYMBOL         =   '!'     ; Transmitted on reply to
                                        ; invalid command.
.equ    ACK_SYMBOL          =   '#'     ; Transmitted on reply to
                                        ; valid command.


;   Command byte binary format
;
;   010x<line number 0000-1111> - command 'turn line X OFF'
;   0110<line number 0000-1111> - command 'turn line X ON'
;   0111<line number 0000-1111> - command 'reset line X'

;   Command byte format exceptions
.equ    EMPTY_COMMAND       =   0       ; Command 'do nothing'
.equ    TR_SIGN_COMMAND     =   '$'     ; Command 'transmit signature'

;   1.2 Line number mask
.equ    LINE_NUMBER_MASK    =   0x0f    ; Mask '00001111' to extract
                                        ; line number from command
                                        ; byte

;   1.3 Command bits
.equ    RESET_LINE          =   4       ; = 1 in 'reset line X' command
                                        ; = 0 in 'turn line X ON' command
                                        ; = x in 'turn line X OFF' command
.equ    TURN_LINE_ON        =   5       ; = 1 in 'turn line X ON' and
                                        ; 'reset line X' commands
                                        ; = 0 in 'turn line X OFF' command
.equ    RESERVED_BIT_1      =   6       ; Must be 1
.equ    RESERVED_BIT_2      =   7       ; Must be 0

;   Line drive commands
.equ    TURN_OFF            =   0       ; Line OFF
.equ    RESET_START         =   1       ; Line reset start
.equ    TURN_ON             =   4       ; Line ON

;   2   Buffers and indeces definitions
;
;   2.1 Length of UART RX and TX buffers
.equ    RX_BUFLEN                     = 32      ; Length of receive buffer
.equ    TX_BUFLEN                     = 8       ; Length of transmit buffer
.equ    RCV_BUF_IDX_MASK              = 0x0f    ; Masks to calculate proper
.equ    SND_BUF_IDX_MASK              = 0x07    ; indeces in RX and TX buffers

;   2.2 Initial values of RX and TX buffer top and bottom pointers
.equ    RX_TOP_INIT                   = 0
.equ    RX_BOT_INIT                   = 0
.equ    TX_TOP_INIT                   = 0
.equ    TX_BOT_INIT                   = 0

;   Register definition
;
;   1   Registers which do not support operations with immediate
;       values
;
;   Register to save status
.def    STATUS_REG                      =   r1
;   Register used in routines to return value
.def    RETVAL_REG                      =   r2
;
;   Registers which support operations with immediate values
;   Register used to pass arguments
.def    ARG_REG                         =   r16
;   Register used to pass one-byte command to command processor
.def    CMD_REG                         =   r17
;   Routines' local registers
.def    LOC_REG_1                       =   r18
.def    LOC_REG_2                       =   r19
.def    LOC_REG_3                       =   r20
.def    LOC_REG_4                       =   r21
;   Register to keep top index in RX buffer
.def    RX_TOP_IDX                      =   r22
;   Register to keep bottom index in RX buffer
.def    RX_BOT_IDX                      =   r23
;   Register to keep top index in TX buffer
.def    TX_TOP_IDX                      =   r24
;   Register to keep bottom index in TX buffer
.def    TX_BOT_IDX                      =   r25

; Data segment (in SRAM).
.dseg
; USART receive buffer
RX_BUF:             .BYTE   RX_BUFLEN
; USART transmit buffer
TX_BUF:             .BYTE   TX_BUFLEN
; Lines commands array
LINE_COMMANDS:      .BYTE   POWER_SW_N_LINES

; Code segment (in Flash).
.cseg
.org    0x0000
        rjmp    BOOT        ; boot/reboot
.org    OVF1addr            ;Overflow1 Interrupt Vector Address
        rjmp    TIMER_ISR
.org    URXCaddr            ;UART Receive Complete Interrupt Vector Address
        rjmp    USART_RX
.org    UDREaddr            ;UART Data Register Empty Interrupt Vector Address
        rjmp    USART_TX

;   Constants

;   Device signature
SIGNATURE: .DB SIGN_BYTE_1, SIGN_BYTE_2, SIGN_BYTE_3, 0x00

;   ISRs
BOOT:
    ; Set stack pointer to the end of the RAM
    ldi     LOC_REG_1,  low(RAMEND)
    out     spl,        LOC_REG_1
    ldi     LOC_REG_1,  high(RAMEND)
    out     sph,        LOC_REG_1
    ; Set power lines in ports B, C and D to output mode
    sbi     DDRB, 0
    sbi     DDRB, 1
    sbi     DDRB, 2
    sbi     DDRB, 3
    sbi     DDRB, 4
    sbi     DDRB, 5

    sbi     DDRC, 0
    sbi     DDRC, 1
    sbi     DDRC, 2
    sbi     DDRC, 3
    sbi     DDRC, 4
    sbi     DDRC, 5

    sbi     DDRD, 2
    sbi     DDRD, 3
    sbi     DDRD, 4
    sbi     DDRD, 5
    sbi     DDRD, 6
    sbi     DDRD, 7
    ; Initialize power line commands array
    ldi     XL,         low(LINE_COMMANDS)
    ldi     XH,         high(LINE_COMMANDS)
    ldi     LOC_REG_1,  TURN_ON
    clr     LOC_REG_2
init_line_command:
    st      X+,         LOC_REG_1
    inc     LOC_REG_2
    cpi     LOC_REG_2,  POWER_SW_N_LINES
    brmi init_line_command
    ; Initialize USART
    rcall    INIT_USART
    ; Initialize timer
    rcall    INIT_TIMER
    ; Enable interrupts
    sei
    ; Call main routine
    rcall    MAIN

; USART receive interrupt routine
USART_RX:
    push    LOC_REG_1
    push    LOC_REG_2
    push    XL
    push    XH
    push    STATUS_REG
    in      STATUS_REG,         SREG
    ; Receive byte and save it on the top of RX buffer
    in      LOC_REG_1,          UDR
    ldi     XL,                 low(RX_BUF)
    ldi     XH,                 high(RX_BUF)
    clr     LOC_REG_2
    add     XL,                 RX_TOP_IDX
    adc     XH,                 LOC_REG_2
    st      X,                  LOC_REG_1
    ; Modify index of the top of RX buffer
    inc     RX_TOP_IDX
    andi    RX_TOP_IDX,         RCV_BUF_IDX_MASK
    cpse    RX_TOP_IDX,         RX_BOT_IDX
    ; modified top != bottom, OK, we may finish now.
    rjmp    usart_rx_return
    ; modified top == bottom, BUMS! we have to rollback top pointer.
    dec     RX_TOP_IDX
    andi    RX_TOP_IDX,         RCV_BUF_IDX_MASK
usart_rx_return:
    ; Restore saved flags and return
    out     SREG,               STATUS_REG
    pop     STATUS_REG
    pop     XH
    pop     XL
    pop     LOC_REG_2
    pop     LOC_REG_1
    reti

; USART transmit interrupt routine
USART_TX:
    push    LOC_REG_1
    push    LOC_REG_2
    push    XL
    push    XH
    push    STATUS_REG
    in      STATUS_REG,         SREG

    ; Check that transmit buffer is not empty
    cp      TX_BOT_IDX,         TX_TOP_IDX
    ; TX buffer is not empty, continue execution from 'mov_tx_byte'
    brne    move_tx_byte
    ; TX buffer is empty, disable UDR interrupt and return
    cbi     UCSRB,              UDRIE
    rjmp    usart_tx_return
move_tx_byte:
    ; Move byte from the bottom of transmit buffer to the transmit register
    ldi     XL,                 low(TX_BUF)
    ldi     XH,                 high(TX_BUF)
    clr     LOC_REG_2
    add     XL,                 TX_BOT_IDX
    adc     XH,                 LOC_REG_2
    ld      LOC_REG_1,          X
    out     UDR,                LOC_REG_1
    ; Modify pointer to the bottom of transmit buffer
    inc     TX_BOT_IDX
    andi    TX_BOT_IDX,         SND_BUF_IDX_MASK
usart_tx_return:
    ; Restore saved flags and return
    out     SREG,               STATUS_REG
    pop     STATUS_REG
    pop     XH
    pop     XL
    pop     LOC_REG_2
    pop     LOC_REG_1
    reti

; Timer ISR
TIMER_ISR:
    push    LOC_REG_1
    push    LOC_REG_2
    push    LOC_REG_3
    push    XL
    push    XH
    push    STATUS_REG
    in      STATUS_REG, SREG

    clr     LOC_REG_2
    clr     LOC_REG_3
    ldi     XL,         low(LINE_COMMANDS + POWER_SW_N_LINES)
    ldi     XH,         high(LINE_COMMANDS + POWER_SW_N_LINES)

get_line_command:
    ld      LOC_REG_1,  -X
    cpi     LOC_REG_1,  TURN_OFF
    breq    line_off
    cpi     LOC_REG_1,  TURN_ON
    breq    line_on
    inc     LOC_REG_1
    st      X,          LOC_REG_1
line_off:
    clc
    rjmp    modify_line
line_on:
    sec
modify_line:
    rol     LOC_REG_2
    rol     LOC_REG_3
    cpi     XH,         high(LINE_COMMANDS)
    brne get_line_command
    cpi     XL,         low(LINE_COMMANDS)
    brne get_line_command

    mov     LOC_REG_4,  LOC_REG_2
    andi    LOC_REG_4,  0xff>>2
    out     PORTB,      LOC_REG_4

    mov     LOC_REG_4,  LOC_REG_3
    andi    LOC_REG_4,  0x0f
    rol     LOC_REG_2
    rol     LOC_REG_4
    rol     LOC_REG_2
    rol     LOC_REG_4
    out     PORTC,      LOC_REG_4

    mov     LOC_REG_4,  LOC_REG_3
    andi    LOC_REG_4,  0xf0
    clc
    ror     LOC_REG_4
    ror     LOC_REG_4
    out     PORTD,      LOC_REG_4

    out     SREG, STATUS_REG
    pop     STATUS_REG
    pop     XH
    pop     XL
    pop     LOC_REG_3
    pop     LOC_REG_2
    pop     LOC_REG_1
    reti

; Initialize USART
INIT_USART:
    ; Baud rate
    ; Clock 16 MHz, baud rate = 115200, error -3.5%
    ldi     LOC_REG_1,  8
    out     UBRRL,      LOC_REG_1
    ldi     LOC_REG_1,  0
    out     UBRRH,      LOC_REG_1
    ; Initialize top and bottom indeces in RX buffer
    ldi     RX_TOP_IDX, RX_TOP_INIT
    ldi     RX_BOT_IDX, RX_BOT_INIT
    ; Initialize top and bottom indeces in TX buffer
    ldi     TX_TOP_IDX, TX_TOP_INIT
    ldi     TX_BOT_IDX, TX_BOT_INIT
    ; UCSRB register
    ldi     LOC_REG_1,  (1<<RXEN)|(1<<TXEN)|(1<<RXCIE)
    out     UCSRB,      LOC_REG_1
    ; UCSRC register
    ldi     LOC_REG_1,  (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)
    out     UCSRC,      LOC_REG_1
    ret

INIT_TIMER:
    ;   Timer 1 (16 bit). Normal mode. Clock 16 Mhz, prescaler 64,
    ;   overflow interrupt enabled. Interrupt period ~0.26 sec.
    ldi     LOC_REG_1,  1<<CS11 + 1<<CS10
    out     TCCR1B,     LOC_REG_1
    ldi     LOC_REG_1,  1<<TOIE1
    out     TIMSK,      LOC_REG_1
    ret

; Main routine
MAIN:
    ; 1) Cleanup command register
    clr     CMD_REG
main_receive_byte:
    ; 2) Receive byte
    rcall    RECEIVE_BYTE
    mov     LOC_REG_1,      RETVAL_REG
    ; 3) Process received byte
    ; 3.1) Is byte CR symbol?
    cpi     LOC_REG_1,      CR_SYMBOL
    breq    main_process_command    ; 'CR', process command
    ; 3.2) Is byte special ASCII symbol?
    cpi     LOC_REG_1,      SPACE_SYMBOL
    brlo    main_receive_byte       ; Special ASCII, receive new byte
    ; 3.3) Byte is command.
    ; Move command byte into command register
    mov     CMD_REG,        LOC_REG_1
    ; Return command byte to sender
    mov     ARG_REG,        LOC_REG_1
    rcall    TRANSMIT_BYTE
    ; Wait CR symbol to execute received command or
    ; new command symbol to overwrite previous one
    rjmp    main_receive_byte
main_process_command:
    rcall    PROCESS_COMMAND
    rjmp    MAIN

; Process command byte
PROCESS_COMMAND:
    push    LOC_REG_1
    push    LOC_REG_2
    push    LOC_REG_3
    ;
    ;   1)      Process command byte
    ;
    ;   1.1)    Exceptions
    ;   1.1.1)  Exception 1: 0x00 (EMPTY_COMMAND) - do nothing.
    cpi     CMD_REG,            EMPTY_COMMAND
    breq    return_ack
    ;   1.1.2)  Exception 2: 0x24 (symbol '$', TR_SIGN_COMMAND) -
    ;           transmit signature bytes.
    cpi     CMD_REG,            TR_SIGN_COMMAND
    breq    return_signature
    ;   1.1.3)  Exception 3: 0x20 (symbol ' ', SPACE_SYMBOL) - do nothing
    cpi     CMD_REG,            SPACE_SYMBOL
    breq    return_ack
    ;
    ;   1.2)    Check command bits to detect forbidden commands
    ;   1.2.1)  Is command forbidden?
    ;           Bit 7 (RESERVED_BIT_2) must be 0.
    sbrc    CMD_REG,            RESERVED_BIT_2
    rjmp    return_nack
    ;   1.2.2)  Is command forbidden?
    ;           Bit 6 (TURN_ON_OFF_BIT) must be 1.
    sbrs    CMD_REG,            RESERVED_BIT_1
    rjmp    return_nack
    ;
    ;   1.3)    Process commands
    ;   1.3.1)  Process command code and line number
    ;           'turn line X ON'
    ;           'turn line X OFF'
    ;           'reset line X'
    mov     LOC_REG_2,          CMD_REG
    andi    LOC_REG_2,          0xf0

    cpi     LOC_REG_2,          0x40
    breq    line_off_cmd

    cpi     LOC_REG_2,          0x50
    breq    line_rst_cmd

    cpi     LOC_REG_2,          0x60
    breq    line_on_cmd

    rjmp    return_nack

line_rst_cmd:
    ldi     LOC_REG_1,          RESET_START
    rjmp    get_line_number
line_off_cmd:
    ldi     LOC_REG_1,          TURN_OFF
    rjmp    get_line_number
line_on_cmd:
    ldi     LOC_REG_1,          TURN_ON
get_line_number:
    mov     LOC_REG_2,          CMD_REG
    andi    LOC_REG_2,          LINE_NUMBER_MASK
    cpi     LOC_REG_2,          POWER_SW_N_LINES
    brge    return_nack
    ;
    ;   1.3.2) Send line drive command to the line array
    clr     LOC_REG_3
    ldi     XL, low(LINE_COMMANDS)
    ldi     XH, high(LINE_COMMANDS)
    add     XL, LOC_REG_2
    adc     XH, LOC_REG_3
    st      X,  LOC_REG_1
    rjmp    return_ack
return_signature:
    rcall    TRANSMIT_SIGNATURE
    rjmp    return_ack
    ;
    ;   3)      Transmit ACK/NACK and return
return_nack:
    ;   3.1)    Invalid command, not executed.
    ;           Transmit NACK symbol '!' and return.
    ldi     ARG_REG,        NACK_SYMBOL
    rcall    TRANSMIT_BYTE
    rjmp    process_command_return
    ;
return_ack:
    ;   3.2)    Valid command, executed.
    ;           Transmit ACK symbol '#' and return.
    ldi     ARG_REG,        ACK_SYMBOL
    rcall    TRANSMIT_BYTE
    ;
process_command_return:
    pop     LOC_REG_3
    pop     LOC_REG_2
    pop     LOC_REG_1
    ret

; Get byte from receive buffer
RECEIVE_BYTE:
    push    LOC_REG_1
    push    LOC_REG_2
    ;   Loop to wait byte from receive buffer
wait_byte:
    cp      RX_BOT_IDX,         RX_TOP_IDX
    breq    wait_byte
    ;   Move received byte to the return register
    clr     LOC_REG_2
    ldi     XL,                 low(RX_BUF)
    ldi     XH,                 high(RX_BUF)
    add     XL,                 RX_BOT_IDX
    adc     XH,                 LOC_REG_2
    ld      RETVAL_REG,         X
    ;   Update pointer to the bottom of RX buffer
    mov     LOC_REG_1,          RX_BOT_IDX
    inc     LOC_REG_1
    andi    LOC_REG_1,          RCV_BUF_IDX_MASK
    mov     RX_BOT_IDX,         LOC_REG_1
    ;   Return
    pop     LOC_REG_2
    pop     LOC_REG_1
    ret

; Put byte on transmit buffer
TRANSMIT_BYTE:
    push    LOC_REG_1
    push    LOC_REG_2
    ;   Check that transmit buffer is not full.
    mov     LOC_REG_1,          TX_TOP_IDX
    inc     LOC_REG_1
    andi    LOC_REG_1,          SND_BUF_IDX_MASK
    cpse    LOC_REG_1,          TX_BOT_IDX
    rjmp    store_byte
    ;   Transmit buffer is full. Enable transmit
    ;   interrupt and await transmit buffer to be free.
    sbi     UCSRB,              UDRIE
    ;   Loop to wait transmit buffer free
wait_trbuf_free:
    cpse    LOC_REG_1,          TX_BOT_IDX
    rjmp    store_byte
    rjmp    wait_trbuf_free
store_byte:
    ;   Put byte to the top of transmit buffer
    clr     LOC_REG_2
    ldi     XL,                 low(TX_BUF)
    ldi     XH,                 high(TX_BUF)
    add     XL,                 TX_TOP_IDX
    adc     XH,                 LOC_REG_2
    st      X,                  ARG_REG
    ;   Update pointer to the top of transmit buffer
    mov     TX_TOP_IDX,         LOC_REG_1
    ;   Enable transmit interrupt
    sbi     UCSRB,              UDRIE
    ;   Return
    pop     LOC_REG_2
    pop     LOC_REG_1
    ret

; Transmit device signature
TRANSMIT_SIGNATURE:
    push    ZL
    push    ZH

    ldi     ZH,                 high(SIGNATURE<<1)
    ldi     ZL,                 low(SIGNATURE<<1)
transmit_signature_loop:
    lpm     ARG_REG,            Z+
    tst     ARG_REG
    breq    transmit_signature_exit
    rcall    TRANSMIT_BYTE
    rjmp    transmit_signature_loop
transmit_signature_exit:
    pop     ZH
    pop     ZL
    ret
