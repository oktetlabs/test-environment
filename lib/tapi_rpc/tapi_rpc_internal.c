/** @file
 * @brief Test API - RPC
 *
 * Internal definitions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_alloc.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"

/* See description in tapi_rpc_internal.h */
te_errno
tapi_rpc_namespace_check(rcf_rpc_server *rpcs, rpc_ptr ptr, const char* ns,
                         const char *function, int line)
{
    char                       *remote_ns;
    te_errno                    rc;
    const rpc_ptr_id_namespace  id = RPC_PTR_ID_GET_NS(ptr);

    if (ptr == 0)
        return 0;

    if (id < rpcs->namespaces_len && rpcs->namespaces[id] != NULL)
    {
        if (strcmp(rpcs->namespaces[id], ns) == 0)
            return 0;
        else
        {
            WARN("%s:%d: Incorrect namespace, "
                 "possible namespace cache is invalid ('%s' != '%s')",
                  function, line, rpcs->namespaces[id], ns);
            rcf_rpc_namespace_free_cache(rpcs);
        }
    }

    rc = rcf_rpc_namespace_id2str(rpcs, id, &remote_ns);
    if (rc)
    {
        rpcs->_errno = rc;
        return rpcs->_errno;
    }

    if (strcmp(remote_ns, ns) != 0)
    {
        ERROR("%s:%d: Incorrect namespace ('%s' != '%s')",
              function, line, remote_ns, ns);
        free(remote_ns);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        return rpcs->_errno;
    }

    if (id >= rpcs->namespaces_len)
    {
        const size_t n = id + 1;
        char **tmp = (char**)realloc(rpcs->namespaces, n * sizeof(char*));

        if (tmp == NULL)
        {
            ERROR("%s:%d: Out of memory!", function, line);
            free(remote_ns);
            rpcs->_errno = TE_RC(TE_TAPI, TE_ENOMEM);
            return rpcs->_errno;
        }
        memset(tmp + rpcs->namespaces_len, 0,
               (n - rpcs->namespaces_len) * sizeof(char*));

        rpcs->namespaces = tmp;
        rpcs->namespaces_len = n;
    }

    rpcs->namespaces[id] = remote_ns;
    return 0;
}

/* See description in tapi_rpc_internal.h */
const char*
tapi_rpc_namespace_get(rcf_rpc_server *rpcs, rpc_ptr ptr)
{
    const rpc_ptr_id_namespace id = RPC_PTR_ID_GET_NS(ptr);
    if (ptr && id < rpcs->namespaces_len && rpcs->namespaces[id] != NULL)
        return rpcs->namespaces[id];
    return NULL;
}

/* See description in tapi_rpc_internal.h */
te_bool rpc_msghdr_msg_flags_init_check_enabled = TRUE;

