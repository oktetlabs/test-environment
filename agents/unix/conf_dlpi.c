/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support using DLPI (implementation).
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

#define TE_LGR_USER     "Unix Conf DLPI"

#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_SYS_DLPI_H
#include <sys/dlpi.h>

#if HAVE_STROPTS_H
#include <stropts.h>
#else
#error DLPI cannot be used without STREAMS
#endif

#include <stdio.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"


#define  MAXADDRLEN      64
#define  MAXSAPLEN       64


/**
 * Open STREAM.
 *
 * @param ifname        Interface name
 *
 * @return Status code.
 */
static te_errno
dlpi_open(const char *ifname, int *fd)
{
    te_errno    rc;
    char        devname[32];

    assert(ifname != NULL);
    assert(fd != NULL);

    if (snprintf(devname, sizeof(devname), "/dev/%s", ifname) >=
            (int)sizeof(devname))
    {
        ERROR("%s:%u: Too small buffer", __FUNCTION__, __LINE__);
        return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
    }

    *fd = open(devname, O_RDWR);
    if (*fd < 0)
    {
        rc = te_rc_os2te(errno);
        if (rc != TE_ENOENT)
            ERROR("%s(): Failed to open device %s: %r", __FUNCTION__,
                  devname, rc);
        return TE_RC(TE_TA_UNIX, rc);
    }

    return 0;
}

