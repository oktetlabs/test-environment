/** @file
 * @brief TAD Data Link Provider Interface
 *
 * Implementation routines to access media through BPF (Berkley Packet
 * Filter) or through AF_PACKET sockets.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAD PF_PACKET/BPF"

#include "te_config.h"

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
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#if HAVE_LINUX_IF_PACKET_H
#include <linux/if_packet.h>
#endif
#if HAVE_LINUX_IF_ETHER_H
#include <linux/if_ether.h>
#endif

#if defined(USE_PF_PACKET) && defined(WITH_PACKET_MMAP_RX_RING)
#include <poll.h>
#include <sys/mman.h>
#endif /* USE_PF_PACKET && WITH_PACKET_MMAP_RX_RING */

#include "te_errno.h"
#include "te_alloc.h"
#include "te_str.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_csap_inst.h"
#include "tad_utils.h"
#include "tad_eth_sap.h"
#include "te_ethernet.h"

/**
 * Number of retries to write data in low layer
 */
#define TAD_WRITE_RETRIES           (128)

/** Maximum number of failed attempts to write data due to ENOBUFS. */
#define TAD_WRITE_NOBUFS            (10000)

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
#ifdef WITH_PACKET_MMAP_RX_RING
    struct tpacket_req  rx_ring_conf;       /**< Rx ring configuration */
    char               *rx_ring;            /**< Rx ring base address */
    unsigned int        rx_ring_frame_cur;  /**< Next frame to check */
#endif /* WITH_PACKET_MMAP_RX_RING */
#else
    pcap_t         *in;         /**< Input handle (for receive) */
    pcap_t         *out;        /**< Output handle (for send) */
    char            errbuf[PCAP_ERRBUF_SIZE]; /**< Error buffer for pcap
                                                   calls error messages */
#endif
    unsigned int    send_mode;  /**< Send mode */
    unsigned int    recv_mode;  /**< Receive mode */

} tad_eth_sap_data;

#ifdef __CYGWIN__
void
check_win_tso_behaviour_and_modify_frame(const char *pkt,
                                         uint32_t len);
#endif

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
        WARN("Frame has been truncated");

#ifdef __CYGWIN__
    check_win_tso_behaviour_and_modify_frame(packet, header->len);
