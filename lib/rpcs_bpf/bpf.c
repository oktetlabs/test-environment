/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023-2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPCs for BPF
 *
 * Implementation of RPCs for BPF
 */

#define TE_LGR_USER "RPC BPF"

#include "te_config.h"
#include "config.h"

#include "logger_api.h"
#include "rpc_server.h"
#include "tarpc.h"
#include "te_errno.h"
#include "te_rpc_bpf.h"
#include "te_alloc.h"
#include "te_queue.h"
#include "te_str.h"

#if HAVE_BPF_BPF_H

#include <bpf/bpf.h>

#if HAVE_AF_XDP

#if HAVE_XDP_XSK_H
#include <xdp/xsk.h>
#else
#include <bpf/xsk.h>
#endif

/**
 * UMEM FILL and COMPLETION rings.
 * A separate pair of these rings should exist for every interface/queue_id
 * pair if UMEM is shared between multiple AF_XDP sockets.
 */
typedef struct ta_xsk_umem_rings {
    /** Queue links */
    TAILQ_ENTRY(ta_xsk_umem_rings) links;

    /** Interface name */
    char if_name[IF_NAMESIZE];
    /** Rx queue id */
    uint32_t queue_id;
    /** FILL ring */
    struct xsk_ring_prod fill;
    /** COMPLETION ring */
    struct xsk_ring_cons comp;
    /** Reference count */
    unsigned int refcount;
} ta_xsk_umem_rings;

/** Head of the queue of UMEM rings */
typedef TAILQ_HEAD(ta_xsk_umem_rings_head, ta_xsk_umem_rings) ta_xsk_umem_rings_head;

/** UMEM data */
typedef struct ta_xsk_umem {
    /** Pointer to UMEM structure in libxdp */
    struct xsk_umem *umem;
    /** Queue of UMEM rings */
    ta_xsk_umem_rings_head rings;
    /** UMEM configuration */
    struct xsk_umem_config config;

    /** Memory allocated for UMEM */
    void *buf;
    /** Size of the memory region */
    uint64_t size;

    /** Number of frames in UMEM */
    uint64_t frames_count;
    /** Stack of currently not used frames */
    uint64_t *frames_stack;
    /** Number of frames in the stack */
    uint64_t stack_count;
} ta_xsk_umem;

/** AF_XDP socket data */
typedef struct ta_xsk_socket {
    /** Pointer to socket structure in libxdp */
    struct xsk_socket *xsk;
    /** Socket file descriptor */
    int fd;
    /** Pointer to UMEM used by the socket */
    ta_xsk_umem *umem;
    /** Pointer to UMEM rings pair */
    ta_xsk_umem_rings *umem_rings;
    /** Rx ring of the socket */
    struct xsk_ring_cons rx;
    /** Tx ring of the socket */
    struct xsk_ring_prod tx;
    /** Socket configuration */
    struct xsk_socket_config config;
} ta_xsk_socket;

/*
 * Find UMEM rings for a socket (adding new rings
 * if they do not exist).
 */
static ta_xsk_umem_rings *
add_or_find_umem_rings(ta_xsk_umem *umem, const char *if_name,
                       unsigned int queue_id)
{
    ta_xsk_umem_rings *umem_rings = NULL;

    TAILQ_FOREACH(umem_rings, &umem->rings, links)
    {
        /*
         * If interface name is empty, this is the first pair
         * of rings created together with UMEM, it should be
         * used for the first AF_XDP socket.
         */
        if (umem_rings->if_name[0] == '\0' ||
            (strcmp(umem_rings->if_name, if_name) == 0 &&
             umem_rings->queue_id == queue_id))
        {
            umem_rings->refcount++;
            return umem_rings;
        }
    }

    umem_rings = TE_ALLOC(sizeof(*umem_rings));

    TE_STRLCPY(umem_rings->if_name, if_name, IF_NAMESIZE);
    umem_rings->queue_id = queue_id;
    TAILQ_INSERT_HEAD(&umem->rings, umem_rings, links);
    umem_rings->refcount = 1;
    return umem_rings;
}