/* See description in tapi_rpc_internal.h */
te_errno
msghdr_rpc2tarpc(const rpc_msghdr *rpc_msg, tarpc_msghdr *tarpc_msg,
                 te_bool recv_call)
{
    int             i;
    tarpc_iovec    *iovec_arr;

    if (rpc_msg->msg_riovlen > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Length of the I/O vector is too long (%u) - "
              "increase RCF_RPC_MAX_IOVEC(%u)",
              rpc_msg->msg_riovlen, RCF_RPC_MAX_IOVEC);
        return TE_ENOMEM;
    }

    if (((rpc_msg->msg_iov != NULL) &&
         (rpc_msg->msg_iovlen > rpc_msg->msg_riovlen)))
    {
        ERROR("Inconsistent real and declared lengths of buffers");
        return TE_EINVAL;
    }

    if (rpc_msg->msg_iov != NULL)
    {
        iovec_arr = TE_ALLOC(sizeof(tarpc_iovec) * rpc_msg->msg_riovlen);
        if (iovec_arr == NULL)
            return TE_ENOMEM;

        for (i = 0; i < (int)rpc_msg->msg_riovlen; i++)
        {
            iovec_arr[i].iov_base.iov_base_val =
                            rpc_msg->msg_iov[i].iov_base;
            iovec_arr[i].iov_base.iov_base_len =
                            rpc_msg->msg_iov[i].iov_rlen;
            iovec_arr[i].iov_len = rpc_msg->msg_iov[i].iov_len;
        }

        tarpc_msg->msg_iov.msg_iov_val = iovec_arr;
        tarpc_msg->msg_iov.msg_iov_len = rpc_msg->msg_riovlen;
    }
    tarpc_msg->msg_iovlen = rpc_msg->msg_iovlen;

    if (rpc_msg->msg_name_mode == RPC_MSGHDR_FIELD_RAW ||
        (rpc_msg->msg_name_mode == RPC_MSGHDR_FIELD_DEFAULT && recv_call))
    {
        tarpc_msg->msg_namelen = rpc_msg->msg_namelen;
        sockaddr_raw2rpc(rpc_msg->msg_name, rpc_msg->msg_rnamelen,
                         &tarpc_msg->msg_name);
        /*
         * This will make it to return computed length of converted address
         * on return.
         */
        tarpc_msg->msg_name.flags |= TARPC_SA_LEN_AUTO;
    }
    else
    {
        sockaddr_input_h2rpc(rpc_msg->msg_name, &tarpc_msg->msg_name);
    }

    if (rpc_msg->msg_namelen_exact)
        tarpc_msg->msg_namelen = rpc_msg->msg_namelen;
    else
        tarpc_msg->msg_namelen = -1;

    tarpc_msg->msg_flags = (int)rpc_msg->msg_flags;
    /*
     * Initialize msg_flags with a random value by default, it
     * should not affect anything since the field is ignored.
     */
    MSGHDR_MSG_FLAGS_INIT(rpc_msg, tarpc_msg);

    tarpc_msg->msg_controllen = -1;

    if (rpc_msg->msg_control != NULL)
    {
        int rc;
        int i;

        struct msghdr   msg = { .msg_control = rpc_msg->msg_control,
                                .msg_controllen = rpc_msg->msg_controllen };
        struct cmsghdr *c = NULL;
        uint8_t        *tail_start = rpc_msg->msg_control;
        unsigned int    tail_len;
        size_t          data_len;
        int             cmsghdr_num;

        if (rpc_msg->real_msg_controllen > 0)
        {
            msg.msg_controllen = rpc_msg->real_msg_controllen;
            tarpc_msg->msg_controllen = rpc_msg->msg_controllen;
        }

        /*
         * By default parse msg_control only for send calls.
         * FIXME: remove this after msg_cmsghdr_num usage in existing
         * tests is fixed so that we never set it to nonzero unless we want
         * control data to be parsed.
         */

        if (rpc_msg->msg_control_mode == RPC_MSGHDR_FIELD_CONVERT ||
            (rpc_msg->msg_control_mode == RPC_MSGHDR_FIELD_DEFAULT &&
             !recv_call))
        {
            cmsghdr_num = rpc_msg->msg_cmsghdr_num;
        }
        else
        {
            cmsghdr_num = 0;
        }

        for (i = 0, c = CMSG_FIRSTHDR(&msg);
             i < cmsghdr_num;
             i++, c = CMSG_NXTHDR(&msg, c))
        {
            if (c == NULL)
            {
                ERROR("%s(): fewer than msg_cmsghdr_num control messages "
                      "are provided", __FUNCTION__);
                return TE_EINVAL;
            }
            if ((uint8_t *)c + c->cmsg_len >
                         (uint8_t *)(msg.msg_control) + msg.msg_controllen)
            {
                ERROR("%s(): control message %d has too big length",
                      __FUNCTION__, i);
                return TE_EINVAL;
            }

            data_len = c->cmsg_len -
                       ((uint8_t *)CMSG_DATA(c) - (uint8_t *)c);
            tail_start = (uint8_t *)c + CMSG_SPACE(data_len);
        }

        if ((size_t)(tail_start - (uint8_t *)rpc_msg->msg_control) <
                                                  msg.msg_controllen)
        {
            tail_len = msg.msg_controllen -
                       (tail_start - (uint8_t *)rpc_msg->msg_control);
        }
        else
        {
            tail_len = 0;
            tail_start = NULL;
        }

        if (tail_len > 0)
        {
            uint8_t *tail_dup = NULL;

            tail_dup = TE_ALLOC(tail_len);
            if (tail_dup == NULL)
                return TE_ENOMEM;
            memcpy(tail_dup, tail_start, tail_len);

            tarpc_msg->msg_control_tail.msg_control_tail_val = tail_dup;
            tarpc_msg->msg_control_tail.msg_control_tail_len = tail_len;
        }

        if (tail_len < msg.msg_controllen)
        {
            rc = msg_control_h2rpc(
                            (uint8_t *)rpc_msg->msg_control,
                            msg.msg_controllen - tail_len,
                            &tarpc_msg->msg_control.msg_control_val,
                            &tarpc_msg->msg_control.msg_control_len,
                            NULL, 0);

            if (rc != 0)
            {
                ERROR("%s(): failed to convert control message to TARPC "
                      "format", __FUNCTION__);
                return rc;
            }
        }
    }
    else
    {
        /* Passing non-zero msg_controllen with NULL msg_control. */
        if (rpc_msg->msg_controllen > 0)
            tarpc_msg->msg_controllen = rpc_msg->msg_controllen;
    }

    return 0;
}

