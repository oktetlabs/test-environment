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
 * $Id$
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
#if HAVE_STROPTS_H
#include <fcntl.h>
#endif
#if HAVE_STROPTS_H
#include <stropts.h>
#endif
#if HAVE_SYS_DLPI_H
#include <sys/dlpi.h>
#define DLPI_SUPPORT 1
#endif
#if HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "tad_eth_sap.h"
#include "tad_utils.h"

#ifdef DLPI_SUPPORT

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
static int
split_dname_unmb(tad_eth_sap *sap)
{
    dlpi_data *dlpi = (dlpi_data *)sap->data;
    char *cp;
    char *eos;
    char *dn = "/dev/";

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

    strcpy(dlpi->name, dn);
    strncpy(dlpi->name + strlen(dn), sap->name, (int)(cp - sap->name));
    dlpi->unit = strtol(cp, &eos, 10);
    if (*eos != '\0')
    {
        ERROR("%s bad unit number", sap->name);
        return -1;
    }
    return 0;
}


/**
 * Return error string in accordance with passed errno.
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
 * Returns an appropriate primitive string in dependence of int value.
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
 * Returns errno on error, 0 on success.
 */
static te_errno
dlpi_request(int fd, char *req, int req_len)
{
    te_errno              rc;
    union  DL_primitives *dlp = (union DL_primitives *)req;
    struct strbuf         ctl;
    int                   flags;

    assert(req);

    ctl.maxlen = 0;
    ctl.len = req_len;
    ctl.buf = req;

    flags = 0;
    if (putmsg(fd, &ctl, (struct strbuf *)NULL, flags) < 0)
    {
        rc = TE_OS_RC(TE_TAD_DLPI, errno);
        ERROR("putmsg(%s) failed, %r",
              dlprim(dlp->dl_primitive), rc);
        return rc;
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
 * Returns errno on error, actual response length on success.
 */
static te_errno
dlpi_ack(int fd, char *resp, int resp_len)
{
    te_errno             rc = 0;
    union DL_primitives *dlp = (union DL_primitives *)resp;
    struct strbuf        ctl;
    int                  flags;

    assert(resp);

    ctl.maxlen = MAXDLBUF;
    ctl.len = 0;
    ctl.buf = resp;

    flags = 0;
    if (getmsg(fd, &ctl, (struct strbuf*)NULL, &flags) < 0)
    {
        rc = TE_OS_RC(TE_TAD_DLPI, errno);
        ERROR("getmsg(%s) failed, %r",
              dlprim(dlp->dl_primitive), rc);
        return rc;
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
                    rc = TE_OS_RC(TE_TAD_DLPI,
                                  dlp->error_ack.dl_unix_errno);
                    ERROR("getmsg(%s), %r", 
                          dlprim(dlp->dl_primitive), rc);
                    break;
                default:
                    ERROR("getmsg(%s) dlerrno:%s",
                          dlprim(dlp->dl_primitive),
                          dlstrerror(dlp->error_ack.dl_errno));
                          rc = TE_RC(TE_TAD_DLPI, TE_EINVAL);
                    break;
            }
            return rc;

            default:
                ERROR("getmsg() unexpected primitive ack %s",
                      dlprim(dlp->dl_primitive));
                return TE_RC(TE_TAD_DLPI, TE_EINVAL);
    }

    if (ctl.len < resp_len)
    {
        ERROR("getmsg(%s) ack too small (%d < %d)",
              dlprim(dlp->dl_primitive), ctl.len, resp_len);
        return TE_RC(TE_TAD_DLPI, TE_EINVAL);
    }
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
    dlpi_data     *dlpi;
    te_errno       rc = 0;
    dl_info_req_t *req;

    assert(sap);
    assert(ifname);

    memset(sap, 0, sizeof(tad_eth_sap));
    strncpy(sap->name, ifname, TAD_ETH_SAP_IFNAME_SIZE);
    sap->data = calloc(1, sizeof(struct dlpi_data));
    assert(sap->data);

    dlpi = (dlpi_data *)sap->data;
    dlpi->buf = calloc(1, MAXDLBUF);
    assert(dlpi->buf);

    req = (dl_info_req_t *)dlpi->buf;

    split_dname_unmb(sap);

    dlpi->fd = open(dlpi->name, O_RDWR);
    if (dlpi->fd < 0)
    {
       rc = TE_OS_RC(TE_TAD_DLPI, errno);
       ERROR("Stream device opening failure, %r", rc);
       goto err_exit;
    }

    req->dl_primitive = DL_INFO_REQ;
    rc = dlpi_request(dlpi->fd, (char *)req, sizeof(dl_info_req_t));
    if (rc != 0)
    {
       ERROR("dlpi_request(DL_INFO_REQ) failed");
       goto err_exit;
    }

    memset(req, 0, sizeof(*req));
    rc = dlpi_ack(dlpi->fd, (char *)req, DL_INFO_ACK_SIZE);
    if (rc != 0)
    {
       ERROR("dlpi_ack(DL_INFO_REQ) failed");
       goto err_exit;
    }

    dlpi->dl_info = *((dl_info_ack_t *)req);

    return 0;

err_exit:
    close(dlpi->fd);
    free(dlpi->buf);
    free(sap->data);
    return rc;
}


/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_detach(tad_eth_sap *sap)
{
    dlpi_data *dlpi = (dlpi_data *)sap->data;
    close(dlpi->fd);
    free(dlpi->buf);
    free(sap->data);
    return 0;
}

static te_errno
dlpi_sap_open(tad_eth_sap *sap, unsigned int mode)
{
    dlpi_data          *dlpi = (dlpi_data *)sap->data;
    te_errno            rc = 0;
    union DL_primitives dlp;


    if (dlpi->dl_info.dl_provider_style == DL_STYLE1)
    {
        ERROR("DLS provider supports DL_STYLE1");
        rc = TE_RC(TE_TAD_DLPI, TE_EINVAL);
        goto err_exit;
    }
    else if (dlpi->dl_info.dl_provider_style == DL_STYLE2)
    {
        dlp.attach_req.dl_primitive = DL_ATTACH_REQ;
        dlp.attach_req.dl_ppa = dlpi->unit;
        rc = dlpi_request(dlpi->fd, (char *)&dlp, sizeof(dlp));
        if (rc != 0)
           goto err_exit;

        memset(&dlp, 0, sizeof(dlp));
        rc = dlpi_ack(dlpi->fd, (char *)&dlp, DL_OK_ACK_SIZE);
        if (rc != 0)
           goto err_exit;
    }
    else
    {
        ERROR("Unknown DL_STYLE");
        rc = TE_RC(TE_TAD_DLPI, TE_EINVAL);
        goto err_exit;
    }

    /* Bind DLSAP to the Stream */
    memset(&dlp, 0, sizeof(dlp));
    dlp.bind_req.dl_primitive = DL_BIND_REQ;
    dlp.bind_req.dl_sap = 0;  /* I am not sure about it absolutely :( */
    dlp.bind_req.dl_service_mode = DL_CLDLS;

    rc = dlpi_request(dlpi->fd, (char *)&dlp, sizeof(dlp));
    if (rc != 0)
        goto err_exit;

    memset(&dlp, 0, sizeof(dlp));
    rc = dlpi_ack(dlpi->fd, (char *)&dlp, DL_BIND_ACK_SIZE);
    if (rc != 0)
        goto err_exit;

    if (mode & (TAD_ETH_RECV_OUT | TAD_ETH_RECV_OTHER))
    {
        if (!(mode & TAD_ETH_RECV_NO_PROMISC))
        {
            /*
             * To catch frames 'To someone else'
             * enable promiscuous DL_PROMISC_PHYS
             */
            memset(&dlp, 0, sizeof(dlp));
            dlp.promiscon_req.dl_primitive = DL_PROMISCON_REQ;
            dlp.promiscon_req.dl_level = DL_PROMISC_PHYS;
            rc = dlpi_request(dlpi->fd, (char *)&dlp, sizeof(dlp));
            if (rc != 0)
            {
                ERROR("Attempt to set DL_PROMISC_PHYS failed");
                goto err_exit;
            }
            
            memset(&dlp, 0, sizeof(dlp));
            rc = dlpi_ack(dlpi->fd, (char *)&dlp, DL_OK_ACK_SIZE);
            if (rc != 0)
                goto err_exit;
        }

        /* Enable promiscuous DL_PROMISC_SAP */
        memset(&dlp, 0, sizeof(dlp));
        dlp.promiscon_req.dl_primitive = DL_PROMISCON_REQ;
        dlp.promiscon_req.dl_level = DL_PROMISC_SAP;
        rc = dlpi_request(dlpi->fd, (char *)&dlp, sizeof(dlp));
        if (rc != 0)
        {
            ERROR("Attempt to set DL_PROMISC_SAP failed");
            goto err_exit;
        }

        memset(&dlp, 0, sizeof(dlp));
        rc = dlpi_ack(dlpi->fd, (char *)&dlp, DL_OK_ACK_SIZE);
        if (rc != 0)
            goto err_exit;
    }

    if (mode & TAD_ETH_RECV_MCAST)
    {
        /*
         * Try to enable multicast (you would have thought
         * promiscuous would be sufficient)
         */
        memset(&dlp, 0, sizeof(dlp));
        dlp.promiscon_req.dl_primitive = DL_PROMISCON_REQ;
        dlp.promiscon_req.dl_level = DL_PROMISC_MULTI;
        rc = dlpi_request(dlpi->fd, (char *)&dlp, sizeof(dlp));
        if (rc != 0)
        {
            ERROR("Attempt to set DL_PROMISC_MULTI failed");
            goto err_exit;
        }

        memset(&dlp, 0, sizeof(dlp));
        rc = dlpi_ack(dlpi->fd, (char *)&dlp, DL_OK_ACK_SIZE);
        if (rc != 0)
            goto err_exit;
    }

    /*
     ** This is a non standard SunOS hack to get the full raw link-layer
     ** processing (M_PROTO message block are not processed).
     */
    {
        struct strioctl str;

        str.ic_cmd = DLIOCRAW;
        str.ic_timout = -1;
        str.ic_len = 0;
        str.ic_dp = NULL;
        rc = ioctl(dlpi->fd, I_STR, &str);
        if (rc < 0)
        {
            rc = TE_OS_RC(TE_TAD_DLPI, errno);
            ERROR("DLIOCRAW: %r", rc);
            goto err_exit;
        }
    }
err_exit:
    return rc;
}


/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send_open(tad_eth_sap *sap,
                      unsigned int mode)
{
    return dlpi_sap_open(sap, mode);
}


/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_send(tad_eth_sap *sap, const tad_pkt *pkt)
{
    dlpi_data          *dlpi = (dlpi_data *)sap->data;
    size_t              iovlen = tad_pkt_seg_num(pkt);
    struct iovec        iov[iovlen];
    int                 pkt_len;
    char               *buf = dlpi->buf;
    te_errno            rc;
    size_t              i;

    /* Convert packet segments to IO vector */
    rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
    if (rc != 0)
    {
        ERROR("Failed to convert segments to I/O vector: %r", rc);
        return rc;
    }

    for (i = 0, pkt_len = 0; i < iovlen; i++)
    {
        pkt_len += iov[i].iov_len;
        if (pkt_len < MAXDLBUF)
        {
            memcpy(buf, iov[i].iov_base, iov[i].iov_len);
            buf += iov[i].iov_len;
        }
        else
        {
            ERROR("The length of DL buffer %d is less than sum "
                  "of segments %d", MAXDLBUF, pkt_len);
            return TE_RC(TE_TAD_DLPI, TE_ENOMEM);
        }
    }
    ERROR("%s: before write()", __FUNCTION__);
    rc = write(dlpi->fd, dlpi->buf, pkt_len);
    if (rc != pkt_len)
    {
        ERROR("write() to DLPI file descriptor returns %d instead of %d",
              rc, pkt_len);
        return TE_RC(TE_TAD_DLPI, TE_ENOMEM);
    }
    ERROR("%s: it was write() -> %d", __FUNCTION__);
    return 0;
}


/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv_open(tad_eth_sap *sap,
                      unsigned int mode)
{
    return dlpi_sap_open(sap, mode);
}


/* See the description in tad_eth_sap.h */
te_errno
tad_eth_sap_recv(tad_eth_sap *sap, unsigned int timeout,
                 tad_pkt *pkt, size_t *pkt_len)
{
    dlpi_data          *dlpi = (dlpi_data *)sap->data;
    struct pollfd       pfd = { dlpi->fd, POLLIN, 0 };
    int                 ret_val;
    te_errno            rc;
    int                 nread;
    int                 flags = 0;

    if (dlpi->fd < 0)
    {
        ERROR("File descriptor is not open");
        return TE_RC(TE_TAD_DLPI, TE_EIO);
    }

    ret_val = poll(&pfd, 1, TE_US2MS(timeout));

    if (ret_val == 0)
    {
        VERB("poll({%d, POLLIN}, %u) timed out",
             dlpi->fd, TE_US2MS(timeout));
        return TE_RC(TE_TAD_DLPI, TE_ETIMEDOUT);
    }

    if (ret_val < 0)
    {
        rc = TE_OS_RC(TE_TAD_DLPI, errno);
        WARN("poll failed: fd=%d: %r", dlpi->fd, rc);
        return rc;
    }

    if (ioctl(dlpi->fd, FIONREAD, &nread) != 0)
    {
        ERROR("FIONREAD failed");
        nread = 0x10000;
    }
    if (nread > (int)tad_pkt_len(pkt))
    {
#if 1
        tad_pkt_free_segs(pkt);
#endif
        size_t       len = nread - tad_pkt_len(pkt);
        tad_pkt_seg *seg = tad_pkt_first_seg(pkt);

        if ((seg != NULL) && (seg->data_ptr == NULL))
        {
            void *mem = malloc(len);

            if (mem == NULL)
            {
                rc = TE_OS_RC(TE_TAD_CSAP, errno);
                assert(rc != 0);
                return rc;
            }
            VERB("%s(): reuse the first segment of packet",
                 __FUNCTION__);
            tad_pkt_put_seg_data(pkt, seg, mem, len,
                                 tad_pkt_seg_data_free);
        }
        else
        {
            seg = tad_pkt_alloc_seg(NULL, len, NULL);
            VERB("%s(): append segment with %u bytes space",
                 __FUNCTION__, (unsigned)len);
            tad_pkt_append_seg(pkt, seg);
        }
    }

    /* Get input message and save it to provided vector */
    {
        size_t               i;
        size_t               to_copy;
        size_t               iovlen = tad_pkt_seg_num(pkt);
        struct iovec         iov[iovlen];
        ssize_t              r;
        struct strbuf        data;
        size_t               data_rest = 0; /* uint*/
        char                *auxp;

        data.maxlen = MAXDLBUF;
        data.len = 0;
        data.buf = dlpi->buf;

        memset(data.buf, 0, MAXDLBUF);
        r = getmsg(dlpi->fd, (struct strbuf*)NULL, &data, &flags);
        if (r < 0)
        {
            rc = TE_OS_RC(TE_TAD_CSAP, errno);
            WARN("getmsg failed: fd=%d: %r", dlpi->fd, rc);
            return rc;
        }
        if (data.len == 0)
        {
            return TE_RC(TE_TAD_DLPI, TE_ETADENDOFDATA);
        }

        /* Convert packet segments to IO vector */
        rc = tad_pkt_segs_to_iov(pkt, iov, iovlen);
        if (rc != 0)
        {
            ERROR("Failed to convert segments to I/O vector: %r", rc);
            return rc;
        }

        /* Save returned data to prepared IO vector */
        data_rest = data.len;
        auxp= data.buf;
        for (i = 0; i < iovlen; i++)
        {
            if ((size_t)(iov[i].iov_len) < data_rest)
            {
                to_copy = iov[i].iov_len;
                data_rest -= iov[i].iov_len;
            }
            else
            {
                to_copy = data_rest;
                data_rest = 0;
            }
            memcpy(iov[i].iov_base, auxp, to_copy);
            if (data_rest == 0)
                break;
            else
                auxp += iov[i].iov_len;
        }

        if (pkt_len != NULL)
            *pkt_len = data.len;
    }
    return 0;
}


/**
 * See the description in tad_eth_sap.h
 * Actually DLPI does not allow separate closing on send/recv and
 * possibly 'ppa' can be detached only.
 */
te_errno
tad_eth_sap_send_close(tad_eth_sap *sap)
{
    int rc = 0;
    UNUSED(sap);

    return rc;
}


/**
 * See the description in tad_eth_sap.h
 * Actually DLPI does not allow separate closing on send/recv and
 * possibly 'ppa' can be detached only.
 */
te_errno
tad_eth_sap_recv_close(tad_eth_sap *sap)
{
    int rc = 0;
    UNUSED(sap);

    return rc;
}

#endif
