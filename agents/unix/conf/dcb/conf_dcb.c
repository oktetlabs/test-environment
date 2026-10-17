/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA DCB configuration support
 *
 * The DCB node, the DCB netlink requests shared by its subtrees, and
 * the registration of the PFC, ETS and DCBX subtrees.
 */

#define TE_LGR_USER     "Unix Conf DCB"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_alloc.h"
#include "te_str.h"

#include "rcf_common.h"
#include "rcf_pch_tree.h"
#include "rcf_pch_ta_cfg.h"

#include "logger_api.h"

#include "conf_dcb.h"
#include "conf_dcbx.h"
#include "conf_ets.h"
#include "conf_pfc.h"

#include <errno.h>
#include <linux/dcbnl.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>

/** Type of TA objects keeping DCB data read in a configuration group. */
#define TA_OBJ_TYPE_IF_DCB_READ "if_dcb_read"

/**
 * How long to wait for a DCB netlink response, in seconds. The kernel
 * handles a request while it is sent, so the response is normally there
 * at once; the timeout only keeps a lost response from blocking the TA.
 */
#define DCB_NETLINK_RECV_TIMEOUT_SEC 10

/** Netlink socket shared by all DCB requests, opened on first use. */
static struct nl_sock *netlink_socket = NULL;

/** DCB data read from the driver in a configuration group. */
typedef struct dcb_read {
    /** Status of the read. */
    te_errno rc;
    /** Data read, valid if @a rc is @c 0. */
    union {
        struct ieee_pfc pfc;
        struct ieee_ets ets;
        uint8_t dcbx;
    } data;
} dcb_read;

/** Context used while processing a DCB netlink response. */
typedef struct dcb_get_ctx {
    /** Type of DCB data expected in the response. */
    ta_unix_conf_dcb_type type;
    /** Data for the response. */
    union {
        /** PFC configuration for @c TA_UNIX_CONF_DCB_TYPE_PFC. */
        struct ieee_pfc *pfc;
        /** ETS configuration for @c TA_UNIX_CONF_DCB_TYPE_ETS. */
        struct ieee_ets *ets;
        /** DCBX operating mode for @c TA_UNIX_CONF_DCB_TYPE_DCBX. */
        uint8_t *dcbx;
        /** Type-independent representation of the data pointer. */
        void *data;
    } data;
    /** Result of processing the netlink response. */
    te_errno rc;
} dcb_get_ctx;

/** Context used while processing a DCB SET netlink response. */
typedef struct dcb_set_ctx {
    /** Type of DCB data being configured. */
    ta_unix_conf_dcb_type type;
    /** Status reported by the driver in the response. */
    te_errno rc;
} dcb_set_ctx;

/** State of a DCB netlink request/response exchange. */
typedef struct dcb_nl_req {
    /** The final ACK or error message has been received. */
    bool done;
    /** Positive errno reported by the kernel, @c 0 if none. */
    int kernel_errno;
} dcb_nl_req;

/**
 * Convert a libnl error to a TE error.
 *
 * @param[in] nl_error  libnl error.
 *
 * @return TE error code.
 * @retval TE_EUNKNOWN  The error has no TE equivalent.
 */
static te_errno
dcb_internal_nl_error2te_errno(int nl_error)
{
    te_errno rc;

    /* libnl functions return negative errors. */
    if (nl_error < 0)
        nl_error = -nl_error;

    switch (nl_error)
    {
        case NLE_SUCCESS:
            rc = 0;
            break;

        case NLE_EXIST:
            rc = TE_RC(TE_TA_UNIX, TE_EEXIST);
            break;

        case NLE_NOMEM:
            rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
            break;

        case NLE_INVAL:
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            break;

        case NLE_OPNOTSUPP:
            rc = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
            break;

        case NLE_AGAIN:
            rc = TE_RC(TE_TA_UNIX, TE_ETIMEDOUT);
            break;

        default:
            WARN("Cannot convert libnl error to TE error: %s",
                 nl_geterror(nl_error));
            rc = TE_RC(TE_TA_UNIX, TE_EUNKNOWN);
            break;
    }

    return rc;
}

/**
 * Open the DCB netlink socket.
 *
 * @return Status code.
 */
