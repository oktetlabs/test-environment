/** @file
 * @brief TAD Data Link Provider Interface
 *
 * Implementation routines to access media through DLPI.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Igor Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id $
 */

#define TE_LGR_USER     "TAD DLPI"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
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
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"


#if 0
#include <unistd.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif


#define MAXDLBUF    8192

typedef struct dlpi_data {
    char          name[TAD_ETH_SAP_IFNAME_SIZE]; /**< Device type */
    unsigned int  unit;    /**< Unit number */
    int           fd;      /**< STREAM device file descriptor */
    dl_info_ack_t dl_info; /**< DLPI stream info */
    char         *buf;
} dlpi_data;


/**
 * Split a device name into a device type name and a unit number.
 *
 * Returns -1 on error, 0 on success.
 */
static char *
split_dname_unmb(tad_eth_sap *sap)
{
    char *cp;
    char *eos;

    /* Look for a number at the end of the device name string */
    cp = sap->name + strlen(sap->name) - 1;
    if (*cp < '0' || *cp > '9')
    {
        ERROR("%s missing unit number", sap->name);
        return -1;
    }

    /* Digits at the end of string are unit number */
    while ((cp - 1) >= sap->name &&
          *(cp - 1) >= '0' && *(cp - 1) <= '9')
        cp--;

    strncpy(sap->data->name, sap->name, (cp - sap->name));
    sap->data->unit = strtol(cp, &eos, 10);
    if (*eos != '\0')
    {
        ERROR("%s bad unit number", sap->name);
        return -1;
    }
    return 0;
}


/**
 * Returns error string in accordance with passed errno.
 */
static char *
dlstrerror(uint32_t dl_errno)
{
    static char errstring[6+2+8+1];
    switch (dl_errno)
    {
        case DL_ACCESS:
            return ("Improper permissions for request");
        case DL_BADADDR:
            return ("DLSAP addr in improper format or invalid");
        case DL_BADCORR:
            return ("Seq number not from outstand DL_CONN_IND");
        case DL_BADDATA:
            return ("User data exceeded provider limit");
        case DL_BADPPA:
            /*
             * We have separate devices for separate devices;
             * the PPA is just the unit number.
             */
            return ("Specified PPA (device unit) was invalid");
        case DL_BADPRIM:
            return ("Primitive received not known by provider");
        case DL_BADQOSPARAM:
            return ("QOS parameters contained invalid values");
        case DL_BADQOSTYPE:
            return ("QOS structure type is unknown/unsupported");
        case DL_BADSAP:
            return ("Bad LSAP selector");
        case DL_BADTOKEN:
            return ("Token used not an active stream");
        case DL_BOUND:
            return ("Attempted second bind with dl_max_conind");
        case DL_INITFAILED:
            return ("Physical link initialization failed");
        case DL_NOADDR:
            return ("Provider couldn't allocate alternate address");
        case DL_NOTINIT:
            return ("Physical link not initialized");
        case DL_OUTSTATE:
            return ("Primitive issued in improper state");
        case DL_SYSERR:
            return ("UNIX system error occurred");
        case DL_UNSUPPORTED:
            return ("Requested service not supplied by provider");
        case DL_UNDELIVERABLE:
            return ("Previous data unit could not be delivered");
        case DL_NOTSUPPORTED:
            return ("Primitive is known but not supported");
        case DL_TOOMANY:
            return ("Limit exceeded");
        case DL_NOTENAB:
            return ("Promiscuous mode not enabled");
        case DL_BUSY:
            return ("Other streams for PPA in post-attached");
        case DL_NOAUTO:
            return ("Automatic handling XID&TEST not supported");
        case DL_NOXIDAUTO:
            return ("Automatic handling of XID not supported");
        case DL_NOTESTAUTO:
            return ("Automatic handling of TEST not supported");
        case DL_XIDAUTO:
            return ("Automatic handling of XID response");
        case DL_TESTAUTO:
            return ("Automatic handling of TEST response");
        case DL_PENDING:
            return ("Pending outstanding connect indications");
        default:
            sprintf(errstring, "Error %02x", dl_errno);
            return (errstring);
    }
}


/**
 * Returns appropriate primitive string in dependence of int value.
 */
