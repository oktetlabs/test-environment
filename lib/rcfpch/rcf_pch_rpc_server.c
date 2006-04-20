/** @file 
 * @brief RCF Portable Command Handler
 *
 * RCF RPC server entry point. The file is included to another one
 * to create TA-builtin and standalone RPC servers.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "ta_common.h"
#include "rpc_transport.h"

#ifdef HAVE_SIGNAL_H
static void
sig_handler(int s)
{
    UNUSED(s);
    exit(1);
}
#endif

/**
 * Entry function for RPC server. 
 *
 * @param name   RPC server name
 *
 * @return NULL
 */
void *
rcf_pch_rpc_server(const char *name)
{
    rpc_transport_handle handle;
    uint8_t             *buf = NULL;
    
#define STOP(msg...)    \
    do {                \
        ERROR(msg);     \
        goto cleanup;   \
    } while (0)
    
#ifdef HAVE_SIGNAL_H
    signal(SIGTERM, sig_handler);
#endif

    if (logfork_register_user(name) != 0)
    {
        fprintf(stderr,
                "logfork_register_user() failed to register %s server\n",
                name);
        fflush(stderr);
    }
    
#ifdef HAVE_PTHREAD_H
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif    

    if (rpc_transport_connect_ta(name, &handle) != 0)
        return NULL;

    if ((buf = malloc(RCF_RPC_HUGE_BUF_LEN)) == NULL)
        STOP("Failed to allocate the buffer for RPC data");

#if defined(__CYGWIN__) && !defined(WINDOWS)
    RING("RPC server '%s' (%u-bit, cygwin) (re-)started (PID %d, TID %u)",
         name, (unsigned)(sizeof(void *) << 3),
         (int)getpid(), (unsigned)thread_self());
#else
    RING("RPC server '%s' (%u-bit) (re-)started (PID %d, TID %u)",
         name, (unsigned)(sizeof(void *) << 3),
         (int)getpid(), (unsigned)thread_self());
#endif             

    while (TRUE)
    {
        char      rpc_name[RCF_RPC_MAX_NAME];        
                                        /* RPC name, i.e. "bind" */
        void     *in = NULL;            /* Input parameter C structure */
        void     *out = NULL;           /* Output parameter C structure */
        rpc_info *info = NULL;          /* RPC information */
        te_bool   result = FALSE;       /* "rc" attribute */
        size_t    len = RCF_RPC_HUGE_BUF_LEN;
        
        strcpy(rpc_name, "Unknown");
        
        if (rpc_transport_recv(handle, buf, &len, 0xFFFF) != 0)
            STOP("Connection with TA is broken!");
            
        if (strcmp((char *)buf, "FIN") == 0)
        {
            if (rpc_transport_send(handle, (uint8_t *)"OK", 
                                   sizeof("OK")) == 0)
            {
                RING("RPC server '%s' finished", name);
            }
            else
            {
                ERROR("Failed to send 'OK' in response to 'FIN'");
            }
            goto cleanup;
        }
        
        if (rpc_xdr_decode_call(buf, len, rpc_name, &in) != 0)
        {
            ERROR("Decoding of RPC %s call failed", rpc_name);
            goto result;
        }
        
        info = rpc_find_info(rpc_name);
        assert(info != NULL);
        
        if ((out = calloc(1, info->out_len)) == NULL)
        {
            ERROR("Memory allocation failure");
            goto result;
        }
        
        result = (info->rpc)(in, out, NULL);
        
    result: /* Send an answer */
        
        if (in != NULL && info != NULL)
            rpc_xdr_free(info->in, in);
        free(in);
            
        len = RCF_RPC_HUGE_BUF_LEN;
        if (rpc_xdr_encode_result(rpc_name, result, (char *)buf, 
                                  &len, out) != 0)
        {
            STOP("Fatal error: encoding of RPC %s output "
                 "parameters failed", rpc_name);
        }
            
        if (info != NULL)
            rpc_xdr_free(info->out, out);
        free(out);
        
        if (rpc_transport_send(handle, buf, len) != 0)
            STOP("Sending data failed in main RPC server loop");
    }

cleanup:    
    rpc_transport_close(handle);
    free(buf);
    
#undef STOP    

    return NULL;
}