/* See description in tapi_rpc_internal.h */
void
tarpc_msghdr_free(tarpc_msghdr *msg)
{
    unsigned int i;

    if (msg == NULL)
        return;

    free(msg->msg_iov.msg_iov_val);

    for (i = 0; i < msg->msg_control.msg_control_len; i++)
        free(msg->msg_control.msg_control_val[i].data.data_val);

    free(msg->msg_control.msg_control_val);
    free(msg->msg_control_tail.msg_control_tail_val);
}

/* See description in tapi_rpc_internal.h */
te_errno
msghdr_tarpc2rpc(const tarpc_msghdr *tarpc_msg, rpc_msghdr *rpc_msg)
{
    unsigned int i = 0;
    te_errno     rc;

    sockaddr_rpc2h(&tarpc_msg->msg_name, rpc_msg->msg_name,
                   rpc_msg->msg_rnamelen,
                   NULL, &rpc_msg->msg_namelen);
    rpc_msg->got_msg_namelen = tarpc_msg->msg_namelen;

    if (tarpc_msg->msg_namelen == 0)
    {
        rpc_msg->msg_namelen = 0;
    }
    else if (!(tarpc_msg->msg_name.flags & TARPC_SA_RAW))
    {
        rpc_msg->msg_namelen = te_sockaddr_get_size(SA(rpc_msg->msg_name));
    }

    for (i = 0; i < rpc_msg->msg_riovlen && rpc_msg->msg_iov != NULL; i++)
    {
        rpc_msg->msg_iov[i].iov_len =
            tarpc_msg->msg_iov.msg_iov_val[i].iov_len;
        memcpy(rpc_msg->msg_iov[i].iov_base,
               tarpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val,
               rpc_msg->msg_iov[i].iov_rlen);
    }

    if (rpc_msg->msg_control != NULL)
    {
        size_t   control_len;

        if (rpc_msg->real_msg_controllen > 0)
            control_len = rpc_msg->real_msg_controllen;
        else
            control_len = rpc_msg->msg_controllen;

        rc = msg_control_rpc2h(
                       tarpc_msg->msg_control.msg_control_val,
                       tarpc_msg->msg_control.msg_control_len,
                       tarpc_msg->msg_control_tail.msg_control_tail_val,
                       tarpc_msg->msg_control_tail.msg_control_tail_len,
                       rpc_msg->msg_control,
                       &control_len);

        if (rc != 0)
            return rc;

        if (control_len > rpc_msg->msg_controllen)
        {
            /*
             * In case of control data truncation it is possible
             * that here more bytes will be required to represent
             * the same data than on TA. Reasons: 1) in case of
             * truncation trailing bytes (as computed by CMSG_SPACE())
             * may be ommitted for the last cmsghdr but here we
             * always include them 2) alignment may differ from
             * that on TA, resulting in more space eaten by each
             * cmsghdr.
             *
             * We do not need this in case we do not check truncated
             * control data as we can set msg_controllen itself to
             * big enough value.
             */
            if (!(tarpc_msg->msg_flags & RPC_MSG_CTRUNC))
            {
                ERROR("%s(): after conversion control data is too "
                      "long while there was no truncation",
                      __FUNCTION__);
                return TE_EINVAL;
            }
        }
        rpc_msg->msg_controllen = control_len;
        rpc_msg->got_msg_controllen = tarpc_msg->msg_controllen;
        rpc_msg->msg_cmsghdr_num = tarpc_msg->msg_control.msg_control_len;
    }

    rpc_msg->msg_flags = (rpc_send_recv_flags)tarpc_msg->msg_flags;
    rpc_msg->in_msg_flags = (rpc_send_recv_flags)tarpc_msg->in_msg_flags;

    return 0;
}

