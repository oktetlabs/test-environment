/*
 * Power switch code moved to Arduino nano platform.
 * Use Arduino IDE to compile this code and load binary.
 */

/* Power switc signature values */
#define POWER_SW_TYPE     '1' /* Single byte command, no lexem parser */
#define POWER_SW_N_LINES  6   /* Now there're 6 lines to control 6 RaPi
                                 devices */
#define RESET_SUPPORT     0   /* Not supported */

/* Device signeture bytes */
#define SIGN_BYTE_1       POWER_SW_TYPE
#define SIGN_BYTE_2       ((1<<6) + (RESET_SUPPORT<<5) + POWER_SW_N_LINES)
#define SIGN_BYTE_3       '0'

/* Command specific symbols */
#define LF_SYMBOL         '\n'
#define CR_SYMBOL         '\r'
#define SPACE_SYMBOL      ' '
#define NACK_SYMBOL       '!'
#define ACK_SYMBOL        '#'
#define EMPTY_COMMAND     0
#define TR_SIGN_COMMAND   '$'

/*
 * Command format
 *
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * |  0  |  1  | 0/1 |  0  | 0/1 | 0/1 | 0/1 | 0/1 |
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 *    |     |     |     |     |     |     |     |
 *    |     |     |     |     +-----+-----+-----+- Line number 0 - 15
 *    |     |     |     |                          in binary representation.
 *    |     |     |     |                          Hihger line numbers depend
 *    |     |     |     |                          on line number limitation
 *    |     |     |     |                          in given design. Here
 *    |     |     |     |                          we have 6 power lines. So
 *    |     |     |     |                          the valid numbers be 0-5.
 *    |     |     |     +------------------------- Reserved bit 3
 *    |     |     +------------------------------- Command bit 1
 *    |     +------------------------------------- Reserved bit 2
 *    +------------------------------------------- Reserved bit 1
 *
 * In the case when command 'Restart line x' is not supported
 * command bit 5 just represents the binary state to set on the power line
 * whose number is coded in bits 0-3. Reserved command bits should have the
 * reserved values.
 *
 * In the case of command 'Restart line x' is supported the 'Reserved bit 3'
 * becomes 'Command bit 2' and together with 'Command bit 1' represents
 * three commands and one invalid depending on their values
 *
 * +-------+-------+---------------+
 * | Bit 1 | Bit 2 | Command       |
 * +-------+-------+---------------+
 * |   0   |   0   | Turn OFF x    |
 * +-------+-------+---------------+
 * |   0   |   1   | Restart x     |
 * +-------+-------+---------------+
 * |   1   |   0   | Turn ON x     |
 * +-------+-------+---------------+
 * |   1   |   1   | Inval cmd     |
 * +-------+-------+---------------+
 *
 * There is one exception - command 'Get signature' whose command byte
 * is represented by '$' sign.
 */

/* Bit mask to extract line number 0-16 from command byte */
#define LINE_NUMBER_MASK  0x0f

/* Command code bits */
#define RESERVED_BIT_1_POS  7 /* Reserved bit 1 position */
#define RESERVED_BIT_1_VAL  0 /* Reserved bit 1 value */
#define RESERVED_BIT_2_POS  6 /* Reserved bit 2 position */
#define RESERVED_BIT_2_VAL  1 /* Reserved bit 2 value */
#define RESERVED_BIT_3_POS  4 /* Reserved bit 3 position */
#define RESERVED_BIT_3_VAL  0 /* Reserved bit 3 value */
#define CMD_BIT_1_POS       5 /* Command bit 1 position */
#define CMD_BIT_2_POS       4 /* Command bit 2 position */

#define RESERVED_BIT_MASK \
    (1<<RESERVED_BIT_1_POS | 1<<RESERVED_BIT_2_POS | 1<<RESERVED_BIT_3_POS)

#define RESERVED_BIT_VALUE \
    (RESERVED_BIT_1_VAL<<RESERVED_BIT_1_POS | \
    RESERVED_BIT_2_VAL<<RESERVED_BIT_2_POS | \
    RESERVED_BIT_3_VAL<<RESERVED_BIT_3_POS)

/*
 * Bit mask to extract command code. With future support of
 * 'Restart line x' command
 */
#define CMD_CODE_MASK   ((1<<CMD_BIT_1_POS) + (1<<CMD_BIT_2_POS))

