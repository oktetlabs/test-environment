/** @file
 * @brief API for processing struct msghdr in RPC calls
 *
 * Implementation of API for processing struct msghdr in RPC calls
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "rpcs_msghdr.h"
#include "te_tools.h"

/** Maximum length of argument name */
#define MAX_ARG_NAME_LEN 1024

/**
 * Extra bytes allocated for some arguments to check that
 * a target function does not change them beyond specified
 * length.
 */
#define ARG_EXTRA_LEN 200

/* See description in rpcs_msghdr.h */
te_errno
rpcs_msghdr_tarpc2h(rpcs_msghdr_check_args_mode check_args,
                    const struct tarpc_msghdr *tarpc_msg,
                    rpcs_msghdr_helper *helper, struct msghdr *msg,
                    checked_arg_list *arglist, const char *name_fmt, ...)
{
    char        name_base_buf[MAX_ARG_NAME_LEN];
    te_string   name_base_str = TE_STRING_BUF_INIT(name_base_buf);
    char        name_buf[MAX_ARG_NAME_LEN];
    te_string   name_str = TE_STRING_BUF_INIT(name_buf);

    const tarpc_sa *tarpc_addr = NULL;
    te_errno        rc = 0;
    unsigned int    i;
    size_t          control_len;
    size_t          max_addr_len = 0;

    va_list ap;

    memset(msg, 0, sizeof(*msg));
    memset(helper, 0, sizeof(*helper));

    va_start(ap, name_fmt);
    rc = te_string_append_va(&name_base_str, name_fmt, ap);
    va_end(ap);
    if (rc != 0)
        return rc;

    tarpc_addr = &tarpc_msg->msg_name;

    if (tarpc_addr->flags & TARPC_SA_NOT_NULL)
    {
        max_addr_len = sizeof(struct sockaddr_storage);
        if (tarpc_addr->flags & TARPC_SA_RAW)
        {
            if (tarpc_addr->raw.raw_len > max_addr_len)
                max_addr_len = tarpc_addr->raw.raw_len;
        }
        if (tarpc_msg->msg_namelen >= 0)
        {
            if ((size_t)(tarpc_msg->msg_namelen) > max_addr_len)
                max_addr_len = tarpc_msg->msg_namelen;
        }

        helper->addr_rlen = max_addr_len + ARG_EXTRA_LEN;
        helper->addr_data = TE_ALLOC(helper->addr_rlen);
        if (helper->addr_data == NULL)
            return TE_ENOMEM;
    }

    rc = sockaddr_rpc2h(tarpc_addr, SA(helper->addr_data),
                        max_addr_len,
                        &helper->addr, &helper->addr_len);
    if (rc != 0)
        return rc;

    msg->msg_name = helper->addr;
    if (tarpc_msg->msg_namelen >= 0)
        msg->msg_namelen = tarpc_msg->msg_namelen;
    else
        msg->msg_namelen = helper->addr_len;

    msg->msg_iovlen = tarpc_msg->msg_iovlen;

    if (tarpc_msg->msg_iov.msg_iov_val != NULL)
    {
        msg->msg_iov = TE_ALLOC(sizeof(*(msg->msg_iov)) *
                                tarpc_msg->msg_iov.msg_iov_len);
        if (msg->msg_iov == NULL)
            return TE_ENOMEM;

        for (i = 0; i < tarpc_msg->msg_iov.msg_iov_len; i++)
        {
            msg->msg_iov[i].iov_base =
                    tarpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val;
            msg->msg_iov[i].iov_len =
                    tarpc_msg->msg_iov.msg_iov_val[i].iov_len;

            if (check_args != RPCS_MSGHDR_CHECK_ARGS_NONE)
            {
                te_string_reset(&name_str);
                rc = te_string_append(&name_str, "%s.msg_iov[%d].iov_val",
                                      name_base_str.ptr, i);
                if (rc != 0)
                    return rc;

                tarpc_init_checked_arg(
                    arglist, msg->msg_iov[i].iov_base,
                    tarpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_len,
                    (check_args == RPCS_MSGHDR_CHECK_ARGS_RECV ?
                                          msg->msg_iov[i].iov_len : 0),
                    name_str.ptr);
            }
        }


    }

    if (tarpc_msg->msg_control.msg_control_val != NULL ||
        tarpc_msg->msg_control_tail.msg_control_tail_val != NULL)
    {
        control_len = tarpc_cmsg_total_len(
                              tarpc_msg->msg_control.msg_control_val,
                              tarpc_msg->msg_control.msg_control_len);
        control_len += tarpc_msg->msg_control_tail.msg_control_tail_len;

        helper->real_controllen = control_len + ARG_EXTRA_LEN;
        msg->msg_control = TE_ALLOC(helper->real_controllen);
        helper->orig_control = TE_ALLOC(control_len);
        if (msg->msg_control == NULL || helper->orig_control == NULL)
            return TE_ENOMEM;
        msg->msg_controllen = control_len;

        rc = msg_control_rpc2h(
                      tarpc_msg->msg_control.msg_control_val,
                      tarpc_msg->msg_control.msg_control_len,
                      tarpc_msg->msg_control_tail.msg_control_tail_val,
                      tarpc_msg->msg_control_tail.msg_control_tail_len,
                      msg->msg_control, &msg->msg_controllen);
        if (rc != 0)
            return rc;

        memcpy(helper->orig_control, msg->msg_control, msg->msg_controllen);
    }
    else
    {
        msg->msg_control = NULL;
        msg->msg_controllen = 0;
    }

    if (tarpc_msg->msg_controllen >= 0)
        msg->msg_controllen = tarpc_msg->msg_controllen;

    helper->orig_controllen = msg->msg_controllen;

    msg->msg_flags = send_recv_flags_rpc2h(tarpc_msg->msg_flags);
    helper->orig_msg_flags = msg->msg_flags;

    if (check_args != RPCS_MSGHDR_CHECK_ARGS_NONE)
    {
        if (msg->msg_name != NULL)
        {
            te_string_reset(&name_str);
            rc = te_string_append(&name_str, "%s.msg_name",
                                   name_base_str.ptr);
             if (rc != 0)
                 return rc;

             tarpc_init_checked_arg(
               arglist, (uint8_t *)helper->addr,
               helper->addr_rlen,
               (check_args == RPCS_MSGHDR_CHECK_ARGS_RECV ?
                                              msg->msg_namelen : 0),
               name_str.ptr);
         }

        if (msg->msg_iov != NULL)
        {
            te_string_reset(&name_str);
            rc = te_string_append(&name_str, "%s.msg_iov",
                                  name_base_str.ptr);
            if (rc != 0)
                return rc;

            tarpc_init_checked_arg(
                arglist, (uint8_t *)msg->msg_iov,
                sizeof(*(msg->msg_iov)) * tarpc_msg->msg_iov.msg_iov_len,
                0, name_str.ptr);
        }

        if (msg->msg_control != NULL)
        {
            te_string_reset(&name_str);
            rc = te_string_append(&name_str, "%s.msg_control",
                                  name_base_str.ptr);
            if (rc != 0)
                return rc;

            tarpc_init_checked_arg(
              arglist, (uint8_t *)msg->msg_control,
              helper->real_controllen,
              (check_args == RPCS_MSGHDR_CHECK_ARGS_RECV ?
                                      msg->msg_controllen : 0),
              name_str.ptr);
        }

        if (check_args == RPCS_MSGHDR_CHECK_ARGS_SEND)
        {
            tarpc_init_checked_arg(
                        arglist, (uint8_t *)msg, sizeof(*msg), 0,
                        name_base_str.ptr);

        }
     }

    return 0;
}