#endif

    if (pktlen > tad_pkt_len(pktptr->pkt))
    {
        rc = tad_pkt_realloc_segs(pktptr->pkt, pktlen);
        if (rc != 0)
            return;
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
#ifdef USE_PF_PACKET
    unsigned int        ifindex;
#endif
    unsigned int        rc_module;
    te_errno            rc;

#ifdef USE_PF_PACKET
    rc_module = TE_TAD_PF_PACKET;
#else
    rc_module = TE_TAD_BPF;
#endif

    if (ifname == NULL || sap == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return TE_RC(rc_module, TE_EFAULT);
    }
    if (strlen(ifname) >= MIN(sizeof(if_req.ifr_name),
                              sizeof(sap->name)))
    {
        ERROR("%s(): Too long interface name", __FUNCTION__);
        return TE_RC(rc_module, TE_E2BIG);
    }

    cfg_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (cfg_socket < 0)
    {
        rc = TE_OS_RC(rc_module, errno);
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
        int efindex = -1, ifindex = -1, vlan = 0;
        FILE *F;
        int ef_type = 0; /* ef* or intf* type of interface name */
        char filename[100], new_ifname[100];
        char str[100];
        unsigned char mac[ETHER_ADDR_LEN];
        unsigned int mac_bytes[ETHER_ADDR_LEN];
        int i;

        te_strlcpy(new_ifname, ifname, sizeof(new_ifname));
        if (strncmp(ifname, "ef", 2) == 0)
        {
          if (strstr(ifname, ".") == NULL)
          {
            sscanf(ifname, "ef%d", &efindex);
            ef_type = 1;
          }
          else
          {
            sscanf(ifname, "ef%d.%d", &efindex, &vlan);
            sprintf(filename, "/tmp/efdata_%d.%d", efindex, vlan);
            ef_type = 2;
          }
        }
        else
        {
          sscanf(ifname, "intf%d", &ifindex);
          ef_type = 0;
        }
        if ((efindex >= 1 && efindex <=2) || (ef_type == 0))
        {
            if (ef_type == 2)
            {
                /* Have already made filename above */
            }
            else
            {
                if (ef_type == 1)
                {
                    sprintf(filename, "/tmp/efdata_%d", efindex);
                }
                else
                {
                    sprintf(filename, "/tmp/intfdata_%d", ifindex);
                }
            }
            F = fopen(filename, "rt");
            if (F != NULL)
            {
                if (fgets(str, 100, F) == NULL)
                {
                    ERROR("Cannot read ifindex from file '%s'", filename);
                    fclose(F);
                    close(cfg_socket);
                    return TE_RC(rc_module, TE_EIO);
                }
                if (str[strlen(str) - 1] == '\n')
                {
                  str[strlen(str) - 1] = 0;
                }
                ifindex = atoi(str);
                if (fgets(str, 100, F) == NULL)
                {
                    ERROR("Cannot read ifname from file '%s'", filename);
                    fclose(F);
                    close(cfg_socket);
                    return TE_RC(rc_module, TE_EIO);
                }
                if (str[strlen(str) - 1] == '\n')
                {
                  str[strlen(str) - 1] = 0;
                }
                TE_SPRINTF(new_ifname, "\\Device\\NPF_%s", str);
                if (fgets(str, 100, F) == NULL)
                {
                    ERROR("Cannot read MAC address from file '%s'", filename);
                    fclose(F);
                    close(cfg_socket);
                    return TE_RC(rc_module, TE_EIO);
                }
                if (str[strlen(str) - 1] == '\n')
                {
                  str[strlen(str) - 1] = 0;
                }
                fclose(F);
                sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
                       &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
                       &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]);
                for (i = 0; i < ETHER_ADDR_LEN; i++)
                    mac[i] = mac_bytes[i];
                memcpy(sap->addr, mac, ETHER_ADDR_LEN);
            }
        }
        te_strlcpy(if_req.ifr_name, new_ifname, sizeof(if_req.ifr_name));
        te_strlcpy(sap->name, new_ifname, sizeof(sap->name));
    }
    else
    {
        te_strlcpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
    }
#ifndef __CYGWIN__
    if (ioctl(cfg_socket, SIOCGIFHWADDR, &if_req))
    {
        rc = TE_OS_RC(rc_module, errno);
        ERROR("%s(): ioctl(%s, SIOCGIFHWADDR) failed: %r",
              __FUNCTION__, ifname, rc);
        close(cfg_socket);
        return rc;
    }
    memcpy(sap->addr, if_req.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
#endif
#endif

#ifdef USE_PF_PACKET
    ifindex = if_nametoindex(ifname);
    if (ifindex == 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): if_nametoindex(%s) failed: %r",
              __FUNCTION__, ifname, rc);
        close(cfg_socket);
        return rc;
    }
#endif

    close(cfg_socket);

    assert(sap->data == NULL);
    sap->data = data = TE_ALLOC(sizeof(*data));
    if (data == NULL)
        return TE_RC(rc_module, TE_ENOMEM);

#ifdef USE_PF_PACKET
    data->ifindex = ifindex;
    data->in = data->out = -1;
#else
    data->in = data->out = NULL;
#endif

#ifndef __CYGWIN__
    te_strlcpy(sap->name, ifname, sizeof(sap->name));
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
    bind_addr.sll_protocol = htons(ETH_P_ALL);
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
    unsigned int        nobufs;
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

    for (retries = 0, ret_val = 0, nobufs = 0;
         ret_val <= 0 && retries < TAD_WRITE_RETRIES &&
         nobufs < TAD_WRITE_NOBUFS;
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

                    nobufs++;
                    retries--;
                    continue;
                }

                default:
                    ERROR("%s(CSAP %d): internal error %r, socket %d",
                          __FUNCTION__, sap->csap->id, rc, fd);
                    return rc;
            }
        }
        nobufs = 0;
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