enum cmd_code
{
    CMD_TURN_OFF,
    CMD_RESTART,
    CMD_TURN_ON,
    CMD_INVAL,
};

/*
 * Electromagnetic contactors block keep contactors coils unlowered when
 * input signal has HIGH level. And coils are powered with LOW level of
 * input signal. This variant of design was cgosen to save electric power:
 * In general user likes to swith loads to powered state with HIGH level of
 * signal and this state is keeped almost all the time switch works so it's
 * reasonable to keep contactors coils unpowered when loads are powered.
 */
#define POWER_OFF LOW
#define POWER_ON  HIGH

/* Power lines pins */
#define D0      8
#define D1      9
#define D2      10
#define D3      11
#define D4      12
#define D5      13

int lines[POWER_SW_N_LINES] = {D0, D1, D2, D3, D4, D5};

void set_dlines_mode_out()
{
    int i;

    for (i = 0; i < POWER_SW_N_LINES; i++)
        pinMode(lines[i], OUTPUT);
}

void set_dlines_initial_on()
{
    int i;

    for (i = 0; i < POWER_SW_N_LINES; i ++)
        digitalWrite(lines[i], POWER_ON);
}

void setup() {
    /* UART 115200: traditionally due to Tilgin choice */
    Serial.begin(115200);

    /* DATA 0-6 output */
    set_dlines_mode_out();

    /*
     * DATA 0-6 logical 0
     * Magnetic contactors with toggling contacts are used to control
     * power load. It is better to use OFF state of such a contactors to
     * turn power load ON. And vice-verse we turn power load off by means of
     * ON state of contactors.
     *
     * So we'll send logical 0 on control line to turn power load ON
     * and logical 1 to turn power OFF.
     */
    set_dlines_initial_on();
}

uint8_t uart_read_byte()
{
    while (Serial.available() == 0);
    return Serial.read();
}

/*
 * Return the last alphanumeric symbol before CR.
 * This symbol will be interpreted as command byte.
 *
 * FIXME
 * Not sure this solution is the best of possible
 * but power switch controllers using this algorithm
 * work reliably for a long time.
 */
uint8_t uart_read_cmd_byte()
{
    uint8_t rx_byte;
    uint8_t cmd_byte = EMPTY_COMMAND;

    do {
        rx_byte = uart_read_byte();

        if (rx_byte == CR_SYMBOL)
            break;

        if (rx_byte < SPACE_SYMBOL)
            continue;

        Serial.write(rx_byte);

        if (rx_byte > SPACE_SYMBOL)
            cmd_byte = rx_byte;
    } while (1);

    return cmd_byte;
}

void loop()
{
    uint8_t cmd_byte;
    uint8_t cmd_code;
    uint8_t pin;
    uint8_t pin_state;

    while (1)
    {
        cmd_byte = 0;

        cmd_byte = uart_read_cmd_byte();

         /* Now process */
         /* May be empty ? */
        if (cmd_byte == EMPTY_COMMAND)
        {
            Serial.println(ACK_SYMBOL);
            continue;
        }

        /* May be TR_SIGN command ? */
        if (cmd_byte == TR_SIGN_COMMAND)
        {
            Serial.write(SIGN_BYTE_1);
            Serial.write(SIGN_BYTE_2);
            Serial.write(SIGN_BYTE_3);
            Serial.println(ACK_SYMBOL);
            continue;
        }

        /* Check reserved bit 1 */
        if ((cmd_byte & RESERVED_BIT_MASK) != RESERVED_BIT_VALUE)
        {
            Serial.println(NACK_SYMBOL);
            continue;
        }

        pin = (cmd_byte & LINE_NUMBER_MASK);

        /* Pin number should be 0-5 */
        if (pin > POWER_SW_N_LINES - 1)
        {
            Serial.println(NACK_SYMBOL);
            continue;
        }

        cmd_code = (cmd_byte & CMD_CODE_MASK)>>4;

        /*
         * The only variants are CMD_TURN_ON and CMD_TURN_OFF.
         * Other will be possible in the design with command
         * 'Restart line x' supported. For future design.
         */
        pin_state = (cmd_code == CMD_TURN_OFF) ? POWER_OFF : POWER_ON;

        digitalWrite(lines[pin], pin_state);

        Serial.println(ACK_SYMBOL);
    }
}
