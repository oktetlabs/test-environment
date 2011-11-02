/** @file
 * @brief Unix Test Agent
 *
 * Routines for telphony testing.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Evgeny Omelchenko <Evgeny.Omelchenko@oktetlabs.ru>
 *
 * $Id: $
 */

#define TE_LGR_USER     "Telephony"

#include "te_config.h"
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include "dahdi_user.h"
#include "logger_api.h"
#include "te_errno.h"

#define BLOCKSIZE       360     /**< Size of block for reading from channel */
#define SAMPLE_RATE     8000.0  /**< Sample rate */
#define SILENCE_TONE    10000.0 /**< Max goertzel result for silence */
#define GET_PHONE       10000    /**< Bytes to wait dialtone */
#define DAHDI_DEV_LEN   40      /**< Length of dahdi channel device name */


/** Frequncies array */
double freqs[] = {
    200.0, 300.0, 330.0, 350.0, 400.0, 413.0, 420.0, 425.0, 438.0, 440.0, 450.0, 660.0, 700.0, 800.0, 1000.0
};

/**
 * Realisation of goertzel algorithm
 *
 * @param seq   input DSP sequence
 * @param len   length of @b seq
 * @param freq  frequency to check
 *
 * @return @b freq DFT component of @b seq
 */

static double
telephony_goertzel(short *buf, int len, double freq)
{
    double s = 0.0;
    double s1 = 0.0;
    double s2 = 0;
    double coeff = 2.0 * cos(2.0 * M_PI * freq / SAMPLE_RATE);
    int i;

    for (i = 0; i < len; i++)
    {
        s = buf[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s;
    }

    return s2*s2 + s1*s1 - coeff*s2*s1;
}

/**
 * Open channel and bind telephony card port with it
 *
 * @param port     number of telephony card port
 *
 * @return Channel file descriptor, otherwise -1 
 */
int
telephony_open_channel(int port)
{
    char                    dahdi_dev[DAHDI_DEV_LEN];
    struct dahdi_bufferinfo bi;
    int                     chan; /* File descriptor of channel */
    int                     param; /* ioctl parameter */

    snprintf(dahdi_dev, DAHDI_DEV_LEN, "/dev/dahdi/%d", port);

    if ((chan = open(dahdi_dev, O_RDWR)) < 0)
    {
        ERROR("open() failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    /* Set the channel to operate with linear mode */
    param = 1;
    if (ioctl(chan, DAHDI_SETLINEAR, &param) < 0)
    {
        close(chan);
        ERROR("setting linear mode failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    memset(&bi, 0, sizeof(bi));
    if (ioctl(chan, DAHDI_GET_BUFINFO, &bi) < 0)
    {
        close(chan);
        ERROR("getting buffer information failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    bi.numbufs = 2;
    bi.bufsize = BLOCKSIZE;
    bi.txbufpolicy = DAHDI_POLICY_IMMEDIATE;
    bi.rxbufpolicy = DAHDI_POLICY_IMMEDIATE;
    if (ioctl(chan, DAHDI_SET_BUFINFO, &bi) < 0)
    {
        close(chan);
        ERROR("setting buffer information failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    return chan;
}

/**
 * Close channel
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int
telephony_close_channel(int chan)
{
    if (close(chan) < 0)
    {
        ERROR("close() on channel failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * Pick up the phone
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int
telephony_pickup(int chan)
{
    int param = DAHDI_OFFHOOK;

    if (ioctl(chan, DAHDI_HOOK, &param) < 0) {
        ERROR("picking up failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    /* Ignore getting the phone sound */
    sleep(2);

    return 0;
}

/**
 * Hang up the phone
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int
telephony_hangup(int chan)
{
    int param = DAHDI_ONHOOK;

    if (ioctl(chan, DAHDI_HOOK, &param) < 0) {
        ERROR("hanging up failed: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    return 0;
}


/**
 * Check dial tone on specified port.
 *
 * @param chan      channel file descriptor
 *
 * @return 0 on dial tone, 1 on another signal or  -1 on failure
 */
int
telephony_check_dial_tone(int chan, int plan)
{
    int             max1 = -1;
    int             max2 = -1;
    int             max3 = -1;
    int             max4 = -1;
    short           buf[BLOCKSIZE];
    int             len;
    unsigned int    i;
    float           pows[sizeof(freqs) / sizeof(double)];

    for (i = 0; i < GET_PHONE / (BLOCKSIZE * 2); i++)
    {
        /* Ignore getting the phone sound */
        len = read(chan, buf, BLOCKSIZE * 2);

        if (len < 0)
        {
            ERROR("unable to read() channel: errno %d (%s)",
                  errno, strerror(errno));
            return -1;
        }

        if (len != BLOCKSIZE * 2)
        {

            int param;

            if (ioctl(chan, DAHDI_GETEVENT, &param) < 0)
            {
                ERROR("unable to getting dahdi event: %d (%s)",
                      errno, strerror(errno));
                return -1;
            }
            i--;
        }
    }

    for (i = 0; i < sizeof(freqs) / sizeof(double); i++)
        pows[i] = telephony_goertzel(buf, BLOCKSIZE, freqs[i]);

    for (i = 0; i < sizeof(freqs) / sizeof(double); i++)
    {
        if (pows[i] < SILENCE_TONE)
            continue;

        if (max1 == -1 || pows[i] > pows[max1])
        {
            max4 = max3;
            max3 = max2;
            max2 = max1;
            max1 = i;
        }
        else if (max2 == -1 || pows[i] > pows[max2])
        {
            max4 = max3;
            max3 = max2;
            max2 = i;
        }
        else if (max3 == -1 || pows[i] > pows[max3])
        {
            max4 = max3;
            max3 = i;
        }
        else if (max4 == -1 || pows[i] > pows[max4])
            max4 = i;
    }

    if (((1 << max1 | 1 << max2 | 1 << max3 | 1 << max4) & plan) != plan)
        return 0;
    else
        return 1;
}

/**
 * Dial number
 *
 * @param chan     channel file descriptor
 * @param number   telephony number to dial
 *
 * @return 0 on success or -1 on failure
 */
int
telephony_dial_number(int chan, const char *number)
{
    struct dahdi_dialoperation dop;

    memset(&dop, 0, sizeof(dop));
    dop.op = DAHDI_DIAL_OP_REPLACE;
    dop.dialstr[0] = 'T';

    dahdi_copy_string(dop.dialstr + 1, number, sizeof(dop.dialstr));

    if (ioctl(chan, DAHDI_DIAL, &dop) < 0) {
        ERROR("unable to dial number: errno %d (%s)", errno, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * Wait to call on the channel
 *
 * @param chan     channel file descriptor
 * @param timeout  timeout in microsecond
 *
 * @return Status code
 */
te_errno
telephony_call_wait(int chan, int timeout)
{
    int     param;
    fd_set  ex_fds;
    struct timeval tval;

    tval.tv_sec = timeout / 1000;
    tval.tv_usec = (timeout % 1000) * 1000;

    do {
        FD_ZERO(&ex_fds);
        FD_SET(chan, &ex_fds);

        param = select(chan + 1, NULL, NULL, &ex_fds, &tval);

        if (param == 0)
            return TE_ERPCTIMEOUT;
        else 
            return TE_RC(TE_TA_UNIX, errno);
        ioctl(chan, DAHDI_GETEVENT, &param);
    } while (param != DAHDI_EVENT_RINGBEGIN);

   return 0;
}

