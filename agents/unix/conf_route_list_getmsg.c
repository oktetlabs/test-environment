/*
 * Kernel routing table readup by getmsg(2)
 * Copyright (C) 1999 Michael Handler
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifdef __sun__

#define TE_LGR_USER     "Unix Conf Route"

#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/stream.h>
#include <sys/tihdr.h>
#include <sys/stropts.h>
#include <stropts.h>

/* Solaris defines these in both <netinet/in.h> and <inet/in.h>, sigh */
#ifdef SUNOS_5
#include <sys/tiuser.h>
#ifndef T_CURRENT
#define T_CURRENT       MI_T_CURRENT
#endif /* T_CURRENT */
#ifndef IRE_CACHE
#define IRE_CACHE               0x0020  /* Cached Route entry */
#endif /* IRE_CACHE */
#ifndef IRE_HOST_REDIRECT
#define IRE_HOST_REDIRECT       0x0200  /* Host route entry from redirects */
#endif /* IRE_HOST_REDIRECT */
#ifndef IRE_CACHETABLE
#define IRE_CACHETABLE          (IRE_CACHE | IRE_BROADCAST | IRE_LOCAL | \
                                 IRE_LOOPBACK)
#endif /* IRE_CACHETABLE */
#undef IPOPT_EOL
#undef IPOPT_NOP
#undef IPOPT_LSRR
#undef IPOPT_RR
#undef IPOPT_SSRR
#endif /* SUNOS_5 */

#include <inet/common.h>
#include <inet/ip.h>
#include <inet/mib2.h>

#include "logger_api.h"
#include "unix_internal.h"
#include "conf_route.h"


/* device to read IP routing table from */
#ifndef _PATH_GETMSG_ROUTE
#define _PATH_GETMSG_ROUTE    "/dev/ip"
#endif /* _PATH_GETMSG_ROUTE */

#define RT_BUFSIZ        8192

static char buf[4096];

