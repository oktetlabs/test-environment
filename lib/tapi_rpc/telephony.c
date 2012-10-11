/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of telephony
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Evgeny I. Omelchenko <Evgeny.Omelchenko@oktetlabs.ru>
 *
 * $Id: $
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "tapi_rpc_internal.h"
#include "te_printf.h"
#include "tapi_rpc_telephony.h"
#include "conf_api.h"
#include "tapi_rpc_misc.h"

int
rpc_telephony_open_channel(rcf_rpc_server *rpcs, int port)
{
    tarpc_telephony_open_channel_in     in;
    tarpc_telephony_open_channel_out    out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_open_channel, -1);
    }

    in.port = port;

    rcf_rpc_call(rpcs, "telephony_open_channel", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_open_channel, "%d", "%d",
                 port, out.retval);
    RETVAL_INT(telephony_open_channel, out.retval);
}

int
rpc_telephony_close_channel(rcf_rpc_server *rpcs, int chan)
{
    tarpc_telephony_close_channel_in    in;
    tarpc_telephony_close_channel_out   out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_close_channel, -1);
    }

    in.chan = chan;

    rcf_rpc_call(rpcs, "telephony_close_channel", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_close_channel, "%d", "%d",
                 chan, out.retval);
    RETVAL_INT(telephony_close_channel, out.retval);
}

int
rpc_telephony_pickup(rcf_rpc_server *rpcs, int chan)
{
    tarpc_telephony_pickup_in   in;
    tarpc_telephony_pickup_out  out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_pickup, -1);
    }

    in.chan = chan;

    rcf_rpc_call(rpcs, "telephony_pickup", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_pickup, "%d", "%d",
                 chan, out.retval);
    RETVAL_INT(telephony_pickup, out.retval);
}

int
rpc_telephony_hangup(rcf_rpc_server *rpcs, int chan)
{
    tarpc_telephony_hangup_in   in;
    tarpc_telephony_hangup_out  out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_hangup, -1);
    }

    in.chan = chan;

    rcf_rpc_call(rpcs, "telephony_hangup", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_hangup, "%d", "%d",
                 chan, out.retval);
    RETVAL_INT(telephony_hangup, out.retval);
}

int
rpc_telephony_check_dial_tone(rcf_rpc_server *rpcs, int chan,
                              enum te_numbering_plan plan, te_bool *state)
{
    tarpc_telephony_check_dial_tone_in  in;
    tarpc_telephony_check_dial_tone_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_check_dial_tone, -1);
    }

    in.chan = chan;
    in.plan = plan;

    rcf_rpc_call(rpcs, "telephony_check_dial_tone", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_check_dial_tone, "%d, %d", "%s",
                 chan, plan,
                 out.retval ? (out.retval == -1 ? "true" : "-1") : "false");

    if (out.retval < 0)
        RETVAL_INT(telephony_check_dial_tone, -1);
    else
    if (state != NULL)
        *state = out.retval ? TRUE : FALSE;

    RETVAL_INT(telephony_check_dial_tone, 0);
}

int
rpc_telephony_dial_number(rcf_rpc_server *rpcs,
                          int chan, const char *number)
{
    tarpc_telephony_dial_number_in  in;
    tarpc_telephony_dial_number_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_dial_number, -1);
    }

    in.chan = chan;
    in.number = (char *)number;

    rcf_rpc_call(rpcs, "telephony_dial_number", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_dial_number, "%d, %s", "%d",
                 chan, number, out.retval);
    RETVAL_INT(telephony_dial_number, out.retval);
}

int
rpc_telephony_call_wait(rcf_rpc_server *rpcs, int chan, int timeout)
{

    tarpc_telephony_call_wait_in    in;
    tarpc_telephony_call_wait_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {

        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(telephony_call_wait, -1);
    }

    in.chan = chan;
    in.timeout = timeout;

    rcf_rpc_call(rpcs, "telephony_call_wait", &in, &out);

    TAPI_RPC_LOG(rpcs, telephony_call_wait, "%d, %d", "%d",
                 chan, timeout, out.retval);
    RETVAL_INT(telephony_call_wait, out.retval);
}

