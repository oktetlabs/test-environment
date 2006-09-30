/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support using routing sockets interface.
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

#ifdef USE_ROUTE_SOCKET

#define TE_LGR_USER     "Unix Conf Route Socket"

#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#if HAVE_NET_ROUTE_H
#include <net/route.h>
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

#include "te_errno.h"
#include "te_sockaddr.h"
#include "logger_api.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "conf_route.h"


#define RETURN_RC(_rc) \
    do {                        \
        te_errno __rc = (_rc);  \
                                \
        EXIT("%r", __rc);       \
        return __rc;            \
    } while (0)


/** Current route request sequence number */
static int rt_seq = 0;


#undef TA_UNIX_CONF_ROUTE_DEBUG
#ifdef TA_UNIX_CONF_ROUTE_DEBUG

/**
 * Convert route message type to string.
 *
 * @param type      Message type (RTM_*)
 *
 * @return String representation of the type ("RTM_*").
 */
static const char *
rt_msghdr_type2str(unsigned int type)
{
    switch (type)
    {
#define RT_MSGHDR_TYPE2STR(_type) \
        case RTM_##_type:   return #_type;

        RT_MSGHDR_TYPE2STR(ADD);
        RT_MSGHDR_TYPE2STR(DELETE);
        RT_MSGHDR_TYPE2STR(CHANGE);
        RT_MSGHDR_TYPE2STR(GET);
        RT_MSGHDR_TYPE2STR(LOSING);
        RT_MSGHDR_TYPE2STR(REDIRECT);
        RT_MSGHDR_TYPE2STR(MISS);
        RT_MSGHDR_TYPE2STR(LOCK);
        RT_MSGHDR_TYPE2STR(OLDADD);
        RT_MSGHDR_TYPE2STR(OLDDEL);
        RT_MSGHDR_TYPE2STR(RESOLVE);
        RT_MSGHDR_TYPE2STR(NEWADDR);
        RT_MSGHDR_TYPE2STR(DELADDR);
        RT_MSGHDR_TYPE2STR(IFINFO);
#ifdef RTM_NEWMADDR
        RT_MSGHDR_TYPE2STR(NEWMADDR);
#endif
#ifdef RTM_DELMADDR
        RT_MSGHDR_TYPE2STR(DELMADDR);
#endif
#ifdef RTM_IFANNOUNCE
        RT_MSGHDR_TYPE2STR(IFANNOUNCE);
#endif
#ifdef RTM_IEEE80211
        RT_MSGHDR_TYPE2STR(IEEE80211);
#endif

#undef RT_MSGHDR_TYPE2STR

        default:    return "<UNKNOWN>";
    }
}

/**
 * Convert route message flags to string.
 *
 * @param flags     Message flags
 *
 * @return String representation of flags.
 *
 * @attentation The function use static buffer for string and,
 * therefore, it is not reenterable.
 */
