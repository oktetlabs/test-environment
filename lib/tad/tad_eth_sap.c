/** @file
 * @brief TAD Data Link Provider Interface
 *
 * Implementation routines to access media through BPF (Berkley Packet
 * Filter) or through AF_PACKET sockets.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD PF_PACKET/BPF"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef PF_PACKET
#define USE_PF_PACKET   1
#elif HAVE_PCAP_H
#include <pcap.h>
#define USE_BPF         1
#endif

#if defined(USE_PF_PACKET) || defined(USE_BPF)

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_csap_inst.h"
#include "tad_utils.h"
#include "tad_eth_sap.h"

/**
 * Number of retries to write data in low layer
 */
#define TAD_WRITE_RETRIES           (128)

/**
 * Default timeout for waiting write possibility. This macro should 
 * be used only for initialization of 'struct timeval' variables. 
 */
#define TAD_WRITE_TIMEOUT_DEFAULT   { 1, 0 }

#ifdef USE_BPF
#define TAD_ETH_SAP_FEXP_SIZE       (128)
#define TAD_ETH_SAP_SNAP_LEN        (0xffff)
#endif

/** Internal data of Ethernet service access point via BPF or AF_SOCKET */
typedef struct tad_eth_sap_data {
#ifdef USE_PF_PACKET
    int             in;         /**< Input socket (for receive) */
    int             out;        /**< Output socket (for send) */
    unsigned int    ifindex;    /**< Interface index */
#else
    pcap_t         *in;         /**< Input handle (for receive) */
    pcap_t         *out;        /**< Output handle (for send) */
    char            errbuf[PCAP_ERRBUF_SIZE]; /**< Error buffer for pcap
                                                   calls error messages */
#endif
    unsigned int    send_mode;  /**< Send mode */
    unsigned int    recv_mode;  /**< Receive mode */

} tad_eth_sap_data;

#ifdef USE_PF_PACKET
/**
 * Close PF_PACKET socket.
 *
 * @param sock      Location of the socket file descriptor
 *
 * @return Status code.
 */
static te_errno
close_socket(int *sock)
{
    assert(sock != NULL);
    if (*sock >= 0)
    {
        if (close(*sock) != 0)
        {
            te_errno rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);

            ERROR("%s(): close() failed: %r", __FUNCTION__, rc);
            return rc;
        }
        INFO("PF_PACKET socket %d closed", *sock);
        *sock = -1;
    }
    return 0;
}
#else
/**
 * Struct to pass to pcap_dispatch() as the last parameter
 */ 
typedef struct pkt_len_pkt {
    tad_pkt *pkt;     /**< Frame to be sent */
    size_t  *pkt_len; /**< Location for real packet length */
} pkt_len_pkt;

/**
 * Callback for pcap_loop() function.
 *
 *  @param ptr     Location of some tad_eth_sap_recv() function parameters
 *  @param header  Location of pcap_pkthdr
 *  @param packet  Location of the first packet
 *
 */ 