static char *
dlprim(uint32_t prim)
{
    static char primbuf[80];

    switch (prim)
    {
        case DL_INFO_REQ:
            return ("DL_INFO_REQ");
        case DL_INFO_ACK:
            return ("DL_INFO_ACK");
        case DL_ATTACH_REQ:
            return ("DL_ATTACH_REQ");
        case DL_DETACH_REQ:
            return ("DL_DETACH_REQ");
        case DL_BIND_REQ:
            return ("DL_BIND_REQ");
        case DL_BIND_ACK:
            return ("DL_BIND_ACK");
        case DL_UNBIND_REQ:
            return ("DL_UNBIND_REQ");
        case DL_OK_ACK:
            return ("DL_OK_ACK");
        case DL_ERROR_ACK:
            return ("DL_ERROR_ACK");
        case DL_SUBS_BIND_REQ:
            return ("DL_SUBS_BIND_REQ");
        case DL_SUBS_BIND_ACK:
            return ("DL_SUBS_BIND_ACK");
        case DL_UNITDATA_REQ:
            return ("DL_UNITDATA_REQ");
        case DL_UNITDATA_IND:
            return ("DL_UNITDATA_IND");
        case DL_UDERROR_IND:
            return ("DL_UDERROR_IND");
        case DL_UDQOS_REQ:
            return ("DL_UDQOS_REQ");
        case DL_CONNECT_REQ:
            return ("DL_CONNECT_REQ");
        case DL_CONNECT_IND:
            return ("DL_CONNECT_IND");
        case DL_CONNECT_RES:
            return ("DL_CONNECT_RES");
        case DL_CONNECT_CON:
            return ("DL_CONNECT_CON");
        case DL_TOKEN_REQ:
            return ("DL_TOKEN_REQ");
        case DL_TOKEN_ACK:
            return ("DL_TOKEN_ACK");
        case DL_DISCONNECT_REQ:
            return ("DL_DISCONNECT_REQ");
        case DL_DISCONNECT_IND:
            return ("DL_DISCONNECT_IND");
        case DL_RESET_REQ:
            return ("DL_RESET_REQ");
        case DL_RESET_IND:
            return ("DL_RESET_IND");
        case DL_RESET_RES:
            return ("DL_RESET_RES");
        case DL_RESET_CON:
            return ("DL_RESET_CON");
        default:
            (void) sprintf(primbuf, "unknown primitive 0x%x", prim);
            return (primbuf);
    }
}


/**
 * Send stream device request.
 *
 * @param fd       Stream device file descriptor
 * @param req      Stream device request
 * @param req_len  Stream device request length
 *
 * Returns -1 on error, 0 on success.
 */
static int
send_request(int fd, char *req, int req_len)
{
    union  DL_primitives *dlp = (union DL_primitives *)req;
    struct strbuf         ctl;
    int                   flags;

    ctl.maxlen = 0;
    ctl.len = req_len;
    ctl.buf = req;

    flags = 0;
    if (putmsg(fd, &ctl, (struct strbuf *)NULL, flags) < 0)
    {
        ERROR("putmsg(%s) failed with errno:%s",
              dlprim(dlp->dl_primitive), strerror(errno));
        return -1;
    }
    return 0;
}


/**
 * Receive stream device acknowledgement.
 *
 * @param fd        Stream device file descriptor
 * @param resp      Stream device response
 * @param resp_len  The length of memory provided for response
 *
 * Returns -1 on error, actual response length on success.
 */
static int
recv_ack(int fd, char *resp, int resp_len)
{
    union DL_primitives *dlp = (union DL_primitives *)resp;
    struct strbuf        ctl;
    int                  flags;

    ctl.maxlen = resp_len;
    ctl.len = 0;
    ctl.buf = resp;

    flags = 0;
    if (getmsg(fd, &ctl, (struct strbuf*)NULL, &flags) < 0)
    {
        ERROR("getmsg(%s) failed with errno:%s",
              dlprim(dlp->dl_primitive), strerror(errno));
        return -1;
    }

    switch (dlp->dl_primitive)
    {
        case DL_BIND_ACK:
        case DL_INFO_ACK:
        case DL_OK_ACK:
            break;

        case DL_ERROR_ACK:
            switch (dlp->error_ack.dl_errno)
            {
                case DL_SYSERR:
                    ERROR("getmsg(%s) UNIX errno:%s",
                          dlprim(dlp->dl_primitive),
                          strerror(dlp->error_ack.dl_unix_errno));
                    break;
                default:
                    ERROR("getmsg(%s) dlerrno:%s",
                          dlprim(dlp->dl_primitive),
                          dlstrerror(dlp->error_ack.dl_errno));
                    break;
            }
            return -1;

            default:
                ERROR("getmsg() unexpected primitive ack %s",
                      dlprim(dlp->dl_primitive));
                return -1;
    }

    if (ctl.len < resp_len)
    {
        ERROR("getmsg(%s) ack too small (%d < %d)",
              dlprim(dlp->dl_primitive), ctl.len, resp_len);
        return -1;
    }
    return ctl.len;
}