static const char *
rt_msghdr_flags2str(unsigned int flags)
{
    static char     buf[256];
    char           *p = buf;
    size_t          rlen = sizeof(buf);
    int             out;

    *p = '\0';

#define RT_MSGHDR_FLAG2STR(_flag) \
    do {                                                \
        if ((RTF_##_flag) & flags)                      \
        {                                               \
            flags &= ~(RTF_##_flag);                    \
            out = snprintf(p, rlen, " " #_flag);        \
            if ((size_t)out >= rlen)                    \
            {                                           \
                ERROR("Too short buffer for RT flags"); \
                return buf;                             \
            }                                           \
            p += out;                                   \
            rlen -= out;                                \
        }                                               \
    } while (0)

    RT_MSGHDR_FLAG2STR(UP);
    RT_MSGHDR_FLAG2STR(GATEWAY);
    RT_MSGHDR_FLAG2STR(HOST);
    RT_MSGHDR_FLAG2STR(REJECT);
    RT_MSGHDR_FLAG2STR(DYNAMIC);
    RT_MSGHDR_FLAG2STR(MODIFIED);
    RT_MSGHDR_FLAG2STR(DONE);
#ifdef RTF_MASK
    RT_MSGHDR_FLAG2STR(MASK);
#endif
    RT_MSGHDR_FLAG2STR(CLONING);
    RT_MSGHDR_FLAG2STR(XRESOLVE);
    RT_MSGHDR_FLAG2STR(LLINFO);
    RT_MSGHDR_FLAG2STR(STATIC);
    RT_MSGHDR_FLAG2STR(BLACKHOLE);
#ifdef RTF_PRIVATE
    RT_MSGHDR_FLAG2STR(PRIVATE);
#endif
    RT_MSGHDR_FLAG2STR(PROTO2);
    RT_MSGHDR_FLAG2STR(PROTO1);
#ifdef RTF_MULTIRT
    RT_MSGHDR_FLAG2STR(MULTIRT);
#endif
#ifdef RTF_SETSRC
    RT_MSGHDR_FLAG2STR(SETSRC);
#endif
#ifdef RTF_PRCLONING
    RT_MSGHDR_FLAG2STR(PRCLONING);
#endif
#ifdef RTF_WASCLONED
    RT_MSGHDR_FLAG2STR(WASCLONED);
#endif
#ifdef RTF_PROTO3
    RT_MSGHDR_FLAG2STR(PROTO3);
#endif
#ifdef RTF_PINNED
    RT_MSGHDR_FLAG2STR(PINNED);
#endif
#ifdef RTF_LOCAL
    RT_MSGHDR_FLAG2STR(LOCAL);
#endif
#ifdef RTF_BROADCAST
    RT_MSGHDR_FLAG2STR(BROADCAST);
#endif
#ifdef RTF_MULTICAST
    RT_MSGHDR_FLAG2STR(MULTICAST);
#endif

#undef RT_MSGHDR_FLAG2STR

    if (flags != 0)
    {
        out = snprintf(p, rlen, " <UNKNOWN>");
        if ((size_t)out >= rlen)
        {
            ERROR("Too short buffer for RT flags");
            return buf;
        }
    }

    return buf;
}

/**
 * Convert route message addresses flags to string.
 *
 * @param flags     Message addresses flags
 *
 * @return String representation of addresses flags.
 *
 * @attentation The function use static buffer for string and,
 * therefore, it is not reenterable.
 */
static const char *
rt_msghdr_addrs2str(unsigned int addrs)
{
    static char     buf[256];
    char           *p = buf;
    size_t          rlen = sizeof(buf);
    int             out;

    *p = '\0';

#define RT_MSGHDR_ADDR2STR(_addr) \
    do {                                                            \
        if ((RTA_##_addr) & addrs)                                  \
        {                                                           \
            addrs &= ~(RTA_##_addr);                                \
            out = snprintf(p, rlen, " " #_addr);                    \
            if ((size_t)out >= rlen)                                \
            {                                                       \
                ERROR("Too short buffer for RT addresses flags");   \
                return buf;                                         \
            }                                                       \
            p += out;                                               \
            rlen -= out;                                            \
        }                                                           \
    } while (0)

    RT_MSGHDR_ADDR2STR(DST);
    RT_MSGHDR_ADDR2STR(GATEWAY);
    RT_MSGHDR_ADDR2STR(NETMASK);
    RT_MSGHDR_ADDR2STR(GENMASK);
    RT_MSGHDR_ADDR2STR(IFP);
    RT_MSGHDR_ADDR2STR(IFA);
    RT_MSGHDR_ADDR2STR(AUTHOR);
    RT_MSGHDR_ADDR2STR(BRD);
#ifdef RTA_SRC
    RT_MSGHDR_ADDR2STR(SRC);
#endif
#ifdef RTA_SRCIFP
    RT_MSGHDR_ADDR2STR(SRCIFP);
#endif

#undef RT_MSGHDR_ADDR2STR

    if (addrs != 0)
    {
        out = snprintf(p, rlen, " <UNKNOWN>");
        if ((size_t)out >= rlen)
        {
            ERROR("Too short buffer for RT addresses flags");
            return buf;
        }
    }
    return buf;
}

/**
 * Convert route message metrics flags to string.
 *
 * @param flags     Message metrics flags
 *
 * @return String representation of metrics flags.
 */
static const char *
rt_msghdr_metrics2str(unsigned int metrics, char *buf, size_t buflen)
{
    char   *p = buf;
    size_t  rlen = buflen;
    int     out;

    if (buflen <= 0)
    {
        ERROR("Too short buffer for RT metrics flags");
        return "";
    }

    *p = '\0';

#define RT_MSGHDR_METRIC2STR(_metric) \
    do {                                                            \
        if ((RTV_##_metric) & metrics)                              \
        {                                                           \
            metrics &= ~(RTV_##_metric);                            \
            out = snprintf(p, rlen, " " #_metric);                  \
            if ((size_t)out >= rlen)                                \
            {                                                       \
                ERROR("Too short buffer for RT metrics flags");     \
                return buf;                                         \
            }                                                       \
            p += out;                                               \
            rlen -= out;                                            \
        }                                                           \
    } while (0)

    RT_MSGHDR_METRIC2STR(MTU);
    RT_MSGHDR_METRIC2STR(HOPCOUNT);
    RT_MSGHDR_METRIC2STR(EXPIRE);
    RT_MSGHDR_METRIC2STR(RPIPE);
    RT_MSGHDR_METRIC2STR(SPIPE);
    RT_MSGHDR_METRIC2STR(SSTHRESH);
    RT_MSGHDR_METRIC2STR(RTT);
    RT_MSGHDR_METRIC2STR(RTTVAR);

#undef RT_MSGHDR_METRIC2STR

    if (metrics != 0)
    {
        out = snprintf(p, rlen, " <UNKNOWN>");
        if ((size_t)out >= rlen)
        {
            ERROR("Too short buffer for RT metrics flags");
            return buf;
        }
    }
    return buf;
}

/** Log routing socket message */
static void
route_log(const char *title, const struct rt_msghdr *rtm)
{
    char                    inits[64];
    char                    locks[64];
    char                    addrs[RTAX_MAX][100];
    const struct sockaddr  *addr = CONST_SA(rtm + 1);
    socklen_t               addrlen;
    unsigned int            i;
    te_bool                 unknown;

    for (unknown = FALSE, i = 0; i < RTAX_MAX; i++)
    {
        if (!unknown && rtm->rtm_addrs & (1 << i))
        {
            if (addr->sa_family == AF_INET)
            {
                inet_ntop(AF_INET, &SIN(addr)->sin_addr,
                          addrs[i], sizeof(addrs[i]));
                addrlen = sizeof(struct sockaddr_in);
            }
            else if (addr->sa_family == AF_INET6)
            {
                inet_ntop(AF_INET6, &SIN6(addr)->sin6_addr,
                          addrs[i], sizeof(addrs[i]));
                addrlen = sizeof(struct sockaddr_in6);
            }
            else if (addr->sa_family == AF_LINK)
            {
                snprintf(addrs[i], sizeof(addrs[i]), " index=%u",
                         ((const struct sockaddr_dl *)addr)->sdl_index);
                addrlen = sizeof(struct sockaddr_dl);
            }
            else
            {
                snprintf(addrs[i], sizeof(addrs[i]), "<Unknown AF=%u>",
                         addr->sa_family);
                addrlen = 0;
            }
#if HAVE_STRUCT_SOCKADDR_SA_LEN
            /* Override length, if it is specified in address */
            if (addr->sa_len != 0)
                addrlen = addr->sa_len;
#endif
            if (addrlen == 0)
                addrlen = sizeof(struct sockaddr);

            addr = CONST_SA(((uint8_t *)addr) + addrlen);
        }
        else
        {
            addrs[i][0] = '\0';
        }
    }

    PRINT("%s\n"
         "len=%u ver=%u type=%s index=%u pid=%ld seq=%d errno=%d use=%d\n"
         "addrs=%s\nflags=%s\ninits=%s\nlocks=%s\n"
         "mtu=%u hops=%u expire=%u recvpipe=%u sendpipe=%u\n"
         "ssthresh=%u rtt=%u rttvar=%u pksent=%u\n"
         "dst=%s\n"
         "gateway=%s\n"
         "netmask=%s\n"
         "genmask=%s\n"
         "ifp=%s\n"
         "ifa=%s\n"
         "author=%s\n"
         "brd=%s\n"
#ifdef RTAX_SRC
         "src=%s\n"
#endif
#ifdef RTAX_SRCIFP
         "srcifp=%s\n"
#endif
         , title,
         rtm->rtm_msglen, rtm->rtm_version,
         rt_msghdr_type2str(rtm->rtm_type), rtm->rtm_index,
         (long)rtm->rtm_pid, rtm->rtm_seq, rtm->rtm_errno, rtm->rtm_use,
         rt_msghdr_addrs2str(rtm->rtm_addrs),
         rt_msghdr_flags2str(rtm->rtm_flags),
         rt_msghdr_metrics2str(rtm->rtm_inits, inits, sizeof(inits)),
         rt_msghdr_metrics2str(rtm->rtm_rmx.rmx_locks, locks, sizeof(locks)),
         (unsigned)rtm->rtm_rmx.rmx_mtu,
         (unsigned)rtm->rtm_rmx.rmx_hopcount,
         (unsigned)rtm->rtm_rmx.rmx_expire,
         (unsigned)rtm->rtm_rmx.rmx_recvpipe,
         (unsigned)rtm->rtm_rmx.rmx_sendpipe,
         (unsigned)rtm->rtm_rmx.rmx_ssthresh,
         (unsigned)rtm->rtm_rmx.rmx_rtt,
         (unsigned)rtm->rtm_rmx.rmx_rttvar,
         (unsigned)rtm->rtm_rmx.rmx_pksent,
         addrs[RTAX_DST], addrs[RTAX_GATEWAY], addrs[RTAX_NETMASK],
         addrs[RTAX_GENMASK], addrs[RTAX_IFP], addrs[RTAX_IFA],
         addrs[RTAX_AUTHOR], addrs[RTAX_BRD]
#ifdef RTAX_SRC
         , addrs[RTAX_SRC]
#endif
#ifdef RTAX_SRCIFP
         , addrs[RTAX_SRCIFP]
#endif
         );
}

#endif /* TA_UNIX_CONF_ROUTE_DEBUG */


/**
 * Convert routing socket message to system-independent route
 * representation.
 *
 * @param msg       Routing socket message
 * @param rt_info   Location for system-independed route representation
 *
 * @return Status code.
 */
static te_errno
rt_msghdr_to_ta_rt_info(const struct rt_msghdr *msg, ta_rt_info_t *rt_info)
{
    const struct sockaddr  *addr = CONST_SA(msg + 1);
    socklen_t               addrlen;
    te_errno                rc;

    ENTRY();

    memset(rt_info, 0, sizeof(*rt_info));

    if (msg->rtm_addrs & RTA_DST)
    {
        addrlen = te_sockaddr_get_size(addr);
        memcpy(&rt_info->dst, addr, addrlen);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }
    else
    {
        WARN("Route without destination address specification");
    }

    
    if (msg->rtm_addrs & RTA_GATEWAY)
    {
        addrlen = te_sockaddr_get_size(addr);
        memcpy(&rt_info->gw, addr, addrlen);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);

        if (msg->rtm_flags & RTF_GATEWAY)
        {
            ta_rt_info_t tmp_rt;

            memcpy(&tmp_rt.dst, &rt_info->gw, addrlen);
            VERB("%s(): Resolve outgoing interface for gateway %s",
                 __FUNCTION__,
                 te_sockaddr_get_ipstr(CONST_SA(&tmp_rt.dst)));
            rc = ta_unix_conf_outgoing_if(&tmp_rt);
            if (rc != 0)
            {
                char    buf[INET6_ADDRSTRLEN];

                ERROR("Failed to resolve outgoing interface name "
                      "for destination %s: %r",
                      inet_ntop(rt_info->dst.ss_family,
                          te_sockaddr_get_netaddr(CONST_SA(&rt_info->dst)),
                          buf, sizeof(buf)), rc);
                RETURN_RC(rc);
            }
            strncpy(rt_info->ifname, tmp_rt.ifname,
                    sizeof(rt_info->ifname));

            /* Route via gateway */
            rt_info->flags |= TA_RT_INFO_FLG_GW;
        }
        else if (rt_info->gw.ss_family == AF_LINK)
        {
            unsigned int ifi =
                ((const struct sockaddr_dl *)&rt_info->gw)->sdl_index;

            /* FreeBSD6 provides link-layer address as gateway */
            if (if_indextoname(ifi, rt_info->ifname) == NULL)
            {
                rc = TE_OS_RC(TE_TA_UNIX, errno);
                ERROR("Failed to map interface index %u to name", ifi);
                RETURN_RC(rc);
            }
            rt_info->flags |= TA_RT_INFO_FLG_IF;
        }
        else
        {
            /* 
             * Gateway address is used to specify interface of the
             * direct route. Address should be own local address.
             * Map address to interface name.
             */
            rc = ta_unix_conf_netaddr2ifname(CONST_SA(&rt_info->gw),
                                             rt_info->ifname);
            if (rc != 0)
            {
                ERROR("Failed to find interface by address %s",
                      te_sockaddr_get_ipstr(CONST_SA(&rt_info->gw)));
                RETURN_RC(rc);
            }
            rt_info->flags |= TA_RT_INFO_FLG_IF;
        }
    }

    if (msg->rtm_addrs & RTA_NETMASK)
    {
        addrlen = te_sockaddr_get_size(addr);
        if (addr->sa_family == AF_INET)
        {
            MASK2PREFIX(ntohl(SIN(addr)->sin_addr.s_addr),
                        rt_info->prefix);
        }
        else if (addr->sa_family == AF_INET6)
        {
            rt_info->prefix = 0; /* FIXME */
        }
        else
        {
            rt_info->prefix = 0; /* FIXME */
            if (addrlen == 0)
                addrlen = sizeof(struct sockaddr);
        }
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }

    if (msg->rtm_addrs & RTA_GENMASK)
    {
        addrlen = te_sockaddr_get_size(addr);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }

    if (msg->rtm_addrs & RTA_IFP)
    {
        addrlen = te_sockaddr_get_size(addr);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }

    if (msg->rtm_addrs & RTA_IFA)
    {
        addrlen = te_sockaddr_get_size(addr);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }

    if (msg->rtm_addrs & RTA_AUTHOR)
    {
        addrlen = te_sockaddr_get_size(addr);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }

    if (msg->rtm_addrs & RTA_BRD)
    {
        addrlen = te_sockaddr_get_size(addr);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }
#ifdef RTA_SRC
    if (msg->rtm_addrs & RTA_SRC)
    {
        addrlen = te_sockaddr_get_size(addr);
        memcpy(&rt_info->src, addr, addrlen);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
        rt_info->flags |= TA_RT_INFO_FLG_SRC;
    }
#endif

#ifdef RTA_SRCIFP
    if (msg->rtm_addrs & RTA_SRCIFP)
    {
        addrlen = te_sockaddr_get_size(addr);
        addr = CONST_SA(((const uint8_t *)addr) + addrlen);
    }
#endif

    rt_info->metric = (msg->rtm_inits & RTV_HOPCOUNT) ?
        (rt_info->flags |= TA_RT_INFO_FLG_METRIC,
         msg->rtm_rmx.rmx_hopcount) : 0;

    rt_info->mtu = (msg->rtm_inits & RTV_MTU) ?
        (rt_info->flags |= TA_RT_INFO_FLG_MTU, msg->rtm_rmx.rmx_mtu) : 0;
    
    rt_info->win = 0; /* No such metric */
    
    rt_info->irtt = (msg->rtm_inits & RTV_RTT) ?
        (rt_info->flags |= TA_RT_INFO_FLG_IRTT, msg->rtm_rmx.rmx_rtt) : 0;
    
    rt_info->tos = 0; /* No such metric/field */

    /* TODO: Wiser mapping */
    rt_info->type = (msg->rtm_flags & RTF_BLACKHOLE) ?
                        TA_RT_TYPE_BLACKHOLE : TA_RT_TYPE_UNICAST;

    RETURN_RC(0);
}

/**
 * Convert system-independent route representation to routing socket
 * message.
 *
 * @param action    Action
 * @param rt_info   System-independed route representation
 * @param msg       Routing socket message
 * @param msglen    Length of memory buffer for the message
 *
 * @return Status code.
 */
static te_errno
ta_rt_info_to_rt_msghdr(ta_cfg_obj_action_e action,
                        const ta_rt_info_t *rt_info,
                        struct rt_msghdr *msg, size_t msglen)
{
    struct sockaddr    *addr = SA((msg + 1));
    socklen_t           addrlen;
    te_errno            rc;

    ENTRY();

    if (msglen < sizeof(*msg))
        RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
    msglen -= sizeof(*msg);

    memset(msg, 0, sizeof(*msg));
    msg->rtm_msglen = sizeof(*msg);
    msg->rtm_version = RTM_VERSION;
    switch (action)
    {
        case TA_CFG_OBJ_CREATE:
            msg->rtm_type = RTM_ADD;
            break;

        case TA_CFG_OBJ_DELETE:
            msg->rtm_type = RTM_DELETE;
            break;

        case TA_CFG_OBJ_SET:
            msg->rtm_type = RTM_CHANGE;
            break;

        default:
            ERROR("Route action %u is supported", (unsigned)action);
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ENOSYS));
    }
    /* msg->rtm_index is 0 */
    msg->rtm_pid = getpid();
    /* msg->rtm_addrs is 0 */
    msg->rtm_seq = ++rt_seq;
    /* msg->rtm_errno is 0 */
    msg->rtm_flags = RTF_UP | RTF_STATIC;
    /* msg->rtm_use is 0 */
    /* msg->rtm_inits is 0 */
    /* msg->rtm_rmx has all zeros */

    /* Route type */
    switch (rt_info->type)
    {
        case TA_RT_TYPE_UNICAST:
            break;

        case TA_RT_TYPE_BLACKHOLE:
            msg->rtm_flags |= RTF_BLACKHOLE;
            break;

        default:
            ERROR("Routes of type %d are not supported yet");
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ENOSYS));
    }

    /* Destination */
    addrlen = te_sockaddr_get_size(CONST_SA(&rt_info->dst));
    if (msglen < addrlen)
        RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
    msglen -= addrlen;
    msg->rtm_msglen += addrlen;
    memcpy(addr, &rt_info->dst, addrlen);

    /* If prefix is less than IP address length, if is route to host */
    if (rt_info->prefix == te_netaddr_get_size(rt_info->dst.ss_family) << 3)
    {
        msg->rtm_flags |= RTF_HOST;
    }

    msg->rtm_addrs |= RTA_DST;
    addr = SA(((const uint8_t *)addr) + addrlen);

    /* Gateway */
    if (rt_info->flags & TA_RT_INFO_FLG_GW)
    {
        addrlen = te_sockaddr_get_size(CONST_SA(&rt_info->gw));
        if (msglen < addrlen)
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
        msglen -= addrlen;
        msg->rtm_msglen += addrlen;
        memcpy(addr, &rt_info->gw, addrlen);
        msg->rtm_addrs |= RTA_GATEWAY;
        msg->rtm_flags |= RTF_GATEWAY;
        addr = SA(((const uint8_t *)addr) + addrlen);
    }
    else if (rt_info->flags & TA_RT_INFO_FLG_IF)
    {
        const void *ifa;

        addrlen = te_sockaddr_get_size_by_af(rt_info->dst.ss_family);
        if (msglen < addrlen)
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
        msglen -= addrlen;
        msg->rtm_msglen += addrlen;
        rc = ta_unix_conf_get_addr(rt_info->ifname, rt_info->dst.ss_family,
                                   &ifa);
        if (rc != 0)
        {
            ERROR("Failed to get interface '%s' address: %r",
                  rt_info->ifname, rc);
            RETURN_RC(rc);
        }
        memset(addr, 0, addrlen);
        addr->sa_family = rt_info->dst.ss_family;
        memcpy(te_sockaddr_get_netaddr(addr), ifa,
               te_netaddr_get_size(addr->sa_family));
        msg->rtm_addrs |= RTA_GATEWAY;
        addr = SA(((const uint8_t *)addr) + addrlen);
    }

    /* Netmask */
    addrlen = te_sockaddr_get_size_by_af(rt_info->dst.ss_family);
    if (msglen < addrlen)
        RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
    msglen -= addrlen;
    msg->rtm_msglen += addrlen;
    rc = te_sockaddr_mask_by_prefix(addr, addrlen, rt_info->dst.ss_family,
                                    rt_info->prefix);
    if (rc != 0)
    {
        ERROR("%s(): te_sockaddr_mask_by_prefix() failed: %r",
              __FUNCTION__, rc);
        RETURN_RC(rc);
    }
    msg->rtm_addrs |= RTA_NETMASK;
    addr = SA(((const uint8_t *)addr) + addrlen);

    /* Interface */
    if (rt_info->flags & TA_RT_INFO_FLG_IF)
    {
        struct sockaddr_dl *ifp = (struct sockaddr_dl *)addr;

        addrlen = sizeof(*ifp);
        if (msglen < addrlen)
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
        msglen -= addrlen;
        msg->rtm_msglen += addrlen;
        memset(ifp, 0, addrlen);
        ifp->sdl_family = AF_LINK;
        ifp->sdl_index = if_nametoindex(rt_info->ifname);
        if (ifp->sdl_index == 0)
        {
            ERROR("Cannot convert interface name '%s' to index",
                  rt_info->ifname);
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESRCH));
        }
        msg->rtm_addrs |= RTA_IFP;
        addr = SA(((const uint8_t *)addr) + addrlen);
    }
#ifdef RTA_SRC
    /* Source */
    if (rt_info->flags & TA_RT_INFO_FLG_SRC)
    {
        addrlen = te_sockaddr_get_size(CONST_SA(&rt_info->src));
        if (msglen < addrlen)
            RETURN_RC(TE_RC(TE_TA_UNIX, TE_ESMALLBUF));
        msglen -= addrlen;
        msg->rtm_msglen += addrlen;

        memcpy(addr, &rt_info->src, addrlen);
        
        addr = SA(((const uint8_t *)addr) + addrlen);
        msg->rtm_addrs |= RTA_SRC;        
        msg->rtm_flags |= RTF_SETSRC;
    }
#endif    

    if (rt_info->flags & TA_RT_INFO_FLG_METRIC)
    {
        msg->rtm_rmx.rmx_hopcount = rt_info->metric;
        msg->rtm_inits |= RTV_HOPCOUNT;
    }

    if (rt_info->flags & TA_RT_INFO_FLG_MTU)
    {
        msg->rtm_rmx.rmx_mtu = rt_info->mtu;
        msg->rtm_inits |= RTV_MTU;
    }

    if (rt_info->flags & TA_RT_INFO_FLG_WIN)
    {
        ERROR("Routes with 'win' specification are not supported");
        RETURN_RC(TE_RC(TE_TA_UNIX, TE_ENOSYS));
    }

    if (rt_info->flags & TA_RT_INFO_FLG_IRTT)
    {
        msg->rtm_rmx.rmx_rtt = rt_info->irtt;
        msg->rtm_inits |= RTV_RTT;
    }

    if (rt_info->flags & TA_RT_INFO_FLG_TOS)
    {
        ERROR("Routes with TOS specification are not supported");
        RETURN_RC(TE_RC(TE_TA_UNIX, TE_ENOSYS));
    }

    RETURN_RC(0);
}


/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_find(ta_rt_info_t *rt_info)
{
    int                 rt_sock = -1;
    size_t              rt_buflen = sizeof(struct rt_msghdr) +
                                    sizeof(struct sockaddr_in6) * RTAX_MAX;
                        /* Assume that IPv6 sockaddr is the biggest */
    uint8_t            *rt_buf = NULL;
    pid_t               rt_pid = getpid();
    te_errno            rc;
    struct rt_msghdr   *rtm;
    socklen_t           addrlen;
    ssize_t             ret;

    ENTRY();

    /*
     * 'man -s 7P route' on SunOS 5.X suggests to use AF_* as the last
     * argument.
     */
    rt_sock = socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC);
    if (rt_sock < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open routing socket: %r", rc);
        goto cleanup;
    }

    rt_buf = calloc(1, rt_buflen);
    if (rt_buf == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        assert(rc != 0);
        goto cleanup;
    }

    rtm = (struct rt_msghdr *)rt_buf;
    rtm->rtm_msglen = sizeof(*rtm);
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = RTA_DST;
    rtm->rtm_pid = rt_pid;
    rtm->rtm_seq = ++rt_seq;

    addrlen = te_sockaddr_get_size(CONST_SA(&rt_info->dst));
    memcpy(rt_buf + rtm->rtm_msglen, &rt_info->dst, addrlen);
    rtm->rtm_msglen += addrlen;

    assert(rtm->rtm_msglen <= rt_buflen);

    VERB("%s(): dst=%s seq=%u", __FUNCTION__,
         te_sockaddr_get_ipstr(CONST_SA(&rt_info->dst)), rtm->rtm_seq);

    ret = write(rt_sock, rt_buf, rtm->rtm_msglen);
    if (ret != rtm->rtm_msglen)
    {
        rc = TE_OS_RC(TE_TA_UNIX, (ret < 0) ? errno : EIO);
        ERROR("%s(): Failed to send route request seq=%u to kernel: %r",
              __FUNCTION__, rtm->rtm_seq, rc);
#ifdef TA_UNIX_CONF_ROUTE_DEBUG
        route_log("ta_unix_conf_route_find() failed", rtm);
#endif
        goto cleanup;
    }

    do {

        ret = read(rt_sock, rt_buf, rt_buflen);
        if (ret < 0)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("Failed to receive route reply from kernel");
            goto cleanup;
        }
        if (ret != rtm->rtm_msglen)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EIO);
            ERROR("Unexpected route reply from kernel");
            goto cleanup;
        }

    } while ((rtm->rtm_type != RTM_GET) || (rtm->rtm_seq != rt_seq) ||
             (rtm->rtm_pid != rt_pid));

#ifdef TA_UNIX_CONF_ROUTE_DEBUG
    /* Reply got */
    route_log(__FUNCTION__, rtm);
#endif

    rc = rt_msghdr_to_ta_rt_info(rtm, rt_info);

cleanup:
    free(rt_buf);
    if (rt_sock >= 0)
        close(rt_sock);

    RETURN_RC(rc);
}

/* See the description in conf_route.h. */
te_errno
ta_unix_conf_route_change(ta_cfg_obj_action_e  action,
                          ta_rt_info_t        *rt_info)
{
    int                 rt_sock = -1;
    size_t              rt_buflen = sizeof(struct rt_msghdr) +
                                    sizeof(struct sockaddr_storage) *
                                        RTAX_MAX;
    uint8_t             rt_buf[rt_buflen];
    unsigned int        rt_cmd;
    pid_t               rt_pid;
    te_errno            rc;
    struct rt_msghdr   *rtm = (struct rt_msghdr *)rt_buf;
    ssize_t             ret;

    ENTRY();

    rc = ta_rt_info_to_rt_msghdr(action, rt_info, rtm, rt_buflen);
    if (rc != 0)
    {
        ERROR("%s(): ta_rt_info_to_rt_msghdr() failed: %r",
              __FUNCTION__, rc);
        RETURN_RC(rc);
    }

    rt_cmd = rtm->rtm_type;
    rt_pid = rtm->rtm_pid;

    /*
     * 'man -s 7P route' on SunOS 5.X suggests to use AF_* as the last
     * argument.
     */
    rt_sock = socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC);
    if (rt_sock < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Cannot open routing socket: %r", rc);
        RETURN_RC(rc);
    }

    VERB("%s(): dst=%s seq=%u", __FUNCTION__,
         te_sockaddr_get_ipstr(CONST_SA(&rt_info->dst)), rtm->rtm_seq);

    ret = write(rt_sock, rt_buf, rtm->rtm_msglen);
    if (ret != rtm->rtm_msglen)
    {
        rc = TE_OS_RC(TE_TA_UNIX, (ret < 0) ? errno : EIO);
        ERROR("%s(): Failed to send route request to kernel sent=%d: %r",
              __FUNCTION__, (int)ret, rc);
#ifdef TA_UNIX_CONF_ROUTE_DEBUG
        /* Reply got */
        route_log("ta_unix_conf_route_change() failed", rtm);
#endif
        goto cleanup;
    }

    do {

        ret = read(rt_sock, rt_buf, rt_buflen);
        if (ret < 0)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("Failed to receive route reply from kernel");
            goto cleanup;
        }
        if (ret != rtm->rtm_msglen)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EIO);
            ERROR("Unexpected route reply from kernel");
            goto cleanup;
        }

    } while ((rtm->rtm_type != rt_cmd) || (rtm->rtm_seq != rt_seq) ||
             (rtm->rtm_pid != rt_pid));

