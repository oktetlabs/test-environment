/** @file
 * @brief Unix Test Agent
 *
 * Routine to control power lines via power switch device.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Igor Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 * @author Ivan V. Soloduha <Ivan.Soloducha@oktetlabs.ru>
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "Power switch"

#include "te_config.h"
#include "agentlib_config.h"

#if defined (ENABLE_POWER_SW) 

#include "logger_api.h"
#include "te_errno.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#if HAVE_LINUX_PPDEV_H
#include <linux/ppdev.h>
#endif

#include "te_power_sw.h"

#define DEV_TYPE_DFLT           DEV_TYPE_PARPORT
#define PARPORT_DEV_DFLT        "/dev/parport0"
#define TTY_DEV_DFLT            "/dev/ttyS0"
#define DIGISPARK_DEV_DFLT      "/dev/ttyACM0"
#define PARPORT_DEV_BITMASK     0xff /* parport, up to 8 lines */
#define TTY_DEV_BITMASK         0xffff /* TTY device, up to 16 lines */
#define REBOOT_SLEEP_TIME       2 /* seconds */
#define TURN_OFF                0
#define TURN_ON                 1
#define RESET                   2
#define TTY_DEV_BAUDRATE        B115200

/**
 * Turn ON or turn OFF power lines by bitmask.
 *
 * @param[in]   fd          Descriptor associated with opened TTY device.
 * @param[in]   mask        Bitmask of power lines to be turned ON or
 *                          turned OFF.
 * @param[in]   sock_num    Number of power lines device can control.
 * @param[in]   cmd         Turn specified lines ON or OFF.
 */
static int
turn_on_off(int fd, unsigned int mask, int sock_num, int cmd)
{
    int             i, j, rc;
    unsigned int    socket;
    char            command[2];
    char            reply[2];

    command[1] = '\r';
    socket = 1;
    for (i = 0; i < sock_num; i++)
    {
        if (mask & socket)
        {
            command[0] = ((cmd == TURN_ON) ?
                            0x60 :
                            (cmd == TURN_OFF) ?
                                0x40 :
                                    0x50 /* RESET command */) | i;

            j = 0;
            rc = -1;
            while (++j < 5)
            {
                if ((rc = write(fd, command, 2)) == -1)
                {
                    ERROR("Failed to send command to TTY device");
                    usleep(100000); /* Repeat attempt 0.1 sec later */
                    continue;
                }

                if ((rc = read(fd, reply, 2)) == -1)
                {
                    ERROR("Failed to receive reply from TTY device");
                    usleep(100000); /* Repeat attempt 0.1 sec later */
                    continue;
                }

                if (command[0] == reply[0] && reply[1] == '#')
                {
                    rc = 0;
                    break;
                }

                ERROR("Command reply from TTY device does "
                      "not match command");
                usleep(100000); /* Repeat attempt 0.1 sec later */
            }

            if (rc != 0)
            {
                ERROR("TTY device did not executed command");
                return rc;
            }
        }

        socket *= 2;
    }

    return 0;
}

/**
 * Get information about the device opened.
 *
 * @param[in]   fd          Descriptor associated with opened TTY device.
 * @param[out]  rebootable  Whether device supports reboot commands.
 * @param[out]  sockets_num Number of power lines device can control.
 *
 * @return      Operation status.
 */
static int
recognize_power_switch(int fd, int *rebootable, int *sockets_num)
{
    char    command[] = "$\r";
    char    reply[5];
    int     i = 0;
    int     rc = 0;

    while (++i < 5)
    {
        /* Send 'get signature' command. */
        rc = write(fd, command, 2);
        /* Read 1 byte of echo, 3 bytes of reply, and 1 more byte for '#'. */
        rc = read(fd, reply, 5);
        /* Check if signature (bytes 1-3) is valid. */
        if (reply[1] != '1' || (reply[2] & 0x40) == 0 || reply[3] != '0' )
        {
            ERROR("Power switch signature was not received on specified"
                  "power TTY device.");

            /*
             * This may happen sometimes.
             * We'll try again several times and cry when
             * all our attempts failed.
             */
        }
        else
        {
            *sockets_num = reply[2] & 0x1F;
            *rebootable = (reply[2] & 0x20)? 1 : 0;
            rc = 1;
            break;
        }

        usleep(100000); /* Repeat attempt 0.1 sec later */
    };

    return rc;
}