static te_errno
dcb_netlink_socket_init(void)
{
    const struct timeval timeout = {
        .tv_sec = DCB_NETLINK_RECV_TIMEOUT_SEC,
    };
    te_errno rc;

    netlink_socket = nl_socket_alloc();
    if (netlink_socket == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    rc = dcb_internal_nl_error2te_errno(nl_connect(netlink_socket,
                                                   NETLINK_ROUTE));
    if (rc == 0 &&
        setsockopt(nl_socket_get_fd(netlink_socket), SOL_SOCKET,
                   SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Failed to set DCB netlink receive timeout: %r", rc);
    }

    if (rc != 0)
    {
        nl_socket_free(netlink_socket);
        netlink_socket = NULL;
    }

    return rc;
}

/** Close the DCB netlink socket if it is open. */
static void
dcb_netlink_socket_fini(void)
{
    if (netlink_socket == NULL)
        return;

    nl_close(netlink_socket);
    nl_socket_free(netlink_socket);
    netlink_socket = NULL;
}

/**
 * Reopen the netlink socket after a failed request/response exchange.
 *
 * The socket is shared by all requests and libnl drops the rest of
 * a response on errors, so an unprocessed response or ACK may remain
 * in the socket. Every following request would then read it, fail on
 * sequence number check and leave its own response in the socket.
 */
static void
dcb_netlink_socket_reset(void)
{
    WARN("Reopening DCB netlink socket to drop unprocessed messages");

    /* The next request opens a new socket. */
    dcb_netlink_socket_fini();
}

/**
 * Get the DCB netlink socket.
 *
 * The socket is opened on first use, so that TA initialization does not
 * depend on it, and again after it is closed on a failure.
 *
 * @param[out] sock     Location for the socket.
 *
 * @return Status code.
 */
static te_errno
dcb_internal_get_sock(struct nl_sock **sock)
{
    te_errno rc;

    if (netlink_socket == NULL)
    {
        rc = dcb_netlink_socket_init();
        if (rc != 0)
        {
            ERROR("Failed to open DCB netlink socket: %r", rc);
            return rc;
        }
    }

    *sock = netlink_socket;
    return 0;
}

/**
 * Get the name of a DCB data type.
 *
 * @param[in] type  DCB data type.
 *
 * @return Name of the type.
 */
static const char *
dcb_type2str(ta_unix_conf_dcb_type type)
{
    return te_enum_map_from_value(ta_unix_conf_dcb_type_map, type);
}

/**
 * Handle the netlink error message that ends a DCB request.
 *
 * @param[in] nla       Address of the sender.
 * @param[in] nlerr     Error message.
 * @param[in] arg       State of the request (dcb_nl_req).
 *
 * @return @c NL_STOP.
 */
static int
dcb_nl_err_cb(struct sockaddr_nl *nla, struct nlmsgerr *nlerr, void *arg)
{
    dcb_nl_req *req = arg;

    UNUSED(nla);

    req->kernel_errno = -nlerr->error;
    req->done = true;

    return NL_STOP;
}

/**
 * Handle the netlink ACK message that ends a DCB request.
 *
 * @param[in] msg       ACK message.
 * @param[in] arg       State of the request (dcb_nl_req).
 *
 * @return @c NL_STOP.
 */
static int
dcb_nl_ack_cb(struct nl_msg *msg, void *arg)
{
    dcb_nl_req *req = arg;

    UNUSED(msg);

    req->done = true;

    return NL_STOP;
}

/**
 * Send a DCB netlink request and process the response up to the final
 * ACK or error message.
 *
 * Errors reported by the kernel (including the ones returned by the
 * driver callbacks) are converted from errno as is and are logged at
 * VERB level only: it is up to the caller to decide whether they are
 * expected (e.g. EOPNOTSUPP is reported for every interface without DCB
 * on each Configurator synchronization).
 *
 * @param msg       Request to send.
 * @param valid_cb  Callback for valid (non-ACK) response messages.
 * @param arg       Argument for @p valid_cb.
 *
 * @return Status code.
 */
static te_errno
dcb_nl_request(struct nl_msg *msg, nl_recvmsg_msg_cb_t valid_cb, void *arg)
{
    dcb_nl_req req = { .done = false, .kernel_errno = 0 };
    struct nl_sock *sock;
    struct nl_cb *cb;
    te_errno rc;
    int nl_rc;

    rc = dcb_internal_get_sock(&sock);
    if (rc != 0)
        return rc;

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (cb == NULL)
    {
        ERROR("Failed to alloc callback handler");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    nl_rc = nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, dcb_nl_ack_cb, &req);
    if (nl_rc >= 0)
        nl_rc = nl_cb_err(cb, NL_CB_CUSTOM, dcb_nl_err_cb, &req);
    if (nl_rc >= 0 && valid_cb != NULL)
        nl_rc = nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_cb, arg);
    if (nl_rc < 0)
    {
        ERROR("Failed to register callback handler: %s", nl_geterror(nl_rc));
        rc = dcb_internal_nl_error2te_errno(nl_rc);
        goto out;
    }

    nl_rc = nl_send_auto(sock, msg);
    if (nl_rc < 0)
    {
        ERROR("Failed to send DCB request: %s", nl_geterror(nl_rc));
        rc = dcb_internal_nl_error2te_errno(nl_rc);
        /*
         * libnl has taken a sequence number for the request but will not
         * see its response, so it would expect the wrong number in the
         * response to the next request.
         */
        dcb_netlink_socket_reset();
        goto out;
    }

    while (!req.done)
    {
        nl_rc = nl_recvmsgs(sock, cb);
        if (req.kernel_errno != 0)
            break;

        if (nl_rc < 0)
        {
            ERROR("Failed to receive DCB response: %s", nl_geterror(nl_rc));
            rc = dcb_internal_nl_error2te_errno(nl_rc);
            dcb_netlink_socket_reset();
            goto out;
        }
    }

    if (req.kernel_errno != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, req.kernel_errno);
        VERB("DCB request failed in kernel: errno %d, %r",
             req.kernel_errno, rc);
    }

out:
    nl_cb_put(cb);

    return rc;
}