/* See description in rpcs_msghdr.h */
te_errno
rpcs_msghdr_h2tarpc(const struct msghdr *msg,
                    const rpcs_msghdr_helper *helper,
                    struct tarpc_msghdr *tarpc_msg)
{
    int                   rc;
    struct tarpc_cmsghdr *rpc_c = NULL;
    unsigned int          rpc_len = 0;
    uint8_t              *tail = NULL;
    unsigned int          tail_len = 0;
    unsigned int          i;

    tarpc_msg->msg_flags = send_recv_flags_h2rpc(msg->msg_flags);
    tarpc_msg->in_msg_flags = send_recv_flags_h2rpc(helper->orig_msg_flags);

    sockaddr_output_h2rpc(msg->msg_name, helper->addr_len,
                          msg->msg_namelen, &tarpc_msg->msg_name);
    tarpc_msg->msg_namelen = msg->msg_namelen;

    if (tarpc_msg->msg_iov.msg_iov_val != NULL)
    {
        for (i = 0; i < tarpc_msg->msg_iov.msg_iov_len; i++)
        {
            tarpc_msg->msg_iov.msg_iov_val[i].iov_len =
                                              msg->msg_iov[i].iov_len;
        }
    }

    if (helper->orig_controllen != msg->msg_controllen ||
        memcmp(helper->orig_control, msg->msg_control,
               msg->msg_controllen) != 0)
    {
        rc = msg_control_h2rpc(msg->msg_control,
                               msg->msg_controllen,
                               &rpc_c, &rpc_len,
                               &tail, &tail_len);
        if (rc != 0)
            return rc;

        free(tarpc_msg->msg_control.msg_control_val);
        tarpc_msg->msg_control.msg_control_val = rpc_c;
        tarpc_msg->msg_control.msg_control_len = rpc_len;
        free(tarpc_msg->msg_control_tail.msg_control_tail_val);
        tarpc_msg->msg_control_tail.msg_control_tail_val = tail;
        tarpc_msg->msg_control_tail.msg_control_tail_len = tail_len;

        tarpc_msg->msg_controllen = msg->msg_controllen;
    }