/*
 * Decrement reference count for UMEM rings; release memory if it reaches
 * zero.
 */
static void
free_umem_rings(ta_xsk_umem *umem, ta_xsk_umem_rings *rings)
{
    if (rings == NULL)
        return;

    rings->refcount--;
    if (rings->refcount == 0)
    {
        TAILQ_REMOVE(&umem->rings, rings, links);
        free(rings);
    }
}

/* Call xsk_umem__create() */
static int
ta_xsk_umem__create(tarpc_xsk_umem__create_in *in,
                    tarpc_xsk_umem__create_out *out)
{
    static rpc_ptr_id_namespace ns_umem = RPC_PTR_ID_NS_INVALID;
    ta_xsk_umem *umem = NULL;
    rpc_ptr umem_ptr = RPC_NULL;
    const struct xsk_umem_config *cfg_ptr = NULL;
    ta_xsk_umem_rings *umem_rings = NULL;
    uint64_t i;
    int rc = 0;

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_umem, RPC_TYPE_NS_XSK_UMEM,
                                           -1);

    umem = TE_ALLOC(sizeof(*umem));

    umem->buf = rcf_pch_mem_get(in->umem_area);
    if (umem->buf == NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                         "invalid buffer is specified for UMEM");
        rc = -1;
        goto finish;
    }

    umem->size = in->size;

    umem_ptr = RCF_PCH_MEM_INDEX_ALLOC(umem, ns_umem);
    if (umem_ptr == RPC_NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOMEM),
                         "Failed to register pointer to ta_xsk_umem");
        rc = -1;
        goto finish;
    }

    TAILQ_INIT(&umem->rings);

    umem_rings = add_or_find_umem_rings(umem, "", 0);
    if (umem_rings == NULL)
    {
        rc = -1;
        goto finish;
    }

    if (in->config.config_len > 0)
    {
        tarpc_xsk_umem_config *conf = in->config.config_val;

        umem->config.fill_size = conf->fill_size;
        umem->config.comp_size = conf->comp_size;
        umem->config.frame_size = conf->frame_size;
        umem->config.frame_headroom = conf->frame_headroom;
        umem->config.flags = conf->flags;

        cfg_ptr = &umem->config;
    }
    else
    {
        cfg_ptr = NULL;

        umem->config.fill_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
        umem->config.comp_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
        umem->config.frame_size = XSK_UMEM__DEFAULT_FRAME_SIZE;
        umem->config.frame_headroom = XSK_UMEM__DEFAULT_FRAME_HEADROOM;
        umem->config.flags = XSK_UMEM__DEFAULT_FLAGS;
    }

    if (in->size < umem->config.frame_size)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOBUFS),
                         "Too little space for UMEM prodived");
        rc = -1;
        goto finish;
    }

    umem->frames_count = in->size / umem->config.frame_size;
    umem->frames_stack = TE_ALLOC(umem->frames_count * sizeof(uint64_t));

    umem->stack_count = umem->frames_count;

    for (i = 0; i < umem->frames_count; i++)
    {
        umem->frames_stack[i] =
              (umem->frames_count - i - 1) * umem->config.frame_size;
    }

    rc = xsk_umem__create(&umem->umem, umem->buf, umem->size,
                          &umem_rings->fill, &umem_rings->comp, cfg_ptr);
    if (rc < 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, -rc),
                         "xsk_umem__create() failed");
        rc = -1;
        goto finish;
    }

    out->umem_ptr = umem_ptr;

finish:

    if (rc < 0)
    {
        RCF_PCH_MEM_INDEX_FREE(umem_ptr, ns_umem);
        if (umem != NULL)
            free(umem->frames_stack);
        free(umem);
        free(umem_rings);
    }

    return rc;
}

TARPC_FUNC_STANDALONE(xsk_umem__create, {},
{
    MAKE_CALL(out->retval = ta_xsk_umem__create(in, out));
})