/**
 * Handle a response to a DCB IEEE GET request.
 *
 * @param[in] msg           Response message.
 * @param[in] user_data     Context of the request (@c dcb_get_ctx).
 *
 * @return @c NL_SKIP for a message of another type, @c NL_STOP otherwise.
 */
static int
dcb_get_ieee_valid_cb(struct nl_msg *msg, void *user_data)
{
    dcb_get_ctx *ctx = user_data;
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[DCB_ATTR_MAX + 1] = {};
    struct nlattr *ieee_attrs[DCB_ATTR_IEEE_MAX + 1] = {};
    struct nlattr *dcb_attr;
    size_t dcb_data_size;
    int dcb_attr_type;
    int nl_rc;

    /* Skip valid messages other than RTM_GETDCB. */
    if (nlh->nlmsg_type != RTM_GETDCB)
        return NL_SKIP;

    if (nlmsg_datalen(nlh) < (int)sizeof(struct dcbmsg))
    {
        ERROR("DCB response is shorter than its header");
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    nl_rc = nlmsg_parse(nlh, sizeof(struct dcbmsg), attrs,
                        DCB_ATTR_MAX, NULL);
    if (nl_rc < 0)
    {
        ERROR("Failed to parse DCB msg: %s", nl_geterror(nl_rc));
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    if (attrs[DCB_ATTR_IEEE] == NULL)
    {
        ERROR("DCB IEEE GET response has no IEEE container");
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    nl_rc = nla_parse_nested(ieee_attrs, DCB_ATTR_IEEE_MAX,
                             attrs[DCB_ATTR_IEEE], NULL);
    if (nl_rc < 0)
    {
        ERROR("Failed to parse IEEE container: %s", nl_geterror(nl_rc));
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    switch (ctx->type)
    {
        case TA_UNIX_CONF_DCB_TYPE_PFC:
            dcb_attr_type = DCB_ATTR_IEEE_PFC;
            dcb_data_size = sizeof(*ctx->data.pfc);
            break;

        case TA_UNIX_CONF_DCB_TYPE_ETS:
            dcb_attr_type = DCB_ATTR_IEEE_ETS;
            dcb_data_size = sizeof(*ctx->data.ets);
            break;

        default:
            ERROR("Unsupported DCB type: %d", ctx->type);
            ctx->rc = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
            return NL_STOP;
    }

    /*
     * The kernel omits the attribute both if the driver has no getter
     * and if the getter fails, so these cases cannot be distinguished.
     */
    dcb_attr = ieee_attrs[dcb_attr_type];
    if (dcb_attr == NULL)
    {
        VERB("No '%s' data in DCB IEEE GET response: driver getter is "
             "absent or failed", dcb_type2str(ctx->type));
        ctx->rc = TE_RC(TE_TA_UNIX, TE_ENODATA);
        return NL_STOP;
    }

    if ((size_t)nla_len(dcb_attr) < dcb_data_size)
    {
        ERROR("DCB attribute is shorter than its data");
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    memcpy(ctx->data.data, nla_data(dcb_attr), dcb_data_size);
    ctx->rc = 0;

    return NL_STOP;
}

/**
 * Handle a response to a DCB GDCBX request.
 *
 * @param[in] msg           Response message.
 * @param[in] user_data     Context of the request (@c dcb_get_ctx).
 *
 * @return @c NL_SKIP for a message of another type, @c NL_STOP otherwise.
 */
static int
dcb_get_dcbx_valid_cb(struct nl_msg *msg, void *user_data)
{
    dcb_get_ctx *ctx = user_data;
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[DCB_ATTR_MAX + 1] = {};
    struct nlattr *dcb_attr;
    size_t dcb_data_size;
    int nl_rc;

    /* Skip valid messages other than RTM_GETDCB. */
    if (nlh->nlmsg_type != RTM_GETDCB)
        return NL_SKIP;

    if (nlmsg_datalen(nlh) < (int)sizeof(struct dcbmsg))
    {
        ERROR("DCB response is shorter than its header");
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    nl_rc = nlmsg_parse(nlh, sizeof(struct dcbmsg), attrs,
                        DCB_ATTR_MAX, NULL);
    if (nl_rc < 0)
    {
        ERROR("Failed to parse DCB msg: %s", nl_geterror(nl_rc));
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    dcb_attr = attrs[DCB_ATTR_DCBX];
    dcb_data_size = sizeof(*ctx->data.dcbx);

    /* The kernel fails the request if the driver has no getdcbx(). */
    if (dcb_attr == NULL)
    {
        ERROR("DCB GDCBX response has no DCBX attribute");
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    if ((size_t)nla_len(dcb_attr) < dcb_data_size)
    {
        ERROR("DCB attribute is shorter than its data");
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    memcpy(ctx->data.data, nla_data(dcb_attr), dcb_data_size);
    ctx->rc = 0;

    return NL_STOP;
}

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_data_get(const char *if_name, ta_unix_conf_dcb_type type,
                          void *data)
{
    struct dcbmsg dcb = {
        .dcb_family = AF_UNSPEC,
    };
    struct nl_msg *msg = NULL;
    nl_recvmsg_msg_cb_t valid_cb;
    dcb_get_ctx ctx = {
        .type = type,
        .data.data = data,
        /* Callback will change this value if the response is received. */
        .rc = TE_RC(TE_TA_UNIX, TE_EPROTO),
    };
    te_errno rc;
    int nl_rc;

    if (if_name == NULL || data == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    switch (type)
    {
        case TA_UNIX_CONF_DCB_TYPE_PFC:
        case TA_UNIX_CONF_DCB_TYPE_ETS:
            dcb.cmd = DCB_CMD_IEEE_GET;
            valid_cb = dcb_get_ieee_valid_cb;
            break;

        case TA_UNIX_CONF_DCB_TYPE_DCBX:
            dcb.cmd = DCB_CMD_GDCBX;
            valid_cb = dcb_get_dcbx_valid_cb;
            break;

        default:
            ERROR("Unknown DCB type: %d", type);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    msg = nlmsg_alloc_simple(RTM_GETDCB, NLM_F_REQUEST | NLM_F_ACK);
    if (msg == NULL)
    {
        ERROR("Failed to allocate netlink msg");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    nl_rc = nlmsg_append(msg, &dcb, sizeof(dcb), NLMSG_ALIGNTO);
    if (nl_rc < 0)
    {
        ERROR("Failed to add DCB hdr to netlink msg: %s", nl_geterror(nl_rc));
        rc = dcb_internal_nl_error2te_errno(nl_rc);
        goto out;
    }

    nl_rc = nla_put_string(msg, DCB_ATTR_IFNAME, if_name);
    if (nl_rc < 0)
    {
        ERROR("Failed to add if_name to netlink msg: %s", nl_geterror(nl_rc));
        rc = dcb_internal_nl_error2te_errno(nl_rc);
        goto out;
    }

    rc = dcb_nl_request(msg, valid_cb, &ctx);
    if (rc == 0)
        rc = ctx.rc;

out:
    nlmsg_free(msg);

    return rc;
}

/**
 * Get the size of the data of a DCB type.
 *
 * @param[in] type  DCB data type.
 *
 * @return Size of the data, or @c 0 if the type is unknown.
 */
static size_t
dcb_type_data_size(ta_unix_conf_dcb_type type)
{
    switch (type)
    {
        case TA_UNIX_CONF_DCB_TYPE_PFC:
            return sizeof(struct ieee_pfc);

        case TA_UNIX_CONF_DCB_TYPE_ETS:
            return sizeof(struct ieee_ets);

        case TA_UNIX_CONF_DCB_TYPE_DCBX:
            return sizeof(uint8_t);
    }

    return 0;
}

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_data_get_group(unsigned int gid, const char *if_name,
                                ta_unix_conf_dcb_type type, void *data)
{
    te_string name = TE_STRING_INIT;
    ta_cfg_obj_t *obj;
    dcb_read *entry;
    size_t size;
    te_errno rc;

    if (if_name == NULL || data == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    size = dcb_type_data_size(type);
    if (size == 0)
    {
        ERROR("Unknown DCB type: %d", type);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    te_string_append(&name, "%s:%s", if_name, dcb_type2str(type));

    obj = ta_obj_find(TA_OBJ_TYPE_IF_DCB_READ, te_string_value(&name), gid);
    if (obj == NULL)
    {
        entry = TE_ALLOC(sizeof(*entry));
        entry->rc = ta_unix_conf_dcb_data_get(if_name, type, &entry->data);

        rc = ta_obj_add(TA_OBJ_TYPE_IF_DCB_READ, te_string_value(&name), "",
                        gid, entry, &free, &obj);
        if (rc != 0)
        {
            free(entry);
            te_string_free(&name);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    te_string_free(&name);

    entry = obj->user_data;
    if (entry->rc == 0)
        memcpy(data, &entry->data, size);

    return entry->rc;
}

/**
 * Handle a response to a DCB SET request.
 *
 * The kernel acknowledges DCB SET requests with zero even if the driver
 * rejects the configuration: the driver status is passed in the response
 * as a u8 attribute instead.
 *
 * - @c DCB_ATTR_IEEE (@c DCB_CMD_IEEE_SET): negative errno returned by
 *   ieee_setets()/ieee_setpfc() truncated to u8, so it is decoded as int8
 *   (as iproute2 does). Errno values above 127 cannot be decoded
 *   correctly (e.g. kernel-internal @c ENOTSUPP (524) is decoded as
 *   @c ENOMEM), so the raw status is logged as well.
 * - @c DCB_ATTR_DCBX (@c DCB_CMD_SDCBX): u8 value returned by setdcbx(),
 *   which is non-zero on failure. In-tree drivers return 1 (ice also
 *   returns 1 if the mode is not changed), which is reported as
 *   @c TE_EFAIL. A driver returning a negative errno from this u8
 *   function makes it truncated, so negative int8 values are decoded as
 *   errno.
 *
 * @param[in] msg           Response message.
 * @param[in] user_data     Context of the request (dcb_set_ctx).
 *
 * @return @c NL_SKIP for a message of another type, @c NL_STOP otherwise.
 */
static int
dcb_set_valid_cb(struct nl_msg *msg, void *user_data)
{
    dcb_set_ctx *ctx = user_data;
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[DCB_ATTR_MAX + 1] = {};
    struct nlattr *status_attr;
    int8_t status;
    int nl_rc;

    /* Skip valid messages other than RTM_SETDCB. */
    if (nlh->nlmsg_type != RTM_SETDCB)
        return NL_SKIP;

    nl_rc = nlmsg_parse(nlh, sizeof(struct dcbmsg), attrs,
                        DCB_ATTR_MAX, NULL);
    if (nl_rc < 0)
    {
        ERROR("Failed to parse DCB SET response: %s", nl_geterror(nl_rc));
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    status_attr = attrs[ctx->type == TA_UNIX_CONF_DCB_TYPE_DCBX ?
                        DCB_ATTR_DCBX : DCB_ATTR_IEEE];
    if (status_attr == NULL || nla_len(status_attr) != sizeof(uint8_t))
    {
        ERROR("DCB SET response for '%s' has no status",
              dcb_type2str(ctx->type));
        ctx->rc = TE_RC(TE_TA_UNIX, TE_EPROTO);
        return NL_STOP;
    }

    status = (int8_t)nla_get_u8(status_attr);
    if (status == 0)
    {
        ctx->rc = 0;
    }
    else
    {
        if (status < 0)
            ctx->rc = TE_OS_RC(TE_TA_UNIX, -status);
        else
            ctx->rc = TE_RC(TE_TA_UNIX, TE_EFAIL);

        RING("Driver rejected '%s' configuration: status %d (raw 0x%x), %r",
             dcb_type2str(ctx->type), status, (uint8_t)status, ctx->rc);
    }

    return NL_STOP;
}

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_data_set(const char *if_name, ta_unix_conf_dcb_type type,
                          const void *data)
{
    struct dcbmsg dcb = {
        .dcb_family = AF_UNSPEC,
    };
    struct ieee_pfc pfc_request = { };
    struct ieee_ets ets_request = { };
    uint8_t dcbx_request = 0;
    void *request;
    struct nl_msg *msg = NULL;
    struct nlattr *ieee_attr;
    size_t dcb_data_size;
    int dcb_attr_type;
    dcb_set_ctx ctx = {
        .type = type,
        /* Callback will change this value if the response is received. */
        .rc = TE_RC(TE_TA_UNIX, TE_EPROTO),
    };
    te_errno rc = 0;
    int nl_rc;

    if (if_name == NULL || data == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    switch (type)
    {
        case TA_UNIX_CONF_DCB_TYPE_PFC:
            memcpy(&pfc_request, data, sizeof(pfc_request));
            request = &pfc_request;
            dcb.cmd = DCB_CMD_IEEE_SET;
            dcb_attr_type = DCB_ATTR_IEEE_PFC;
            dcb_data_size = sizeof(pfc_request);
            break;

        case TA_UNIX_CONF_DCB_TYPE_ETS:
            memcpy(&ets_request, data, sizeof(ets_request));
            request = &ets_request;
            dcb.cmd = DCB_CMD_IEEE_SET;
            dcb_attr_type = DCB_ATTR_IEEE_ETS;
            dcb_data_size = sizeof(ets_request);
            break;

        case TA_UNIX_CONF_DCB_TYPE_DCBX:
            memcpy(&dcbx_request, data, sizeof(dcbx_request));
            request = &dcbx_request;
            dcb.cmd = DCB_CMD_SDCBX;
            dcb_attr_type = DCB_ATTR_DCBX;
            dcb_data_size = sizeof(dcbx_request);
            break;

        default:
            ERROR("Unknown DCB type: %d", type);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    msg = nlmsg_alloc_simple(RTM_SETDCB, NLM_F_REQUEST | NLM_F_ACK);
    if (msg == NULL)
    {
        ERROR("Failed to allocate netlink msg");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    nl_rc = nlmsg_append(msg, &dcb, sizeof(dcb), NLMSG_ALIGNTO);
    if (nl_rc < 0)
    {
        ERROR("Failed to append header to netlink msg: %s", nl_geterror(nl_rc));
        rc = dcb_internal_nl_error2te_errno(nl_rc);
        goto out;
    }

    nl_rc = nla_put_string(msg, DCB_ATTR_IFNAME, if_name);
    if (nl_rc < 0)
    {
        ERROR("Failed to add attribute to netlink msg: %s", nl_geterror(nl_rc));
        rc = dcb_internal_nl_error2te_errno(nl_rc);
        goto out;
    }

    if (type == TA_UNIX_CONF_DCB_TYPE_DCBX)
    {
        nl_rc = nla_put(msg, DCB_ATTR_DCBX, dcb_data_size, request);
        if (nl_rc < 0)
        {
            ERROR("Failed to add DCB_ATTR_DCBX to netlink msg: %s",
                  nl_geterror(nl_rc));
            rc = dcb_internal_nl_error2te_errno(nl_rc);
            goto out;
        }
    }
    else
    {
        ieee_attr = nla_nest_start(msg, DCB_ATTR_IEEE);
        if (ieee_attr == NULL)
        {
            ERROR("Failed to add nested DCB_ATTR_IEEE attr to netlink msg");
            rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
            goto out;
        }

        nl_rc = nla_put(msg, dcb_attr_type, dcb_data_size, request);
        if (nl_rc < 0)
        {
            ERROR("Failed to add IEEE attr to netlink msg: %s",
                  nl_geterror(nl_rc));
            rc = dcb_internal_nl_error2te_errno(nl_rc);
            goto out;
        }

        nla_nest_end(msg, ieee_attr);
    }

    rc = dcb_nl_request(msg, dcb_set_valid_cb, &ctx);
    if (rc == 0)
        rc = ctx.rc;

out:
    nlmsg_free(msg);

    return rc;
}

/**
 * Get the list of instances of a node backed by DCB data.
 *
 * The node is not listed if the data cannot be retrieved because DCB is
 * not supported by the interface, the interface is gone or (unless
 * @p nodata_ok is @c true) the driver provides no data. On other errors
 * the node is listed, so that getting its value reports the error.
 *
 * @param[in] ctx           Context of the list request.
 * @param[in] type          Type of DCB data backing the node.
 * @param[in] nodata_ok     List the node even if the driver provides
 *                          no data of @p type.
 * @param[out] names        Vector to append the instance names to.
 *
 * @return Status code.
 */
static te_errno
dcb_list_by_data(const ta_conf_ctx *ctx, ta_unix_conf_dcb_type type,
                 bool nodata_ok, te_vec *names)
{
    const char *if_name = ta_conf_ctx_inst(ctx, "interface");
    union {
        struct ieee_pfc pfc;
        struct ieee_ets ets;
        uint8_t dcbx;
    } data;
    char *name;
    te_errno rc;

    rc = ta_unix_conf_dcb_data_get_group(ta_conf_ctx_gid(ctx), if_name,
                                         type, &data);
    if (rc != 0 && !(nodata_ok && TE_RC_GET_ERROR(rc) == TE_ENODATA))
    {
        switch (TE_RC_GET_ERROR(rc))
        {
            case TE_ENODATA:
            case TE_EOPNOTSUPP:
            case TE_ENODEV:
                return 0;

            default:
                WARN("Failed to get '%s' data of interface '%s': %r",
                     dcb_type2str(type), if_name, rc);
                break;
        }
    }

    /* The node has a single instance with an empty name. */
    name = TE_STRDUP("");
    TE_VEC_APPEND(names, name);

    return 0;
}

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_list(const ta_conf_ctx *ctx, ta_unix_conf_dcb_type type,
                      te_vec *names)
{
    return dcb_list_by_data(ctx, type, false, names);
}

/**
 * List the DCB node: it exists if the interface supports DCB netlink
 * requests, i.e. the driver provides DCB operations.
 *
 * @param[in] ctx       Context of the list request.
 * @param[out] names    Vector to append the instance names to.
 *
 * @return Status code.
 */
static te_errno
dcb_list(const ta_conf_ctx *ctx, te_vec *names)
{
    /*
     * IEEE GET succeeds if the driver has DCB operations, even if it
     * does not provide ETS data.
     */
    return dcb_list_by_data(ctx, TA_UNIX_CONF_DCB_TYPE_ETS, true, names);
}

static const ta_conf_node *const node_dcb =
    TA_CONF_LIST("dcb", dcb_list);

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_index_parse(const char *instance, unsigned int *index)
{
    unsigned long parsed;
    te_errno rc;

    if (instance == NULL || index == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = te_strtoul(instance, 10, &parsed);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (parsed >= TA_UNIX_CONF_DCB_MAX_TCS)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *index = parsed;
    return 0;
}

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_index_list(const ta_conf_ctx *ctx, te_vec *names)
{
    unsigned int i;

    UNUSED(ctx);

    for (i = 0; i < TA_UNIX_CONF_DCB_MAX_TCS; i++)
    {
        char *name = te_string_fmt("%u", i);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/* See the description in conf_dcb.h. */
te_errno
ta_unix_conf_dcb_init(void)
{
    te_errno rc;

    rc = ta_conf_register("/agent/interface", node_dcb);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, rc);

    rc = ta_unix_conf_pfc_init();
    if (rc != 0)
    {
        ERROR("Failed to configure PFC configuration subtree: %r", rc);
        return rc;
    }

    rc = ta_unix_conf_ets_init();
    if (rc != 0)
    {
        ERROR("Failed to configure ETS configuration subtree: %r", rc);
        return rc;
    }

    rc = ta_unix_conf_dcbx_init();
    if (rc != 0)
    {
        ERROR("Failed to configure DCBX configuration subtree: %r", rc);
        return rc;
    }

    return 0;
}

/* See the description in conf_dcb.h. */
void
ta_unix_conf_dcb_fini(void)
{
    dcb_netlink_socket_fini();
}
