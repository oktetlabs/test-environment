/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from aio.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_AIO_H
#include <aio.h>
#endif

#include "te_rpc_defs.h"
#include "te_rpc_aio.h"


/** Convert RPC lio_listio opcode to string */
const char *
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

#if HAVE_AIO_H

/** Convert RPC lio_listio opcode to native one */
int
lio_opcode_rpc2h(rpc_lio_opcode opcode)
{
    switch (opcode)
    {
        RPC2H_CHECK(LIO_READ);
        RPC2H_CHECK(LIO_WRITE);
        RPC2H_CHECK(LIO_NOP);   
        default: return 
#ifdef LIO_READ        
            LIO_READ + 
#endif            
#ifdef LIO_WRITE
            LIO_WRITE + 
#endif            
#ifdef LIO_NOP
            LIO_NOP + 
#endif            
            1;
    }
}

/** Convert native lio_listio opcode to RPC one */
rpc_lio_opcode
lio_opcode_h2rpc(int opcode)
{
    switch (opcode)
    {
        H2RPC_CHECK(LIO_READ); 
        H2RPC_CHECK(LIO_WRITE);
        H2RPC_CHECK(LIO_NOP);  
        default: return RPC_LIO_UNKNOWN;
    }
}

#endif /* HAVE_AIO_H */


/** Convert RPC lio_listio mode to string */
const char *
lio_mode_rpc2str(rpc_lio_mode mode)
{
    switch (mode)
    {
        RPC2STR(LIO_WAIT); 
        RPC2STR(LIO_NOWAIT);
        default: return "LIO_MODE_UNKNOWN";
    }
}

#ifdef HAVE_AIO_H

/** Convert RPC lio_listio option to native one */
int
lio_mode_rpc2h(rpc_lio_mode mode)
{
    switch (mode)
    {
        RPC2H_CHECK(LIO_WAIT);
        RPC2H_CHECK(LIO_NOWAIT);
        default: return 
#ifdef LIO_WAIT        
            LIO_WAIT + 
#endif
#ifdef LIO_NOWAIT            
            LIO_NOWAIT + 
#endif            
            1;
    }
}

/** Convert native lio_listio mode to RPC one */
rpc_lio_mode
lio_mode_h2rpc(int mode)
{
    switch (mode)
    {
        H2RPC_CHECK(LIO_WAIT); 
        H2RPC_CHECK(LIO_NOWAIT);
        default: return RPC_LIO_MODE_UNKNOWN;
    }
}
#endif /* HAVE_AIO_H */


/** Convert RPC aio_cancel return to string */
const char *
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

#ifdef HAVE_AIO_H

/** Convert RPC aio_cancel return value to native one */
int
aio_cancel_retval_rpc2h(rpc_aio_cancel_retval ret)
{
    switch (ret)
    {
        case -1: return -1;
        RPC2H_CHECK(AIO_CANCELED);
        RPC2H_CHECK(AIO_NOTCANCELED);
        RPC2H_CHECK(AIO_ALLDONE);
        default: return -1; /* FIXME */
    }
}

/** Convert native aio_cancel return to RPC one */
rpc_aio_cancel_retval
aio_cancel_retval_h2rpc(int ret)
{
    switch (ret)
    {
        case -1: return -1;
        H2RPC_CHECK(AIO_CANCELED);
        H2RPC_CHECK(AIO_NOTCANCELED);
        H2RPC_CHECK(AIO_ALLDONE);
        default: return RPC_AIO_UNKNOWN;
    }
}

#endif /* HAVE_AIO_H */