    return 0;
}

/* See description in rpcs_msghdr.h */
void
rpcs_msghdr_helper_clean(rpcs_msghdr_helper *h, struct msghdr *msg)
{
    if (h != NULL)
    {
        free(h->addr_data);
        free(h->orig_control);
    }

    if (msg != NULL)
    {
        free(msg->msg_iov);
        free(msg->msg_control);
    }
}

/* See description in rpcs_msghdr.h */
te_errno
rpcs_mmsghdrs_tarpc2h(rpcs_msghdr_check_args_mode check_args,
                      const tarpc_mmsghdr *tarpc_mmsgs,
                      unsigned int num,
                      rpcs_msghdr_helper **helpers,
                      struct mmsghdr **mmsgs,
                      checked_arg_list *arglist)
{
    unsigned int i;
    te_errno     rc = 0;

    char        name_buf[MAX_ARG_NAME_LEN];
    te_string   name_str = TE_STRING_BUF_INIT(name_buf);

    rpcs_msghdr_helper *helpers_aux = NULL;
    struct mmsghdr     *mmsgs_aux = NULL;

    if (mmsgs == NULL || helpers == NULL)
    {
        ERROR("%s(): mmsgs and helpers arguments must not be NULL",
              __FUNCTION__);
        return TE_EINVAL;
    }

    mmsgs_aux = TE_ALLOC(sizeof(struct mmsghdr) * num);
    helpers_aux = TE_ALLOC(sizeof(rpcs_msghdr_helper) * num);
    if (mmsgs_aux == NULL || helpers_aux == NULL)
    {
        ERROR("%s(): out of memory", __FUNCTION__);
        rc = TE_ENOMEM;
        goto finish;
    }

    for (i = 0; i < num; i++)
    {
        te_string_reset(&name_str);
        rc = te_string_append(&name_str, "mmsgs[%u]", i);
        if (rc != 0)
            goto finish;

        mmsgs_aux[i].msg_len = tarpc_mmsgs[i].msg_len;
        rc = rpcs_msghdr_tarpc2h(check_args, &tarpc_mmsgs[i].msg_hdr,
                                 &helpers_aux[i], &mmsgs_aux[i].msg_hdr,
                                 arglist, "%s", name_str.ptr);
        if (rc != 0)
            goto finish;
    }

    *helpers = helpers_aux;
    *mmsgs = mmsgs_aux;

finish:

    if (rc != 0)
        rpcs_mmsghdrs_helpers_clean(helpers_aux, mmsgs_aux, num);

    return rc;
}

/* See description in rpcs_msghdr.h */
te_errno
rpcs_mmsghdrs_h2tarpc(const struct mmsghdr *mmsgs,
                      const rpcs_msghdr_helper *helpers,
                      struct tarpc_mmsghdr *tarpc_mmsgs,
                      unsigned int num)
{
    unsigned int  i;
    te_errno      rc;

    if (num == 0)
        return 0;

    if (mmsgs == NULL || helpers == NULL || tarpc_mmsgs == NULL)
    {
        ERROR("%s(): some arguments are NULL", __FUNCTION__);
        return TE_EINVAL;
    }

    for (i = 0; i < num; i++)
    {
        tarpc_mmsgs[i].msg_len = mmsgs[i].msg_len;
        rc = rpcs_msghdr_h2tarpc(&mmsgs[i].msg_hdr, &helpers[i],
                                 &tarpc_mmsgs[i].msg_hdr);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in rpcs_msghdr.h */
void
rpcs_mmsghdrs_helpers_clean(rpcs_msghdr_helper *helpers,
                            struct mmsghdr *mmsgs,
                            unsigned int num)
{
    unsigned int i;

    if (helpers != NULL && mmsgs != NULL)
    {
        for (i = 0; i < num; i++)
        {
            rpcs_msghdr_helper_clean(&helpers[i], &mmsgs[i].msg_hdr);
        }
    }

    free(helpers);
    free(mmsgs);
}