#ifdef USE_PF_PACKET
static inline te_bool
tad_eth_sap_pkt_vlan_tag_valid(uint16_t    tp_vlan_tci,
                               uint32_t    tp_status)
{
#ifdef TP_STATUS_VLAN_VALID
    return ((tp_vlan_tci != 0) || ((tp_status & TP_STATUS_VLAN_VALID) != 0));
#else /* !TP_STATUS_VLAN_VALID */
    UNUSED(tp_status);

    /* This is not 100% correct check, but it's the only solution */
    return (tp_vlan_tci != 0);
#endif /* TP_STATUS_VLAN_VALID */
}
#endif /* USE_PF_PACKET */

#ifdef WITH_PACKET_MMAP_RX_RING
#define ETH_SAP_PKT_RX_RING_NB_FRAMES_MIN   256
#define ETH_SAP_PKT_RX_RING_NB_FRAMES_MAX   1024
#define ETH_SAP_PKT_RX_RING_FRAME_LEN \
    te_round_up_pow2(TPACKET2_HDRLEN + ETHER_HDR_LEN + TAD_VLAN_TAG_LEN + \
                     UINT16_MAX + ETHER_CRC_LEN)

static te_errno
tad_eth_sap_pkt_rx_ring_setup(tad_eth_sap *sap)
{
    tad_eth_sap_data   *data;
    csap_p              csap;
    tad_recv_context   *rx_ctx;
    unsigned int        nb_frames;
    int                 version;
    struct tpacket_req *tp;
    te_errno            rc;

    if (sap == NULL)
        return TE_RC(TE_TAD_PF_PACKET, TE_EINVAL);

    data = sap->data;
    if (data == NULL)
        return TE_RC(TE_TAD_PF_PACKET, TE_EINVAL);

    tp = &data->rx_ring_conf;

    csap = sap->csap;
    if (csap == NULL)
        return TE_RC(TE_TAD_PF_PACKET, TE_EINVAL);

    rx_ctx = csap_get_recv_context(csap);
    if (rx_ctx == NULL)
        return TE_RC(TE_TAD_PF_PACKET, TE_EINVAL);

    version = TPACKET_V2;
    if (setsockopt(data->in, SOL_PACKET, PACKET_VERSION, &version,
                   sizeof(version)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(PACKET_VERSION) failed: %r", rc);
        return rc;
    }

    nb_frames = te_round_up_pow2(rx_ctx->ptrn_data.n_units);
    nb_frames = MAX(nb_frames, ETH_SAP_PKT_RX_RING_NB_FRAMES_MIN);
    nb_frames = MIN(nb_frames, ETH_SAP_PKT_RX_RING_NB_FRAMES_MAX);

    tp->tp_frame_nr = nb_frames;
    tp->tp_frame_size = ETH_SAP_PKT_RX_RING_FRAME_LEN;
    tp->tp_block_size = tp->tp_frame_nr * tp->tp_frame_size;
    tp->tp_block_nr = 1;

    if (setsockopt(data->in, SOL_PACKET, PACKET_RX_RING,
                   (void *)tp, sizeof(*tp)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(PACKET_RX_RING) failed: %r", rc);
        return rc;
    }

    data->rx_ring = mmap(NULL, tp->tp_block_size * tp->tp_block_nr,
                         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED,
                         data->in, 0);
    if (data->rx_ring == NULL)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): mmap() failed: %r", rc);
        return rc;
    }

    data->rx_ring_frame_cur = 0;

    return 0;
}

static void
tad_eth_sap_pkt_rx_ring_release(tad_eth_sap *sap)
{
    tad_eth_sap_data   *data;
    struct tpacket_req *tp;

    if (sap == NULL)
        return;

    data = sap->data;
    if (data == NULL)
        return;

    tp = &data->rx_ring_conf;

    if (munmap(data->rx_ring, tp->tp_block_size * tp->tp_block_nr) != 0)
        ERROR("%s(): munmap() failed: %r", TE_OS_RC(TE_TAD_PF_PACKET, errno));
}

static te_errno
tad_eth_sap_pkt_rx_ring_recv(tad_eth_sap        *sap,
                             unsigned int        timeout,
                             tad_pkt            *pkt,
                             size_t             *pkt_len,
                             struct sockaddr_ll *from)
{
    tad_eth_sap_data       *data;
    struct tpacket_req     *tp;
    struct tpacket2_hdr    *ph;
    te_bool                 vlan_tag_valid;
    size_t                  seg_len;
    uint8_t                *seg_data = NULL;
    size_t                  copy_len;
    size_t                  remaining_len;
    size_t                  data_off;
    tad_pkt_seg            *seg;

