/** @file
 * @brief PowerSwitch control tool.
 *
 * Copyright (C) 2010 OKTET Labs, St.-Petersburg, Russia
 *
 * @author Igor Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 * @author Ivan V. Soloduha <Ivan.Soloducha@oktetlabs.ru>
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>

#define     DEV_TYPE_PARPORT        "parport"
#define     DEV_TYPE_TTY            "tty"
#define     DEV_TYPE_DFLT           DEV_TYPE_PARPORT
#define     PARPORT_DEV_DFLT        "/dev/parport0"
#define     TTY_DEV_DFLT            "/dev/ttyS0"
#define     PARPORT_DEVICE_BITMASK  0xff
#define     TTY_DEVICE_BITMASK      0xffff
#define     COMMAND_OFF             "off"
#define     COMMAND_ON              "on"
#define     COMMAND_RST             "rst"
#define     REBOOT_SLEEP_TIME       2 /* seconds */
#define     TURN_OFF                0
#define     TURN_ON                 1
#define     RESET                   2

/**
 * Parse parse invocation command line to extract parameters.
 *
 * @param       argc    Quantity of command line arguments.
 * @param       argv    Array of commnad line arguments.
 * @param       type    Place for --type|-t option value.
 * @param       dev     Place for --dev|-d option value.
 * @param       mask    Place for 'mask' parameter value.
 * @param       command Place for 'command' parameter value.
 */
parse_cmd_line(int argc, char **argv, const char **type, const char **dev,
               unsigned int *mask, const char **command)
{
    int                     c;
    static const char       type_dflt[] = DEV_TYPE_DFLT;
    static const char       tty_dev_dflt[] = TTY_DEV_DFLT;
    static char             parport_dev_dflt[] = PARPORT_DEV_DFLT;
    static struct option    long_options[]={{"type", 1, 0, 't'},
                                            {"dev", 1, 0, 'd'},
                                            {0,0,0,0}};

    *type = NULL;
    *dev = NULL;
    *mask = 0;
    *command = NULL;

    if (argc < 3)
    {
        printf("\nToo few invocation parameters\n");
        return;
    }

    /* Extract options --type|-t and --dev|-d */
    while((c = getopt_long(argc, argv, "t:d:", long_options, NULL)) > 0)
    {
        switch(c) {
            case 't':
                *type = optarg;
                if (strcmp(*type, DEV_TYPE_PARPORT) != 0 &&
                    strcmp(*type, DEV_TYPE_TTY) != 0)
                {
                    printf("\nInvalid --type|-t option "
                           "value %s\n" , *type);
                    *type = NULL;
                    return;
                }
                break;
            case 'd':
                *dev = optarg;
                break;
            default:
                printf("\nUnknown option\n");
                return;
        }
    }

    if(*type == NULL)
    {
        /* --type|-t is not specified assign default value */
        *type = type_dflt;
    }

    if (*dev == NULL)
    {
        /* --dev|-d is not specified, assign default value */
        *dev = (strcmp(*type, DEV_TYPE_PARPORT) == 0) ?
                                            parport_dev_dflt :
                                                        tty_dev_dflt;
    }

    /* Extract control bitmask */
    if (sscanf(argv[argc - 2], "%x", mask) != 1)
    {
        printf("\nFailed to extract control bitmask "
               "from specification %s\n", argv[argc - 2]);
        mask = 0;
        return;
    }

    /* Extract control command */
    if ((*command = argv[argc - 1]) == NULL)
    {
        printf("\nFailed to extract control command "
               "from command line\n");
        return;
    }

    if (strcmp(*command, COMMAND_ON) != 0 &&
        strcmp(*command, COMMAND_OFF) != 0 &&
        strcmp(*command, COMMAND_RST) != 0)
    {
        printf("\nInvalid command value %s\n", *command);
        *command = NULL;
        return;
    }
}

/**
 * Print brief usage info.
 */
void
usage()
{
  printf("\nUsage: power_sw [options] mask command\n\n"
         "Parameters:\n\n"
         "   command     control command %s|%s|%s\n"
         "   mask        bitmask of power lines in hex format\n"
         "               position of each nonzero bit in bitmask denotes\n"
         "               number of power line to apply specified command\n\n"
         "Options:\n\n"
         "   --type|-t   type of control device %s|%s\n"
         "               parport on default\n"
         "   --dev|-d    device name,\n"
         "               default parport device - /dev/parport0\n"
         "               default tty device - /dev/ttyS0\n",
         COMMAND_ON, COMMAND_OFF, COMMAND_RST,
         DEV_TYPE_TTY, DEV_TYPE_PARPORT);
}

void
turn_on_off(int fd, unsigned int mask, int sock_num, int command_code)
{
    int             i;
    unsigned int    socket;
    char            command[2];

    command[1] = '\r';
    socket = 1;
    for (i = 0; i < sock_num; i++)
    {
        if (mask & socket)
        {
            command[0] = ((command_code == TURN_ON) ?
                            0x60 :
                            (command_code == TURN_OFF) ?
                                0x40 :
                                    0x50 /* RESET command */) | i;
            write(fd, command, 2);
        }

        socket *= 2;
    }
}

