;   ATtiny2313 controller
.include "tn2313def.inc"

;   Definitions
;
;   Device type and properties
.equ    DEVICE_TYPE                 =   '1'     ; Power switch. Single byte
                                                ; command
.equ    SIGN_SYMBOL_UNSPEC          =   '0'     ; Signature symbol denoting
                                                ; unspecified value
.equ    POWER_SW_NO_RESET           =   0x40    ; Power switch with no
                                                ; 'reset line X' command
.equ    POWER_SW_N_LINES            =   0x04    ; Power switch with four
                                                ; servicable lines
;   ASCII symbols
.equ    CR_SYMBOL                   =   0x0d    ; Provokes command execution
                                                ; when received.

.equ    NACK_SYMBOL                 =   '!'     ; Transmitted on reply to
                                                ; invalid command.

.equ    ACK_SYMBOL                  =   '#'     ; Transmitted on reply to
                                                ; valid command.

;   Command bytes
.equ    EMPTY_COMMAND               =   0       ; Command 'do nothing'
.equ    TR_SIGN_COMMAND             =   '$'     ; Command 'transmit signature'

;   1.2 Masks
.equ    COMMAND_BYTE_MASK           =   0xe0    ; Mask '11100000' to detect
                                                ; command symbols and
                                                ; special UART symbols.

.equ    CHANNEL_NUMBER_MASK         =   0x03    ; Mask '00000011' to extract
                                                ; port number from command byte

.equ    CHANNEL_NUMBER_0_MASK       =   0x01    ; Mask '00000001' to TURN ON
                                                ; channel number 0

;   1.3 Command bits
.equ    TURN_ON_OFF_BIT             =   6       ; Bit in command byte to
                                                ; denote TURN ON/OFF commands.

.equ    TURN_ON_BIT                 =   5       ; Bit in command byte to
                                                ; denote TURN ON commands.

.equ    RESERVED_BIT                =   4       ; Reserved bit in command byte.
                                                ; Must be 0.
.equ    RESERVED_BIT_2              =   7       ; Reserved bit in command byte.
                                                ; Must be 0.

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
;   Routines' local registers
.def    LOC_REG_1                     = r2
.def    LOC_REG_2                     = r3
;   Registers used to keep pointer to RX buffer
.def    RX_PTR_L                      = r6
.def    RX_PTR_H                      = r7
;   Registers used to keep pointer to TX buffer
.def    TX_PTR_L                      = r8
.def    TX_PTR_H                      = r9
;   Local register user to keep current bottom index in RX buffer
.def    RX_BOT_IDX                    = r10
;   Local register user to keep current top index in TX buffer
.def    TX_TOP_IDX                    = r11
;   Local registers used by RX and TX ISRs
.def    RX_ISR_LOC_REG_1              = r12
.def    RX_ISR_LOC_REG_2              = r13
.def    TX_ISR_LOC_REG_1              = r14
.def    TX_ISR_LOC_REG_2              = r15
;
;   Registers which support operations with immediate values
;   Routines' local registers
.def    LOC_I_REG_1                   = r16
.def    LOC_I_REG_2                   = r17
;   Register used to pass arguments
.def    ARG_REG                       = r20
;   Register used in routines as return value storage
.def    RETVAL_REG                    = r21
;   Register used co pass one-byte command to command processor
.def    CMD_REG                       = r23
;   Register user to keep top index in RX buffer
.def    RX_TOP_IDX                    = r24
;   Register user to keep bottom index in TX buffer
.def    TX_BOT_IDX                    = r25

; Data segment (in SRAM).
.dseg
; USART receive buffer
RX_BUF:     .BYTE   RX_BUFLEN
; USART transmit buffer
TX_BUF:     .BYTE   TX_BUFLEN

; Code segment (in Flash).
.cseg
.org    0x0000
        rjmp    RESET_ISR   ; Reset/Power_ON
.org    INT0addr            ; External Interrupt Request 0
        rjmp    EMPTY_ISR
.org    INT1addr            ; External Interrupt Request 1
        rjmp    EMPTY_ISR
.org    ICP1addr            ; Timer/Counter1 Capture Event
        rjmp    EMPTY_ISR