    if ((sap == NULL) || (pkt == NULL) || (pkt_len == NULL) || (from == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    data = sap->data;
    if ((data == NULL) || (data->rx_ring == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    tp = &data->rx_ring_conf;
    if (tp->tp_frame_nr == 0)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    ph = (struct tpacket2_hdr *)((uint8_t *)data->rx_ring +
                                 (data->rx_ring_frame_cur *
                                  tp->tp_frame_size));

    if ((ph->tp_status & TP_STATUS_USER) == 0)
    {
        struct pollfd   pollset;
        int             ret_val;

        pollset.fd = data->in;
        pollset.events = POLLIN;
        pollset.revents = 0;

        ret_val = poll(&pollset, 1, TE_US2MS(timeout));
        if (ret_val == 0)
            return TE_RC(TE_TAD_CSAP, TE_ETIMEDOUT);

        if (ret_val < 0)
            return TE_OS_RC(TE_TAD_CSAP, errno);
    }

    VERB("%s: tpacket_req tp_frame_nr=%u tp_frame_size=%u",
         __func__, tp->tp_frame_nr, tp->tp_frame_size);
    VERB("%s: tpacket2_hdr tp_status=%u tp_len=%u tp_snaplen=%u tp_mac=%u "
         "tp_net=%u tp_sec=%u tp_nsec=%u tp_vlan_tci=0x%x tp_vlan_tpid=0x%x",
         __func__, ph->tp_status, ph->tp_len, ph->tp_snaplen, ph->tp_mac,
         ph->tp_net, ph->tp_sec, ph->tp_nsec, ph->tp_vlan_tci,
#ifdef TP_STATUS_VLAN_TPID_VALID
         ph->tp_vlan_tpid
#else
         UINT16_MAX
#endif
         );

    vlan_tag_valid = tad_eth_sap_pkt_vlan_tag_valid(ph->tp_vlan_tci,
                                                    ph->tp_status);

    seg_len = (vlan_tag_valid) ? ph->tp_len + TAD_VLAN_TAG_LEN : ph->tp_len;
    seg_data = TE_ALLOC(seg_len);
    if (seg_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    remaining_len = seg_len;

    copy_len = MIN(remaining_len, 2 * ETHER_ADDR_LEN);
    memcpy(seg_data, (uint8_t *)ph + ph->tp_mac, copy_len);
    data_off = copy_len;
    remaining_len -= copy_len;

    if (vlan_tag_valid && remaining_len >= TAD_VLAN_TAG_LEN)
    {
        struct tad_vlan_tag *tag;

        tag = (struct tad_vlan_tag *)(seg_data + data_off);

#ifdef TP_STATUS_VLAN_TPID_VALID
        tag->vlan_tpid = htons((ph->tp_status & TP_STATUS_VLAN_TPID_VALID) ?
                               ph->tp_vlan_tpid : ETH_P_8021Q);
#else
        tag->vlan_tpid = htons(ETH_P_8021Q);
#endif
        tag->vlan_tci = htons(ph->tp_vlan_tci);

        data_off += TAD_VLAN_TAG_LEN;
        remaining_len -= TAD_VLAN_TAG_LEN;
    }

    memcpy(seg_data + data_off, (uint8_t *)ph + ph->tp_mac + (2 * ETHER_ADDR_LEN),
           remaining_len);

    /*
     * It is not guaranteed that the TAD packet consists of exactly one
     * segment, so it is reasonable to re-allocate the entire packet
     */
    tad_pkt_free_segs(pkt);
    seg = tad_pkt_alloc_seg(seg_data, seg_len, tad_pkt_seg_data_free);
    if (seg == NULL)
    {
        free(seg_data);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    tad_pkt_append_seg(pkt, seg);
    *pkt_len = seg_len;

    memcpy(from, (uint8_t *)ph + TPACKET_ALIGN(sizeof(*ph)), sizeof(*from));

    /* Return the entry to the kernel */
    ph->tp_status = 0;
    __sync_synchronize();

    /* Update the ring offset to point to the next entry */
    data->rx_ring_frame_cur = (data->rx_ring_frame_cur + 1) % tp->tp_frame_nr;

    return 0;
}
#endif /* WITH_PACKET_MMAP_RX_RING */

/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_open(tad_eth_sap *sap, unsigned int mode)
{
    tad_eth_sap_data   *data;
    te_errno            rc;
#ifdef USE_PF_PACKET
    int                 use_packet_auxdata;
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

#ifndef WITH_PACKET_MMAP_RX_RING
    use_packet_auxdata = 1;
    if (setsockopt(data->in, SOL_PACKET, PACKET_AUXDATA, &use_packet_auxdata,
                   sizeof(use_packet_auxdata)) != 0)
    {
        rc = TE_OS_RC(TE_TAD_PF_PACKET, errno);
        ERROR("%s(): setsockopt(PACKET_AUXDATA) failed: %r", rc);
        goto error_exit;
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
#else /* WITH_PACKET_MMAP_RX_RING */
    UNUSED(use_packet_auxdata);
    UNUSED(buf_size);
#endif /* WITH_PACKET_MMAP_RX_RING */

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

#ifdef WITH_PACKET_MMAP_RX_RING
    rc = tad_eth_sap_pkt_rx_ring_setup(sap);
    if (rc != 0)
        goto error_exit;
#endif /* WITH_PACKET_MMAP_RX_RING */
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

#if defined(USE_PF_PACKET) && !defined(WITH_PACKET_MMAP_RX_RING)
static int
tad_eth_sap_parse_ancillary_data(int msg_flags, tad_pkt *pkt, size_t *pkt_len,
                                 void *cmsg_buf, size_t cmsg_buf_len)
{
    struct msghdr           msg;
    struct cmsghdr         *cmsg;
    te_errno                rc;

    /* We re-create msghdr structure partially */
    memset(&msg, 0, sizeof(msg));
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = cmsg_buf_len;

    if (msg_flags & MSG_CTRUNC)
        WARN("%s(): MSG_CTRUNC flag was set by recvmsg(); will parse available "
             "amount of ancillary data only", __FUNCTION__);

    for (cmsg = CMSG_FIRSTHDR(&msg);
         cmsg != NULL;
         cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        struct tpacket_auxdata *aux;
        struct tad_vlan_tag    *tag;
        tad_pkt_seg            *cur_seg;
        uint8_t                *new_seg_data;
        size_t                  bytes_remain;

        if (cmsg->cmsg_len < CMSG_LEN(sizeof(struct tpacket_auxdata)) ||
            cmsg->cmsg_level != SOL_PACKET ||
            cmsg->cmsg_type != PACKET_AUXDATA)
            continue;

        aux = (struct tpacket_auxdata *)CMSG_DATA(cmsg);

        if (!tad_eth_sap_pkt_vlan_tag_valid(aux->tp_vlan_tci, aux->tp_status))
            continue;

        cur_seg = CIRCLEQ_FIRST(&pkt->segs);

        for (bytes_remain = 2 * ETHER_ADDR_LEN;
             bytes_remain >= cur_seg->data_len;
             bytes_remain -= cur_seg->data_len,
             cur_seg = CIRCLEQ_NEXT(cur_seg, links));

        new_seg_data = malloc(cur_seg->data_len + TAD_VLAN_TAG_LEN);

        if (new_seg_data == NULL)
        {
            rc = TE_OS_RC(TE_TAD_CSAP, errno);
            ERROR("%s(): malloc() failed: %r", __FUNCTION__, rc);
            return rc;
        }

        if (bytes_remain > 0)
            memcpy(new_seg_data, cur_seg->data_ptr, bytes_remain);

        memcpy(new_seg_data + bytes_remain + TAD_VLAN_TAG_LEN,
               cur_seg->data_ptr + bytes_remain,
               cur_seg->data_len - bytes_remain);

        tag = (struct tad_vlan_tag *)(new_seg_data + bytes_remain);

#ifdef TP_STATUS_VLAN_TPID_VALID
        tag->vlan_tpid = htons((aux->tp_status & TP_STATUS_VLAN_TPID_VALID) ?
                               aux->tp_vlan_tpid : ETH_P_8021Q);
#else
        tag->vlan_tpid = htons(ETH_P_8021Q);
#endif
        tag->vlan_tci = htons(aux->tp_vlan_tci);

        tad_pkt_put_seg_data(pkt, cur_seg, new_seg_data,
                             cur_seg->data_len + TAD_VLAN_TAG_LEN,
                             tad_pkt_seg_data_free);

        *pkt_len += TAD_VLAN_TAG_LEN;
    }

    return 0;
}
#endif /* USE_PF_PACKET && !WITH_PACKET_MMAP_RX_RING */

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
    int                 msg_flags;
    union {
        struct cmsghdr  cmsg;
        char            buf[CMSG_SPACE(sizeof(struct tpacket_auxdata))];
    } cmsg_buf;
    size_t              cmsg_buf_len = sizeof(cmsg_buf);
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
#ifdef WITH_PACKET_MMAP_RX_RING
    UNUSED(data);
    UNUSED(fromlen);
    UNUSED(msg_flags);
    UNUSED(cmsg_buf);
    UNUSED(cmsg_buf_len);

    memset(&from, 0, sizeof(from));

    rc = tad_eth_sap_pkt_rx_ring_recv(sap, timeout, pkt, pkt_len, &from);
    if (rc != 0)
        return rc;

#else /* !WITH_PACKET_MMAP_RX_RING */
    rc = tad_common_read_cb_sock(sap->csap, data->in, MSG_TRUNC, timeout,
                                 pkt, SA(&from), &fromlen, pkt_len,
                                 &msg_flags, &cmsg_buf, &cmsg_buf_len);
    if (rc != 0)
        return rc;

    rc = tad_eth_sap_parse_ancillary_data(msg_flags, pkt, pkt_len,
                                          &cmsg_buf, cmsg_buf_len);
    if (rc != 0)
        return rc;
#endif /* WITH_PACKET_MMAP_RX_RING */

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
#ifdef WITH_PACKET_MMAP_RX_RING
    tad_eth_sap_pkt_rx_ring_release(sap);
#endif /* WITH_PACKET_MMAP_RX_RING */
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

#ifdef __CYGWIN__
#define ETH_STD_HDR      14
#define ETH_VLAN_HDR     18
#define PROTO_TYPE_IP    htons(0x800)
#define PROTO_TYPE_VLAN  htons(0x810)

#define IS_VLAN_FRAME(_pkt)         \
    ((*((uint16_t *)_pkt + 6)) == PROTO_TYPE_VLAN)

/**
 * Used to retrieve the total length field of the IP header,
 * if pkt is an IP packet.
 *  @param  pkt      Location of the packet
 *  @param  field    OUT Location of the total length field
 *  @return 0 for an IP packet, -1 otherwise.
 */
inline int
get_ip_total_len(const char *pkt, uint16_t **field)
{
    char *t = pkt;

    if (!IS_VLAN_FRAME(t))
    {
        t += 12;
    }
    else
    {
        t += 16;
    }

    *field = (uint16_t *) t;
    if (**field == PROTO_TYPE_IP)
    {
        *field += 2;
        return 0;
    }
    return -1;
}

/**
 * Detects Windows TSO behaviour when the total length field of the
 * IP header is set to zero for the packets which are expcted to be
 * segmentated. Sets the field to a meaningful value.
 *
 *  @param  pkt Location of the packet
 *  @param  len The actual packet length
 * */

void
check_win_tso_behaviour_and_modify_frame(const char *pkt, uint32_t len)
{
    uint16_t*   ip_tot_len;

    if ((get_ip_total_len(pkt, &ip_tot_len) == 0) && (*ip_tot_len == 0))
    {
        *ip_tot_len = IS_VLAN_FRAME(pkt) ? ntohs(len - ETH_VLAN_HDR) :
                                          ntohs(len - ETH_STD_HDR);
    }
}
#undef PROTO_TYPE_VLAN
#undef PROTO_TYPE_IP
#undef ETH_VLAN_HDR
#undef ETH_STD_HDR
#endif

#endif /* defined(USE_PF_PACKET) || defined(USE_BPF) */