/**
 * Add prober TTY settins: Baud 115200, parity check OFF, 8bit.
 *
 * @param[in]   fd  Device descriptor.
 *
 * @return      0 - success, -1 fail.
 */
static int
check_dev_params(int fd)
{
    struct termios term;

    if (tcgetattr(fd, &term) < 0)
    {
        ERROR("Failed to get device attributes.\n");
        return -1;
    }

    term.c_iflag = 0;
    term.c_oflag = 0;
    term.c_cflag = CREAD | CLOCAL | CS8;
    term.c_lflag = 0;

    if (cfsetospeed(&term, B115200) < 0)
    {
        ERROR("Failed to set output baudrate\n");
        return -1;
    }

    if (cfsetispeed(&term, B115200) < 0)
    {
        ERROR("Failed to set input baudrate\n");
        return -1;
    }

    if (tcsetattr(fd, TCSADRAIN, &term) < 0)
    {
        ERROR("Applying parameters failed\n");
        return -1;
    }

    return 0;
}

/**
 * Process power switch command for parport power switch
 *
 * @param dev       power switch device name
 * @param mask      power lines bitmask
 * @param cmd       power switch command turn ON, turn OFF or restart
 *
 * @return          0, otherwise -1
 */
static int
process_parport_cmd(const char *dev, int mask, int cmd)
{
    int             fd;
    unsigned char   mode = 0;

    mask &= PARPORT_DEV_BITMASK;

    if ((fd = open(dev, O_RDWR)) < 0)
    {
        ERROR("Failed to open parport device %s.", dev);
        return -1;
    }

    /*
     * FIXME To prevent race condition when different
     * processes do IOCTLs on parport device.
     */
    if (flock(fd, LOCK_EX) < 0)
    {
        ERROR("Failed to lock parport device file %s.", dev);
        close(fd);
        return -1;
    }

    if (ioctl(fd, PPCLAIM) < 0)
    {
        ERROR("ioctl(PPCLAIM) failed.");
        close(fd);
        return -1;
    }

    /* get current per port mode */
    if (ioctl(fd, PPRDATA, &mode) < 0)
    {
        ERROR("ioctl(PPRDATA) failed - mode 'off'.");
        close(fd);
        return -1;
    }

    /* set per port on/off/rst mode */
    if (cmd == CMD_TURN_OFF)
    {
        mode &= ~mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            ERROR("ioctl(PPWDATA) failed - mode 'off'.");
    }
    else if (cmd == CMD_TURN_ON)
    {
        mode |= mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            ERROR("ioctl(PPWDATA) failed - mode 'on'.");
    }
    else
    {
        /* command 'restart' */
        /* first turn off */
        mode &= ~mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            ERROR("ioctl(PPWDATA) failed - mode 'rst-off'.");
        sleep(REBOOT_SLEEP_TIME);
        /* then turn on after sleep */
        mode |= mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            ERROR("ioctl(PPWDATA) failed - mode 'rst-on'.");
    }

    if (ioctl(fd, PPRELEASE) < 0)
        ERROR("ioctl(PPRELEASE) failed.");

    close(fd);

    return 0;
}

/**
 * Process power switch command for tty power switch
 *
 * @param dev       power switch device name
 * @param mask      power lines bitmask
 * @param cmd       power switch command turn ON, turn OFF or restart
 *
 * @return          0, otherwise -1
 */