/* Call xsk_umem__delete() */
static int
ta_xsk_umem__delete(tarpc_xsk_umem__delete_in *in,
                    tarpc_xsk_umem__delete_out *out)
{
    static rpc_ptr_id_namespace ns_umem = RPC_PTR_ID_NS_INVALID;
    ta_xsk_umem *umem = NULL;
    ta_xsk_umem_rings *umem_rings = NULL;
    int rc;

    UNUSED(out);

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_umem, RPC_TYPE_NS_XSK_UMEM,
                                           -1);

    RCF_PCH_MEM_INDEX_TO_PTR_RPC(umem, in->umem_ptr, ns_umem, -1);

    rc = xsk_umem__delete(umem->umem);
    if (rc < 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, -rc),
                         "xsk_umem__delete() failed");
        return -1;
    }

    while ((umem_rings = TAILQ_FIRST(&umem->rings)) != NULL)
    {
        TAILQ_REMOVE(&umem->rings, umem_rings, links);
        free(umem_rings);
    }

    RCF_PCH_MEM_INDEX_FREE(in->umem_ptr, ns_umem);
    free(umem);
    return 0;
}

TARPC_FUNC_STANDALONE(xsk_umem__delete, {},
{
    MAKE_CALL(out->retval = ta_xsk_umem__delete(in, out));
})

/* Convert libxdp flags to native value */
static void
xsk_libxdp_flags_rpc2h(uint32_t flags, struct xsk_socket_config *config)
{
#ifdef XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD
    if (flags & RPC_XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD)
        config->libxdp_flags |= XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD;
#elif defined(XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD)
    /*
     * The case when there is no separate libxdp and libbpf
     * provides API instead.
     */
    if (flags & RPC_XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD)
        config->libbpf_flags |= XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD;
#endif
}

/* Convert XDP bind flags to native value */
static int
xdp_bind_flags_rpc2h(uint32_t flags, struct xsk_socket_config *config)
{
    config->bind_flags = 0;

#ifdef XDP_SHARED_UMEM
    if (flags & RPC_XDP_BIND_SHARED_UMEM)
        config->bind_flags |= XDP_SHARED_UMEM;
#endif

#ifdef XDP_COPY
    if (flags & RPC_XDP_BIND_COPY)
        config->bind_flags |= XDP_COPY;
#endif

#ifdef XDP_ZEROCOPY
    if (flags & RPC_XDP_BIND_ZEROCOPY)
        config->bind_flags |= XDP_ZEROCOPY;
#endif

#ifdef XDP_USE_NEED_WAKEUP
    if (flags & RPC_XDP_BIND_USE_NEED_WAKEUP)
        config->bind_flags |= XDP_USE_NEED_WAKEUP;
#endif

#ifdef XDP_USE_SG
    if (flags & RPC_XDP_BIND_USE_SG)
        config->bind_flags |= XDP_USE_SG;
#else
    if (flags & RPC_XDP_BIND_USE_SG)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP),
                         "XDP_USE_SG is not supported by local headers/libxdp");
        return -1;
    }
#endif

    return 0;
}

