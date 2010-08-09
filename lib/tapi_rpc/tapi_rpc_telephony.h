/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of telephony.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Evgeny Omelchenko <Evgeny.Omelchenko@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_TAPI_RPC_TELEPHONY_H__
#define __TE_TAPI_RPC_TELEPHONY_H__

#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Numbering plans */
enum te_numbering_plan {
    TE_TELEPHONY_NANP        = (1 << 3) | (1 << 9),     /**< North America numbering plan */
    TE_TELEPHONY_FRANCE      = (1 << 9),                /**< France numbering plan */
    TE_TELEPHONY_EUROPE_425  = (1 << 7),                /**< Most Europe numbering plan */
    TE_TELEPHONY_JP          = (1 << 4),                /**< Japan numbering plan */
    TE_TELEPHONY_CHINA       = (1 << 10)                /**< China numbering plan */
};


/**
  * Open channel and bind telephony card port with it
  *
  * @param rpcs     RPC server handle
  * @param port     number of telephony card port 
  *
  * @return Channel file descriptor, otherwise -1 
  */
extern int rpc_telephony_open_channel(rcf_rpc_server *rpcs, int port);

/**
  * Close channel
  *
  * @param rpcs     RPC server handle
  * @param chan     channel file descriptor
  *
  * @return 0 on success or -1 on failure
  */
extern int rpc_telephony_close_channel(rcf_rpc_server *rpcs, int chan);

/**
  * Pick up the phone
  *
  * @param rpcs     RPC server handle
  * @param chan     channel file descriptor
  *
  * @return 0 on success or -1 on failure
  */
extern int rpc_telephony_pickup(rcf_rpc_server *rpcs, int chan);

/**
  * Hang up the phone
  *
  * @param rpcs     RPC server handle
  * @param chan     channel file descriptor
  *
  * @return 0 on success or -1 on failure
  */
extern int rpc_telephony_hangup(rcf_rpc_server *rpcs, int chan);

/**
 * Check dial tone on specified channel.
 *
 * @param rpcs      RPC server handle
 * @param chan      channel file descriptor
 * @param plan      numbering plan
 * @param state     pointer on result of telephony_check_dial_tone() (OUT)
 *
 * @return 0 on success or -1 on failure
 */
extern int rpc_telephony_check_dial_tone(rcf_rpc_server *rpcs, 
                                         int chan, enum te_numbering_plan plan,
                                         te_bool *state);

/**
  * Dial number
  *
  * @param rpcs     RPC server handle
  * @param chan     channel file descriptor
  * @param number   telephony number to dial
  *
  * @return 0 on success or -1 on failure
  */
extern int rpc_telephony_dial_number(rcf_rpc_server *rpcs, 
                                     int chan, const char *number);

/**
  * Wait to call on the channel
  *
  * @param rpcs     RPC server handle
  * @param chan     channel file descriptor
  * @param timeout  timeout in microsecond
  *
  * @return 0 on success or -1 on failure
  */
extern int rpc_telephony_call_wait(rcf_rpc_server *rpcs, int chan, int timeout);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_TELEPHONY_H__ */