static int
process_tty_cmd(const char *dev, int mask, int cmd)
{
    int fd;
    int is_rebootable = 0;
    int sockets_num = 0;
    int rc;

    /* prepare list of power sockets */
    mask &= TTY_DEV_BITMASK;

    if ((fd = open(dev, O_RDWR)) < 0)
    {
        ERROR("Failed to open TTY device %s.", dev);
        return -1;
    }

    if (check_dev_params(fd) < 0)
    {
        ERROR("Error while checking parameters "
              "of TTY device %s.", dev);
        close(fd);
        return -1;
    }

    if (!recognize_power_switch(fd, &is_rebootable, &sockets_num))
    {
        ERROR("Power switch was not "
              "recognized on TTY device %s.", dev);
        close(fd);
        return -1;
    }

    if (cmd == CMD_RESTART)
    {
        if (is_rebootable == 1)
        {
            rc = turn_on_off(fd, mask, sockets_num, RESET);
        }
        else
        {
            if ((rc = turn_on_off(fd, mask, sockets_num,
                                  TURN_OFF)) == 0)
            {
                sleep(REBOOT_SLEEP_TIME);
                rc = turn_on_off(fd, mask, sockets_num, TURN_ON);
            }
        }
    }
    else
    {
        rc = turn_on_off(fd, mask, sockets_num,
                    (cmd == CMD_TURN_ON) ? TURN_ON : TURN_OFF);
    }

    close(fd);

    if (rc != 0)
        return -1;

    return 0;
}

/**
 * Process power switch command for digispark power switch
 *
 * @param dev       power switch device name
 * @param socket_id power socket id
 * @param cmd       power switch command turn ON, turn OFF or restart
 *
 * @return          status code
 */
static int
process_digispark_cmd(const char *dev, int socket_id, int cmd)
{
    int     fd;
    int     rc;
    char    command[2] = {0, '\r'};

    /* Check socket id. Should be integer 1-4 */
    if (socket_id == 0 || socket_id > 4)
        return -1;

    if (cmd == CMD_RESTART)
        command[0] = 0x50 | (socket_id - 1);
    else if (cmd == CMD_TURN_ON)
        command[0] = 0x60 | (socket_id - 1);
    else if (cmd == CMD_TURN_OFF)
        command[0] = 0x40 | (socket_id - 1);
    else
        return -1;

    if ((fd = open(dev, O_WRONLY)) < 0)
    {
        ERROR("Failed to open device %s to write.", dev);
        return -1;
    }

    rc = write(fd, command, sizeof(command));

    close(fd);

    if (rc == sizeof(command))
    {
        return 0;
    }
    else
    {
        ERROR("Failed to send command to %s device", dev);
        return -1;
    }
}

/**
 * Turn ON, turn OFF or restart power lines specified by mask
 *
 * @param type      power switch device type tty/parport
 * @param dev       power switch device name
 * @param mask      power lines bitmask
 * @param cmd       power switch command turn ON, turn OFF or restart
 *
 * @return          0, otherwise -1
 */
int
power_sw(int type, const char *dev, int mask, int cmd)
{
    if (cmd == CMD_UNSPEC)
        return 0;

    /* Substitute default device type if not specified */
    if (type == DEV_TYPE_UNSPEC)
        type = DEV_TYPE_DFLT;

    /* Substitute default device name if not specified */
    if (dev == NULL || strcmp(dev, "unspec") == 0)
    {
        if (type == DEV_TYPE_PARPORT)
            dev = PARPORT_DEV_DFLT;
        else if (type == DEV_TYPE_TTY)
            dev = TTY_DEV_DFLT;
        else
            dev = DIGISPARK_DEV_DFLT;
    }

    if (type == DEV_TYPE_PARPORT)
        return process_parport_cmd(dev, mask, cmd);
    else if (type == DEV_TYPE_TTY)
        return process_tty_cmd(dev, mask, cmd);
    else
        return process_digispark_cmd(dev, mask, cmd);
}

#endif /* ENABLE_POWER_SW */
