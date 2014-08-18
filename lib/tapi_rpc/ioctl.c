/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of ioctl() function
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_SCSI_SG_H
#include <scsi/sg.h>
#endif
#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"

#include "tarpc.h"


int
rpc_ioctl(rcf_rpc_server *rpcs,
          int fd, rpc_ioctl_code request, ...)
{
    tarpc_ioctl_in  in;
    tarpc_ioctl_out out;

    ioctl_request   req;
    va_list         ap;
    int            *arg;
    const char     *req_val;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ioctl, -1);
    }

    va_start(ap, request);
    arg = va_arg(ap, int *);
    va_end(ap);

    in.s = fd;
    in.code = request;

    if (arg != NULL)
    {
        memset(&req, 0, sizeof(req));
        in.req.req_val = &req;
        in.req.req_len = 1;
    }

    switch (request)
    {
        case RPC_SIOCGSTAMP:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_TIMEVAL;
                in.req.req_val[0].ioctl_request_u.req_timeval.tv_sec =
                    ((struct tarpc_timeval *)arg)->tv_sec;
                in.req.req_val[0].ioctl_request_u.req_timeval.tv_usec =
                    ((struct tarpc_timeval *)arg)->tv_usec;
            }
            break;

        case RPC_SIOCGSTAMPNS:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_TIMESPEC;
                in.req.req_val[0].ioctl_request_u.req_timespec.tv_sec =
                    ((struct tarpc_timespec *)arg)->tv_sec;
                in.req.req_val[0].ioctl_request_u.req_timespec.tv_nsec =
                    ((struct tarpc_timespec *)arg)->tv_nsec;
            }
            break;

        case RPC_FIONBIO:
        case RPC_SIOCSPGRP:
        case RPC_FIOASYNC:
        case RPC_SIO_FLUSH:
            in.access = IOCTL_WR;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_INT;
                in.req.req_val[0].ioctl_request_u.req_int = *arg;
            }
            break;

        case RPC_SIOCOUTQ:
        case RPC_TIOCOUTQ:
        case RPC_FIONREAD:
        case RPC_SIOCATMARK:
        case RPC_SIOCINQ:
        case RPC_SIOCGPGRP:
        case RPC_SIOUNKNOWN:
        case RPC_FIONCLEX:
        case RPC_FIOCLEX:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_INT;
                in.req.req_val[0].ioctl_request_u.req_int = *arg;
            }
            break;

        case RPC_SIOCGIFCONF:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_IFCONF;
                in.req.req_val[0].ioctl_request_u.req_ifconf.nmemb =
                    ((struct ifconf *)arg)->ifc_len / sizeof(struct ifreq);
                in.req.req_val[0].ioctl_request_u.req_ifconf.extra =
                    ((struct ifconf *)arg)->ifc_len % sizeof(struct ifreq);
            }
            break;

        case RPC_SIOCSIFADDR:
        case RPC_SIOCSIFNETMASK:
        case RPC_SIOCSIFBRDADDR:
        case RPC_SIOCSIFDSTADDR:
            in.access = IOCTL_WR;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_IFREQ;
                if (((struct ifreq *)arg)->ifr_addr.sa_family == AF_INET)
                {
                    sockaddr_input_h2rpc(&((struct ifreq *)arg)->ifr_addr,
                                         &in.req.req_val[0].ioctl_request_u.
                                             req_ifreq.rpc_ifr_addr);
                }
                else
                {
                    WARN("As-is converter is used for 'ifr_addr'");
                    sockaddr_raw2rpc(&((struct ifreq *)arg)->ifr_addr,
                        sizeof(((struct ifreq *)arg)->ifr_addr),
                        &in.req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_addr);
                }
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_val =
                    ((struct ifreq *)arg)->ifr_name;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_len =
                    sizeof(((struct ifreq *)arg)->ifr_name);
            }
            break;

        case RPC_SIOCGIFADDR:
        case RPC_SIOCGIFNETMASK:
        case RPC_SIOCGIFBRDADDR:
        case RPC_SIOCGIFDSTADDR:
        case RPC_SIOCGIFHWADDR:
        case RPC_SIOCGIFFLAGS:
        case RPC_SIOCGIFMTU:
        case RPC_SIOCGIFINDEX:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_IFREQ;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_val =
                    ((struct ifreq *)arg)->ifr_name;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_len =
                    sizeof(((struct ifreq *)arg)->ifr_name);

                sockaddr_input_h2rpc(&((struct ifreq *)arg)->ifr_addr,
                                     &in.req.req_val[0].ioctl_request_u.
                                         req_ifreq.rpc_ifr_addr);
            }
            break;

        case RPC_SIOCGIFNAME:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_IFREQ;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_val =
                    ((struct ifreq *)arg)->ifr_name;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_len =
                    sizeof(((struct ifreq *)arg)->ifr_name);
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_ifindex = 
                    ((struct ifreq *)arg)->ifr_ifindex;
            }
            break;

        case RPC_SIOCSIFFLAGS:
            in.access = IOCTL_WR;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_IFREQ;
                in.req.req_val[0].ioctl_request_u.req_ifreq.rpc_ifr_flags =
                    if_fl_h2rpc((uint32_t)((struct ifreq *)arg)->ifr_flags);
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_val =
                    ((struct ifreq *)arg)->ifr_name;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_len =
                    sizeof(((struct ifreq *)arg)->ifr_name);
            }
            break;

        case RPC_SIOCSIFMTU:
            in.access = IOCTL_WR;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_IFREQ;
                in.req.req_val[0].ioctl_request_u.req_ifreq.rpc_ifr_mtu =
                    ((struct ifreq *)arg)->ifr_mtu;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_val =
                    ((struct ifreq *)arg)->ifr_name;
                in.req.req_val[0].ioctl_request_u.req_ifreq.
                    rpc_ifr_name.rpc_ifr_name_len =
                    sizeof(((struct ifreq *)arg)->ifr_name);
            }
            break;

