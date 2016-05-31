/** @file
 * @brief RPC for DPDK EAL
 *
 * RPC routines implementation to call DPDK (rte_eal_*) functions.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC DPDK EAL"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "rte_config.h"
#include "rte_eal.h"

#include "logger_api.h"

#include "unix_internal.h"
#include "tarpc_server.h"
#include "rpcs_dpdk.h"


TARPC_FUNC(rte_eal_init, {},
{
    char **argv;
    int i;

    if (in->argc > 0)
    {
        argv = calloc(in->argc, sizeof(*argv));
        if (argv == NULL)
        {
            out->common._errno = TE_RC(TE_RPCS, TE_ENOMEM);
            out->retval = -out->common._errno;
            goto done;
        }
        for (i = 0; i < in->argc; ++i)
            argv[i] = in->argv.argv_val[i].str;
    }
    else
    {
        argv = NULL;
    }


    MAKE_CALL(out->retval = func(in->argc, argv););
    neg_errno_h2rpc(&out->retval);

    free(argv);

done:
    ;
})