/* Call xsk_socket__create() */
static int
ta_xsk_socket__create(tarpc_xsk_socket__create_in *in,
                      tarpc_xsk_socket__create_out *out)
{
    static rpc_ptr_id_namespace ns_sock = RPC_PTR_ID_NS_INVALID;
    static rpc_ptr_id_namespace ns_umem = RPC_PTR_ID_NS_INVALID;
    ta_xsk_socket *sock = NULL;
    ta_xsk_umem *umem = NULL;
    rpc_ptr sock_ptr = RPC_NULL;
    const struct xsk_socket_config *cfg_ptr = NULL;
    ta_xsk_umem_rings *umem_rings = NULL;
    int rc = 0;

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_sock, RPC_TYPE_NS_XSK_SOCKET,
                                           -1);
    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_umem, RPC_TYPE_NS_XSK_UMEM,
                                           -1);

    RCF_PCH_MEM_INDEX_TO_PTR_RPC(umem, in->umem_ptr, ns_umem, -1);

    umem_rings = add_or_find_umem_rings(umem, in->if_name, in->queue_id);
    if (umem_rings == NULL)
        return -1;

    sock = TE_ALLOC(sizeof(*sock));

    sock_ptr = RCF_PCH_MEM_INDEX_ALLOC(sock, ns_sock);
    if (sock_ptr == RPC_NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOMEM),
                         "Failed to register pointer to ta_xsk_socket");
        rc = -1;
        goto finish;
    }

    if (in->config.config_len > 0)
    {
        tarpc_xsk_socket_config *conf = in->config.config_val;

        sock->config.rx_size = conf->rx_size;
        sock->config.tx_size = conf->tx_size;
        xsk_libxdp_flags_rpc2h(conf->libxdp_flags, &sock->config);
        sock->config.xdp_flags = conf->xdp_flags;
        rc = xdp_bind_flags_rpc2h(conf->bind_flags, &sock->config);
        if (rc != 0)
            goto finish;

        cfg_ptr = &sock->config;
    }
    else
    {
        cfg_ptr = NULL;

        sock->config.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
        sock->config.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
    }

    if (in->shared_umem)
    {
        rc = xsk_socket__create_shared(&sock->xsk, in->if_name,
                                       in->queue_id, umem->umem,
                                       &sock->rx, &sock->tx,
                                       &umem_rings->fill, &umem_rings->comp,
                                       cfg_ptr);
    }
    else
    {
        rc = xsk_socket__create(&sock->xsk, in->if_name,
                                in->queue_id, umem->umem,
                                &sock->rx, &sock->tx, cfg_ptr);
    }
    if (rc < 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, -rc),
                         "xsk_socket__create() failed");
        rc = -1;
        goto finish;
    }

    sock->fd = xsk_socket__fd(sock->xsk);
    if (sock->fd < 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_TA_UNIX, -sock->fd),
                         "xsk_socket__fd() failed");
        rc = -1;
        goto finish;
    }

    sock->umem = umem;

    sock->umem_rings = umem_rings;
    /*
     * Set these values here because these may be the first rings
     * created when UMEM itself was created, not bound to any
     * if_name/queue_id then.
     */
    TE_STRLCPY(umem_rings->if_name, in->if_name, IF_NAMESIZE);
    umem_rings->queue_id = in->queue_id;

    rc = sock->fd;

finish:
    if (rc < 0)
    {
        if (sock->xsk != NULL)
            xsk_socket__delete(sock->xsk);

        if (sock_ptr != RPC_NULL)
            RCF_PCH_MEM_INDEX_FREE(sock_ptr, ns_sock);

        free(sock);

        free_umem_rings(umem, umem_rings);
    }
    else
    {
        out->socket_ptr = sock_ptr;
    }

    return rc;
}

TARPC_FUNC_STANDALONE(xsk_socket__create, {},
{
    MAKE_CALL(out->retval = ta_xsk_socket__create(in, out));
})

/* Call xsk_socket__delete() */
static int
ta_xsk_socket__delete(tarpc_xsk_socket__delete_in *in,
                      tarpc_xsk_socket__delete_out *out)
{
    static rpc_ptr_id_namespace ns_socket = RPC_PTR_ID_NS_INVALID;
    ta_xsk_socket *sock = NULL;

    UNUSED(out);

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_socket,
                                           RPC_TYPE_NS_XSK_SOCKET,
                                           -1);

    RCF_PCH_MEM_INDEX_TO_PTR_RPC(sock, in->socket_ptr, ns_socket, -1);

    xsk_socket__delete(sock->xsk);

    free_umem_rings(sock->umem, sock->umem_rings);
    RCF_PCH_MEM_INDEX_FREE(in->socket_ptr, ns_socket);
    free(sock);

    return 0;
}

TARPC_FUNC_STANDALONE(xsk_socket__delete, {},
{
    MAKE_CALL(out->retval = ta_xsk_socket__delete(in, out));
})