#define FILL_ARPREQ_ADDR(type_) \
    do {                                                                \
        tarpc_sa *rpc_addr = &in.req.req_val[0].ioctl_request_u.        \
                                 req_arpreq.rpc_arp_##type_;            \
        struct sockaddr *addr = &((struct arpreq *)arg)->arp_##type_;   \
                                                                        \
        sockaddr_input_h2rpc(addr, rpc_addr);                           \
    } while (0)

        case RPC_SIOCSARP:
            in.access = IOCTL_WR;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_ARPREQ;
                /* Copy protocol address */
                FILL_ARPREQ_ADDR(pa);
                /* Copy HW address */
                FILL_ARPREQ_ADDR(ha);
                /* Copy flags */
                in.req.req_val[0].ioctl_request_u.req_arpreq.rpc_arp_flags =
                    arp_fl_h2rpc(((struct arpreq *)arg)->arp_flags);
            }
            break;

        case RPC_SIOCDARP:
            in.access = IOCTL_WR;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_ARPREQ;
                FILL_ARPREQ_ADDR(pa);
            }
            break;

        case RPC_SIOCGARP:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_ARPREQ;
                /* Copy protocol address */
                FILL_ARPREQ_ADDR(pa);
                /* Copy HW address */
                FILL_ARPREQ_ADDR(ha);
                 /* Copy device */
                in.req.req_val[0].ioctl_request_u.req_arpreq.
                    rpc_arp_dev.rpc_arp_dev_val =
                    ((struct arpreq *)arg)->arp_dev;
                in.req.req_val[0].ioctl_request_u.req_arpreq.
                    rpc_arp_dev.rpc_arp_dev_len =
                    sizeof(((struct arpreq *)arg)->arp_dev);
            }
            break;