/**
 * Close DLPI stream device and allocated resources.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
static te_errno
dlpi_close(tad_eth_sap *sap)
{
    int rc = 0;

    close(sap->data->fd);
    free(sap->data->buf);
    free(sap->data);
    return rc;
}


/**
 * See tad_eth_sap.h for description.
 * Actually, we open STREAM device and returns its info.
 * SAP is neigher sending nor receiving after attach.
 *
 * @note It is assumed that ancillary information is constant and will
 *       not be modified before close.
 *
 * @param ifname        Name of the interface/service
 * @param sap           Location for SAP description structure
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_open(), tad_eth_sap_recv_open(),
 *     tad_eth_sap_detach()
 */
te_errno
tad_eth_sap_attach(const char *ifname, tad_eth_sap *sap)
{
    int            rc = 0;
    dl_info_req_t *req;

    memset(sap, 0, sizeof(tad_eth_sap));
    strncpy(sap->name, ifname, TAD_ETH_SAP_IFNAME_SIZE);
    sap->data = calloc(1, sizeof(struct dlpi_data));
    sap->data->buf = calloc(1, MAXDLBUF);
    req = sap->data->buf;

    split_dname_unmb(sap);

    sap->data->fd = open(sap->data->name, O_RDWR);
    if (sap->data->fd == -1)
    {
       ERROR("Stream device opening failure");
       rc = TE_RC(TE_TAD_DLPI, TE_EINVAL);
       goto err_exit;
    }

    req->dl_primitive = DL_INFO_REQ;
    rc = send_request(sap->data->fd, (char *)req, MAXDLBUF);
    if (rc == -1)
    {
       rc = TE_RC(TE_TAD_DLPI, TE_EINVAL);
       goto err_exit;
    }

    req->dl_primitive = DL_INFO_REQ;
    rc = recv_ack(sap->data->fd, (char *)req, DL_INFO_ACK_SIZE);
    if (rc == -1)
    {
       rc = TE_RC(TE_TAD_DLPI, TE_EINVAL);
       goto err_exit;
    }

    sap->data->dl_info = *((dl_info_ack_t *)req);

    return 0;

err_exit:
    close(sap->data->fd);
    free(sap->data->buf);
    free(sap->data);
    return rc;
}

/**
 * Detach Ethernet service access point from service provider and
 * free all allocated resources.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
te_errno
tad_eth_sap_detach(tad_eth_sap *sap)
{
    return dlpi_close(sap);
}


QUESTIONS:
(1) NO send/recv_open, NO tad_eth_sap_send/recv_mode
    tad_eth_sap_open(tad_eth_sap *sap,
                     tad_eth_sap_mode mode);


/**
 * Open Ethernet service access point for sending.
 *
 * @param sap           SAP description structure
 * @param mode          Send mode
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_close(), tad_eth_sap_recv_open()
 */
extern te_errno tad_eth_sap_send_open(tad_eth_sap           *sap,
                                      tad_eth_sap_send_mode  mode);

/**
 * Send Ethernet frame using service access point opened for sending.
 *
 * @param sap           SAP description structure
 * @param pkt           Frame to be sent
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_send_open(), tad_eth_sap_recv()
 */
extern te_errno tad_eth_sap_send(tad_eth_sap *sap, const tad_pkt *pkt);



/**
 * Open Ethernet service access point for receiving.
 *
 * @param sap           SAP description structure
 * @param mode          Receive mode
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_recv_close(), tad_eth_sap_send_open()
 */
extern te_errno tad_eth_sap_recv_open(tad_eth_sap           *sap,
                                      tad_eth_sap_recv_mode  mode);