.org    OC1Aaddr            ; Timer/Counter1 Compare Match A
        rjmp    EMPTY_ISR
.org    OVF1addr            ; Timer/Counter1 Overflow
        rjmp    EMPTY_ISR
.org    OVF0addr            ; Timer/Counter0 Overflow
        rjmp    EMPTY_ISR
.org    URXCaddr            ; USART, Rx Complete
        rjmp    USART_RX_ISR
.org    UDREaddr            ; USART Data Register Empty
        rjmp    USART_TX_ISR
.org    UTXCaddr            ; USART, Tx Complete
        rjmp    EMPTY_ISR
.org    ACIaddr             ; Analog Comparator
        rjmp    EMPTY_ISR
.org    PCIaddr             ;
        rjmp    EMPTY_ISR
.org    OC1Baddr            ;
        rjmp    EMPTY_ISR
.org    OC0Aaddr            ;
        rjmp    EMPTY_ISR
.org    OC0Baddr            ;
        rjmp    EMPTY_ISR
.org    USI_STARTaddr       ; USI Start Condition
        rjmp    EMPTY_ISR
.org    USI_OVFaddr         ; USI Overflow
        rjmp    EMPTY_ISR
.org    ERDYaddr            ;
        rjmp    EMPTY_ISR
.org    WDTaddr             ; Watchdog Timer Overflow
        rjmp    EMPTY_ISR

.org    INT_VECTORS_SIZE

;   ISRs
; Called on startup
RESET_ISR:
    ; Initialization
    ;
    ; Save initial value of status register
    in      LOC_REG_1,      SREG
    ; Set stack pointer to the end of the RAM
    ldi     LOC_I_REG_1,    low(RAMEND)
    out     spl,            LOC_I_REG_1
    ; Save return address 0x0000 in the stack
    ldi     LOC_I_REG_1,    0x00
    push    LOC_I_REG_1
    push    LOC_I_REG_1
    ; Set PORT B to output mode and turn ON all its lines
    ldi     LOC_I_REG_1,    0xff
    out     DDRB,           LOC_I_REG_1
    out     PORTB,          LOC_I_REG_1
    ; Initialize USART
    rcall   INIT_USART
    ; Enable interrupts
    sei
    ; Call main routine
    rcall   MAIN
    ; TODO
    ; Main routine returned. Then soft reboot.
    ; We'll go to address 0x0000 where instruction
    ; 'rjmp RESET_ISR' is located. This will make
    ; processor repeat initialization steps and
    ; call main routine again.
    out     SREG,           LOC_REG_1
    reti

; USART receive interrupt routine
USART_RX_ISR:
    push    ZL
    push    ZH
    ; Save flags
    in      RX_ISR_LOC_REG_1,   SREG
    ; Receive byte and save it on the top of RX buffer
    in      RX_ISR_LOC_REG_2,   UDR
    movw    ZL,                 RX_PTR_L
    add     ZL,                 RX_TOP_IDX
    st      Z,                  RX_ISR_LOC_REG_2
    ; Modify index of the top of RX buffer
    inc     RX_TOP_IDX
    andi    RX_TOP_IDX,         RCV_BUF_IDX_MASK
    cpse    RX_TOP_IDX,         RX_BOT_IDX
    ; modified top != bottom, OK, we may finish now.
    rjmp    usart_rx_isr_return
    ; modified top == bottom, BUMS! we have to rollback top pointer.
    dec     RX_TOP_IDX
    andi    RX_TOP_IDX,         RCV_BUF_IDX_MASK

usart_rx_isr_return:
    ; Restore saved flags and return
    out     SREG,               RX_ISR_LOC_REG_1
    pop     ZH
    pop     ZL
    reti

; USART transmit interrupt routine
USART_TX_ISR:
    push    ZL
    push    ZH
    ; Save flags
    in      TX_ISR_LOC_REG_1,   SREG

    ; Check that transmit buffer is not empty
    cp      TX_BOT_IDX,         TX_TOP_IDX
    ; TX buffer is not empty, continue execution from 'mov_tx_byte'
    brne    move_tx_byte
    ; TX buffer is empty, disable UDR interrupt and return
    cbi     UCSRB,              UDRIE
    rjmp    usart_tx_isr_return

