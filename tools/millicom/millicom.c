/*****************************************************************
 * 
 * Test Environment.
 * TTY interaction implementation
 * 
 * Author: Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 * 
 * $Id$
 *
 ****************************************************************/

#include <stdio.h>
#include <termio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#undef DEBUG

#ifdef DEBUG
#define DBG(x) do { \
    x;\
} while (0)
#else
#define DBG(x...)
#endif

#define MIN_ARG_COUNT   2

#define BUF_SIZE        32

/**
 * Mapping structure between baud_rate in integer representation and baud_rate
 * bits in cflag field of struct termios
 */
typedef struct speed_mapping_s {
    unsigned int speed;
    unsigned int b_speed;
} speed_mapping_t;

/**
 * Mapping table between baud_rate in integer representation and baud_rate bits
 * in cflag field of struct termios
 */
static speed_mapping_t speed_map[] = {
    {0, B0},
    {50, B50},
    {75, B75},
    {110, B110},
    {134, B134},
    {150, B150},
    {200, B200},
    {300, B300},
    {600, B600},
    {1200, B1200},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400},
    {57600, B57600},
    {115200, B115200},
    {230400, B230400},
    {460800, B460800},
    {500000, B500000},
    {576000, B576000},
    {921600, B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {2000000, B2000000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000}
};

#define SPEED_MAP_SIZE  (sizeof(speed_map) / sizeof(speed_mapping_t))

/**
 * Signal handler to be registered for SIGINT signal.
 * 
 * @param sig   Signal number
 */
static void
sigint_handler(int sig)
{
    /*
     * We can't log here using TE logging facilities, but we need
     * to make a mark, that TA was killed.
     */
    fprintf(stderr, "Millicom session has been killed by %d signal\n", sig);
    exit(0);
}

/**
 * Debug printing of struct termio.
 * 
 * @param tty   struct termios
 */
void
print_attr(struct termios *tty)
{
    int i;
    
    fprintf(stderr,
            "TTY attributes\n"
            "  IFLAGS = %08X\n"
            "  OFLAGS = %08X\n"
            "  CFLAGS = %08X\n"
            "  LFLAGS = %08X\n"
            "  LINE   = %02X\n"
            "  CC     = ",
            tty->c_iflag,
            tty->c_oflag,
            tty->c_cflag,
            tty->c_lflag,
            tty->c_line);

    for (i = 0; i < NCCS; i++) {
        fprintf(stderr, "%02X ", tty->c_cc[i]);
    }

    fprintf(stderr, "\n"
            "  ISPEED = %08X\n"
            "  OSPEED = %08X\n",
            tty->c_ispeed,
            tty->c_ospeed);
}


/**
 * Main loop.
 */
int
main(int argc, char *argv[])
{
    struct termios tty;
    int tty_fd = -1;
    long flags;

    /* Handle SIGINT (CTL-C) correctly */
    (void)signal(SIGINT, sigint_handler);
    
    /* Make stdin stream non-blocking and stdout stream non-buffered */
    flags = fcntl(fileno(stdin), F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fileno(stdin), F_SETFL, flags);
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < MIN_ARG_COUNT) {
        fprintf(stderr, "Error program usage:\n"
                "  Use: millicom <dev-name> [-speed=<speed>]\n");
        goto err;
    }
    
    if ((tty_fd = open(argv[1], O_RDWR | O_SYNC | O_NONBLOCK)) < 0) {
        fprintf(stderr, "Error opening device \"%s\"\n", argv[1]);
        goto err;
    }
    
    /* Get TTY attributes */
    if (tcgetattr(tty_fd, &tty) != 0) {
        fprintf(stderr, "Error getting tty attributes\n");
        goto err;
    }

    DBG(fprintf(stderr, "Attributes before\n"));
    DBG(print_attr(&tty));

    /* Modify TTY attributes (stealed from minicom sources) */
    tty.c_iflag &= ~(IGNCR | INLCR | ICRNL | IUCLC | IXANY | IXON |
                     IXOFF | INPCK | ISTRIP | BRKINT | IGNPAR);
    tty.c_iflag |= (IGNBRK);
    tty.c_oflag &= ~OPOST;
    tty.c_cflag |= (CREAD | CLOCAL | HUPCL | CRTSCTS);
    tty.c_lflag &= ~(XCASE | ECHONL | NOFLSH | ICANON | ISIG | ECHO);
    tty.c_cc[VTIME] = 5;
    tty.c_cc[VMIN] = 1;

    /* Set input baud rate the same as output */
    cfsetispeed(&tty, B0);

    if (argc > 2) {
        int     i;
        int     speed;
        speed_t b_speed = B0;

        if ((sscanf(argv[2], "-speed=%d", &speed)) != 1) {
            fprintf(stderr, "Error: invalid speed parameter\n");
            goto err;
        }
        
        for (i = 0; i < SPEED_MAP_SIZE; i++) {
            if (speed == speed_map[i].speed) {
                b_speed = speed_map[i].b_speed;
                break;
            }
        }

        if (b_speed != B0)
            cfsetospeed(&tty, b_speed);
    }

    DBG(fprintf(stderr, "Attributes after\n"));
    DBG(print_attr(&tty));

    /* Set TTY attributes now */
    if (tcsetattr(tty_fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error getting tty attributes\n");
        goto err;
    }

    /* Main loop */
    for (;;)
    {
        ssize_t len;
        struct timeval tv;
        char buf[BUF_SIZE];

        /* Read from stdin if available and write to TTY */
        DBG(fprintf(stderr, "\nstdin->tty\n"));
        while ((len = fread(buf, 1, BUF_SIZE, stdin)) > 0) {
            DBG(fprintf(stderr, "\nRead from stdin: %d bytes", len));
            if (write(tty_fd, buf, len) != len) {
                fprintf(stderr, "I/O error: failed write to \"%s\"\n", argv[1]);
                goto err;
            }
        }
        if ((len < 0) && (errno != EAGAIN)) {
            fprintf(stderr, "I/O error: failed read from stdin, rc=%d, errno=%d\n",
                   len, errno );
            goto err;
        }
        
        /* Read from TTY if available and write to stdout */
        DBG(fprintf(stderr, "\ntty->stdout\n"));
        while ((len = read(tty_fd, buf, BUF_SIZE)) > 0) {
            DBG(fprintf(stderr, "\nRead from tty: %d bytes", len));
            if (fwrite(buf, 1, len, stdout) != len) {
                fprintf(stderr, "I/O error: failed write stdout\n");
                goto err;
            }
        }
        if ((len < 0) && (errno != EAGAIN)) {
            fprintf(stderr, "I/O error: failed read from \"%s\", rc=%d, errno=%d\n",
                   argv[1], len, errno);
            goto err;
        }
    }

    /* Finish (unreachable) */
    close(tty_fd);

    return 0;

err:
    if (tty_fd >= 0)
        close(tty_fd);

    return -1;
}
