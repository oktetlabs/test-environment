/** @file
 * @brief Unix Test Agent
 *
 * Routines for telphony testing, prototypes.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef _TELEPHONY_H
#define _TELEPHONY_H

#include "te_config.h"
#include "agentlib_config.h"

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

/**
 * Open channel and bind telephony card port with it
 *
 * @param port     number of telephony card port
 *
 * @return Channel file descriptor, otherwise -1
 */
int telephony_open_channel(int port);

/**
 * Close channel
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int telephony_close_channel(int chan);

/**
 * Pick up the phone
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int telephony_pickup(int chan);

/**
 * Hang up the phone
 *
 * @param chan     channel file descriptor
 *
 * @return 0 on success or -1 on failure
 */
int telephony_hangup(int chan);

/**
 * Check dial tone on specified port.
 *
 * @param chan      channel file descriptor
 *
 * @return 0 on dial tone, 1 on another signal or  -1 on failure
 */
int telephony_check_dial_tone(int chan, int plan);

/**
 * Dial number
 *
 * @param chan     channel file descriptor
 * @param number   telephony number to dial
 *
 * @return 0 on success or -1 on failure
 */
int telephony_dial_number(int chan, const char *number);

/**
 * Wait to call on the channel
 *
 * @param chan     channel file descriptor
 * @param timeout  timeout in microsecond
 *
 * @return Status code
 */
te_errno telephony_call_wait(int chan, int timeout);
#endif /* _TELEPHONY_H */