/**
 * Get information about the device opened.
 *
 * @param[in]   fd          Descriptor associated with the device.
 * @param[out]  rebootable  Whether reboot is supported.
 * @param[out]  sockets_num Number of sockets(power lines) on the device.
 *
 * @return      Operation status.
 */
int
recognize_power_switch(int fd, int *rebootable, int *sockets_num)
{
    char    command[] = "$\r";
    char    reply[5];
    int     rc;

    memset(reply, 0, 5);
    /* Send 'get signature' command. */
    rc = write(fd, command, 2);
    /* Read 1 byte of echo, 3 bytes of reply, and 1 more byte for '#'. */
    rc = read(fd, reply, 5);
    /* Check if signature (bytes 1-3) is valid. */
    if (reply[1] != '1' || !reply[2] & 0x40 || reply[3] != '0' )
    {
        printf("Device is not a Power Switch. %s\n", reply);
        return 0;
    }
    *sockets_num = reply[2] & 0x1F;
    *rebootable = (reply[2] & 0x20)? 1 : 0;
    return 1;
}

/**
 * Check if device speed is set to 115200 bps and parity check is off.
 * If not, fix them.
 *
 * @param[in]   fd          Descriptor associated with the device.
 *
 * @return      Operation status.
 */
int
check_dev_params(int fd)
{
    struct termios term;

    if (tcgetattr(fd, &term) < 0)
    {
        printf("Failed to get device attributes.\n");
        return -1;
    }

    term.c_iflag = 0;
    term.c_oflag = 0;
    term.c_cflag = CREAD | CLOCAL | CS8;
    term.c_lflag = 0;

    if (cfsetospeed(&term, B115200) < 0)
    {
        printf("Failed to set output baudrate\n");
        return -1;
    }

    if (cfsetispeed(&term, B115200) < 0)
    {
        printf("Failed to set input baudrate\n");
        return -1;
    }

    if (tcsetattr(fd, TCSADRAIN, &term) < 0)
    {
        printf("Applying parameters failed\n");
        return -1;
    }

    return 0;
}

int
main(int argc, char **argv)
{
    const char     *type;
    const char     *device;
    const char     *command;
    unsigned int    mask;
    unsigned char   mode;
    int             fd;
    int             is_rebootable;
    int             sockets_num;

    parse_cmd_line(argc, argv, &type, &device, &mask, &command);

    if (type == NULL || device == NULL || command == NULL)
    {
        usage();
        return 1;
    }

    if (strcmp(type, DEV_TYPE_PARPORT) == 0)
    {
        /* parport-like device */
        mask &= PARPORT_DEVICE_BITMASK;
        mode = 0;

        if ((fd = open(device, O_RDWR)) < 0)
            perror("Failed to open parport device");

        if (ioctl(fd, PPCLAIM) < 0)
            perror("ioctl(PPCLAIM) failed");

        /* get current per port mode */
        if (ioctl(fd, PPRDATA, &mode) < 0)
            perror("ioctl(PPRDATA) failed - mode 'off'");

        /* set per port on/off/rst mode */
        if (strcmp(command, COMMAND_OFF) == 0)
        {
            mode &= ~mask;
            if (ioctl(fd, PPWDATA, &mode) < 0)
                perror("ioctl(PPWDATA) failed - mode 'off'");
        }
        else if (strcmp(command, COMMAND_ON) == 0)
        {
            mode |= mask;
            if (ioctl(fd, PPWDATA, &mode) < 0)
                perror("ioctl(PPWDATA) failed - mode 'on'");
        }
        else
        {
            /* command 'restart' */
            /* first turn off */
            mode &= ~mask;
            if (ioctl(fd, PPWDATA, &mode) < 0)
                perror("ioctl(PPWDATA) failed - mode 'rst-off'");
            sleep(REBOOT_SLEEP_TIME);
            /* then turn on after sleep */
            mode |= mask;
            if (ioctl(fd, PPWDATA, &mode) < 0)
                perror("ioctl(PPWDATA) failed - mode 'rst-on'");
        }

        if (ioctl(fd, PPRELEASE) < 0)
            perror("ioctl(PPRELEASE) failed");

        close(fd);
    }
    else
    {
        /* tty device */
        /* prepare list of power sockets */
        mask &= TTY_DEVICE_BITMASK;

        if ((fd = open(device, O_RDWR)) < 0)
        {
            printf("Failed to open TTY device %s\n", device);
            return 2;
        }

        if (check_dev_params(fd) < 0)
        {
            printf("Error while checking parameters %s\n", device);
            close(fd);
            return 3;
        }

        if (!recognize_power_switch(fd, &is_rebootable, &sockets_num))
        {
            printf("Power switch was not "
                   "recognized on device %s\n", device);
            close(fd);
            return 4;
        }

        if (strcmp(command, COMMAND_RST) == 0)
        {
            if (is_rebootable == 1)
            {
                turn_on_off(fd, mask, sockets_num, RESET);
            }
            else
            {
                turn_on_off(fd, mask, sockets_num, TURN_OFF);
                sleep(REBOOT_SLEEP_TIME);
                turn_on_off(fd, mask, sockets_num, TURN_ON);
            }
        }
        else
        {
            turn_on_off(fd, mask, sockets_num,
                        (strcmp(command, COMMAND_ON) == 0) ?
                                                TURN_ON :
                                                    TURN_OFF);
        }

        close(fd);
    }

    return 0;
}