/* Set or remove entry in XSK map */
static int
ta_xsk_map_set(tarpc_xsk_map_set_in *in, tarpc_xsk_map_set_out *out)
{
    int rc;
    uint32_t key;
    uint32_t value;

    UNUSED(out);

    key = in->key;
    if (in->fd < 0)
    {
        rc = bpf_map_delete_elem(in->map_fd, &key);
        if (rc < 0)
        {
            te_rpc_error_set(TE_OS_RC(TE_RPC, -rc),
                             "bpf_map_delete_elem() failed");
            return -1;
        }
    }
    else
    {
        value = in->fd;

        rc = bpf_map_update_elem(in->map_fd, &key, &value, 0);
        if (rc < 0)
        {
            te_rpc_error_set(TE_OS_RC(TE_RPC, -rc),
                             "bpf_map_update_elem() failed");
            return -1;
        }
    }

    return 0;
}

TARPC_FUNC_STANDALONE(xsk_map_set, {},
{
    MAKE_CALL(out->retval = ta_xsk_map_set(in, out));
})

/* Add some descriptors to FILL ring for receiving packets */
static ssize_t
ta_xsk_rx_fill_simple(tarpc_xsk_rx_fill_simple_in *in,
                      tarpc_xsk_rx_fill_simple_out *out)
{
    static rpc_ptr_id_namespace ns_umem = RPC_PTR_ID_NS_INVALID;
    ta_xsk_umem *umem = NULL;
    ta_xsk_umem_rings *umem_rings = NULL;
    uint32_t idx;
    size_t real_count;
    size_t i;

    UNUSED(out);

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_umem, RPC_TYPE_NS_XSK_UMEM,
                                           -1);
    RCF_PCH_MEM_INDEX_TO_PTR_RPC(umem, in->umem_ptr, ns_umem, -1);

    umem_rings = add_or_find_umem_rings(umem, in->if_name, in->queue_id);
    if (umem_rings == NULL)
        return -1;
    umem_rings->refcount--;

    real_count = MIN(umem->stack_count, in->frames_cnt);
    if (real_count == 0)
        return 0;

    real_count = xsk_ring_prod__reserve(&umem_rings->fill, real_count,
                                        &idx);
    if (real_count == 0)
        return 0;

    for (i = 0; i < real_count; i++)
    {
        *xsk_ring_prod__fill_addr(&umem_rings->fill, idx++) =
                      umem->frames_stack[--umem->stack_count];
    }

    xsk_ring_prod__submit(&umem_rings->fill, real_count);
    return real_count;
}

TARPC_FUNC_STANDALONE(xsk_rx_fill_simple, {},
{
    MAKE_CALL(out->retval = ta_xsk_rx_fill_simple(in, out));
})

static bool
xsk_rx_desc_is_contd(const struct xdp_desc *rx_desc)
{
#ifdef XDP_PKT_CONTD
    return ((rx_desc->options & XDP_PKT_CONTD) != 0);
#else
    return false;
#endif
}