/* See description in tapi_rpc_internal.h */
te_errno
mmsghdrs_rpc2tarpc(const struct rpc_mmsghdr *rpc_mmsgs, unsigned int num,
                   tarpc_mmsghdr **tarpc_mmsgs, te_bool recv_call)
{
    unsigned int     i;
    te_errno         rc = 0;
    tarpc_mmsghdr   *tarpc_mmsgs_aux = NULL;

    if (tarpc_mmsgs == NULL)
    {
        ERROR("%s(): tarpc_mmsgs argument must not be NULL", __FUNCTION__);
        return TE_EINVAL;
    }

    tarpc_mmsgs_aux = TE_ALLOC(sizeof(tarpc_mmsghdr) * num);
    if (tarpc_mmsgs_aux == NULL)
        return TE_ENOMEM;

    for (i = 0; i < num; i++)
    {
        rc = msghdr_rpc2tarpc(&rpc_mmsgs[i].msg_hdr,
                              &tarpc_mmsgs_aux[i].msg_hdr,
                              recv_call);
        if (rc != 0)
        {
            ERROR("%s(): conversion failed for mmsg[%u], rc=%r",
                  __FUNCTION__, i, rc);
            goto finish;
        }

        tarpc_mmsgs_aux[i].msg_len = rpc_mmsgs[i].msg_len;
    }

    *tarpc_mmsgs = tarpc_mmsgs_aux;

finish:

    if (rc != 0)
        tarpc_mmsghdrs_free(tarpc_mmsgs_aux, num);

    return rc;
}

/* See description in tapi_rpc_internal.h */
void
tarpc_mmsghdrs_free(tarpc_mmsghdr *tarpc_mmsgs, unsigned int num)
{
    unsigned int i;

    if (tarpc_mmsgs == NULL)
        return;

    for (i = 0; i < num; i++)
    {
        tarpc_msghdr_free(&tarpc_mmsgs[i].msg_hdr);
    }

    free(tarpc_mmsgs);
}

/* See description in tapi_rpc_internal.h */
te_errno
mmsghdrs_tarpc2rpc(const tarpc_mmsghdr *tarpc_mmsgs,
                   struct rpc_mmsghdr *rpc_mmsgs,
                   unsigned int num)
{
    unsigned int i;
    te_errno     rc;

    for (i = 0; i < num; i++)
    {
        rc = msghdr_tarpc2rpc(&tarpc_mmsgs[i].msg_hdr,
                              &rpc_mmsgs[i].msg_hdr);
        if (rc != 0)
        {
            ERROR("%s(): conversion failed for mmsg[%u], rc=%r",
                  __FUNCTION__, i, rc);
            return rc;
        }

        rpc_mmsgs[i].msg_len = tarpc_mmsgs[i].msg_len;
    }

    return 0;
}
