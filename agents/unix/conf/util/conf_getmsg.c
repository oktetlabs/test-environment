/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support using getmsg()/putmsg() routines.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */

#define TE_LGR_USER     "Unix Conf GetMsg"

#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_STROPTS_H
#if HAVE_SYS_TIHDR_H
#if HAVE_INET_MIB2_H

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stropts.h>
#include <sys/tihdr.h>
#include <inet/common.h>
#include <inet/ip.h>
#include <inet/arp.h>
#include <inet/mib2.h>

#include "te_errno.h"
#include "te_sockaddr.h"
#include "logger_api.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "conf_route.h"


#define PATH_GETMSG_DEV     "/dev/arp"

#ifndef IRE_CACHETABLE
#define IRE_CACHETABLE          (IRE_CACHE | IRE_BROADCAST | IRE_LOCAL | \
                                 IRE_LOOPBACK)
#endif /* IRE_CACHETABLE */

static void *getmsg_buf = NULL;
static size_t getmsg_buflen = 0;

static char hugebuf[8192];


/* See the description in */
static te_errno
ta_unix_conf_get_mib(unsigned int mib_level, unsigned int mib_name,
                     void **buf, size_t *buflen, size_t *miblen)
{
    static int  dev = -1;

    uintptr_t   ctrlbuf[(MAX(sizeof(struct T_optmgmt_req),
                             MAX(sizeof(struct T_optmgmt_ack),
                                 sizeof(struct T_error_ack))) +
                         sizeof(struct opthdr)) / sizeof(uintptr_t) + 1];

    te_errno                rc = 0;
    int                     ret;
    int                     flags;
    struct T_optmgmt_req   *req = (struct T_optmgmt_req *)ctrlbuf;
    struct T_optmgmt_ack   *ack = (struct T_optmgmt_ack *)ctrlbuf;
    struct T_error_ack     *err = (struct T_error_ack *)ctrlbuf;
    struct opthdr          *hdr;
    struct strbuf           ctrl, data;
    size_t                  used = 0;


    if (dev < 0)
    {
        dev = open(PATH_GETMSG_DEV, O_RDWR);
        if (dev < 0)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): Unable to open %s: %r", __FUNCTION__,
                  PATH_GETMSG_DEV, rc);
            return rc;
        }
    }

    req->PRIM_type = T_SVR4_OPTMGMT_REQ;
    req->OPT_offset = sizeof(*req);
    req->OPT_length = sizeof(*hdr);
    req->MGMT_flags = T_CURRENT;

    hdr = (struct opthdr *)(req + 1);
    hdr->level = MIB2_IP;
    hdr->name = 0;
    hdr->len = 0;

    ctrl.buf = (char *)ctrlbuf;
    ctrl.len = req->OPT_offset + req->OPT_length;
    flags = 0;
    if (putmsg(dev, &ctrl, NULL, flags) == -1)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s(): putmsg(ctrl) failed: %r", __FUNCTION__, rc);
        goto exit;
    }

    /*
     * Header is interesting in the case of positive reply only.
     * It is located just after the control message header.
     */
    hdr = (struct opthdr *)(ack + 1);
    /* Maximum control bufffer length may be set only once */
    ctrl.maxlen = sizeof(ctrlbuf);

    for (;;)
    {
        flags = 0;
        ret = getmsg(dev, &ctrl, NULL, &flags);
        if (ret == -1)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): getmsg(ctrl) failed: %r", __FUNCTION__, rc);
            goto exit;
        }
        if (ret == 0 &&
            ctrl.len >= (int)sizeof(struct T_optmgmt_ack) &&
            ack->PRIM_type == T_OPTMGMT_ACK &&
            ack->MGMT_flags == T_SUCCESS &&
            hdr->len == 0)
        {
            /* Successfull */
            VERB("%s(): getmsg() returned end-of-data "
                 "(level %u, name %u) - read %u", __FUNCTION__,
                 (unsigned)hdr->level, (unsigned)hdr->name,
                 (unsigned)used);
            break;
        }

        if (ctrl.len >= (int)sizeof(struct T_error_ack) &&
            err->PRIM_type == T_ERROR_ACK)
        {
            ERROR("%s(): getmsg(ctrl) - T_ERROR_ACK: TLI_error = 0x%x, "
                  "UNIX_error = 0x%x", __FUNCTION__,
                  (unsigned)err->TLI_error, (unsigned)err->UNIX_error);
            rc = TE_OS_RC(TE_TA_UNIX, (err->TLI_error == TSYSERR) ?
                                      err->UNIX_error : EPROTO);
            goto exit;
        }

        if (ret != MOREDATA ||
            ctrl.len < (int)sizeof(struct T_optmgmt_ack) ||
            ack->PRIM_type != T_OPTMGMT_ACK ||
            ack->MGMT_flags != T_SUCCESS)
        {
            ERROR("%s(): getmsg(ctrl) %d returned %d, ctrl.len = %d, "
                  "PRIM_type = %ld", __FUNCTION__, ret,
                  (int)ctrl.len, (long int)ack->PRIM_type);
            rc = TE_RC(TE_TA_UNIX, TE_ENOMSG);
            goto exit;
        }

        VERB("%s(): level=%d name=%d len=%u", __FUNCTION__,
             (int)hdr->level, (int)hdr->name, (unsigned)hdr->len);

        if (*buflen < used + hdr->len)
        {
            *buf = realloc(*buf, used + hdr->len);
            if (*buf == NULL)
            {
                rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto exit;
            }
        }

        data.maxlen = hdr->len;
        data.buf    = (char *)(*buf) + used;
        data.len    = 0;
        flags = 0;
        ret = getmsg(dev, NULL, &data, &flags);
        if (ret == -1)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): getmsg(data) failed: %r", __FUNCTION__, rc);
            goto exit;
        }
        else if (ret != 0)
        {
            ERROR("%s(): getmsg(data) returned %d, data.maxlen = %d, "
                  "data.len = %d", __FUNCTION__, ret, data.maxlen,
                  data.len);
            rc = TE_RC(TE_TA_UNIX, TE_EIO);
            goto exit;
        }

        if (hdr->level == mib_level && hdr->name == mib_name)
        {
            used += hdr->len;
        }
    }

    if (used == 0)
        rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
    else if (miblen != NULL)
        *miblen = used;