/* Read a packet from Rx queue */
static ssize_t
ta_xsk_receive_simple(tarpc_xsk_receive_simple_in *in,
                      tarpc_xsk_receive_simple_out *out)
{
    static rpc_ptr_id_namespace ns_sock = RPC_PTR_ID_NS_INVALID;
    ta_xsk_socket *sock = NULL;
    bool pkt_finished = false;
    size_t data_off = 0;
    size_t segs_num = 0;
    size_t pkt_len = 0;
    uint32_t fill_idx;
    uint32_t idx;
    size_t count;
    size_t i;
    int rc;

    const struct xdp_desc *rx_desc = NULL;
    uint64_t addr;
    uint8_t *pkt;
    uint8_t *buf;

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_sock, RPC_TYPE_NS_XSK_SOCKET,
                                           -1);

    RCF_PCH_MEM_INDEX_TO_PTR_RPC(sock, in->socket_ptr, ns_sock, -1);

    count = xsk_ring_cons__peek(&sock->rx,
                                (sock->config.rx_size == 0 ?
                                 1 : sock->config.rx_size),
                                &idx);
    if (count == 0)
        return 0;

    for (i = 0; i < count; i++)
    {
        rx_desc = xsk_ring_cons__rx_desc(&sock->rx, idx + i);

        if (pkt_len + rx_desc->len < pkt_len)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EOVERFLOW),
                             "packet length overflows size_t");
            xsk_ring_cons__cancel(&sock->rx, count);
            return -1;
        }

        pkt_len += rx_desc->len;
        segs_num++;

        if (!xsk_rx_desc_is_contd(rx_desc))
        {
            pkt_finished = true;
            break;
        }
    }

    if (!pkt_finished)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EAGAIN),
                         "incomplete multi-segment packet in RX batch");
        xsk_ring_cons__cancel(&sock->rx, count);
        return -1;
    }

    if (count > segs_num)
        xsk_ring_cons__cancel(&sock->rx, count - segs_num);

    buf = TE_ALLOC(pkt_len);

    for (i = 0; i < segs_num; i++)
    {
        rx_desc = xsk_ring_cons__rx_desc(&sock->rx, idx + i);
        addr = xsk_umem__add_offset_to_addr(rx_desc->addr);
        pkt = xsk_umem__get_data(sock->umem->buf, addr);

        memcpy(buf + data_off, pkt, rx_desc->len);
        data_off += rx_desc->len;
    }

    /*
     * Submitting buffer back to FILL ring of UMEM, so that it can be
     * reused for receiving other packets.
     */

    rc = xsk_ring_prod__reserve(&sock->umem_rings->fill, segs_num, &fill_idx);
    if (rc != (int)segs_num)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOBUFS),
                         "xsk_ring_prod__reserve() did not reserve "
                         "requested number of descriptors");
        xsk_ring_cons__cancel(&sock->rx, segs_num);
        free(buf);
        return -1;
    }

    for (i = 0; i < segs_num; i++)
    {
        rx_desc = xsk_ring_cons__rx_desc(&sock->rx, idx + i);
        *xsk_ring_prod__fill_addr(&sock->umem_rings->fill, fill_idx + i) =
            xsk_umem__extract_addr(rx_desc->addr);
    }

    xsk_ring_prod__submit(&sock->umem_rings->fill, segs_num);
    xsk_ring_cons__release(&sock->rx, segs_num);

    out->data.data_val = buf;
    out->data.data_len = pkt_len;

    return pkt_len;
}

TARPC_FUNC_STANDALONE(xsk_receive_simple, {},
{
    MAKE_CALL(out->retval = ta_xsk_receive_simple(in, out));
})

/* Send a packet from AF_XDP socket */
static int
ta_xsk_send_simple(tarpc_xsk_send_simple_in *in,
                   tarpc_xsk_send_simple_out *out)
{
    static rpc_ptr_id_namespace ns_sock = RPC_PTR_ID_NS_INVALID;
    ta_xsk_socket *sock = NULL;
    ta_xsk_umem *umem = NULL;
    size_t popped_num = 0;
    size_t data_off = 0;
    uint64_t *umem_addrs;
    bool *comp_seen;
    uint64_t comp_addr;
    size_t frame_size;
    size_t segs_num;
    size_t seg_len;
    uint32_t j;
    size_t i;

    struct xdp_desc *tx_desc;
    uint32_t completed = 0;
    bool submitted = false;
    int retval = -1;
    uint8_t *buf;
    uint32_t idx;
    bool found;
    uint32_t comp_idx;
    int rc;

    UNUSED(out);

    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns_sock, RPC_TYPE_NS_XSK_SOCKET,
                                           -1);
    RCF_PCH_MEM_INDEX_TO_PTR_RPC(sock, in->socket_ptr, ns_sock, -1);

    umem = sock->umem;
    frame_size = umem->config.frame_size;
    if (frame_size == 0)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                         "zero UMEM frame size");
        return -1;
    }

    segs_num = (in->data.data_len == 0 ? 1 :
                ((in->data.data_len + frame_size - 1) / frame_size));

#ifndef XDP_PKT_CONTD
    if (segs_num > 1)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP),
                         "multi-segment TX is not supported by local headers/libxdp");
        return -1;
    }