static void 
pkt_handl(u_char *ptr, const struct pcap_pkthdr *header,
          const u_char *packet)
{
    bpf_u_int32   pktlen = header->caplen;
    pkt_len_pkt  *pktptr = (pkt_len_pkt *)ptr;
    te_errno      rc;

    if (header->len != pktlen)
        WARN("Frame has been trunced");

    if (pktlen > tad_pkt_len(pktptr->pkt))
    {
        tad_pkt_free_segs(pktptr->pkt);

        size_t       len = pktlen - tad_pkt_len(pktptr->pkt);
        tad_pkt_seg *seg = tad_pkt_first_seg(pktptr->pkt);

        if ((seg != NULL) && (seg->data_ptr == NULL))
        {
            void *mem = malloc(len);

            if (mem == NULL)
            {
                rc = TE_OS_RC(TE_TAD_CSAP, errno);
                assert(rc != 0);
                return;
            }
            VERB("%s(): reuse the first segment of packet", __FUNCTION__);
            tad_pkt_put_seg_data(pktptr->pkt, seg,
                                 mem, len, tad_pkt_seg_data_free);
        }
        else
        {
            seg = tad_pkt_alloc_seg(NULL, len, NULL);
            VERB("%s(): append segment with %u bytes space", __FUNCTION__,
                 (unsigned)len);
            tad_pkt_append_seg(pktptr->pkt, seg);
        }
    }

    /* Logical block to allocate iov of volatile length on the stack */
    {
        size_t          iovlen = tad_pkt_seg_num(pktptr->pkt);
        struct iovec    iov[iovlen];
        int             i;
        int             rest = pktlen;
        u_char         *ptr = (u_char *)packet;

        /* Convert packet segments to IO vector */
        rc = tad_pkt_segs_to_iov(pktptr->pkt, iov, iovlen);
        if (rc != 0)
        {
            ERROR("Failed to convert segments to I/O vector: %r", rc);
            return;
        }

        /* Copy packet into IO vector */
        for (i = 0; rest > 0; i++)
        {
            memcpy(iov[i].iov_base, ptr, iov[i].iov_len);
            rest -= iov[i].iov_len;
            ptr += iov[i].iov_len;
        }

        if (pktptr->pkt_len != NULL)
            *(pktptr->pkt_len) = (ssize_t)pktlen;
    }

    return;
}
#endif

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_attach(const char *ifname, tad_eth_sap *sap)
{
    tad_eth_sap_data   *data;
    int                 cfg_socket;
    struct ifreq        if_req;
    te_errno            rc;

    if (ifname == NULL || sap == NULL) 
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
#ifdef USE_PF_PACKET
        return TE_RC(TE_TAD_PF_PACKET, TE_EFAULT);
#else
        return TE_RC(TE_TAD_BPF, TE_EFAULT);
#endif
    }
    if (strlen(ifname) >= MIN(sizeof(if_req.ifr_name),
                              sizeof(sap->name)))
    {
        ERROR("%s(): Too long interface name", __FUNCTION__);
#ifdef USE_PF_PACKET
        return TE_RC(TE_TAD_PF_PACKET, TE_E2BIG);
#else
        return TE_RC(TE_TAD_BPF, TE_E2BIG);
#endif
    }

    cfg_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (cfg_socket < 0)
    {
#ifdef USE_PF_PACKET
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
#else
        rc = TE_OS_RC(TE_TAD_BPF, errno);
#endif
        ERROR("%s(): socket(AF_INET, SOCK_DGRAM, 0) failed: %r",
              __FUNCTION__, rc);
        return rc;
    }

#ifdef SIOCGIFHWADDR /* FIXME */
    memset(&if_req, 0, sizeof(if_req));
    if (strncmp(ifname, "ef", 2) == 0 ||
        strncmp(ifname, "intf", 4) == 0)
    {
        /* Reading real interface index from file */
        int efindex = -1, ifindex = -1;
        FILE *F;
        int ef_type = 0; /* ef* or intf* type of interface name */
        char filename[100], new_ifname[100];
        char str[100];
        unsigned char mac[ETHER_ADDR_LEN];
        unsigned int mac_bytes[ETHER_ADDR_LEN];
        int i;

        strcpy(new_ifname, ifname);
        if (strncmp(ifname, "ef", 2) == 0)
        {
          sscanf(ifname, "ef%d", &efindex);
          ef_type = 1;
        }
        else
        {
          sscanf(ifname, "intf%d", &ifindex);
          ef_type = 0;
        }
        if ((efindex >= 1 && efindex <=2) || (ef_type == 0))
        {
            if (ef_type == 1)
            {
                sprintf(filename, "/tmp/efdata_%d", efindex);
            }
            else
            {
                sprintf(filename, "/tmp/intfdata_%d", ifindex);
            }
            F = fopen(filename, "rt");
            if (F != NULL)
            {
                fgets(str, 100, F);
                if (str[strlen(str) - 1] == '\n')
                {
                  str[strlen(str) - 1] = 0;
                }
                ifindex = atoi(str);
                fgets(str, 100, F);
                if (str[strlen(str) - 1] == '\n')
                {
                  str[strlen(str) - 1] = 0;
                }
                sprintf(new_ifname, "\\Device\\NPF_%s", str);
                fgets(str, 100, F);
                if (str[strlen(str) - 1] == '\n')
                {
                  str[strlen(str) - 1] = 0;
                }
                fclose(F);
                sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
                       &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
                       &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]);
                for (i = 0; i < 6; i++)
                    mac[i] = mac_bytes[i];
                memcpy(sap->addr, mac, ETHER_ADDR_LEN);
            }
        }
        strncpy(if_req.ifr_name, new_ifname, sizeof(if_req.ifr_name));
        strcpy(sap->name, new_ifname);
    }
    else
    {
        strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
    }