#ifdef TA_UNIX_CONF_ROUTE_DEBUG
    route_log(__FUNCTION__, rtm);
#endif

cleanup:
    if (rt_sock != -1)
        close(rt_sock);

    RETURN_RC(rc);
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_blackhole_list(char **list)
{
    *list = NULL;
    return 0;
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_blackhole_add(ta_rt_info_t *rt_info)
{
    UNUSED(rt_info);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}

/* See the description in conf_route.h */
te_errno 
ta_unix_conf_route_blackhole_del(ta_rt_info_t *rt_info)
{
    UNUSED(rt_info);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}


#if HAVE_SYS_SYSCTL_H
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/sysctl.h>

#if 0
/*
 * Kernel routing table read by sysctl function.
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
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

/* Kernel routing table read up by sysctl function. */
int
route_read ()
{
  caddr_t buf, end, ref;
  size_t bufsiz;
  struct rt_msghdr *rtm;
  void rtm_read (struct rt_msghdr *);
  
#define MIBSIZ 6
  int mib[MIBSIZ] = 
  {
    CTL_NET,
    PF_ROUTE,
    0,
    0,
    NET_RT_DUMP,
    0
  };
		      
  /* Get buffer size. */
  if (sysctl (mib, MIBSIZ, NULL, &bufsiz, NULL, 0) < 0) 
    {
      zlog_warn ("sysctl fail: %s", strerror (errno));
      return -1;
    }

  /* Allocate buffer. */
  ref = buf = XMALLOC (MTYPE_TMP, bufsiz);
  
  /* Read routing table information by calling sysctl(). */
  if (sysctl (mib, MIBSIZ, buf, &bufsiz, NULL, 0) < 0) 
    {
      zlog_warn ("sysctl() fail by %s", strerror (errno));
      return -1;
    }

  for (end = buf + bufsiz; buf < end; buf += rtm->rtm_msglen) 
    {
      rtm = (struct rt_msghdr *) buf;
      rtm_read (rtm);
    }

  /* Free buffer. */
  XFREE (MTYPE_TMP, ref);

  return 0;
}
#endif

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_list(char **list)
{
    WARN("List of routes via sysctl is not implemented yet");
    *list = NULL;
    return 0;
}

#endif /* HAVE_SYS_SYSCTL_H && !defined(USE_NETLINK) */

#endif /* USE_ROUTE_SOCKET */