#undef FILL_ARPREQ_ADDR

        case RPC_SG_IO:
            in.access = IOCTL_RD;

            if (arg != NULL)
            {
                in.req.req_val[0].type = IOCTL_SGIO;
                
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    interface_id = ((sg_io_hdr_t *) arg)->interface_id;
                
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    dxfer_direction = ((sg_io_hdr_t *) arg)->
                    dxfer_direction;
                
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    cmd_len = ((sg_io_hdr_t *) arg)->cmd_len;
                
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    mx_sb_len = ((sg_io_hdr_t *) arg)->mx_sb_len;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    iovec_count = ((sg_io_hdr_t *) arg)->iovec_count;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    dxfer_len = ((sg_io_hdr_t *) arg)->dxfer_len;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    dxferp.dxferp_len = ((sg_io_hdr_t *) arg)->dxfer_len;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    dxferp.dxferp_val = ((sg_io_hdr_t *) arg)->dxferp;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    cmdp.cmdp_len = ((sg_io_hdr_t *) arg)->cmd_len;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    cmdp.cmdp_val = ((sg_io_hdr_t *) arg)->cmdp;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    sbp.sbp_len = ((sg_io_hdr_t *) arg)->mx_sb_len;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    sbp.sbp_val = ((sg_io_hdr_t *) arg)->sbp;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    timeout = ((sg_io_hdr_t *) arg)->timeout;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    flags = ((sg_io_hdr_t *) arg)->flags;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    pack_id = ((sg_io_hdr_t *) arg)->pack_id;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    usr_ptr.usr_ptr_len = 0;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    usr_ptr.usr_ptr_val = NULL;
#if 0
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    status = ((sg_io_hdr_t *) arg)->status;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    masked_status = ((sg_io_hdr_t *) arg)->masked_status;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    msg_status = ((sg_io_hdr_t *) arg)->msg_status;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    sb_len_wr = ((sg_io_hdr_t *) arg)->sb_len_wr;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    host_status = ((sg_io_hdr_t *) arg)->host_status;

                in.req.req_val[0].ioctl_request_u.req_sgio.
                    driver_status = ((sg_io_hdr_t *) arg)->driver_status;
                
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    resid = ((sg_io_hdr_t *) arg)->resid;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                    duration = ((sg_io_hdr_t *) arg)->duration;
                in.req.req_val[0].ioctl_request_u.req_sgio.
                   info = ((sg_io_hdr_t *) arg)->info;
#endif
                break;
            }

        case RPC_SIOCGHWTSTAMP:
        case RPC_SIOCSHWTSTAMP:
            in.access = IOCTL_RD;
            if (arg != NULL)
            {
                struct ifreq *ifr = (struct ifreq *)arg;
                tarpc_ifreq  *rpc_ifreq =
                        &in.req.req_val[0].ioctl_request_u.req_ifreq;

                in.req.req_val[0].type = IOCTL_IFREQ;
                if (ifr == NULL)
                    break;
                rpc_ifreq->rpc_ifr_name.rpc_ifr_name_val = ifr->ifr_name;
                rpc_ifreq->rpc_ifr_name.rpc_ifr_name_len =
                        sizeof(ifr->ifr_name);

                if (ifr->ifr_data == NULL)
                    break;

                memcpy(&rpc_ifreq->rpc_ifr_hwstamp, ifr->ifr_data,
                       sizeof(tarpc_hwtstamp_config));
            }
            break;

        case RPC_SIOCETHTOOL:
        {
            int           size = 0;
            struct ifreq *ifr = (struct ifreq *)arg;
            tarpc_ifreq  *rpc_ifreq = 
                    &in.req.req_val[0].ioctl_request_u.req_ifreq;

            in.req.req_val[0].type = IOCTL_IFREQ;
            if (ifr == NULL)
                break;
            rpc_ifreq->rpc_ifr_name.rpc_ifr_name_val = ifr->ifr_name;
            rpc_ifreq->rpc_ifr_name.rpc_ifr_name_len = 
                    sizeof(ifr->ifr_name);

            if (ifr->ifr_data == NULL)
                break;
            rpc_ifreq->rpc_ifr_ethtool.command =
                    *((tarpc_ethtool_command *)(ifr->ifr_data));
            rpc_ifreq->rpc_ifr_ethtool.data.type = ethtool_cmd2type(
                                        rpc_ifreq->rpc_ifr_ethtool.command);

            switch (rpc_ifreq->rpc_ifr_ethtool.data.type)
            {
                case TARPC_ETHTOOL_CMD:
                    size = sizeof(tarpc_ethtool_cmd);
                    break;

                case TARPC_ETHTOOL_PADDR:
                    size = sizeof(tarpc_ethtool_perm_addr);
                    break;

                case TARPC_ETHTOOL_VALUE:
                    size = sizeof(tarpc_ethtool_value);
                    break;

                default:
                    size = 0;
                    break;
            }

            memcpy(&rpc_ifreq->rpc_ifr_ethtool.data.tarpc_ethtool_data_u,
                   ifr->ifr_data, size);

            switch (rpc_ifreq->rpc_ifr_ethtool.command)
            {
                case RPC_ETHTOOL_GSET:
                case RPC_ETHTOOL_GMSGLVL:
                case RPC_ETHTOOL_GLINK:
                case RPC_ETHTOOL_GRXCSUM:
                case RPC_ETHTOOL_GTXCSUM:
                case RPC_ETHTOOL_GSG:
                case RPC_ETHTOOL_GTSO:
                case RPC_ETHTOOL_GGSO:
                case RPC_ETHTOOL_GGRO:
                case RPC_ETHTOOL_GFLAGS:
                case RPC_ETHTOOL_PHYS_ID:
                case RPC_ETHTOOL_GUFO:
                case RPC_ETHTOOL_RESET:
                    in.access = IOCTL_RD;
                    break;

                case RPC_ETHTOOL_SSET:
                case RPC_ETHTOOL_SMSGLVL:
                case RPC_ETHTOOL_SRXCSUM:
                case RPC_ETHTOOL_STXCSUM:
                case RPC_ETHTOOL_SSG:
                case RPC_ETHTOOL_STSO:
                case RPC_ETHTOOL_SGSO:
                case RPC_ETHTOOL_SGRO:
                case RPC_ETHTOOL_SFLAGS:
                case RPC_ETHTOOL_SUFO:
                    in.access = IOCTL_WR;
                    break;

                default:
                    break;
            }
            break;
        }

        default:
            ERROR("Unsupported ioctl code: %d", request);
            rpcs->_errno = TE_RC(TE_RCF, TE_EOPNOTSUPP);
            RETVAL_INT(ioctl, -1);;
    }

    rcf_rpc_call(rpcs, "ioctl", &in, &out);

    if (out.retval == 0 && out.req.req_val != NULL && in.access == IOCTL_RD)
    {
        assert(arg != NULL);

        switch (in.req.req_val[0].type)
        {
            case IOCTL_INT:
                *((int *)arg) = out.req.req_val[0].ioctl_request_u.req_int;
                break;

            case IOCTL_TIMEVAL:
                ((struct tarpc_timeval *)arg)->tv_sec =
                    out.req.req_val[0].ioctl_request_u.req_timeval.tv_sec;
                ((struct tarpc_timeval *)arg)->tv_usec =
                    out.req.req_val[0].ioctl_request_u.req_timeval.tv_usec;
                break;

            case IOCTL_TIMESPEC:
                ((struct tarpc_timespec *)arg)->tv_sec =
                    out.req.req_val[0].ioctl_request_u.req_timespec.tv_sec;
                ((struct tarpc_timespec *)arg)->tv_nsec =
                    out.req.req_val[0].ioctl_request_u.req_timespec.tv_nsec;
                break;

            case IOCTL_IFREQ:
            {
                struct ifreq        *ifreq = (struct ifreq *)arg;
                struct tarpc_ifreq  *rpc_ifreq = 
                        &out.req.req_val[0].ioctl_request_u.req_ifreq;

                switch (request)
                {
                    case RPC_SIOCGIFADDR:
                    case RPC_SIOCGIFNETMASK:
                    case RPC_SIOCGIFBRDADDR:
                    case RPC_SIOCGIFDSTADDR:
                    case RPC_SIOCGIFHWADDR:
                        sockaddr_rpc2h(&rpc_ifreq->rpc_ifr_addr,
                                       &ifreq->ifr_addr,
                                       sizeof(ifreq->ifr_addr),
                                       NULL, NULL);
                        break;

                    case RPC_SIOCGIFMTU:
                        ifreq->ifr_mtu = rpc_ifreq->rpc_ifr_mtu;
                        break;

                    case RPC_SIOCGIFFLAGS:
                        ifreq->ifr_flags =
                            if_fl_rpc2h((uint32_t)(unsigned short int)
                                        rpc_ifreq->rpc_ifr_flags);
                        break;

                    case RPC_SIOCETHTOOL:
                        {
                            int size;

                            switch (rpc_ifreq->rpc_ifr_ethtool.data.type)
                            {
                                case TARPC_ETHTOOL_CMD:
                                    size = sizeof(tarpc_ethtool_cmd);
                                    break;

                                case TARPC_ETHTOOL_PADDR:
                                    size = sizeof(tarpc_ethtool_perm_addr);
                                    break;

                                case TARPC_ETHTOOL_VALUE:
                                    size = sizeof(tarpc_ethtool_value);
                                    break;

                                default:
                                    size = 0;
                                    break;
                            }

                            memcpy(ifreq->ifr_data,
                                   &rpc_ifreq->rpc_ifr_ethtool.data.
                                                tarpc_ethtool_data_u,
                                   size);

                            *((tarpc_ethtool_command *)(ifreq->ifr_data)) = 
                                    rpc_ifreq->rpc_ifr_ethtool.command;
                            break;
                        }

                    case RPC_SIOCGIFINDEX:
                        ifreq->ifr_ifindex = rpc_ifreq->rpc_ifr_ifindex;

                    case RPC_SIOCGIFNAME:
                        memcpy(ifreq->ifr_name,
                               rpc_ifreq->rpc_ifr_name.rpc_ifr_name_val,
                               sizeof(ifreq->ifr_name));

                    default:
                        break;
                }
                break;
            }

            case IOCTL_IFCONF:
            {
                struct tarpc_ifreq *rpc_req =
                    out.req.req_val[0].ioctl_request_u.req_ifconf.
                    rpc_ifc_req.rpc_ifc_req_val;

                struct ifreq *req = ((struct ifconf *)arg)->ifc_req;
                    
                int n = out.req.req_val[0].ioctl_request_u.req_ifconf.nmemb;
                
                if (((struct ifconf *)arg)->ifc_len != 0 &&
                    (int)(n * sizeof(struct ifreq)) > 
                    ((struct ifconf *)arg)->ifc_len)
                {
                    ERROR("TA returned too many interfaces from "
                          "ioctl(SIOCGIFCONF) - it seems that it "
                          "ignores ifc_len");
                }
                
                ((struct ifconf *)arg)->ifc_len = n * sizeof(struct ifreq) +
                    out.req.req_val[0].ioctl_request_u.req_ifconf.extra;

                if (req == NULL)
                   break;

                for (; n > 0; n--, req++, rpc_req++)
                {
                    strcpy(req->ifr_name,
                           rpc_req->rpc_ifr_name.rpc_ifr_name_val);
                    sockaddr_rpc2h(&rpc_req->rpc_ifr_addr,
                                   &req->ifr_addr, sizeof(req->ifr_addr),
                                   NULL, NULL);
                }
                break;
            }
            case IOCTL_ARPREQ:
            {
                sockaddr_rpc2h(&out.req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha,
                               &((struct arpreq *)arg)->arp_ha,
                               sizeof(((struct arpreq *)arg)->arp_ha),
                               NULL, NULL);
                ((struct arpreq *)arg)->arp_flags =
                     arp_fl_rpc2h(out.req.req_val[0].ioctl_request_u.
                                  req_arpreq.rpc_arp_flags);
                break;
            }
            case IOCTL_SGIO:
            {

                RING("IOCTL SG: status=0x%x, host_status=0x%x, "
                     "driver_status=0x%x",
                     out.req.req_val[0].ioctl_request_u.req_sgio.status, 
                     out.req.req_val[0].ioctl_request_u.
                     req_sgio.host_status, 
                     out.req.req_val[0].ioctl_request_u.
                     req_sgio.driver_status);

                memcpy(((sg_io_hdr_t *) arg)->dxferp, 
                       out.req.req_val[0].ioctl_request_u.req_sgio. 
                       dxferp.dxferp_val, 
                       out.req.req_val[0].ioctl_request_u.req_sgio.
                       dxferp.dxferp_len);
                memcpy(((sg_io_hdr_t *) arg)->cmdp, 
                       out.req.req_val[0].ioctl_request_u.req_sgio. 
                       cmdp.cmdp_val, 
                       out.req.req_val[0].ioctl_request_u.req_sgio.
                       cmdp.cmdp_len);
                memcpy(((sg_io_hdr_t *) arg)->sbp, 
                       out.req.req_val[0].ioctl_request_u.req_sgio. 
                       sbp.sbp_val, 
                       out.req.req_val[0].ioctl_request_u.req_sgio.
                       sbp.sbp_len);
                ((sg_io_hdr_t *) arg)->status =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    status;
                ((sg_io_hdr_t *) arg)->masked_status =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    masked_status;
                ((sg_io_hdr_t *) arg)->msg_status =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    msg_status;
                ((sg_io_hdr_t *) arg)->sb_len_wr =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    sb_len_wr;
                ((sg_io_hdr_t *) arg)->host_status =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    host_status;
                ((sg_io_hdr_t *) arg)->driver_status =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    driver_status;
                ((sg_io_hdr_t *) arg)->resid =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    resid;
                ((sg_io_hdr_t *) arg)->duration =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    duration;
                ((sg_io_hdr_t *) arg)->info =
                    out.req.req_val[0].ioctl_request_u.req_sgio.
                    info;
                break;
            }

            default:
                assert(FALSE);
        }
    }
    
    switch (in.req.req_val != NULL ? in.req.req_val[0].type : IOCTL_UNKNOWN)
    {
        case IOCTL_SGIO:
            {
                req_val = "OK";
            }
            break;
        case IOCTL_TIMEVAL:
            req_val = tarpc_timeval2str((struct tarpc_timeval *)arg);
            break;
        case IOCTL_TIMESPEC:
            req_val = tarpc_timespec2str((struct tarpc_timespec *)arg);
            break;

        case IOCTL_INT:
        {
            static char int_buf[128];

            snprintf(int_buf, sizeof(int_buf), "%d", *((int *)arg));
            req_val = int_buf;
            break;
        }

        case IOCTL_IFREQ:
        {
            static char ifreq_buf[1024];

            snprintf(ifreq_buf, sizeof(ifreq_buf),
                     " interface %s: ", ((struct ifreq *)arg)->ifr_name);
            req_val = ifreq_buf;

            switch (request)
            {
                case RPC_SIOCGIFADDR:
                case RPC_SIOCSIFADDR:
                case RPC_SIOCGIFNETMASK:
                case RPC_SIOCSIFNETMASK:
                case RPC_SIOCGIFBRDADDR:
                case RPC_SIOCSIFBRDADDR:
                case RPC_SIOCGIFDSTADDR:
                case RPC_SIOCSIFDSTADDR:
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "%s: %s ",
                             ((request == RPC_SIOCGIFADDR) ? "addr" :
                              (request == RPC_SIOCGIFNETMASK) ? "netmask" :
                              (request == RPC_SIOCGIFBRDADDR) ? "braddr" :
                              (request == RPC_SIOCGIFDSTADDR) ? "dstaddr" :
                              ""),
                              inet_ntoa(SIN(&(((struct ifreq *)arg)->
                                            ifr_addr))->sin_addr));
                    break;

                case RPC_SIOCGIFHWADDR:
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "hwaddr: %02x:%02x:%02x:%02x:%02x:%02x",
                             (unsigned char)((struct ifreq *)arg)->
                                 ifr_hwaddr.sa_data[0],
                             (unsigned char)((struct ifreq *)arg)->
                                 ifr_hwaddr.sa_data[1],
                             (unsigned char)((struct ifreq *)arg)->
                                 ifr_hwaddr.sa_data[2],
                             (unsigned char)((struct ifreq *)arg)->
                                 ifr_hwaddr.sa_data[3],
                             (unsigned char)((struct ifreq *)arg)->
                                 ifr_hwaddr.sa_data[4],
                             (unsigned char)((struct ifreq *)arg)->
                                 ifr_hwaddr.sa_data[5]);
                    break;

                case RPC_SIOCGIFMTU:
                case RPC_SIOCSIFMTU:
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "mtu: %d ",
                             ((struct ifreq *)arg)->ifr_mtu);
                    break;

                case RPC_SIOCGIFNAME:
                case RPC_SIOCGIFINDEX:
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "ifindex: %u ",
                             ((struct ifreq *)arg)->ifr_ifindex);
                    break;
                
                case RPC_SIOCGIFFLAGS:
                case RPC_SIOCSIFFLAGS:
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "flags: %s ",
                             if_fl_rpc2str(if_fl_h2rpc(
                                 (uint32_t)(unsigned short int)
                                     ((struct ifreq *)arg)->ifr_flags)));
                    break;

                case RPC_SIOCSHWTSTAMP:
                case RPC_SIOCGHWTSTAMP:
                {
                    struct ifreq *ifr = (struct ifreq *)arg;
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "HW timestamp config: %s",
                                tarpc_hwtstamp_config2str(
                                    (struct tarpc_tarpc_hwtstamp_config *)
                                        ifr->ifr_data));
                    break;
                }
                case RPC_SIOCETHTOOL:
                {
                    struct ifreq            *ifr = (struct ifreq *)arg;
                    tarpc_ethtool_command    cmd = 
                            *((tarpc_ethtool_command *)ifr->ifr_data);
                    tarpc_ethtool_type       type = ethtool_cmd2type(cmd);
                    
                    snprintf(ifreq_buf + strlen(ifreq_buf),
                             sizeof(ifreq_buf) - strlen(ifreq_buf),
                             "ethtool %s: ", ethtool_cmd_rpc2str(cmd));
                    switch (type)
                    {
                        case TARPC_ETHTOOL_CMD:
                        {
                            struct tarpc_ethtool_cmd *ecmd =
                                (struct tarpc_ethtool_cmd *)
                                                            ifr->ifr_data;
                                
                            snprintf(ifreq_buf + strlen(ifreq_buf),
                                     sizeof(ifreq_buf) - strlen(ifreq_buf),
                                     "supported %x, advertising %x, "
                                     "speed %u, duplex %u, port %u, "
                                     "phy_address %u, transceiver %u, "
                                     "autoneg %u, maxtxpkt %u, "
                                     "maxrxpkt %u",
                                     ecmd->supported, ecmd->advertising,
                                     ecmd->speed, ecmd->duplex, ecmd->port,
                                     ecmd->phy_address, ecmd->transceiver,
                                     ecmd->autoneg, ecmd->maxtxpkt,
                                     ecmd->maxrxpkt);
                            break;
                        }

                        case TARPC_ETHTOOL_PADDR:
                        {
                            struct tarpc_ethtool_perm_addr *eaddr = 
                                (struct tarpc_ethtool_perm_addr *)
                                                            ifr->ifr_data;
                            snprintf(ifreq_buf + strlen(ifreq_buf),
                                     sizeof(ifreq_buf) - strlen(ifreq_buf),
                                     "hwaddr: %02x:%02x:%02x:%02x:%02x"
                                     ":%02x",
                                     (unsigned char)(eaddr->data.data[0]),
                                     (unsigned char)(eaddr->data.data[1]),
                                     (unsigned char)(eaddr->data.data[2]),
                                     (unsigned char)(eaddr->data.data[3]),
                                     (unsigned char)(eaddr->data.data[4]),
                                     (unsigned char)(eaddr->data.data[5]));
                            break;
                        }

                        case TARPC_ETHTOOL_VALUE:
                        {
                            struct tarpc_ethtool_value *evalue =
                                (struct tarpc_ethtool_value *)ifr->ifr_data;

                            if (cmd == RPC_ETHTOOL_RESET)
                                snprintf(
                                    ifreq_buf + strlen(ifreq_buf),
                                    sizeof(ifreq_buf) -
                                                    strlen(ifreq_buf),
                                    "data %s",
                                    ethtool_reset_flags_rpc2str(
                                                            evalue->data));
                            else if (cmd == RPC_ETHTOOL_GFLAGS ||
                                     cmd == RPC_ETHTOOL_SFLAGS)
                                snprintf(
                                    ifreq_buf + strlen(ifreq_buf),
                                    sizeof(ifreq_buf) -
                                                    strlen(ifreq_buf),
                                    "data %s",
                                    ethtool_flags_rpc2str(evalue->data));
                            else
                                snprintf(ifreq_buf + strlen(ifreq_buf),
                                         sizeof(ifreq_buf) -
                                                strlen(ifreq_buf),
                                         "data %u", evalue->data);
                            break;
                        }

                        default:
                            snprintf(ifreq_buf + strlen(ifreq_buf),
                                     sizeof(ifreq_buf) - strlen(ifreq_buf),
                                     "unknown ethtool type");
                    }
                    break;
                }

                default:
                    req_val = " unknown request ";
            }
            break;
        }
        case IOCTL_ARPREQ:
        {
            static char arpreq_buf[1024];
            snprintf(arpreq_buf, sizeof(arpreq_buf), " ARP entry ");

            req_val = arpreq_buf;

            switch (request)
            {
                case RPC_SIOCGARP:
                case RPC_SIOCSARP:
                {
                    static char flags[10];
                    int arp_flags = ((struct arpreq *)arg)->arp_flags;

                    snprintf(arpreq_buf + strlen(arpreq_buf),
                             sizeof(arpreq_buf) - strlen(arpreq_buf),
                             "get/set: ");
                    snprintf(arpreq_buf + strlen(arpreq_buf),
                             sizeof(arpreq_buf) - strlen(arpreq_buf),
                             "protocol address %s, ",
                              inet_ntoa(SIN(&(((struct arpreq *)arg)->
                                            arp_pa))->sin_addr));
                    snprintf(arpreq_buf + strlen(arpreq_buf),
                             sizeof(arpreq_buf) - strlen(arpreq_buf),
                             "HW address: family %d, "
                             "addr %02x:%02x:%02x:%02x:%02x:%02x ",
                             ((struct arpreq *)arg)->arp_ha.sa_family,
                             (unsigned char)((struct arpreq *)arg)->
                                 arp_ha.sa_data[0],
                             (unsigned char)((struct arpreq *)arg)->
                                 arp_ha.sa_data[1],
                             (unsigned char)((struct arpreq *)arg)->
                                 arp_ha.sa_data[2],
                             (unsigned char)((struct arpreq *)arg)->
                                 arp_ha.sa_data[3],
                             (unsigned char)((struct arpreq *)arg)->
                                 arp_ha.sa_data[4],
                             (unsigned char)((struct arpreq *)arg)->
                                 arp_ha.sa_data[5]);
                    {
                         memset(flags, 0, sizeof(flags));
                         if (arp_flags & ATF_COM)
                             strcat(flags, "C");
                         if (arp_flags & ATF_PERM)
                             strcat(flags, "M");
                         if (arp_flags & ATF_PUBL)
                             strcat(flags, "P");
                         if (arp_flags & ATF_MAGIC)
                             strcat(flags, "A");
                         if (arp_flags & ATF_DONTPUB)
                             strcat(flags, "!");
                         if (arp_flags & ATF_USETRAILERS)
                             strcat(flags, "T");
                         if (!(arp_flags & ATF_COM) &&
                             !(arp_flags & ATF_PUBL))
                             strcat(flags, "(incomplete)");
                    }
                    snprintf(arpreq_buf + strlen(arpreq_buf),
                             sizeof(arpreq_buf) - strlen(arpreq_buf),
                             "arp flags %s", flags);
                    break;
                }
                case RPC_SIOCDARP:
                {
                    snprintf(arpreq_buf + strlen(arpreq_buf),
                             sizeof(arpreq_buf) - strlen(arpreq_buf),
                             "delete: ");
                    snprintf(arpreq_buf + strlen(arpreq_buf),
                             sizeof(arpreq_buf) - strlen(arpreq_buf),
                             "protocol address %s, ",
                              inet_ntoa(SIN(&(((struct arpreq *)arg)->
                                            arp_pa))->sin_addr));
                    break;
                }
                default:
                    req_val = " unknown request ";
            }
            break;
        }
        default:
            req_val = "";
            break;
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ioctl, out.retval);
    TAPI_RPC_LOG(rpcs, ioctl, "%d, %s, %p(%s)", "%d",
                 fd, ioctl_rpc2str(request), arg, req_val, out.retval);
    RETVAL_INT(ioctl, out.retval);
}
