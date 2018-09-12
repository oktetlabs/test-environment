/** @file
 * @brief RPC for DPDK EAL
 *
 * RPC routines implementation to call DPDK (rte_eal_*) functions.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC DPDK EAL"

#include "te_config.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "rte_config.h"
#include "rte_version.h"
#include "rte_eal.h"
#include "rte_interrupts.h"

#include "logger_api.h"

#include "rpc_server.h"
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

TARPC_FUNC(rte_eal_process_type, {},
{
    enum rte_proc_type_t retval;

    MAKE_CALL(retval = func());

    switch (retval)
    {
        case RTE_PROC_AUTO:
            out->retval = TARPC_RTE_PROC_AUTO;
            break;

        case RTE_PROC_PRIMARY:
            out->retval = TARPC_RTE_PROC_PRIMARY;
            break;

        case RTE_PROC_SECONDARY:
            out->retval = TARPC_RTE_PROC_SECONDARY;
            break;

        case RTE_PROC_INVALID:
            out->retval = TARPC_RTE_PROC_INVALID;
            break;

        default:
            out->retval = TARPC_RTE_PROC__UNKNOWN;
            break;
    }
})

TARPC_FUNC_STANDALONE(dpdk_get_version, {},
{
    out->year = RTE_VER_YEAR;
    out->month = RTE_VER_MONTH;
    out->minor = RTE_VER_MINOR;
    out->release = RTE_VER_RELEASE;
})

TARPC_FUNC(rte_eal_hotplug_add, {},
{
    MAKE_CALL(out->retval = func(in->busname, in->devname, in->devargs));
    neg_errno_h2rpc(&out->retval);
})

TARPC_FUNC(rte_eal_hotplug_remove, {},
{
    MAKE_CALL(out->retval = func(in->busname, in->devname));
    neg_errno_h2rpc(&out->retval);
})

static void
tarpc_rte_epoll_data2rpc(const struct rte_epoll_data *rte,
                         struct tarpc_rte_epoll_data *rpc)
{
    rpc->event = rte->event;
    rpc->data = (uint64_t)rte->data;
}

static void
tarpc_rte_epoll_event2rpc(const struct rte_epoll_event *rte,
                          struct tarpc_rte_epoll_event *rpc)
{
    rpc->status = rte->status;
    rpc->fd = rte->fd;
    rpc->epfd = rte->epfd;
    tarpc_rte_epoll_data2rpc(&rte->epdata, &rpc->epdata);
}

TARPC_FUNC(rte_epoll_wait,
{
    COPY_ARG(events);
},
{
    struct rte_epoll_event *events = NULL;
    int i;

    if (out->events.events_len != 0)
    {
        events = calloc(in->maxevents, sizeof(*events));
        if (events == NULL)
        {
            out->common._errno = TE_RC(TE_RPCS, TE_ENOMEM);
            out->retval = -out->common._errno;
            goto done;
        }
    }

    MAKE_CALL(out->retval = func(in->epfd,
                                 events,
                                 in->maxevents,
                                 in->timeout));

    for (i = 0; i < out->retval; i++)
        tarpc_rte_epoll_event2rpc(&events[i], &out->events.events_val[i]);

    free(events);
    neg_errno_h2rpc(&out->retval);

done:
    ;
})
