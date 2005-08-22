/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from aio.h.
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_AIO_H__
#define __TE_RPC_AIO_H__

#include "te_rpc_defs.h"


/** TA-independent operation code for lio_listio function */
typedef enum rpc_lio_opcode {
    RPC_LIO_READ,
    RPC_LIO_WRITE,
    RPC_LIO_NOP,
    RPC_LIO_UNKNOWN
} rpc_lio_opcode; 

#ifdef HAVE_AIO_H

/** Convert RPC lio_listio opcode to native one */
static inline int
lio_opcode_rpc2h(rpc_lio_opcode opcode)
{
    switch (opcode)
    {
        RPC2H(LIO_READ);
        RPC2H(LIO_WRITE);
        RPC2H(LIO_NOP);   
        default: return LIO_READ + LIO_WRITE + LIO_NOP + 1;
    }
}

/** Convert native lio_listio opcode to RPC one */
static inline rpc_lio_opcode
lio_opcode_h2rpc(int opcode)
{
    switch (opcode)
    {
        H2RPC(LIO_READ); 
        H2RPC(LIO_WRITE);
        H2RPC(LIO_NOP);  
        default: return RPC_LIO_UNKNOWN;
    }
}
#endif

/** Convert RPC lio_listio opcode to string */
static inline const char *
lio_opcode_rpc2str(rpc_lio_opcode opcode)
{
    switch (opcode)
    {
        RPC2STR(LIO_READ); 
        RPC2STR(LIO_WRITE);
        RPC2STR(LIO_NOP);  
        default: return "LIO_UNKNOWN";
    }
}


/** TA-independent modes for lio_listio function */
typedef enum rpc_lio_mode {
    RPC_LIO_WAIT,
    RPC_LIO_NOWAIT,
    RPC_LIO_MODE_UNKNOWN
} rpc_lio_mode; 

#ifdef HAVE_AIO_H

/** Convert RPC lio_listio option to native one */
static inline int
lio_mode_rpc2h(rpc_lio_mode mode)
{
    switch (mode)
    {
        RPC2H(LIO_WAIT);
        RPC2H(LIO_NOWAIT);
        default: return LIO_WAIT + LIO_NOWAIT + 1;
    }
}

/** Convert native lio_listio mode to RPC one */
static inline rpc_lio_mode
lio_mode_h2rpc(int mode)
{
    switch (mode)
    {
        H2RPC(LIO_WAIT); 
        H2RPC(LIO_NOWAIT);
        default: return RPC_LIO_MODE_UNKNOWN;
    }
}
#endif

/** Convert RPC lio_listio mode to string */
static inline const char *
lio_mode_rpc2str(rpc_lio_mode mode)
{
    switch (mode)
    {
        RPC2STR(LIO_WAIT); 
        RPC2STR(LIO_NOWAIT);
        default: return "LIO_MODE_UNKNOWN";
    }
}


/** TA-independent return values for aio_cancel function */
typedef enum rpc_aio_cancel_retval {
    RPC_AIO_CANCELED,
    RPC_AIO_NOTCANCELED,
    RPC_AIO_ALLDONE,
    RPC_AIO_UNKNOWN
} rpc_aio_cancel_retval; 

#ifdef HAVE_AIO_H

/** Convert RPC aio_cancel return value to native one */
static inline int
aio_cancel_retval_rpc2h(rpc_aio_cancel_retval ret)
{
    switch (ret)
    {
        RPC2H(AIO_CANCELED);
        RPC2H(AIO_NOTCANCELED);
        RPC2H(AIO_ALLDONE);
    }
}

/** Convert native aio_cancel return to RPC one */
static inline rpc_aio_cancel_retval
aio_cancel_retval_h2rpc(int ret)
{
    switch (ret)
    {
        H2RPC(AIO_CANCELED);
        H2RPC(AIO_NOTCANCELED);
        H2RPC(AIO_ALLDONE);
        default: return RPC_AIO_UNKNOWN;
    }
}
#endif

/** Convert RPC aio_cancel return to string */
static inline const char *
aio_cancel_retval_rpc2str(rpc_aio_cancel_retval ret)
{
    switch (ret)
    {
        RPC2STR(AIO_CANCELED);
        RPC2STR(AIO_NOTCANCELED);
        RPC2STR(AIO_ALLDONE);
        default: return "AIO_UNKNOWN";
    }
}


#endif /* !__TE_RPC_AIO_H__ */