#ifndef __CYGWIN__
    if (ioctl(cfg_socket, SIOCGIFHWADDR, &if_req))
    {
#ifdef USE_PF_PACKET
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
#else
        rc = TE_OS_RC(TE_TAD_BPF, errno);
#endif
        ERROR("%s(): ioctl(%s, SIOCGIFHWADDR) failed: %r",
              __FUNCTION__, ifname, rc);
        close(cfg_socket);
        return rc;
    }
    memcpy(sap->addr, if_req.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
#endif
#endif

#ifdef USE_PF_PACKET
    memset(&if_req, 0, sizeof(if_req));
    strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
    if (ioctl(cfg_socket, SIOCGIFINDEX, &if_req))
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): ioctl(%s, SIOCGIFINDEX) failed: %r",
              __FUNCTION__, ifname, rc);
        close(cfg_socket);
        return rc;
    }
#endif

    close(cfg_socket);

    assert(sap->data == NULL);
    sap->data = data = TE_ALLOC(sizeof(*data));
    if (data == NULL)
#ifdef USE_PF_PACKET
        return TE_RC(TE_TAD_PF_PACKET, TE_ENOMEM);
#else
        return TE_RC(TE_TAD_BPF, TE_ENOMEM);
#endif

#ifdef USE_PF_PACKET
    data->ifindex = if_req.ifr_ifindex;
    data->in = data->out = -1;
#else
    data->in = data->out = NULL;
#endif
    
#ifndef __CYGWIN__
    strcpy(sap->name, ifname);