#endif

    if (umem->stack_count < segs_num)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOBUFS),
                         "no free frames left in UMEM");
        return -1;
    }

    umem_addrs = TE_ALLOC(segs_num * sizeof(*umem_addrs));
    comp_seen = TE_ALLOC(segs_num * sizeof(*comp_seen));
    memset(comp_seen, 0, segs_num * sizeof(*comp_seen));

    rc = xsk_ring_prod__reserve(&sock->tx, segs_num, &idx);
    if (rc != (int)segs_num)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOBUFS),
                         "xsk_ring_prod__reserve() cannot reserve TX descriptors");
        goto finish;
    }

    for (i = 0; i < segs_num; i++)
    {
        seg_len = MIN(frame_size, in->data.data_len - data_off);
        umem_addrs[i] = umem->frames_stack[--umem->stack_count];
        popped_num++;
        buf = (uint8_t *)(umem->buf) + umem_addrs[i];
        if (seg_len > 0)
            memcpy(buf, in->data.data_val + data_off, seg_len);
        data_off += seg_len;

        tx_desc = xsk_ring_prod__tx_desc(&sock->tx, idx + i);
        tx_desc->addr = umem_addrs[i];
        tx_desc->len = seg_len;
#ifdef XDP_PKT_CONTD
        tx_desc->options = ((i + 1 < segs_num) ? XDP_PKT_CONTD : 0);
#endif
    }

    xsk_ring_prod__submit(&sock->tx, segs_num);
    submitted = true;

    /*
     * Call sendto() to let kernel know that something should be sent
     * from TX queue.
     */
    rc = sendto(sock->fd, NULL, 0, MSG_DONTWAIT, NULL, 0);
    if (rc < 0)
    {
        /*
         * List of acceptable errors was taken from kick_tx(),
         * tools/testing/selftests/bpf/xskxceiver.c in Linux kernel
         * sources.
         */
        if (!(errno == ENOBUFS || errno == EAGAIN ||
              errno == EBUSY || errno == ENETDOWN))
        {
            te_rpc_error_set(TE_OS_RC(TE_TA_UNIX, errno),
                             "sendto() failed with unexpected errno");
            goto finish;
        }
    }

    while (completed < segs_num)
    {
        rc = xsk_ring_cons__peek(&sock->umem_rings->comp, segs_num - completed,
                                 &comp_idx);
        if (rc > 0)
        {
            for (j = 0; j < (uint32_t)rc; j++)
            {
                comp_addr =
                    *xsk_ring_cons__comp_addr(&sock->umem_rings->comp,
                                              comp_idx + j);
                found = false;

                for (i = 0; i < segs_num; i++)
                {
                    if (!comp_seen[i] && umem_addrs[i] == comp_addr)
                    {
                        comp_seen[i] = true;
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    xsk_ring_cons__release(&sock->umem_rings->comp, rc);
                    te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EFAIL),
                                     "completion ring contains unknown UMEM address");
                    goto finish;
                }
            }

            completed += rc;
            xsk_ring_cons__release(&sock->umem_rings->comp, rc);
        }
        else
        {
            usleep(1);
        }
    }

    for (i = 0; i < segs_num; i++)
        umem->frames_stack[umem->stack_count++] = umem_addrs[i];

    retval = 0;

finish:

    if (!submitted)
    {
        for (i = 0; i < popped_num; i++)
            umem->frames_stack[umem->stack_count++] = umem_addrs[i];
    }

    free(comp_seen);
    free(umem_addrs);
    return retval;
}

TARPC_FUNC_STANDALONE(xsk_send_simple, {},
{
    MAKE_CALL(out->retval = ta_xsk_send_simple(in, out));
})

#endif /* HAVE_AF_XDP */

/*
 * Call bpf_obj_get(). Can be used to obtain FD of a map
 * pinned from another process.
 */
static int
ta_bpf_obj_get(tarpc_bpf_obj_get_in *in,
               tarpc_bpf_obj_get_out *out)
{
    int fd;

    UNUSED(out);

    fd = bpf_obj_get(in->path);

    if (fd < 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, -fd),
                         "bpf_obj_get() failed");
        return -1;
    }

    return fd;
}

TARPC_FUNC_STANDALONE(bpf_obj_get, {},
{
    MAKE_CALL(out->retval = ta_bpf_obj_get(in, out));
})

#endif /* if HAVE_BPF_BPF_H */