/* See the description in conf_dlpi.h */
te_errno
ta_unix_conf_dlpi_phys_addr_get(const char *name, void *addr,
                                size_t *addrlen)
{
    te_errno                rc;
    int                     res;
    int                     fd = -1;
    size_t                  size;
    union DL_primitives    *prim = NULL;
    dl_info_req_t           info_req = { DL_INFO_REQ };
    dl_info_ack_t          *info_ack;
    dl_phys_addr_req_t      pa_req = { DL_PHYS_ADDR_REQ,
                                       DL_CURR_PHYS_ADDR };
    dl_phys_addr_ack_t     *pa_ack;
    dl_error_ack_t         *err;
    struct strbuf           msg;
    int                     msg_flags = 0;


    if (name == NULL || addrlen == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    rc = dlpi_open(name, &fd);
    if (rc != 0)
        return rc;

    size = DL_INFO_ACK_SIZE;
    size += sizeof(union DL_qos_types) + sizeof(union DL_qos_types);
    size += MAXADDRLEN + MAXSAPLEN;
    size += MAXADDRLEN;
    size = MAX(size, DL_PHYS_ADDR_ACK_SIZE + MAXADDRLEN);
    size = MAX(size, DL_ERROR_ACK_SIZE);

    if ((prim = malloc(size)) == NULL)
    {
        rc = TE_ENOMEM;
        goto exit;
    }

    /* Send DL_INFO_REQ */
    msg.buf = (char *)&info_req;
    msg.len = sizeof(info_req);
    if (putmsg(fd, &msg, NULL, RS_HIPRI) < 0)
    {
        rc = te_rc_os2te(errno);
        assert(rc != 0);
        ERROR("%s(): putmsg() failed: %r", __FUNCTION__, rc);
        goto exit;
    }

    /* Receive reply */
    msg.buf = (char *)prim;
    msg.len = 0;
    msg.maxlen = size;
    if ((res = getmsg(fd, &msg, NULL, &msg_flags)) < 0)
    {
        rc = te_rc_os2te(errno);
        assert(rc != 0);
        ERROR("%s(): getmsg() failed: %r", __FUNCTION__, rc);
        goto exit;
    }
    if (res & (MORECTL | MOREDATA))
    {
        ERROR("%s(): Invalid reply for DL_INFO_REQ", __FUNCTION__);
        rc = TE_EPROTO;
        goto exit;
    }

    if (msg.len < (int)DL_INFO_ACK_SIZE)
    {
        ERROR("%s(): Reply for DL_INFO_REQ is too short", __FUNCTION__);
        errno = TE_EBADMSG;
        goto exit;
    }

    info_ack = &prim->info_ack;
    if (info_ack->dl_primitive != DL_INFO_ACK ||
        info_ack->dl_version != DL_VERSION_2)
    {
        ERROR("%s(): Unexpected reply for DL_INFO_REQ", __FUNCTION__);
        errno = TE_EPROTO;
        goto exit;
    }

    if (info_ack->dl_addr_length == 0)
    {
        ERROR("%s(): Zero address length in DL_INFO_ACK", __FUNCTION__);
        errno = TE_EPROTO;
        goto exit;
    }
    if (info_ack->dl_addr_length <= abs(info_ack->dl_sap_length))
    {
        ERROR("%s(): Invalid address length in DL_INFO_ACK", __FUNCTION__);
        errno = TE_EPROTO;
        goto exit;
    }
    if (*addrlen < info_ack->dl_addr_length - abs(info_ack->dl_sap_length))
    {
        ERROR("%s(): Too small buffer for physical address",
              __FUNCTION__);
        errno = TE_ESMALLBUF;
        goto exit;
    }
    *addrlen = info_ack->dl_addr_length - abs(info_ack->dl_sap_length);

    if (addr != NULL)
    {
        if (info_ack->dl_addr_offset != 0)
        {
            /* Address is present in DL_INFO_ACK */
            memcpy(addr, (char *)prim + info_ack->dl_addr_offset,
                   *addrlen);
        }
        else
        {
            /* Send DL_PHYS_ADDR_REQ */
            msg.buf = (char *)&pa_req;
            msg.len = sizeof(pa_req);
            if (putmsg(fd, &msg, NULL, 0) < 0)
            {
                rc = te_rc_os2te(errno);
                assert(rc != 0);
                goto exit;
            }

            /* Receive reply */
            msg.buf = (char *)prim;
            msg.len = 0;
            msg.maxlen = size;
            if ((res = getmsg(fd, &msg, NULL, &msg_flags)) < 0)
            {
                rc = te_rc_os2te(errno);
                assert(rc != 0);
                goto exit;
            }
            if (res & (MORECTL | MOREDATA))
            {
                rc = TE_EPROTO;
                goto exit;
            }
            size = msg.len;

            if (size < sizeof(t_uscalar_t))
            {
                errno = TE_EBADMSG;
                goto exit;
            }

            switch (prim->dl_primitive)
            {
                case DL_PHYS_ADDR_ACK:
                    if (size < DL_PHYS_ADDR_ACK_SIZE)
                    {
                        rc = TE_EBADMSG;
                        break;
                    }
                    
                    pa_ack = &prim->physaddr_ack;
                    if (pa_ack->dl_addr_offset != 0)
                    {
                        if (pa_ack->dl_addr_length != *addrlen)
                        {
                            ERROR("%s(): Unexpected length (%u vs %u) of "
                                  "address in DL_PHYS_ADDR_ACK",
                                  __FUNCTION__,
                                  (unsigned)pa_ack->dl_addr_length,
                                  (unsigned)*addrlen);
                            rc = TE_EPROTO;
                            break;
                        }
                        memcpy(addr,
                               (char *)prim + pa_ack->dl_addr_offset,
                               *addrlen);
                    }
                    else
                    {
                        ERROR("%s(): No address in DL_PHYS_ADDR_ACK",
                              __FUNCTION__);
                        rc = TE_ENOENT;
                    }
                    break;

                case DL_ERROR_ACK:
                    if (size < DL_ERROR_ACK_SIZE)
                    {
                        rc = TE_EBADMSG;
                        break;
                    }

                    err = &prim->error_ack;
                    switch (err->dl_errno)
                    {
                        case DL_SYSERR:
                            rc = te_rc_os2te(err->dl_unix_errno);
                            break;

                        default:
                            rc = TE_EPROTO;
                            break;
                    }
                    break;

                default:
                    rc = TE_EBADMSG;
            }
        }
    }

exit:
    free(prim);
    close(fd);

    return TE_RC(TE_TA_UNIX, rc);
}

/* See the description in conf_dlpi.h */
te_errno
ta_unix_conf_dlpi_phys_addr_set(const char *name, const void *addr, 
                                size_t addrlen)
{
    te_errno                rc;
    int                     res;
    int                     fd = -1;
    size_t                  size;
    union DL_primitives    *prim = NULL;
    dl_set_phys_addr_req_t *req;
    dl_error_ack_t         *err;
    struct strbuf           msg;
    int                     msg_flags = 0;


    if (name == NULL || addrlen == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    rc = dlpi_open(name, &fd);
    if (rc != 0)
        return rc;

    size = DL_SET_PHYS_ADDR_REQ_SIZE + addrlen;
    size = MAX(size, DL_OK_ACK_SIZE);
    size = MAX(size, DL_ERROR_ACK_SIZE);

    if ((prim = malloc(size)) == NULL)
    {
        rc = TE_ENOMEM;
        goto exit;
    }

    /* Prepare DL_SET_PHYS_ADDR_REQ */
    req = &prim->set_physaddr_req;
    req->dl_primitive = DL_SET_PHYS_ADDR_REQ;
    req->dl_addr_length = addrlen;
    req->dl_addr_offset = DL_SET_PHYS_ADDR_REQ_SIZE;
    memcpy((char *)prim + DL_SET_PHYS_ADDR_REQ_SIZE, addr, addrlen);

    /* Send DL_SET_PHYS_ADDR_REQ */
    msg.buf = (char *)prim;
    msg.len = DL_SET_PHYS_ADDR_REQ_SIZE + addrlen;
    if (putmsg(fd, &msg, NULL, 0) < 0)
    {
        rc = te_rc_os2te(errno);
        assert(rc != 0);
        ERROR("%s(): putmsg() failed: %r", __FUNCTION__, rc);
        goto exit;
    }

    /* Receive reply */
    msg.buf = (char *)prim;
    msg.len = 0;
    msg.maxlen = size;
    if ((res = getmsg(fd, &msg, NULL, &msg_flags)) < 0)
    {
        rc = te_rc_os2te(errno);
        assert(rc != 0);
        ERROR("%s(): getmsg() failed: %r", __FUNCTION__, rc);
        goto exit;
    }
    if (res & (MORECTL | MOREDATA))
    {
        ERROR("%s(): Invalid reply for DL_INFO_REQ", __FUNCTION__);
        rc = TE_EPROTO;
        goto exit;
    }

    if (size < sizeof(t_uscalar_t))
    {
        errno = TE_EBADMSG;
        goto exit;
    }

    switch (prim->dl_primitive)
    {
        case DL_OK_ACK:
            if (size < DL_OK_ACK_SIZE)
            {
                rc = TE_EBADMSG;
                break;
            }
            break;

        case DL_ERROR_ACK:
            if (size < DL_ERROR_ACK_SIZE)
            {
                rc = TE_EBADMSG;
                break;
            }

            err = &prim->error_ack;
            switch (err->dl_errno)
            {
                case DL_BADADDR:
                     rc = TE_EINVAL;
                     break;

                case DL_NOTSUPPORTED:
                     rc = TE_EOPNOTSUPP;
                     break;

                case DL_SYSERR:
                    rc = te_rc_os2te(err->dl_unix_errno);
                    break;

                default:
                    rc = TE_EPROTO;
                    break;
            }
            break;

        default:
            rc = TE_EBADMSG;
    }

exit:
    free(prim);
    close(fd);

    return TE_RC(TE_TA_UNIX, rc);
}

#endif /* HAVE_SYS_DLPI_H */