exit:
    return rc;
}


/* See the description in */
te_errno
ta_unix_conf_neigh_list(const char *iface, te_bool is_static, char **list)
{
    static unsigned int ipNetToMediaEntrySize = 0;

    size_t                      iface_len = strlen(iface);
    size_t                      miblen;
    te_errno                    rc;
    mib2_ipNetToMediaEntry_t   *ip4;
    size_t                      off = 0;

    if (ipNetToMediaEntrySize == 0)
    {
        rc = ta_unix_conf_get_mib(MIB2_IP, 0,
                                  &getmsg_buf, &getmsg_buflen, NULL);
        if (rc != 0)
        {
            ERROR("Failed to get MIB2_IP: %r", rc);
            return rc;
        }
        ipNetToMediaEntrySize =
            ((mib2_ip_t *)getmsg_buf)->ipNetToMediaEntrySize;
        assert(ipNetToMediaEntrySize != 0);
    }

    *hugebuf = '\0';

    rc = ta_unix_conf_get_mib(MIB2_IP, MIB2_IP_MEDIA,
                              &getmsg_buf, &getmsg_buflen, &miblen);
    if (rc != 0)
    {
        ERROR("Failed to get MIB2_IP_MEDIA: %r", rc);
        return rc;
    }

    for (ip4 = (mib2_ipNetToMediaEntry_t *)getmsg_buf;
         (void *)ip4 < (void *)((uint8_t *)getmsg_buf + miblen);
         ip4 = (mib2_ipNetToMediaEntry_t *)
            ((uint8_t *)ip4 + ipNetToMediaEntrySize))
    {
        if (/* Filter out published entries */
            (~ip4->ipNetToMediaInfo.ntm_flags & ACE_F_PUBLISH) &&
            /* Filter out dying entries */
            (~ip4->ipNetToMediaInfo.ntm_flags & ACE_F_DYING) &&
            /* Filter out multicast mapping entries */
            (~ip4->ipNetToMediaInfo.ntm_flags & ACE_F_MAPPING) &&
            /* Filter out incomplete entries */
            (ip4->ipNetToMediaInfo.ntm_flags & ACE_F_RESOLVED) &&
            /* Are static or dynamic entries requested? */
            (!(ip4->ipNetToMediaInfo.ntm_flags & ACE_F_PERMANENT) ==
                 !is_static) &&
            /* Filter by interface name */
            (ip4->ipNetToMediaIfIndex.o_length == (int)iface_len) &&
            (memcmp(ip4->ipNetToMediaIfIndex.o_bytes, iface,
                    iface_len) == 0))
        {
            if (inet_ntop(AF_INET, &ip4->ipNetToMediaNetAddress,
                          hugebuf + off, sizeof(hugebuf) - off) == NULL ||
                (off += strlen(hugebuf + off)) == sizeof(hugebuf))
            {
                ERROR("%s(): hugebuf is too small", __FUNCTION__);
                return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
            }
            strcat(hugebuf + off++, " ");
        }
    }

    INFO("%s(): %s neighbours: %s", __FUNCTION__,
         (is_static) ? "Static" : "Dynamic", hugebuf);
    if ((*list = strdup(hugebuf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return rc;
}


#ifdef TA_UNIX_CONF_ROUTE_DEBUG

/**
 * Log routing table entry using RING log level.
 *
 * @param rt    Routing table entry to be logged
 */
static void
route_entry_log(const mib2_ipRouteEntry_t *rt)
{
    const struct ipRouteInfo_s *rt_info = &rt->ipRouteInfo;

    char buf1[INET_ADDRSTRLEN];
    char buf2[INET_ADDRSTRLEN];
    char buf3[INET_ADDRSTRLEN];
    char ifname[LIFNAMSIZ + 1];

    assert(rt->ipRouteIfIndex.o_length < (int)sizeof(ifname));
    memcpy(ifname, rt->ipRouteIfIndex.o_bytes,
           rt->ipRouteIfIndex.o_length);
    ifname[rt->ipRouteIfIndex.o_length] = '\0';
    RING("Route: dst=%s mask=%s gw=%s if=%s type=%d proto=%d age=%d "
          "m1=%d m2=%d m3=%d m4=%d m5=%d",
          inet_ntop(AF_INET, &rt->ipRouteDest, buf1, sizeof(buf1)),
          inet_ntop(AF_INET, &rt->ipRouteMask, buf2, sizeof(buf2)),
          inet_ntop(AF_INET, &rt->ipRouteNextHop, buf3, sizeof(buf3)),
          ifname, rt->ipRouteType, rt->ipRouteProto, rt->ipRouteAge,
          rt->ipRouteMetric1, rt->ipRouteMetric2, rt->ipRouteMetric3,
          rt->ipRouteMetric4, rt->ipRouteMetric5);

    assert(rt_info->re_in_ill.o_length < (int)sizeof(ifname));
    memcpy(ifname, rt_info->re_in_ill.o_bytes,
           rt_info->re_in_ill.o_length);
    ifname[rt_info->re_in_ill.o_length] = '\0';
    RING("Info: max_frag=%u rtt=%u ref=%u frag_flag=%d src=%s ire_type=%d "
         "obpkt=%u ibpkt=%u flags=%d in_ill=%s in_src=%s",
         (unsigned)rt_info->re_max_frag,
         (unsigned)rt_info->re_rtt,
         (unsigned)rt_info->re_ref,
         rt_info->re_frag_flag,
         inet_ntop(AF_INET, &rt_info->re_src_addr, buf1, sizeof(buf1)),
         rt_info->re_ire_type, (unsigned)rt_info->re_obpkt,
         (unsigned)rt_info->re_ibpkt, rt_info->re_flags, ifname,
         inet_ntop(AF_INET, &rt_info->re_in_src_addr, buf2, sizeof(buf2)));
}

#endif /* TA_UNIX_CONF_ROUTE_DEBUG */

/**
 * Process routing table entry (add it in the list of routes located
 * in @b hugebuf buffer.
 *
 * @param rt    Routing table entry to be logged
 *
 * @return Status code.
 */
static te_errno
route_entry_process(const mib2_ipRouteEntry_t *rt)
{
    char            ifname[LIFNAMSIZ + 1];
    char           *p = hugebuf + strlen(hugebuf);
    unsigned int    prefixlen;
    te_errno        rc;

    /* Discard cached, broadcast, local and loopback entries */
    if (rt->ipRouteInfo.re_ire_type & IRE_CACHETABLE)
        return 0;

#if TA_UNIX_CONF_ROUTE_DEBUG
    route_entry_log(rt);
#endif

    if (rt->ipRouteIfIndex.o_length >= (int)sizeof(ifname))
    {
        ERROR("%s(): Too long interface name", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
    }
    memcpy(ifname, rt->ipRouteIfIndex.o_bytes,
           rt->ipRouteIfIndex.o_length);
    ifname[rt->ipRouteIfIndex.o_length] = '\0';

    if (*ifname == '\0')
    {
        ta_rt_info_t rt_info;

        ta_rt_info_init(TA_RT_TYPE_UNICAST, &rt_info);
        
        rt_info.dst.ss_family = AF_INET;
        SIN(&rt_info)->sin_addr.s_addr = rt->ipRouteNextHop;

        rc = ta_unix_conf_outgoing_if(&rt_info);
        if (rc != 0)
            return rc;
        assert(sizeof(ifname) > strlen(rt_info.ifname));
        strcpy(ifname, rt_info.ifname);
    }
    
    if (!ta_interface_is_mine(ifname))
        return 0;

    MASK2PREFIX(ntohl(rt->ipRouteMask), prefixlen);
        
    inet_ntop(AF_INET, &rt->ipRouteDest, p, INET_ADDRSTRLEN);
    p += strlen(p);
    p += sprintf(p, "|%u", prefixlen);

    /* No support for 'metric' and 'tos' yet, assume defaults */

    p += sprintf(p, " ");

    return 0;
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_list(char **list)
{
    static unsigned int ipRouteEntrySize = 0;

    size_t                  miblen;
    te_errno                rc;
    mib2_ipRouteEntry_t    *rt;

    if (ipRouteEntrySize == 0)
    {
        rc = ta_unix_conf_get_mib(MIB2_IP, 0,
                                  &getmsg_buf, &getmsg_buflen, NULL);
        if (rc != 0)
        {
            ERROR("Failed to get MIB2_IP: %r", rc);
            return rc;
        }
        ipRouteEntrySize = ((mib2_ip_t *)getmsg_buf)->ipRouteEntrySize;
        assert(ipRouteEntrySize != 0);
    }

    *hugebuf = '\0';

    rc = ta_unix_conf_get_mib(MIB2_IP, MIB2_IP_ROUTE,
                              &getmsg_buf, &getmsg_buflen, &miblen);
    if (rc != 0)
    {
        ERROR("Failed to get MIB2_IP_ROUTE: %r", rc);
        return rc;
    }

    for (rt = (mib2_ipRouteEntry_t *)getmsg_buf;
         (void *)rt < (void *)((uint8_t *)getmsg_buf + miblen);
         rt = (mib2_ipRouteEntry_t *) ((uint8_t *)rt + ipRouteEntrySize))
    {
        /* Ignore errors */
        (void)route_entry_process(rt);
    }

    INFO("%s(): Routes: %s", __FUNCTION__, hugebuf);
    if ((*list = strdup(hugebuf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return rc;
}


#endif /* HAVE_INET_MIB2_H */
#endif /* HAVE_SYS_TIHDR_H */
#endif /* HAVE_STROPTS_H */
