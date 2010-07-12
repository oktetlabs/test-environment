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

#ifdef WITH_DAHDI_SUPPORT
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <dahdi/user.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include "logger_api.h"
#include "te_errno.h"

#define BLOCKSIZE       183     /* Size of block for reading from channel */
#define SAMPLE_RATE     8000.0  /* Sample rate */
#define SILENCE_TONE    10000.0 /* Max goertzel result for silence */
#define GET_PHONE       9000    /* Bytes to wait dialtone */

enum {

        HZ350 = 0, HZ440 = 1, HZ480 = 2, HZ620 = 3
};

float freqs[4] = {
    350.0, 440.0, 480.0, 620.0};

/**
 * Realisation of goertzel algorithm
 *
 * @param seq   input DSP sequence
 * @param len   length of @b seq
 * @param freq  frequency to check
 *
 * @return @b freq DFT component of @b seq
 */
float
telephony_goertzel(short *seq, int len, float freq)
{

    int s = 0;
    int s1 = 0;
    int s2 = 0;
    int chunky = 0;
    int fac = (int)(32768.0 * 2.0 * cos(2.0 * M_PI * freq / SAMPLE_RATE));
    int i;

    for (i = 0; i < len; i++)
    {

        s2 = s1;
        s1 = s;

        s = ((fac * s1) >> 15) - s2 + (seq[i] >> chunky);

        if (abs(s) > 32768) {

            chunky++;
            s >>=  1;
            s1 >>= 1;
            s2 >>= 1;
        }
    }

    return (float)((s * s) + (s1 * s1) -  ((s1 * s) >> 15) * fac) *
        (float)(1 << (chunky * 2));
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
    char dahdi_dev[40];
    struct dahdi_bufferinfo bi;
    int chan; /* File descriptor of channel */
    int param; /* ioctl parameter */

    snprintf(dahdi_dev, 40, "/dev/dahdi/%d", port);

    if ((chan = open(dahdi_dev, O_RDWR)) < 0)
    {
        ERROR("open() failed: errno %d", errno);

        return -1;
    }

    /* Set the channel to operate with linear mode */
    param = 1;
    if (ioctl(chan, DAHDI_SETLINEAR, &param))
    {
        close(chan);
        ERROR("setting linear mode failed: errno %d", errno);

        return -1;
    }

    memset(&bi, 0, sizeof(struct dahdi_bufferinfo));
    if (ioctl(chan, DAHDI_GET_BUFINFO, &bi))
    {
        close(chan);
        ERROR("getting buffer information failed: errno %d", errno);

        return -1;
    }

    bi.numbufs = 2;
    bi.bufsize = BLOCKSIZE;
    bi.txbufpolicy = DAHDI_POLICY_IMMEDIATE;
    bi.rxbufpolicy = DAHDI_POLICY_IMMEDIATE;
    if (ioctl(chan, DAHDI_SET_BUFINFO, &bi))
    {
        close(chan);
        ERROR("setting buffer information failed: errno %d\n", errno);

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
    if (close(chan) != 0)
    {
        ERROR("close() on channel failed: errno %d", errno);

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
    
    if (ioctl(chan, DAHDI_HOOK, &param)) {
        ERROR("picking up failed: errno %d\n", errno);

        return -1;
    }

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
    
    if (ioctl(chan, DAHDI_HOOK, &param)) {
        ERROR("hanging up failed: errno %d\n", errno);

        return -1;
    }
}


/**
 * Check dial tone on specified port.
 *
 * @param chan      channel file descriptor
 *
 * @return 0 on dial tone, 1 on another signal or  -1 on failure
 */
int
telephony_check_dial_tone(int chan)
{
    short buf[BLOCKSIZE];
    int len;
    int i;
    float pows[4];

    /* Ignore getting the phone sound */
    sleep(2);

    for (i = 0; i < GET_PHONE / (BLOCKSIZE * 2); i++)
    {

        /* Ignore getting the phone sound */
        len = read(chan, buf, BLOCKSIZE * 2);

        if (len != BLOCKSIZE * 2)
        {

            int dummy;

            ioctl(chan, DAHDI_GETEVENT, &dummy);
            i--;
        }
    }

    pows[HZ350] = telephony_goertzel(buf, BLOCKSIZE, freqs[HZ350]);
    pows[HZ440] = telephony_goertzel(buf, BLOCKSIZE, freqs[HZ440]);
    pows[HZ480] = telephony_goertzel(buf, BLOCKSIZE, freqs[HZ480]);
    pows[HZ620] = telephony_goertzel(buf, BLOCKSIZE, freqs[HZ620]);

    if (pows[HZ350] < SILENCE_TONE || pows[HZ440] < SILENCE_TONE)
        return 0;

    if (pows[HZ350] < (pows[HZ480] * 5.0) || pows[HZ350] < (pows[HZ620] * 5.0))
        return 0;

    if (pows[HZ440] < (pows[HZ480] * 5.0) || pows[HZ440] < (pows[HZ620] * 5.0))
        return 0;

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
telephony_dial_number(int chan, char *number)
{
    struct dahdi_dialoperation dop;

    memset(&dop, 0, sizeof(dop));
    dop.op = DAHDI_DIAL_OP_REPLACE;
    dop.dialstr[0] = 'T';
    
    dahdi_copy_string(dop.dialstr + 1, number, sizeof(dop.dialstr));

    if (ioctl(chan, DAHDI_DIAL, &dop)) {
        ERROR("Unable to dial: errno %d", errno);

        return -1;
    }

    return 0;
}

/**
 * Wait to call on the channel
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int
telephony_call_wait(int chan)
{
    int param;
    do {            
        for(;;)
        {
            param = DAHDI_IOMUX_SIGEVENT;

            if (ioctl(chan, DAHDI_IOMUX, &param))
            {

                ERROR("Unable to waiting call: %d", errno);

                return -1;
            }

            if (param & DAHDI_IOMUX_SIGEVENT) 
                break;
        }

        ioctl(chan, DAHDI_GETEVENT, &param);
    } while (param != 18);

    return 0;
}

#endif /* WITH_DAHDI_SUPPORT */