static void
handle_route_entry(mib2_ipRouteEntry_t *routeEntry)
{
    char           *p = buf + strlen(buf);
    unsigned int    prefixlen;

    /* arybchik: I don't know what it means, but it is from Zebra src */
    if (routeEntry->ipRouteInfo.re_ire_type & IRE_CACHETABLE)
        return;

    MASK2PREFIX(ntohl(routeEntry->ipRouteMask), prefixlen);
        
    inet_ntop(AF_INET, &routeEntry->ipRouteDest, p, INET_ADDRSTRLEN);
    p += strlen(p);
    p += sprintf(p, "|%u", prefixlen);

    /* No support for 'metric' and 'tos' yet, assume defaults */

    p += sprintf(p, " ");
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_list(char **list)
{
    te_errno                rc = 0;
    char                    storage[RT_BUFSIZ];

    struct T_optmgmt_req   *TLIreq = (struct T_optmgmt_req *)storage;
    struct T_optmgmt_ack   *TLIack = (struct T_optmgmt_ack *)storage;
    struct T_error_ack     *TLIerr = (struct T_error_ack *)storage;

    struct opthdr          *MIB2hdr;

    mib2_ipRouteEntry_t    *routeEntry, *lastRouteEntry;

    struct strbuf           msgdata;
    int                     flags, dev, retval, process;

    buf[0] = '\0';

    if ((dev = open(_PATH_GETMSG_ROUTE, O_RDWR)) == -1)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("can't open %s: %r", _PATH_GETMSG_ROUTE, rc);
        return rc;
    }

    TLIreq->PRIM_type = T_OPTMGMT_REQ;
    TLIreq->OPT_offset = sizeof(struct T_optmgmt_req);
    TLIreq->OPT_length = sizeof(struct opthdr);
    TLIreq->MGMT_flags = T_CURRENT;

    MIB2hdr = (struct opthdr *)&TLIreq[1];

    MIB2hdr->level = MIB2_IP;
    MIB2hdr->name = 0;
    MIB2hdr->len = 0;
    
    msgdata.buf = storage;
    msgdata.len = sizeof(struct T_optmgmt_req) + sizeof(struct opthdr);

    flags = 0;

    if (putmsg(dev, &msgdata, NULL, flags) == -1)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("putmsg failed: %r", rc);
        close(dev);
        return rc;
    }

    MIB2hdr = (struct opthdr *) &TLIack[1];
    msgdata.maxlen = sizeof(storage);

    while (1)
    {
        flags = 0;
        
        retval = getmsg(dev, &msgdata, NULL, &flags);
        if (retval == -1)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("getmsg(ctl) failed: %r", rc);
            break;
        }

        /* This is normal loop termination */
        if (retval == 0 &&
            msgdata.len >= (int)sizeof(struct T_optmgmt_ack) &&
            TLIack->PRIM_type == T_OPTMGMT_ACK &&
            TLIack->MGMT_flags == T_SUCCESS &&
            MIB2hdr->len == 0)
            break;

        if (msgdata.len >= (int)sizeof(struct T_error_ack) &&
            TLIerr->PRIM_type == T_ERROR_ACK)
        {
            rc = TE_OS_RC(TE_TA_UNIX, (TLIerr->TLI_error == TSYSERR) ?
                                      TLIerr->UNIX_error : EPROTO);
            ERROR("getmsg(ctl) returned T_ERROR_ACK: %r", rc);
            break;
        }

        /* should dump more debugging info to the log statement,
           like what GateD does in this instance, but not
           critical yet. */
        if (retval != MOREDATA ||
            msgdata.len < (int)sizeof(struct T_optmgmt_ack) ||
            TLIack->PRIM_type != T_OPTMGMT_ACK ||
            TLIack->MGMT_flags != T_SUCCESS)
        {
            rc = TE_RC(TE_TA_UNIX, TE_ENOMSG);
            ERROR("getmsg(ctl) returned bizarreness");
            break;
        }

        /* MIB2_IP_21 is the the pseudo-MIB2 ipRouteTable
           entry, see <inet/mib2.h>. "This isn't the MIB data
           you're looking for." */
        process = (MIB2hdr->level == MIB2_IP &&
            MIB2hdr->name == MIB2_IP_21) ? 1 : 0;

        /* getmsg writes the data buffer out completely, not
           to the closest smaller multiple. Unless reassembling
           data structures across buffer boundaries is your idea
           of a good time, set maxlen to the closest smaller
           multiple of the size of the datastructure you're
           retrieving. */
        msgdata.maxlen = sizeof(storage) - (sizeof(storage)
            % sizeof(mib2_ipRouteEntry_t));

        msgdata.len = 0;
        flags = 0;

        do {
            retval = getmsg (dev, NULL, &msgdata, &flags);
            if (retval == -1)
            {
                rc = TE_OS_RC(TE_TA_UNIX, errno);
                ERROR("getmsg(data) failed: %r", rc);
                break;
            }

            if (!(retval == 0 || retval == MOREDATA))
            {
                rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
                ERROR("getmsg(data) returned %d", retval);
                break;
            }

            if (process)
            {
                if (msgdata.len % sizeof(mib2_ipRouteEntry_t) != 0)
                {
                    rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
                    ERROR("getmsg(data) returned msgdata.len = %d "
                          "(%% sizeof(mib2_ipRouteEntry_t) != 0)",
                          msgdata.len);
                    break;
                }

                routeEntry = (mib2_ipRouteEntry_t *)msgdata.buf;
                lastRouteEntry = (mib2_ipRouteEntry_t *)
                    (msgdata.buf + msgdata.len);
                do {
                    handle_route_entry(routeEntry);
                } while (++routeEntry < lastRouteEntry);
            }
        } while (retval == MOREDATA);
    }

    close(dev);

    INFO("%s: Routes: %s", __FUNCTION__, buf);
    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return rc;
}


#endif /* __sun__ */