move_tx_byte:
    ; Move byte from the bottom of transmit buffer to the transmit register
    movw    ZL,                 TX_PTR_L
    add     ZL,                 TX_BOT_IDX
    ld      TX_ISR_LOC_REG_2,   Z
    out     UDR,                TX_ISR_LOC_REG_2
    ; Modify pointer to the bottom of transmit buffer
    inc     TX_BOT_IDX
    andi    TX_BOT_IDX,         SND_BUF_IDX_MASK

usart_tx_isr_return:
    ; Restore saved flags and return
    out     SREG,               TX_ISR_LOC_REG_1
    pop     ZH
    pop     ZL
    reti

; Do nothing, but return.
EMPTY_ISR:
    reti

; Initialize USART
INIT_USART:
    ; Baud rate
    ; Clock 20 MHz, baud rate = 115200
    ldi     LOC_I_REG_1,        10
    out     UBRRL,              LOC_I_REG_1
    ldi     LOC_I_REG_1,        0
    out     UBRRH,              LOC_I_REG_1
    ; Pointers to RX buffer
    ldi     LOC_I_REG_1,        0
    mov     RX_PTR_H,           LOC_I_REG_1
    ldi     LOC_I_REG_1,        RX_BUF
    mov     RX_PTR_L,           LOC_I_REG_1
    ; Pointers to TX buffer
    ldi     LOC_I_REG_1,        0
    mov     TX_PTR_H,           LOC_I_REG_1
    ldi     LOC_I_REG_1,        TX_BUF
    mov     TX_PTR_L,           LOC_I_REG_1
    ; Initialize top and bottom indeces in RX buffer
    ldi     LOC_I_REG_1,        RX_TOP_INIT
    mov     RX_TOP_IDX,         LOC_I_REG_1
    ldi     LOC_I_REG_1,        RX_BOT_INIT
    mov     RX_BOT_IDX,         LOC_I_REG_1
    ; Initialize top and bottom indeces in TX buffer
    ldi     LOC_I_REG_1,        TX_TOP_INIT
    mov     TX_TOP_IDX,         LOC_I_REG_1
    ldi     LOC_I_REG_1,        TX_BOT_INIT
    mov     TX_BOT_IDX,         LOC_I_REG_1
    ; UCSRB register
    ldi     LOC_I_REG_1,        (1<<RXEN)|(1<<TXEN)|(1<<RXCIE)
    out     UCSRB,              LOC_I_REG_1
    ; UCSRC register
    ldi     LOC_I_REG_1,        (1<<UCSZ1)|(1<<UCSZ0)
    out     UCSRC,              LOC_I_REG_1
    ret

; Main routine
MAIN:
    push    LOC_REG_1
main_loop:
    ; 1) Cleanup command register
    ldi     CMD_REG,        EMPTY_COMMAND
main_receive_byte:
    ; 2) Receive byte
    rcall   RECEIVE_BYTE
    mov     LOC_REG_1,      RETVAL_REG
    ; 3) Process received byte
    ;
    ; 3.1) Is byte CR symbol?
main_is_cr:
    mov     LOC_I_REG_1,    LOC_REG_1
    cpi     LOC_I_REG_1,    CR_SYMBOL
    brne    main_is_special         ; Byte is not CR, continue loop.
    rcall   PROCESS_COMMAND         ; Byte is 'CR', process command.
    rjmp    main_loop
main_is_special:
    ; 3.2) Is byte special symbol?
    mov     LOC_I_REG_1,    LOC_REG_1
    andi    LOC_I_REG_1,    COMMAND_BYTE_MASK
    brne    main_is_command         ; Byte is command, continue loop.
    rjmp    main_receive_byte       ; Byte is special, receive new byte.