/**
 * Receive Ethernet frame using service access point opened for
 * receiving.
 *
 * @param sap           SAP description structure
 * @param timeout       Receive operation timeout
 * @param pkt           Frame to be sent
 * @param pkt_len       Location for real packet length
 *
 * @return Status code.
 *
 * @sa tad_eth_sap_recv_open(), tad_eth_sap_send()
 */
extern te_errno tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                                 const tad_pkt *pkt, size_t *pkt_len);








/**
 * Close Ethernet service access point for sending.
 * Actually DLPI does not allow separate closing on send/recv and
 * possibly 'ppa' can be detached only.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
te_errno
tad_eth_sap_send_close(tad_eth_sap *sap)
{
    int rc = 0;
    UNUSED(sap);
    return 0;
}


/**
 * Close Ethernet service access point for receiving.
 * Actually DLPI does not allow separate closing on send/recv and
 * possibly 'ppa' can be detached only.
 *
 * @param sap           SAP description structure
 *
 * @return Status code.
 */
te_errno
tad_eth_sap_recv_close(tad_eth_sap *sap)
{
    int rc = 0;
    UNUSED(sap);
    return rc;
}

#if 0
{
    int rc = 0;

    int ppa = 0;

    char      *dev ="/dev/efge";
//    char      *dev ="/dev/bge";
    char       ebuf[EBUF_SIZE];
    uint32_t   buf[MAXDLBUF];

    dl_info_ack_t *infop;


    printf("\nAttempt to open %s\n", dev);
    fd = open(dev, O_RDWR);
    if (fd == -1)
    {
       perror("open(dev)");
       goto err_exit;
    }
    printf("\n device %s is opened: descr=%d\n", dev, fd);

    if (dlinforeq(fd, ebuf) < 0 ||
            dlinfoack(fd, (char *)buf, ebuf) < 0)
    {
        printf("\n DL_INFO_REQ || DL_INFO_ACK are failed \n");
        goto err_exit;
    }

    infop = &((union DL_primitives *)buf)->info_ack;
    if (infop->dl_provider_style == DL_STYLE1)
    {
        printf("\n+++ DL_STYLE1 +++\n");
        goto err_exit;
    }
    else if (infop->dl_provider_style == DL_STYLE2)
    {
        printf("\n+++ DL_STYLE2 +++\n");
        if ((dlattachreq(fd, ppa, ebuf) < 0 ||
             dlokack(fd, "attach", (char *)buf, ebuf) < 0))
        {
            printf("\n DL_ATTACH_REQ || DL_OK_ACK are failed \n");
            goto err_exit;
        }
    }
    else
    {
        printf("\n+++ Unknown DL_STYLE +++\n");
        goto err_exit;
    }

    if (dlbindreq(fd, 0, ebuf) < 0 ||
        dlbindack(fd, (char *)buf, ebuf) < 0)
    {
        printf("\n DL_BIND_REQ || DL_BIND_ACK are failed \n");
        goto err_exit;
    }

    /* Enable promiscuous */
    if (dlpromisconreq(fd, DL_PROMISC_PHYS, ebuf) < 0 ||
        dlokack(fd, "promisc_phys", (char *)buf, ebuf) < 0)
    {
        printf("\n+++ Set promiscuous failed +++\n");
        goto err_exit;
    }

    if (dlpromisconreq(fd, DL_PROMISC_SAP, ebuf) < 0 ||
        dlokack(fd, "promisc_sap", (char *)buf, ebuf) < 0)
    {
        /* Not fatal if promisc since the DL_PROMISC_PHYS worked */
            printf("WARNING: DL_PROMISC_SAP failed (%s)\n", ebuf);
    }


#if 0
    /*
     ** Try to enable multicast (you would have thought
     ** promiscuous would be sufficient)
     */
    if (dlpromisconreq(fd, DL_PROMISC_MULTI, ebuf) < 0 ||
        dlokack(fd, "promisc_multi", (char *)buf, ebuf) < 0)
    {
        printf("WARNING: DL_PROMISC_MULTI failed (%s)\n", ebuf);
        goto err_exit;
    }
#endif

#if 1
    /*
     ** This is a non standard SunOS hack to get the full raw link-layer
     ** header.
     */
    if (strioctl(fd, DLIOCRAW, 0, NULL) < 0)
    {
        printf("DLIOCRAW: %s", strerror(errno));
        goto err_exit;
    }
#endif

#if 0
    /*
     ** As the last operation flush the read side.
     */
    if (ioctl(fd, I_FLUSH, FLUSHR) != 0) 
    {
        printf("FLUSHR: %s", strerror(errno));
        goto err_exit;
    }
#endif

    do {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
//            tv.tv_sec  = 0;
//            tv.tv_usec = 100 * 1000;
        printf("\n*** Enter to select() \n");
        rc = select(fd+1, &readfds, NULL, NULL, NULL/*&tv*/);
        printf("\n*** After select()  \n");
        if (rc < 0)
        {
            if (errno != EINTR)
            {
                perror("select");
                goto err_exit;
            }
        }
    } while(0);

    do {

       uint8_t   dst[6];
       uint8_t   src[6];
       uint8_t   type[2];

       long                  sndbuf[MAXDLBUF];
       union DL_primitives  *dlp;
       struct strbuf         snddata, sndctl;
       uint8_t               datap[300];

       int i;
       int    flags;
       struct strbuf data;
       struct strbuf ctl;
#define MSG_BUF_SIZE 4096
       unsigned char       buffer[MSG_BUF_SIZE];
       uint32_t            ctlbuf[MAXDLBUF];
       dl_unitdata_ind_t  *ind = (dl_unitdata_ind_t *)ctlbuf;

       ctl.maxlen = MAXDLBUF;
       ctl.len = 0;
       ctl.buf = (char*)ctlbuf;

       data.maxlen = MSG_BUF_SIZE;
       data.len = 0;
       data.buf = buffer;
       if (getmsg(fd, &ctl, &data, &flags) < 0)
       {
           /* Don't choke when we get ptraced */
           if (errno == EINTR)
           {
                printf("\ngetmsg() interrupted\n");
                continue;
           }
           printf("\ngetmsg() failed with errno %s\n", strerror(errno));
                goto err_exit;
       }

       if (ctl.len == -1)
       {
           printf("\n No control info conveyed +++\n");
       }
       else  if (ind->dl_primitive == DL_UNITDATA_IND)
       {
           printf("\n_____ DL_UNITDATA_IND  ____\n");
           printf(" dstaddr:");
           for (i = 0; (unsigned int)i < ind->dl_dest_addr_length; i++)
           {
               printf("%02x", *(buffer + ind->dl_dest_addr_offset + i));
           }
           printf("\n srcaddr:");
           for (i = 0; (unsigned int)i < ind->dl_src_addr_length; i++)
           {
               printf("%02x", *(buffer + ind->dl_src_addr_offset + i));
           }
           printf("\n");
       }

       printf("\n");
       for (i = 0; i < data.len; i++)
           printf("%02x", buffer[i]);
       printf("\ngetmsg() returned %d bytes\n", data.len);


       for (i = 0; i < 6; i++)
           dst[i] = buffer[i];
       for (i = 0; i < 6; i++)
           src[i] = buffer[i + 6];
       type[0] = buffer[12];
       type[1] = buffer[13];

       printf("\n dst=%02x:%02x:%02x:%02x:%02x:%02x "
              " src=%02x:%02x:%02x:%02x:%02x:%02x type:[%02x][%02x]",
              dst[0], dst[1], dst[2], dst[3], dst[4], dst[5],
              src[0], src[1], src[2], src[3], src[4], src[5],
              type[0], type[1]);


       memset(datap, 'a', 300);
       dlp = (union DL_primitives*) sndbuf;

       dlp->unitdata_req.dl_primitive = DL_UNITDATA_REQ;
       dlp->unitdata_req.dl_dest_addr_length = 8 /*addrlen=phys+ppa*/;
       dlp->unitdata_req.dl_dest_addr_offset = sizeof (dl_unitdata_req_t);
       dlp->unitdata_req.dl_priority.dl_min = 0;
       dlp->unitdata_req.dl_priority.dl_max = 0;

       (void) memcpy (dlp + sizeof(dl_unitdata_req_t),
                      src, 6);

       ctl.maxlen = 0;
       ctl.len = sizeof (dl_unitdata_req_t) + 6;
       ctl.buf = (char *) sndbuf;

       data.maxlen = 0;
       data.len = 300;
       data.buf = (char *)datap;

       if (putmsg (fd, &ctl, &data, 0) < 0)
       {
            print("dlunitdatareq: putmsg: %s",
                   strerror(errno));
            return (-1);
       }

    } while (1);

#endif








    
};