#endif

    return 0;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send_open(tad_eth_sap *sap, unsigned int mode)
{
    tad_eth_sap_data   *data;
    te_errno            rc;
#ifdef USE_PF_PACKET
    int                 buf_size;
    struct sockaddr_ll  bind_addr;
#endif

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

#ifdef USE_PF_PACKET
    if (data->out >= 0)
        return 0;
#else
    if (data->out != NULL)
        return 0;
#endif

#ifdef USE_PF_PACKET
    /* 
     * Create PF_PACKET socket:
     *  - type: SOCK_RAW - full control over Ethernet header
     *  - protocol: 0 - do not receive any packets
     */
    data->out = socket(PF_PACKET, SOCK_RAW, htons(0));
    if (data->out < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): socket(PF_PACKET, SOCK_RAW, 0) failed: %r",
              __FUNCTION__, rc);
        return rc;
    }

    /*
     * Set send buffer size.
     * TODO: reasonable size of send buffer to be investigated.
     */
    buf_size = 0x100000; 
    if (setsockopt(data->out, SOL_SOCKET, SO_SNDBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(SO_SNDBUF) failed: %r", rc);
        goto error_exit;
    }

    /* 
     * Bind PF_PACKET socket:
     *  - sll_protocol: 0 - do not receive any packets
     *  - sll_hatype. sll_pkttype, sll_halen, sll_addr are not used for
     *    binding
     */
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(0);
    bind_addr.sll_ifindex = data->ifindex;

    if (bind(data->out, SA(&bind_addr), sizeof(bind_addr)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("Failed to bind PF_PACKET socket: %r", rc);
        goto error_exit;
    }
#else
    /* 
     *  Obtain a packet capture descriptor
     */
    if ((data->out = pcap_open_live(sap->name, TAD_ETH_SAP_SNAP_LEN,
                                    0, 0, data->errbuf)) == NULL)
    {
        rc = TE_OS_RC(TE_TAD_BPF, errno);
        ERROR("%s(): pcap_open_live(%s, %d, %d, %d, %d) failed: %r",
              __FUNCTION__, sap->name, TAD_ETH_SAP_SNAP_LEN, 1,
              0, data->errbuf, rc);

        return rc;
    }
#endif

    data->send_mode = mode;

#ifdef USE_PF_PACKET
    INFO("PF_PACKET socket %d opened and bound for send", data->out);
#else
    INFO("BPF opened for send");
#endif

    return 0;
#ifdef USE_PF_PACKET
error_exit:
    if (close(data->out) < 0)
        assert(FALSE);
    data->out = -1;
    return rc;
#endif
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send(tad_eth_sap *sap, const tad_pkt *pkt)
{
    tad_eth_sap_data   *data;
    size_t              iovlen = tad_pkt_seg_num(pkt);
    struct iovec        iov[iovlen];
    te_errno            rc;
    unsigned int        retries = TAD_WRITE_RETRIES;
    int                 ret_val;
    fd_set              write_set;
    int                 fd;
#ifdef __CYGWIN__
    unsigned char      *packet_data = NULL, *cur_data = NULL;
    int                 seg;
#endif
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);
#ifdef USE_PF_PACKET
    if (data->out < 0)
    {
        ERROR("%s(): no output socket", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }
    fd = data->out;
#else
    if (data->out == NULL)
    {
        ERROR("%s(): no output socket", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }
#ifndef __CYGWIN__
    if ((fd = pcap_fileno(data->out)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        ERROR("%s(): pcap_fileno() returned %d : %r",
              __FUNCTION__, fd, rc);
        return rc;
    }
#endif
#endif

#ifndef __CYGWIN__
    F_VERB("%s: writing data to socket: %d", __FUNCTION__, fd);
#endif

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to I/O vector: %r", rc);
        return rc;
    }

#ifdef __CYGWIN__
    /* Prepare block of data to send */
    packet_data = malloc(pkt->segs_len);
    if (NULL == packet_data)
    {
        ERROR("Failed to allocate buffer for all packet data");
        return TE_RC(TE_TAD_CSAP, TE_ENOBUFS);
    }
    cur_data = packet_data;
    for (seg = 0; seg < iovlen; seg++)
    {
      memcpy(cur_data, iov[seg].iov_base, iov[seg].iov_len);
      cur_data += iov[seg].iov_len;
    }
    if ((unsigned int)(cur_data - packet_data) != pkt->segs_len)
    {
        ERROR("Size error while creating full packet from segments");
        free(packet_data);
        return TE_RC(TE_TAD_CSAP, TE_ENOBUFS);
    }
#endif

    for (retries = 0, ret_val = 0;
         ret_val <= 0 && retries < TAD_WRITE_RETRIES;
         retries++)
    {
#ifndef __CYGWIN__
        struct timeval timeout = TAD_WRITE_TIMEOUT_DEFAULT; 
    
        FD_ZERO(&write_set);
        FD_SET(fd, &write_set);

        ret_val = select(fd + 1, NULL, &write_set, NULL, &timeout);
        if (ret_val == 0)
        {
            F_INFO("%s(): select to write timed out, retry %u", retries);
            continue;
        }

        if (ret_val == 1)
            ret_val = writev(fd, iov, iovlen);

        if (ret_val < 0)
        {
            rc = te_rc_os2te(errno);
            VERB("CSAP #%d, errno %r, retry %d",
                 sap->csap->id, rc, retries);
            switch (rc)
            {
                case TE_ENOBUFS:
                {
                    /* 
                     * It seems that 0..127 microseconds is enough
                     * to hope that buffers will be cleared and
                     * does not fall down performance.
                     */
                    struct timeval clr_delay = { 0, rand() & 0x3f };

                    select(0, NULL, NULL, NULL, &clr_delay);
                    continue;
                }

                default:
                    ERROR("%s(CSAP %d): internal error %r, socket %d", 
                          __FUNCTION__, sap->csap->id, rc, fd);
                    return rc;
            }
        } 
#else /* !__CYGWIN */
        if (0 == pcap_sendpacket(data->out, packet_data, pkt->segs_len))
        {
          ret_val = pkt->segs_len;
          break;
        }
        else
        {
          ret_val = -1;
        }
#endif /* !__CYGWIN__ */
    }

#ifdef __CYGWIN__
    free(packet_data);
#endif

    if (retries == TAD_WRITE_RETRIES)
    {
        ERROR("CSAP #%d, too many retries made, failed", sap->csap->id);
        return TE_RC(TE_TAD_CSAP, TE_ENOBUFS);
    }

    F_VERB("CSAP #%d, system write return %d", 
            sap->csap->id, ret_val); 

    if (ret_val < 0)
        return TE_OS_RC(TE_TAD_CSAP, errno);

    return 0;
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send_close(tad_eth_sap *sap)
{
    tad_eth_sap_data   *data;
    fd_set              write_set;
    struct timeval      timeout = TAD_WRITE_TIMEOUT_DEFAULT; 
    int                 ret_val;
    int                 fd;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

#ifdef USE_PF_PACKET
    fd = data->out;
#else
    assert(data->out != NULL);
#ifndef __CYGWIN__
    if ((fd = pcap_fileno(data->out)) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TAD_CSAP, errno);

        ERROR("%s(): pcap_fileno() returned %d : %r",
              __FUNCTION__, fd, rc);
        return rc;
    }
#else /* !__CYGWIN__ */
    fd = -1;
#endif /* !__CYGWIN__ */
#endif

    if (fd >= 0)
    {
        /* check that all data in socket is sent */
        FD_ZERO(&write_set);
        FD_SET(fd, &write_set);

        ret_val = select(fd + 1, NULL, &write_set, NULL, &timeout);
        if (ret_val == 0)
        {
            WARN("Ethernet (socket %d) SAP is still sending", fd);
        }
        else if (ret_val < 0)
        {
#ifdef USE_PF_PACKET
            te_errno    rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
#else
            te_errno    rc = TE_OS_RC(TE_TAD_BPF, errno);
#endif

            ERROR("%s(): select() failed: %r", __FUNCTION__, rc);
        }
        /* Close in any case */
    }

#ifdef USE_PF_PACKET
    return close_socket(&data->out);
#else
    pcap_close(data->out);
    data->out = NULL;
    return 0;
#endif
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_open(tad_eth_sap *sap, unsigned int mode)
{
    tad_eth_sap_data   *data;
    te_errno            rc;
#ifdef USE_PF_PACKET
    int                 buf_size;
    struct sockaddr_ll  bind_addr;
    struct packet_mreq  mr;
#endif

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

#ifdef USE_PF_PACKET
    if (data->in >= 0)
        return 0;
#else
    if (data->in != NULL)
        return 0;
#endif

#ifdef USE_PF_PACKET
    /* 
     * Create PF_PACKET socket:
     *  - type: SOCK_RAW - full control over Ethernet header
     *  - protocol: 0 - receive nothing before bind to interface
     */
    data->in = socket(PF_PACKET, SOCK_RAW, htons(0));
    if (data->in < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("socket(PF_PACKET, SOCK_RAW, 0) failed: %r", rc);
        return rc;
    }

    /*
     * Set receive buffer size.
     * TODO: reasonable size of receive buffer to be investigated.
     */
    buf_size = 0x100000; 
    if (setsockopt(data->in, SOL_SOCKET, SO_RCVBUF,
                   &buf_size, sizeof(buf_size)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(SO_RCVBUF) failed: %r", rc);
        goto error_exit;
    }

    if ((mode & TAD_ETH_RECV_OTHER) && !(mode & TAD_ETH_RECV_NO_PROMISC))
    {
        /*
         * Enable promiscuous mode for the socket on specified interface.
         */
        memset(&mr, 0, sizeof(mr));
        mr.mr_ifindex = data->ifindex;
        mr.mr_type = PACKET_MR_PROMISC;
        if (setsockopt(data->in, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                       &mr, sizeof(mr)) != 0)
        {
            rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
            ERROR("%s(): setsockopt: PACKET_ADD_MEMBERSHIP failed: %r",
                  __FUNCTION__, rc); 
            goto error_exit;
        }
    }

    /* 
     * Bind PF_PACKET socket:
     *  - sll_protocol: ETH_P_ALL - receive everything
     *  - sll_hatype. sll_pkttype, sll_halen, sll_addr are not used for
     *    binding
     */
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(ETH_P_ALL);
    bind_addr.sll_ifindex = data->ifindex;

    if (bind(data->in, SA(&bind_addr), sizeof(bind_addr)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("Failed to bind PF_PACKET socket: %r", rc);
        goto error_exit;
    }
#else
    /*  Obtain a packet capture descriptor */
    data->in = pcap_open_live(sap->name, TAD_ETH_SAP_SNAP_LEN,
                              ((mode & TAD_ETH_RECV_OTHER) &&
                               !(mode & TAD_ETH_RECV_NO_PROMISC)) ? 1 : 0,
                              10 /* read timeout in ms */, data->errbuf);
    if (data->in == NULL)
    {
        rc = TE_OS_RC(TE_TAD_BPF, errno);
        ERROR("%s(): pcap_open_live(%s, %d, %d, %d, %s) failed: %r",
              __FUNCTION__, sap->name, TAD_ETH_SAP_SNAP_LEN, 1,
              0, data->errbuf, rc);
        return rc;
    }
#endif

    data->recv_mode = mode;
#ifdef USE_PF_PACKET
    INFO("PF_PACKET socket %d opened and bound for receive", data->in);
#else
    INFO("BPF opened and bound for receive on %s", sap->name);
#endif

    return 0;

#ifdef USE_PF_PACKET
error_exit:
    if (close(data->in) < 0)
        assert(FALSE);
    data->in = -1;
    return rc;
#endif
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                 tad_pkt *pkt, size_t *pkt_len)
{
    tad_eth_sap_data   *data;
    te_errno            rc;
#ifdef USE_PF_PACKET
    struct sockaddr_ll  from;
    socklen_t           fromlen = sizeof(from);
#else
    int                 fd;
    struct timeval      tv = { 0, timeout};
    int                 ret_val;
    fd_set              readfds;
    pkt_len_pkt         ptr;
#endif

    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);

#ifdef USE_PF_PACKET
    rc = tad_common_read_cb_sock(sap->csap, data->in, MSG_TRUNC, timeout,
                                 pkt, SA(&from), &fromlen, pkt_len);
    if (rc != 0)
        return rc;

    switch (from.sll_pkttype)
    {
        case PACKET_HOST:
            if ((data->recv_mode & TAD_ETH_RECV_HOST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_BROADCAST:
            if ((data->recv_mode & TAD_ETH_RECV_BCAST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_MULTICAST:
            if ((data->recv_mode & TAD_ETH_RECV_MCAST) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_OTHERHOST:
            if ((data->recv_mode & TAD_ETH_RECV_OTHER) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        case PACKET_OUTGOING:
            if ((data->recv_mode & TAD_ETH_RECV_OUT) == 0)
                return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
            break;

        default:
            WARN("%s(): Unknown type %u of packet received",
                 __FUNCTION__, from.sll_pkttype);
            return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
    }

    return 0;
#else
#ifndef __CYGWIN__
    if ((fd = pcap_get_selectable_fd(data->in)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_BPF, errno);
        ERROR("%s(): pcap_get_selectable_fd() returned %d",
              __FUNCTION__, fd);
        return rc;
    }

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    ret_val = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret_val == 0)
    {
        F_VERB(CSAP_LOG_FMT "select(%d, {%d}, NULL, NULL, {%d, %d}) timed "
               "out", CSAP_LOG_ARGS(sap->csap), fd + 1, fd, tv.tv_sec,
               tv.tv_usec);
        return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
    }

    if (ret_val < 0)
    {
        rc = TE_OS_RC(TE_TAD_CSAP, errno);
        WARN(CSAP_LOG_FMT "select() failed: sock=%d: %r",
             CSAP_LOG_ARGS(sap->csap), fd, rc);
        return rc;
    }
#else
    if ((fd = pcap_fileno(data->in)) < 0)
    {
        rc = TE_OS_RC(TE_TAD_BPF, errno);
        ERROR("%s(): pcap_fileno() returned %d",
              __FUNCTION__, fd);
        return rc;
    }
#endif
    ptr.pkt = pkt;
    ptr.pkt_len = pkt_len;
    rc = pcap_dispatch(data->in, 1, pkt_handl, (u_char *)&ptr);
    if (rc < 0)
    {
        rc = TE_OS_RC(TE_TAD_BPF, errno);
        ERROR("%s(): pcap_dispatch() returned %d",
              __FUNCTION__, rc);
        return rc;
    }
    if (rc == 0)
    {
        F_VERB(CSAP_LOG_FMT "select(%d, {%d}, NULL, NULL, {%d, %d}) timed "
               "out", CSAP_LOG_ARGS(sap->csap), fd + 1, fd, tv.tv_sec,
               tv.tv_usec);
        return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);
    }
    
    return 0;
#endif
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_close(tad_eth_sap *sap)
{
    tad_eth_sap_data *data;
    
    assert(sap != NULL);
    data = sap->data;
    assert(data != NULL);
#ifdef USE_PF_PACKET
    return close_socket(&data->in);
#else
    pcap_close(data->in);
    data->in = NULL;
    return 0;
#endif
}

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_detach(tad_eth_sap *sap)
{
    tad_eth_sap_data   *data;
    te_errno            result = 0;
#ifdef USE_PF_PACKET
    te_errno            rc;
#endif

    assert(sap != NULL);
    data = sap->data;
    sap->data = NULL;
    assert(data != NULL);

#ifdef USE_PF_PACKET
    if (data->in != -1)
    {
        WARN("Force close of input PF_PACKET socket on detach");
        rc = close_socket(&data->in);
        TE_RC_UPDATE(result, rc);
    }
    if (data->out != -1)
    {
        WARN("Force close of output PF_PACKET socket on detach");
        rc = close_socket(&data->out);
        TE_RC_UPDATE(result, rc);
    }
#else
    if (data->in != NULL)
    {
        WARN("Force close of input BPF on detach");
        pcap_close(data->in);
    }
    if (data->out != NULL)
    {
        WARN("Force close of output BPF on detach");
        pcap_close(data->out);
    }
#endif

    free(data);

    return result;
}

#endif /* defined(USE_PF_PACKET) || defined(USE_BPF) */