main_is_command:
    ; 3.3) Byte is command symbol.
    ; Legal command, move command byte into command register
    mov     CMD_REG,        LOC_REG_1
    ; Return received command symbol to sender
    mov     ARG_REG,        LOC_REG_1
    rcall   TRANSMIT_BYTE
    ; Wait CR symbol to execute received command or
    ; new command symbol to overwrite previous one
    rjmp    main_receive_byte
    ;
    ; TODO:
    ; Return from 'MAIN' routine to make soft reboot.
    ; Is not supported now. Main loop is endless.
main_return:
    pop     LOC_REG_1
    ret

; Process command byte
PROCESS_COMMAND:
    push    LOC_I_REG_1
    push    LOC_I_REG_2
    push    LOC_REG_1
    in      LOC_REG_1,          SREG
    ;
    ;   1)      Process command byte
    ;
    ;   1.1)    Exceptions
    ;   1.1.1)  Is command empty?
    ;           Exception 1: 0x00 (EMPTY_COMMAND) - do nothing.
    cpi     CMD_REG,            EMPTY_COMMAND
    breq    process_command_ack
    ;   1.1.2)  Is command 'transmit signature'?
    ;           Exception 2: 0x24 (symbol '$', TR_SIGN_COMMAND) -
    ;           transmit signature bytes.
    cpi     CMD_REG,            TR_SIGN_COMMAND
    breq    process_command_signature
    ;
    ;   1.2)    Check command bits to detect forbidden commands
    ;   1.2.1)  Is command forbidden?
    ;           Bit 7 (RESERVED_BIT_2) must be 0.
    sbrc    CMD_REG,            RESERVED_BIT_2
    rjmp    process_command_nack
    ;   1.2.2)  Is command forbidden?
    ;           Bit 6 (TURN_ON_OFF_BIT) must be 1.
    sbrs    CMD_REG,            TURN_ON_OFF_BIT
    rjmp    process_command_nack
    ;   1.2.3)  Is command forbidden?
    ;           Bit 4 (RESERVED_BIT)    must be 0.
    sbrc    CMD_REG,            RESERVED_BIT
    rjmp    process_command_nack
    ;
    ;   1.3)    Process TURN ON/OFF command.
    ;   1.3.1)  Extract port number from command byte
    ;           and prepare channels bitmask.
    mov     LOC_I_REG_1,        CMD_REG
    andi    LOC_I_REG_1,        CHANNEL_NUMBER_MASK
    clr     LOC_I_REG_2
    sbr     LOC_I_REG_2,        CHANNEL_NUMBER_0_MASK
process_command_prepare_mask_loop:
    tst     LOC_I_REG_1
    breq    process_command_prepare_mask_end
    lsl     LOC_I_REG_2
    dec     LOC_I_REG_1
    rjmp    process_command_prepare_mask_loop
process_command_prepare_mask_end:
    ;   1.3.2)  Is command TURN ON?
    sbrc    CMD_REG,            TURN_ON_BIT
    rjmp    process_command_turn_on
    ;   1.3.3)  This is TURN OFF command and nothing else.
    ;           We do not need to jump to execute it.
    ;
    ;   2)  Execute commands
    ;
process_command_turn_off:
    mov     ARG_REG,    LOC_I_REG_2
    rcall   TURN_OFF_B_BY_MASK
    rjmp    process_command_ack
    ;
process_command_turn_on:
    mov     ARG_REG,    LOC_I_REG_2
    rcall   TURN_ON_B_BY_MASK
    rjmp    process_command_ack
    ;
process_command_signature:
    rcall   TRANSMIT_SIGNATURE
    rjmp    process_command_ack
    ;
    ;   3)      Return
    ;
process_command_nack:
    ;   3.1)    Invalid command, not executed.
    ;           Transmit NACK symbol '!' and return.
    ldi     ARG_REG,        NACK_SYMBOL
    rcall   TRANSMIT_BYTE
    rjmp    process_command_return
    ;
process_command_ack:
    ;   3.2)    Valid command, executed.
    ;           Transmit ACK symbole '#' and return.
    ldi     ARG_REG,        ACK_SYMBOL
    rcall   TRANSMIT_BYTE
    ;
process_command_return:
    out     SREG,               LOC_REG_1
    pop     LOC_REG_1
    pop     LOC_I_REG_2
    pop     LOC_I_REG_1
    ret

; Turn ON port B by mask
TURN_ON_B_BY_MASK:
    push    LOC_REG_1
    push    LOC_REG_2
    in      LOC_REG_2,              SREG

    in      LOC_REG_1,              PORTB
    or      ARG_REG,                LOC_REG_1
    out     PORTB,                  ARG_REG

    out     SREG,                   LOC_REG_2
    pop     LOC_REG_2
    pop     LOC_REG_1
    ret

; Turn OFF port B by mask
TURN_OFF_B_BY_MASK:
    push    LOC_REG_1
    push    LOC_REG_2
    in      LOC_REG_2,              SREG

    in      LOC_REG_1,              PORTB
    com     ARG_REG
    and     ARG_REG,                LOC_REG_1
    out     PORTB,                  ARG_REG

    out     SREG,                   LOC_REG_2
    pop     LOC_REG_2
    pop     LOC_REG_1
    ret

; Get byte from receive buffer
RECEIVE_BYTE:
    push    LOC_I_REG_1
    push    LOC_REG_1
    in      LOC_REG_1,          SREG

;   Loop to wait byte from receive buffer
get_byte_loop:
    cp      RX_BOT_IDX,         RX_TOP_IDX
    breq    get_byte_loop
;   Move received byte to the return register
    movw    XL,                 RX_PTR_L
    add     XL,                 RX_BOT_IDX
    ld      RETVAL_REG,         X
;   Update pointer to the bottom of RX buffer
    mov     LOC_I_REG_1,        RX_BOT_IDX
    inc     LOC_I_REG_1
    andi    LOC_I_REG_1,        RCV_BUF_IDX_MASK
    mov     RX_BOT_IDX,         LOC_I_REG_1

    out     SREG,               LOC_REG_1
    pop     LOC_REG_1
    pop     LOC_I_REG_1
    ret

; Put byte on transmit buffer
TRANSMIT_BYTE:
    push    LOC_I_REG_1
    push    LOC_REG_1
    in      LOC_REG_1,          SREG

    ;   Check that transmit buffer is not full.
    mov     LOC_I_REG_1,        TX_TOP_IDX
    inc     LOC_I_REG_1
    andi    LOC_I_REG_1,        SND_BUF_IDX_MASK
    cpse    LOC_I_REG_1,        TX_BOT_IDX
    rjmp    transmit_byte_store
    ;   Transmit buffer is full. Enable transmit
    ;   interrupt and await transmit buffer to be free.
    sbi     UCSRB,              UDRIE
transmit_byte_await_loop:
    cpse    LOC_I_REG_1,        TX_BOT_IDX
    rjmp    transmit_byte_store
    rjmp    transmit_byte_await_loop

transmit_byte_store:
    ;   Put byte to the top of transmit buffer
    movw    YL,                 TX_PTR_L
    add     YL,                 TX_TOP_IDX
    st      Y,                  ARG_REG

    ;   Update pointer to the top of transmit buffer
    mov     TX_TOP_IDX,         LOC_I_REG_1

    ;   Enable transmit interrupt
    sbi     UCSRB,              UDRIE

    out     SREG,               LOC_REG_1
    pop     LOC_REG_1
    pop     LOC_I_REG_1
    ret

; Put byte on transmit buffer
TRANSMIT_SIGNATURE:
    push    ZL
    push    ZH
    push    LOC_REG_1
    in      LOC_REG_1,          SREG

    ldi     ZH,                 high(SIGNATURE<<1)
    ldi     ZL,                 low(SIGNATURE<<1)

transmit_signature_loop:
    lpm     ARG_REG,            Z+
    tst     ARG_REG
    breq    transmit_signature_exit
    rcall   TRANSMIT_BYTE
    rjmp    transmit_signature_loop

transmit_signature_exit:
    out     SREG,               LOC_REG_1
    pop     LOC_REG_1
    pop     ZH
    pop     ZL
    ret

SIGNATURE:  .DB     DEVICE_TYPE, POWER_SW_NO_RESET + POWER_SW_N_LINES, SIGN_SYMBOL_UNSPEC
            .DB     0x00
