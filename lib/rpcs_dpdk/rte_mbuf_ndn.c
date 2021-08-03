/** @file
 * @brief RPC for RTE mbuf CSAP layer API
 *
 * RPC routines to access RTE mbuf CSAP layer functionality
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC RTE mbuf layer"

#include "te_config.h"

#include "rte_config.h"
#include "rte_ether.h"
#include "rte_ip.h"
#include "rte_mbuf.h"
#include "rte_net.h"
#include "rte_ring.h"
#include "rte_tcp.h"
#include "rte_udp.h"

#include "asn_usr.h"
#include "asn_impl.h"
#include "ndn_rte_mbuf.h"
#include "tad_utils.h"
#include "tad_api.h"
#include "tad_csap_inst.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"
#include "te_alloc.h"
#include "te_errno.h"
#include "te_defs.h"

static te_errno
rte_mbuf_csap_add_layer(asn_value       **csap_spec,
                        const asn_type   *layer_type,
                        const char       *layer_choice,
                        asn_value       **layer_spec)
{
    asn_value  *csap_spec_new = NULL;
    asn_value  *layers_new = NULL;
    asn_value  *gen_layer = NULL;
    asn_value  *layer = NULL;
    asn_value  *layers;
    te_errno    rc;

    if (layer_type == NULL || layer_choice == NULL || csap_spec == NULL)
    {
        rc = TE_EINVAL;
        goto fail;
    }

    if (*csap_spec == NULL)
    {
        csap_spec_new = asn_init_value(ndn_csap_spec);
        if (csap_spec_new == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        *csap_spec = csap_spec_new;
    }

    rc = asn_get_child_value(*csap_spec, (const asn_value **)&layers,
                             PRIVATE, NDN_CSAP_LAYERS);
    if (rc == TE_EASNINCOMPLVAL)
    {
        layers_new = asn_init_value(ndn_csap_layers);
        if (layers_new == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        layers = layers_new;

        rc = asn_put_child_value(*csap_spec, layers,
                                    PRIVATE, NDN_CSAP_LAYERS);
        if (rc != 0)
            goto fail;
    }
    else if (rc != 0)
    {
        goto fail;
    }

    gen_layer = asn_init_value(ndn_generic_csap_layer);
    if (gen_layer == NULL)
    {
        rc = TE_ENOMEM;
        goto fail;
    }

    rc = asn_insert_indexed(layers, gen_layer, -1, "");
    if (rc != 0)
        goto fail;

    layer = asn_init_value(layer_type);
    if (layer == NULL)
    {
        rc = TE_ENOMEM;
        goto fail;
    }

    rc = asn_put_child_value_by_label(gen_layer, layer, layer_choice);
    if (rc != 0)
        goto fail;

    if (layer_spec != NULL)
        *layer_spec = layer;

    return 0;

fail:
    if (gen_layer != NULL)
        (void)asn_remove_indexed(layers, -1, "");

    if (layers_new != NULL)
        (void)asn_free_child_value(*csap_spec, PRIVATE, NDN_CSAP_LAYERS);

    if (csap_spec_new != NULL)
    {
        asn_free_value(*csap_spec);
        *csap_spec = NULL;
    }

    if (layer_spec != NULL)
        *layer_spec = NULL;

    return rc;
}

static int
rte_mbuf_nds_str2csap_layers_stack(const char         *nds_str,
                                   const asn_type     *ndn_type,
                                   asn_value         **nds_out,
                                   char              **stack_out,
                                   asn_value         **csap_spec_out,
                                   asn_value         **rte_mbuf_layer_out)
{
    int             num_symbols_parsed;
    unsigned int    nds_len;
    unsigned int    i;
    const char     *protocol_name;
    size_t          protocol_name_len;
    size_t          layer_choice_len;
    const asn_type *layer_type;
    asn_value      *rte_mbuf_layer;
    te_errno        rc;
    asn_value      *nds = NULL;
    asn_value      *nds_unified = NULL;
    char           *stack = NULL;
    char           *stack_new;
    size_t          stack_len = 1;          /* 1 for the future trailing zero */
    char           *layer_choice = NULL;
    char           *layer_choice_new;
    asn_value      *csap_spec = NULL;

    rc = asn_parse_value_text(nds_str, ndn_type, &nds, &num_symbols_parsed);
    if (rc != 0)
        goto fail;

    if (ndn_type == ndn_traffic_template)
    {
        nds_unified = nds;
    }
    else
    {
        /*
         * Here we rely on an assumption that the same PDU sequence is present
         * in all the pattern units and take the first one as a reference
         */
        rc = asn_get_indexed(nds, &nds_unified, 0, "");
        if (rc != 0)
            goto fail;
    }

    nds_len = (unsigned int)asn_get_length(nds_unified, "pdus");

    for (i = 0; i < nds_len; ++i)
    {
        asn_value      *gen_pdu;
        asn_tag_value   pdu_tag;

        rc = asn_get_indexed(nds_unified, &gen_pdu, i, "pdus");
        if (rc != 0)
            goto fail;

        rc = asn_get_choice_value(gen_pdu, NULL, NULL, &pdu_tag);
        if (rc != 0)
            goto fail;

        protocol_name = te_proto_to_str(pdu_tag);
        if (protocol_name == NULL)
        {
            rc = TE_EINVAL;
            goto fail;
        }

        protocol_name_len = strlen(protocol_name);

        stack_new = realloc(stack, stack_len + protocol_name_len + 1);
        if (stack_new == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        stack = stack_new;

        if ((unsigned int)sprintf(stack + stack_len - 1, "%s.",
                                  protocol_name) != (protocol_name_len + 1))
        {
            rc = TE_ENOBUFS;
            goto fail;
        }

        stack_len += (protocol_name_len + 1);

        /* For the future '#' sign */
        layer_choice_len = protocol_name_len + 1;

        /* For the trailing zero */
        layer_choice_new = realloc(layer_choice, layer_choice_len + 1);
        if (layer_choice_new == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        layer_choice = layer_choice_new;

        if ((unsigned int)sprintf(layer_choice, "#%s", protocol_name) !=
            layer_choice_len)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        rc = asn_impl_find_subtype(ndn_generic_csap_layer, protocol_name,
                                   &layer_type);
        if (rc != 0)
            goto fail;

        rc = rte_mbuf_csap_add_layer(&csap_spec, layer_type,
                                     layer_choice, NULL); /* protocol_name */
        if (rc != 0)
            goto fail;
    }

    free(layer_choice);

    protocol_name = te_proto_to_str(TE_PROTO_RTE_MBUF);
    protocol_name_len = strlen(protocol_name);
    stack_new = realloc(stack, stack_len + protocol_name_len);
    if (stack_new == NULL)
    {
        rc = TE_ENOMEM;
        goto fail;
    }

    stack = stack_new;

    if ((unsigned int)sprintf(stack + stack_len - 1, "%s", protocol_name) !=
        protocol_name_len)
    {
        rc = TE_ENOBUFS;
        goto fail;
    }

    rc = rte_mbuf_csap_add_layer(&csap_spec, ndn_rte_mbuf_csap,
                                 "#rtembuf", &rte_mbuf_layer);
    if (rc != 0)
        goto fail;

    *nds_out = nds;
    *stack_out = stack;
    *csap_spec_out = csap_spec;
    *rte_mbuf_layer_out = rte_mbuf_layer;

    return 0;

fail:
    free(layer_choice);
    asn_free_value(csap_spec);
    free(stack);
    asn_free_value(nds);

    *nds_out = NULL;
    *stack_out = NULL;
    *csap_spec_out = NULL;
    *rte_mbuf_layer_out = NULL;

    return rc;
}

static int
rte_mbuf_config_init_csap(asn_value          *rte_mbuf_layer,
                          unsigned int        ring_num_entries_desired,
                          struct rte_mempool *mp,
                          asn_value          *csap_spec,
                          char               *stack,
                          struct rte_ring   **ring_out,
                          csap_p             *csap_instance_out)
{
    size_t              csap_spec_str_len;
    csap_p              csap_instance = NULL;
    te_errno            rc;
    struct rte_ring    *ring = NULL;
    char               *csap_spec_str = NULL;

    /* Allocate RTE ring and fill in 'rtembuf' layer settings */
    ring = rte_ring_create("mbuf_ring",
                           te_round_up_pow2(ring_num_entries_desired + 1),
                           mp->socket_id, 0);
    if (ring == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rc = asn_write_string(rte_mbuf_layer, ring->name, "pkt-ring.#plain");
    if (rc != 0)
        goto out;

    rc = asn_write_string(rte_mbuf_layer, mp->name, "pkt-pool.#plain");
    if (rc != 0)
        goto out;

    /* Prepare a textual representation of the CSAP spec */
    csap_spec_str_len = asn_count_txt_len(csap_spec, 0) + 1;
    csap_spec_str = calloc(1, csap_spec_str_len);
    if (csap_spec_str == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    if (asn_sprint_value(csap_spec, csap_spec_str, csap_spec_str_len, 0) <= 0)
    {
        rc = TE_EINVAL;
        goto out;
    }

    /* Init TAD if need be */
    rc = rcf_ch_tad_init();
    if (rc != 0 && rc != TE_RC(TE_TAD_CH, TE_EALREADY))
        goto out;

    /* Create a CSAP instance */
    rc = tad_csap_create(stack, csap_spec_str, &csap_instance);
    if (rc != 0)
        goto out;

    free(csap_spec_str);

    *ring_out = ring;
    *csap_instance_out = csap_instance;

    return 0;

out:
    free(csap_spec_str);
    rte_ring_free(ring);

    *ring_out = NULL;
    *csap_instance_out = NULL;

    return rc;
}

static int
rte_mk_mbuf_from_template(tarpc_rte_mk_mbuf_from_template_in     *in,
                          tarpc_rte_mk_mbuf_from_template_out    *out)
{
    tad_send_tmpl_unit_data tu_data;
    tad_reply_context       reply_ctx;
    csap_p                  csap_instance;
    tad_reply_spec         *reply_spec;
    unsigned int            i;
    te_errno                rc;
    struct rte_mempool     *mp = NULL;
    asn_value              *template = NULL;
    char                   *stack = NULL;
    asn_value              *csap_spec = NULL;
    asn_value              *rte_mbuf_layer = NULL;
    csap_p                  dummy_csap_instance = NULL;
    unsigned int            num_pkts = 0;
    struct rte_ring        *ring = NULL;
    te_bool                 csap_created = FALSE;
    struct rte_mbuf       **mbufs = NULL;
#ifdef HAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE
    unsigned int            ret;
#endif /* HAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE */

    memset(&tu_data, 0, sizeof(tu_data));
    memset(&reply_ctx, 0, sizeof(reply_ctx));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MEMPOOL, {
        mp = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mp, ns);
    });

    if (mp == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    /*
     * 1) Convert the traffic template to ASN.1 representation
     * 2) Build up a string representation of CSAP's stack and
     *    add the corresponding layers to CSAP
     * 3) Add the bottom layer of type 'rtembuf' to CSAP
     *    and store the layer's pointer to be used later
     */
    rc = rte_mbuf_nds_str2csap_layers_stack(in->template,
                                            ndn_traffic_template,
                                            &template, &stack,
                                            &csap_spec,
                                            &rte_mbuf_layer);
    if (rc != 0)
        goto out;

    /*
     * Find out the size for the RTE ring based on the number of packets
     * to be produced by the template
     */

    /*
     * Due to the fact that tad_send_preprocess_args() needs a valid
     * CSAP instance for logging (id, state) we have to create a blank
     * dummy instance and pass it to the function
     */
    dummy_csap_instance = calloc(1, sizeof(dummy_csap_instance));
    if (dummy_csap_instance == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rc = tad_send_preprocess_args(dummy_csap_instance, template, &tu_data);
    if (rc != 0)
        goto out;

    /*
     * We rely here on the template supplier's decency and suppose that we
     * don't get tad_iterate_tmpl_args() == -TE_EOPNOTSUPP before the end
     * of the actual 'arg-sets' sub-value in the template
     */
    do num_pkts++; while (tad_iterate_tmpl_args(tu_data.arg_specs,
                                                tu_data.arg_num,
                                                tu_data.arg_iterated) > 0);

    /* Configure 'rtembuf' layer and create a CSAP instance */
    rc = rte_mbuf_config_init_csap(rte_mbuf_layer, num_pkts, mp,
                                   csap_spec, stack, &ring, &csap_instance);
    if (rc != 0)
        goto out;

    csap_created = TRUE;

    /* Create a dummy reply context */
    reply_ctx.spec = calloc(1, sizeof(tad_reply_spec));
    if (reply_ctx.spec == NULL)
        goto out;

    reply_spec = (tad_reply_spec *)reply_ctx.spec;

    reply_ctx.opaque = NULL;
    reply_spec->opaque_size = 0;

    /* Shove the template into the CSAP */
    rc = tad_send_start_prepare(csap_instance, in->template,
                                TRUE, &reply_ctx);
    if (rc != 0)
        goto out;

    /* "Send" */
    rc = tad_send_do(csap_instance);
    if (rc != 0)
        goto out;

    /* Allocate an array to deliver resulting mbufs back */
    out->mbufs.mbufs_len = rte_ring_count(ring);
    out->mbufs.mbufs_val = calloc(out->mbufs.mbufs_len, sizeof(tarpc_rte_mempool));
    if ((out->mbufs.mbufs_len > 0) && (out->mbufs.mbufs_val == NULL))
    {
        rc = TE_ENOMEM;
        goto out;
    }

    /* Allocate a temporary array of mbuf pointers */
    mbufs = calloc(out->mbufs.mbufs_len, sizeof(*mbufs));
    if (mbufs == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    /* Pull out the resulting RTE mbuf pointers to the temporary array */
#ifdef HAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE
    ret = rte_ring_dequeue_bulk(ring, (void **)mbufs, out->mbufs.mbufs_len,
                                NULL);
    if (ret != out->mbufs.mbufs_len)
    {
        rc = TE_EFAULT;
        goto out;
    }
#else /* !HAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE */
    rc = rte_ring_dequeue_bulk(ring, (void **)mbufs, out->mbufs.mbufs_len);
    neg_errno_h2rpc(&rc);
    if (rc != 0)
        goto out;
#endif /* HAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE */

    /* Map the RTE mbuf pointers to the corresponding PCH MEM indexes */
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < out->mbufs.mbufs_len; ++i)
            out->mbufs.mbufs_val[i] = RCF_PCH_MEM_INDEX_ALLOC(mbufs[i], ns);
    });

out:
    /* Free the temporary array of RTE mbuf pointers */
    free(mbufs);

    /* Destroy the dummy reply context */
    free((void *)reply_ctx.spec);

    /* Destroy CSAP instance (it will also free 'csap_spec', and it's unfair) */
    if (csap_created)
        (void)tad_csap_destroy(csap_instance);

    /* Free RTE ring */
    rte_ring_free(ring);

    /* Free the dummy CSAP instance */
    free(dummy_csap_instance);

    asn_free_value(rte_mbuf_layer);
    free(stack);
    asn_free_value(template);

    return -TE_RC(TE_RPCS, rc);
}

TARPC_FUNC_STATIC(rte_mk_mbuf_from_template, {},
{
    MAKE_CALL(out->retval = func(in, out));
}
)

/** Auxiliary description of storage for matching packets */
struct rte_mbuf_tad_reply_opaque {
    unsigned int    added;
    tarpc_string   *pkt_nds_storage;
};

static te_errno
rte_mbuf_store_matching_packets(void               *opaque,
                                const asn_value    *pkt_nds)
{
    struct rte_mbuf_tad_reply_opaque   *desc;
    size_t                              pkt_nds_str_len;
    te_errno                            rc;

    /* Remove the dummy PDU added by tad_recv_get_packets() */
    rc = asn_remove_indexed((asn_value *)pkt_nds, -1, "pdus");
    if (rc != 0)
        return rc;

    desc = (struct rte_mbuf_tad_reply_opaque *)opaque;

    pkt_nds_str_len = asn_count_txt_len(pkt_nds, 0) + 1;

    desc->pkt_nds_storage[desc->added].str = calloc(pkt_nds_str_len,
                                                    sizeof(char));
    if (desc->pkt_nds_storage[desc->added].str == NULL)
    {
        rc = TE_ENOMEM;
        goto fail;
    }

    if (asn_sprint_value(pkt_nds, desc->pkt_nds_storage[desc->added].str,
                         pkt_nds_str_len, 0) <= 0)
    {
        rc = TE_ENOBUFS;
        goto fail;
    }

    desc->added++;

    return 0;

fail:
    free(desc->pkt_nds_storage[desc->added].str);

    return rc;
}

static int
rte_mbuf_match_pattern(tarpc_rte_mbuf_match_pattern_in  *in,
                       tarpc_rte_mbuf_match_pattern_out *out)
{
    unsigned int                        recv_flags;
    tad_reply_context                   reply_ctx;
    csap_p                              csap_instance;
    tad_reply_spec                     *reply_spec;
    unsigned int                        i;
    tad_recv_context                   *receiver;
    struct rte_mbuf_tad_reply_opaque   *pkt_storage_desc;
    te_errno                            rc;
    struct rte_mbuf                    *m = NULL;
    asn_value                          *pattern = NULL;
    char                               *stack = NULL;
    asn_value                          *csap_spec = NULL;
    asn_value                          *rte_mbuf_layer = NULL;
    struct rte_ring                    *ring = NULL;
    te_bool                             csap_created = FALSE;
    struct rte_mbuf                   **mbufs = NULL;
    unsigned int                        got_pkts = 0;
#ifdef HAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE
    unsigned int                        ret;
#endif /* HAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE */

    memset(&reply_ctx, 0, sizeof(reply_ctx));

    if (in->mbufs.mbufs_len == 0)
    {
        rc = TE_ENOENT;
        goto out;
    }

    /*
     * In order to obtain a dummy (but valid) mempool pointer to be set
     * as a CSAP parameter, obtain the first mbuf pointer from the list
     */
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mbufs.mbufs_val[0], ns);
    });

    if (m == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    /*
     * 1) Convert the traffic pattern to ASN.1 representation
     * 2) Build up a string representation of CSAP's stack and
     *    add the corresponding layers to CSAP
     * 3) Add the bottom layer of type 'rtembuf' to CSAP
     *    and store the layer's pointer to be used later
     */
    rc = rte_mbuf_nds_str2csap_layers_stack(in->pattern,
                                            ndn_traffic_pattern,
                                            &pattern, &stack,
                                            &csap_spec,
                                            &rte_mbuf_layer);
    if (rc != 0)
        goto out;

    /* Configure 'rtembuf' layer and create a CSAP instance */
    rc = rte_mbuf_config_init_csap(rte_mbuf_layer, in->mbufs.mbufs_len, m->pool,
                                   csap_spec, stack, &ring, &csap_instance);
    if (rc != 0)
        goto out;

    csap_created = TRUE;

    /* Create a dummy reply context */
    reply_ctx.spec = calloc(1, sizeof(tad_reply_spec));
    if (reply_ctx.spec == NULL)
        goto out;

    reply_spec = (tad_reply_spec *)reply_ctx.spec;

    reply_ctx.opaque = NULL;
    reply_spec->opaque_size = 0;

    /* Allocate a temporary array of mbuf pointers */
    mbufs = calloc(in->mbufs.mbufs_len, sizeof(*mbufs));
    if (mbufs == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    /* Map PCH MEM indexes supplied by the caller to RTE mbuf pointers */
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        for (i = 0; i < in->mbufs.mbufs_len; ++i)
        {
            mbufs[i] = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->mbufs.mbufs_val[i], ns);
            if (mbufs[i] == NULL)
            {
                rc = TE_EINVAL;
                goto out;
            }
        }
    });

    /* Shove RTE mbuf pointers into the ring to be inspected by the CSAP */
#ifdef HAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE
    ret = rte_ring_enqueue_bulk(ring, (void **)mbufs, in->mbufs.mbufs_len,
                                NULL);
    if (ret != in->mbufs.mbufs_len)
    {
        rc = TE_EFAULT;
        goto out;
    }
#else /* !HAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE */
    rc = rte_ring_enqueue_bulk(ring, (void **)mbufs, in->mbufs.mbufs_len);
    neg_errno_h2rpc(&rc);
    if (rc != 0)
        goto out;
#endif /* HAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE */

    recv_flags = RCF_CH_TRRECV_PACKETS;
    if (in->seq_match == TRUE)
        recv_flags |= RCF_CH_TRRECV_PACKETS_SEQ_MATCH;

    /* Shove the pattern into the CSAP */
    rc = tad_recv_start_prepare(csap_instance, in->pattern, in->mbufs.mbufs_len,
                                TAD_TIMEOUT_INF, recv_flags, &reply_ctx);
    if (rc != 0)
        goto out;

    /* "Receive" */
    rc = tad_recv_do(csap_instance);
    if (rc != 0)
    {
        if (rc != TE_RC(TE_TAD_CH, TE_EINTR))
            goto out;
        else
            rc = 0;
    }

    receiver = csap_get_recv_context(csap_instance);

    out->matched = receiver->match_pkts;

    /*
     * We return from the RPC if we are not asked to report
     * the matching packets
     */
    if (out->matched == 0 || !(in->return_matching_pkts))
        goto out;

    /*
     * To grab the matching packets from the CSAP queue we have to
     * set up our custom structure for the TAD reply opaque data and use
     * a special function as a callback for accessing the opaque data
     */
    reply_ctx.opaque = calloc(1, sizeof(struct rte_mbuf_tad_reply_opaque));
    if (reply_ctx.opaque == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    pkt_storage_desc = (struct rte_mbuf_tad_reply_opaque *)(reply_ctx.opaque);

    /*
     * Allocate an array of strings to store the matching packets
     * NDS in textual representation
     */
    out->packets.packets_len = out->matched;
    out->packets.packets_val = calloc(out->matched, sizeof(*out->packets.packets_val));
    if (out->packets.packets_val == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    pkt_storage_desc->pkt_nds_storage = out->packets.packets_val;

    /* Set our custom callback to store the matching packets */
    reply_spec->pkt = &rte_mbuf_store_matching_packets;

    /* Grab the matching packets from the CSAP queue */
    rc = tad_recv_get_packets(csap_instance, &reply_ctx, FALSE, &got_pkts);
    if (rc != 0)
        goto out;

    if (out->matched != got_pkts)
        rc = TE_ENOENT;

out:
    /* Free the reply context opaque data (if any) */
    free(reply_ctx.opaque);

    /* Free the temporary array of RTE mbuf pointers */
    free(mbufs);

    /* Destroy the dummy reply context */
    free((void *)reply_ctx.spec);

    /* Destroy CSAP instance (it will also free 'csap_spec', and it's unfair) */
    if (csap_created)
        (void)tad_csap_destroy(csap_instance);

    /* Free RTE ring */
    rte_ring_free(ring);

    asn_free_value(rte_mbuf_layer);
    free(stack);
    asn_free_value(pattern);

    return -TE_RC(TE_RPCS, rc);
}

TARPC_FUNC_STATIC(rte_mbuf_match_pattern, {},
{
    MAKE_CALL(out->retval = func(in, out));
}
)

struct rte_mbuf_parse_ctx {
    /* Information obtained by rte_mbuf_detect_layers() */
    size_t   innermost_l3_ofst;
    size_t   innermost_l4_ofst;
    uint32_t innermost_layers;
    size_t   outer_l3_ofst;
    size_t   outer_l4_ofst;
    uint32_t outer_layers;
    size_t   header_size;
    size_t   pld_size;

    /* Information obtained by rte_mbuf_match_tx_rx_learn() */
    uint8_t  tcp_flags;
};

/*
 * Detect L2/L3/L4 in the outermost header and, if encapsulation is used, do it
 * for the inner header, too. Make two sets of ptype flags, correspondingly. In
 * both, the masks are: RTE_PTYPE_L2_MASK, RTE_PTYPE_L3_MASK, RTE_PTYPE_L4_MASK.
 *
 * The reason behind having this function is that there are no flags PKT_TX_TCP
 * and PKT_TX_UDP in DPDK for 'lib/tad/tad_rte_mbuf_sap.c' to set automatically.
 */
static te_errno
rte_mbuf_detect_layers(struct rte_mbuf_parse_ctx *parse_ctx,
                       struct rte_mbuf           *m)
{
    uint32_t layers;
    uint32_t mask;

    parse_ctx->outer_l3_ofst = m->outer_l2_len;
    parse_ctx->outer_l4_ofst = parse_ctx->outer_l3_ofst + m->outer_l3_len;
    parse_ctx->innermost_l3_ofst = parse_ctx->outer_l4_ofst + m->l2_len;
    parse_ctx->innermost_l4_ofst = parse_ctx->innermost_l3_ofst + m->l3_len;
    parse_ctx->header_size = parse_ctx->innermost_l4_ofst + m->l4_len;
    parse_ctx->pld_size = m->pkt_len - parse_ctx->header_size;

    /*
     * In fact, rte_net_get_ptype() accepts multi-seg mbufs.
     *
     * This check protects "tampering" part (see below) and
     * may also be useful to other callers of this function.
     */
    if (m->data_len < parse_ctx->header_size)
    {
        ERROR("m: non-contiguous header (unsupported)");
        return TE_EINVAL;
    }

    mask = RTE_PTYPE_L2_MASK | RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK;

    /*
     * rte_net_get_ptype() is VXLAN-unaware, so 'mask' intentionally doesn't
     * request tunnel and inner packet type discovery. Invoke it to discover
     * L2 (unused here), L3 and L4 in the outermost header. If encapsulation
     * is used, tamper with the mbuf to "decapsulate" the packet temporarily
     * and invoke rte_net_get_ptype() for the second time to parse the inner
     * header. Roll back results of the prior tampering with the mbuf fields.
     */
    layers = rte_net_get_ptype(m, NULL, mask);

    if (m->outer_l2_len != 0)
    {
        uint16_t data_off_orig = m->data_off;
        size_t   shift;

        parse_ctx->outer_layers = layers;

	/* Tamper with the mbuf to "decapsulate" the packet. */
        assert(m->l2_len >= RTE_ETHER_HDR_LEN);

        m->data_off += parse_ctx->innermost_l3_ofst;
        m->data_off -= RTE_ETHER_HDR_LEN; /* API contract: no VLAN tags here. */
        shift = m->data_off - data_off_orig;
        m->data_len -= shift;
        m->pkt_len -= shift;

        /*
	 * Fields m->[...]lX_len are ignored by rte_net_get_ptype(),
	 * so no discrepancies will be encountered during parsing.
         */
        layers = rte_net_get_ptype(m, NULL, mask);

	/* Restore the original mbuf meta fields. */
        m->data_off = data_off_orig;
        m->data_len += shift;
        m->pkt_len += shift;
    }
    else
    {
        parse_ctx->outer_layers = 0;
    }

    parse_ctx->innermost_layers = layers;

    return 0;
}

struct rte_mbuf_cksum_ctx {
    te_bool                is_ipv4;
    uint16_t               cksum;
    const struct rte_mbuf *m;
};

/* Fill in cksum_ctx->m before invocation. */
static te_errno
rte_mbuf_tcp_first_pkt_get_cksum(const struct rte_mbuf_parse_ctx *parse_ctx,
                                 struct rte_mbuf_cksum_ctx       *cksum_ctx)
{
    size_t      first_pkt_pld_size = (cksum_ctx->m->tso_segsz != 0) ?
                                     MIN(cksum_ctx->m->tso_segsz,
                                         parse_ctx->pld_size) :
                                     parse_ctx->pld_size;
    uint8_t    *bounce_buf_pld = NULL;
    uint8_t    *bounce_buf_pkt = NULL;
    const void *buf_pld;
    te_errno    rc = 0;

    bounce_buf_pld = TE_ALLOC(first_pkt_pld_size);
    if (bounce_buf_pld == NULL)
        return TE_ENOMEM;

    bounce_buf_pkt = TE_ALLOC(parse_ctx->header_size + first_pkt_pld_size);
    if (bounce_buf_pkt == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rte_memcpy(bounce_buf_pkt, rte_pktmbuf_mtod(cksum_ctx->m, const void *),
               parse_ctx->header_size);

    buf_pld = rte_pktmbuf_read(cksum_ctx->m, parse_ctx->header_size,
                               first_pkt_pld_size, &bounce_buf_pld);
    if (buf_pld == NULL)
    {
        rc = TE_EFAULT;
        goto out;
    }

    rte_memcpy(bounce_buf_pkt + parse_ctx->header_size, buf_pld,
               first_pkt_pld_size);

    if (cksum_ctx->m->tso_segsz != 0 && parse_ctx->pld_size > cksum_ctx->m->tso_segsz)
    {
        struct rte_tcp_hdr *tcph;

        tcph = (struct rte_tcp_hdr *)(bounce_buf_pkt +
                                      parse_ctx->innermost_l4_ofst);

        tcph->tcp_flags &= ~(RTE_TCP_FIN_FLAG | RTE_TCP_PSH_FLAG);
    }

    if (cksum_ctx->is_ipv4)
    {
        struct rte_ipv4_hdr *ipv4h;

        ipv4h = (struct rte_ipv4_hdr *)(bounce_buf_pkt +
                                        parse_ctx->innermost_l3_ofst);

        if (cksum_ctx->m->tso_segsz != 0)
        {
            uint16_t total_length = rte_be_to_cpu_16(ipv4h->total_length);
            /* API contract: the original header counts for "zero payload". */
            total_length += first_pkt_pld_size;
            ipv4h->total_length = rte_cpu_to_be_16(total_length);
        }

        cksum_ctx->cksum = rte_ipv4_udptcp_cksum(ipv4h,
                                                 bounce_buf_pkt +
                                                 parse_ctx->innermost_l4_ofst);
    }
    else
    {
        struct rte_ipv6_hdr *ipv6h;

        ipv6h = (struct rte_ipv6_hdr *)(bounce_buf_pkt +
                                        parse_ctx->innermost_l3_ofst);

        if (cksum_ctx->m->tso_segsz != 0)
        {
            uint16_t payload_len = rte_be_to_cpu_16(ipv6h->payload_len);
            /* API contract: the original header counts for "zero payload". */
            payload_len += first_pkt_pld_size;
            ipv6h->payload_len = rte_cpu_to_be_16(payload_len);
        }

        cksum_ctx->cksum = rte_ipv6_udptcp_cksum(ipv6h,
                                                 bounce_buf_pkt +
                                                 parse_ctx->innermost_l4_ofst);
    }

out:
    free(bounce_buf_pkt);
    free(bounce_buf_pld);

    return rc;
}

static int
rte_mbuf_match_tx_rx_pre(struct rte_mbuf *m)
{
    struct rte_mbuf_cksum_ctx  cksum_ctx = { .m = m };
    struct rte_mbuf_parse_ctx  parse_ctx = { 0 };
    struct rte_ipv4_hdr       *ipv4h;
    struct rte_ipv6_hdr       *ipv6h;
    struct rte_udp_hdr        *udph;
    struct rte_tcp_hdr        *tcph;
    te_errno                   rc;

    rc = rte_mbuf_detect_layers(&parse_ctx, m);
    if (rc != 0)
        return -TE_RC(TE_RPCS, rc);

    /*
     * Spoil the checksums. For IPv4, the "bad" value is 0xffff. For UDP, use
     * 0x0; this means "no checksum" and can be perceived as a "bad" checksum.
     * In the case of TCP, no constant "bad" value exists. Simply take a look
     * at the first packet in the projected Rx burst to calculate the correct
     * value first and after that make it incorrect by incrementing it by one.
     *
     * When the real Rx burst has been received, the comparison API will look
     * at the "bad" values to compare them with the checksums in the first Rx
     * packet to discern effective offloads and proceed with their validation.
     *
     * The API contract demands that the length fields in the header be based
     * on zero L4 payload size. This is needed in the case of TSO. But in the
     * case when TSO is not needed, this is incorrect, so fix the fields here.
     */

    switch (parse_ctx.outer_layers & RTE_PTYPE_L3_MASK)
    {
        case RTE_PTYPE_UNKNOWN:
            break;
        case RTE_PTYPE_L3_IPV6:
        case RTE_PTYPE_L3_IPV6_EXT:
        case RTE_PTYPE_L3_IPV6_EXT_UNKNOWN:
            if (m->tso_segsz == 0)
            {
                ipv6h = rte_pktmbuf_mtod_offset(m, struct rte_ipv6_hdr *,
                                               parse_ctx.outer_l3_ofst);

                ipv6h->payload_len =
                    rte_cpu_to_be_16(parse_ctx.header_size -
                                     parse_ctx.outer_l3_ofst -
                                     sizeof(*ipv6h) + parse_ctx.pld_size);
            }
            break;
        case RTE_PTYPE_L3_IPV4:
        case RTE_PTYPE_L3_IPV4_EXT:
        case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN:
            ipv4h = rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr *,
                                            parse_ctx.outer_l3_ofst);
            ipv4h->hdr_checksum = RTE_BE16(0xffff);
            if (m->tso_segsz == 0)
            {
                ipv4h->total_length =
                    rte_cpu_to_be_16(parse_ctx.header_size -
                                     parse_ctx.outer_l3_ofst +
                                     parse_ctx.pld_size);
            }
            break;
        default:
            ERROR("m: unsupported outer L3");
            return -TE_RC(TE_RPCS, TE_EINVAL);
    }

    switch (parse_ctx.outer_layers & RTE_PTYPE_L4_MASK)
    {
        case RTE_PTYPE_UNKNOWN:
            break;
        case RTE_PTYPE_L4_UDP:
            udph = rte_pktmbuf_mtod_offset(m, struct rte_udp_hdr *,
                                           parse_ctx.outer_l4_ofst);
            udph->dgram_cksum = RTE_BE16(0);
            if (m->tso_segsz == 0)
            {
                udph->dgram_len =
                    rte_cpu_to_be_16(parse_ctx.header_size -
                                     parse_ctx.outer_l4_ofst +
                                     parse_ctx.pld_size);
            }
            break;
        default:
            ERROR("m: unsupported outer L4");
            return -TE_RC(TE_RPCS, TE_EINVAL);
    }

    switch (parse_ctx.innermost_layers & RTE_PTYPE_L3_MASK)
    {
        case RTE_PTYPE_L3_IPV6:
        case RTE_PTYPE_L3_IPV6_EXT:
        case RTE_PTYPE_L3_IPV6_EXT_UNKNOWN:
            if (m->tso_segsz == 0)
            {
                ipv6h = rte_pktmbuf_mtod_offset(m, struct rte_ipv6_hdr *,
                                                parse_ctx.innermost_l3_ofst);

                ipv6h->payload_len =
                    rte_cpu_to_be_16(parse_ctx.header_size -
                                     parse_ctx.innermost_l3_ofst -
                                     sizeof(*ipv6h) + parse_ctx.pld_size);
            }
            cksum_ctx.is_ipv4 = FALSE;
            break;
        case RTE_PTYPE_L3_IPV4:
        case RTE_PTYPE_L3_IPV4_EXT:
        case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN:
            ipv4h = rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr *,
                                            parse_ctx.innermost_l3_ofst);
            ipv4h->hdr_checksum = RTE_BE16(0xffff);
            if (m->tso_segsz == 0)
            {
                ipv4h->total_length =
                    rte_cpu_to_be_16(parse_ctx.header_size -
                                     parse_ctx.innermost_l3_ofst +
                                     parse_ctx.pld_size);
            }
            cksum_ctx.is_ipv4 = TRUE;
            break;
        default:
            ERROR("m: unsupported innermost L3");
            return -TE_RC(TE_RPCS, TE_EINVAL);
    }

    switch (parse_ctx.innermost_layers & RTE_PTYPE_L4_MASK)
    {
        case RTE_PTYPE_L4_TCP:
            rc = rte_mbuf_tcp_first_pkt_get_cksum(&parse_ctx, &cksum_ctx);
            if (rc != 0)
                return -TE_RC(TE_RPCS, rc);

            tcph = rte_pktmbuf_mtod_offset(m, struct rte_tcp_hdr *,
                                           parse_ctx.innermost_l4_ofst);
            tcph->cksum = cksum_ctx.cksum + 1;
            break;
        case RTE_PTYPE_L4_UDP:
            udph = rte_pktmbuf_mtod_offset(m, struct rte_udp_hdr *,
                                           parse_ctx.innermost_l4_ofst);
            udph->dgram_cksum = RTE_BE16(0);
            udph->dgram_len = rte_cpu_to_be_16(parse_ctx.header_size -
                                               parse_ctx.innermost_l4_ofst +
                                               parse_ctx.pld_size);
            break;
        default:
            ERROR("m: unsupported innermost L4");
            return -TE_RC(TE_RPCS, TE_EINVAL);
    }

    return 0;
}

TARPC_FUNC_STATIC(rte_mbuf_match_tx_rx_pre, {},
{
    struct rte_mbuf *m = NULL;

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m, ns);
    });

    MAKE_CALL(out->retval = func(m));
}
)

struct rte_mbuf_cmp_ctx {
    /* This figure corresponds to rx_burst[rx_idx] payload size. */
    size_t           m_rx_pld_size;
    /* It's 0 for rx_idx = 0; otherwise, rx_burst[rx_idx - 1] payload size. */
    size_t           prev_m_rx_pld_size;
    /* Current compare start position inside the original Tx payload. */
    size_t           m_tx_pld_cur_pos;
    /* These 4 fields hold recomputed checksums in the Rx frame. */
    uint16_t         innermost_ip_cksum;
    uint16_t         innermost_l4_cksum;
    uint16_t         outer_udp_cksum;
    uint16_t         outer_ip_cksum;
    /* These 2 fields help to keep track of the comparison loop. */
    unsigned int     rx_idx;
    unsigned int     nb_rx;
    /* This refers to the only Tx mbuf; no const qualifier here. */
    struct rte_mbuf *m_tx;
    /* This refers to rx_burst[rx_idx]; no const qualifier here. */
    struct rte_mbuf *m_rx;
};

/* Fill in cmp_ctx->m_tx and cmp_ctx->m_rx (rx_burst[0]) before invocation. */
static te_errno
rte_mbuf_match_tx_rx_learn(struct rte_mbuf_parse_ctx     *parse_ctx,
                           const struct rte_mbuf_cmp_ctx *cmp_ctx,
                           struct tarpc_rte_mbuf_report  *report)
{
    struct rte_mbuf       *m_tx = cmp_ctx->m_tx;
    const struct rte_mbuf *m_rx = cmp_ctx->m_rx;
    struct rte_tcp_hdr    *tcph_tx;
    struct rte_tcp_hdr    *tcph_rx;
    struct rte_ipv4_hdr   *ipv4h;
    struct rte_udp_hdr    *udph;

    if (m_rx->nb_segs != 1)
    {
        ERROR("rx_burst[0]: multi-seg (unsupported)");
        return TE_EOPNOTSUPP;
    }

    if (m_rx->pkt_len < parse_ctx->header_size)
    {
        ERROR("rx_burst[0]: insufficient data count");
        return TE_ETADLESSDATA;
    }

    if (m_tx->ol_flags & PKT_TX_VLAN_PKT)
    {
        uint64_t rx_vlan_strip = PKT_RX_VLAN | PKT_RX_VLAN_STRIPPED;

        /*
         * Tx VLAN insertion was requested. Assume that Rx VLAN stripping is
         * always on and get the offload status from the Rx meta information.
         */
        if ((m_rx->ol_flags & rx_vlan_strip) == rx_vlan_strip)
                report->ol_vlan = TARPC_RTE_MBUF_OL_DONE;
        else
                report->ol_vlan = TARPC_RTE_MBUF_OL_NOT_DONE;
    }

    switch (parse_ctx->outer_layers & RTE_PTYPE_L3_MASK)
    {
        case RTE_PTYPE_L3_IPV4:
        case RTE_PTYPE_L3_IPV4_EXT:
        case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN:
            ipv4h = rte_pktmbuf_mtod_offset(m_rx, struct rte_ipv4_hdr *,
                                            parse_ctx->outer_l3_ofst);
            if (ipv4h->hdr_checksum == RTE_BE16(0xffff))
            {
                report->ol_outer_ip_cksum = TARPC_RTE_MBUF_OL_NOT_DONE;
            }
            else
            {
                report->ol_outer_ip_cksum = TARPC_RTE_MBUF_OL_DONE;
                ipv4h = rte_pktmbuf_mtod_offset(m_tx, struct rte_ipv4_hdr *,
                                                parse_ctx->outer_l3_ofst);
                ipv4h->hdr_checksum = RTE_BE16(0);
            }
            break;
        default:
            break;
    }

    switch (parse_ctx->outer_layers & RTE_PTYPE_L4_MASK)
    {
        case RTE_PTYPE_L4_UDP:
            udph = rte_pktmbuf_mtod_offset(m_rx, struct rte_udp_hdr *,
                                           parse_ctx->outer_l4_ofst);
            if (udph->dgram_cksum == RTE_BE16(0))
            {
                report->ol_outer_udp_cksum = TARPC_RTE_MBUF_OL_NOT_DONE;
            }
            else
            {
                report->ol_outer_udp_cksum = TARPC_RTE_MBUF_OL_DONE;
                udph = rte_pktmbuf_mtod_offset(m_tx, struct rte_udp_hdr *,
                                               parse_ctx->outer_l4_ofst);
                udph->dgram_cksum = RTE_BE16(0xffff);
            }
            break;
        default:
            break;
    }

    switch (parse_ctx->innermost_layers & RTE_PTYPE_L3_MASK)
    {
        case RTE_PTYPE_L3_IPV4:
        case RTE_PTYPE_L3_IPV4_EXT:
        case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN:
            ipv4h = rte_pktmbuf_mtod_offset(m_rx, struct rte_ipv4_hdr *,
                                            parse_ctx->innermost_l3_ofst);
            if (ipv4h->hdr_checksum == RTE_BE16(0xffff))
            {
                report->ol_innermost_ip_cksum = TARPC_RTE_MBUF_OL_NOT_DONE;
            }
            else
            {
                report->ol_innermost_ip_cksum = TARPC_RTE_MBUF_OL_DONE;
                ipv4h = rte_pktmbuf_mtod_offset(m_tx, struct rte_ipv4_hdr *,
                                                parse_ctx->innermost_l3_ofst);
                ipv4h->hdr_checksum = RTE_BE16(0);
            }
            break;
        default:
            break;
    }

    switch (parse_ctx->innermost_layers & RTE_PTYPE_L4_MASK)
    {
        case RTE_PTYPE_L4_TCP:
            tcph_tx = rte_pktmbuf_mtod_offset(m_tx, struct rte_tcp_hdr *,
                                              parse_ctx->innermost_l4_ofst);
            tcph_rx = rte_pktmbuf_mtod_offset(m_rx, struct rte_tcp_hdr *,
                                              parse_ctx->innermost_l4_ofst);
            if (tcph_rx->cksum == tcph_tx->cksum)
            {
                report->ol_innermost_l4_cksum = TARPC_RTE_MBUF_OL_NOT_DONE;
            }
            else
            {
                report->ol_innermost_l4_cksum = TARPC_RTE_MBUF_OL_DONE;
                tcph_tx->cksum = RTE_BE16(0);
            }
            parse_ctx->tcp_flags = tcph_tx->tcp_flags;
            break;
        case RTE_PTYPE_L4_UDP:
            udph = rte_pktmbuf_mtod_offset(m_rx, struct rte_udp_hdr *,
                                           parse_ctx->innermost_l4_ofst);
            if (udph->dgram_cksum == RTE_BE16(0))
            {
                report->ol_innermost_l4_cksum = TARPC_RTE_MBUF_OL_NOT_DONE;
            }
            else
            {
                report->ol_innermost_l4_cksum = TARPC_RTE_MBUF_OL_DONE;
                udph = rte_pktmbuf_mtod_offset(m_tx, struct rte_udp_hdr *,
                                               parse_ctx->innermost_l4_ofst);
                udph->dgram_cksum = RTE_BE16(0xffff);
            }
            break;
        default:
            break;
    }

    return 0;
}

static int
rte_mbuf_match_tx_rx_cmp_vlan(const struct rte_mbuf_cmp_ctx      *cmp_ctx,
                              const struct tarpc_rte_mbuf_report *report)
{
    uint64_t rx_vlan_strip = PKT_RX_VLAN | PKT_RX_VLAN_STRIPPED;

    if (report->ol_vlan == TARPC_RTE_MBUF_OL_DONE)
    {
        if ((cmp_ctx->m_rx->ol_flags & rx_vlan_strip) != rx_vlan_strip)
        {
            ERROR("rx_burst[%u]: VLAN offload flags mismatch", cmp_ctx->rx_idx);
            return TE_ETADNOTMATCH;
        }
        else if (cmp_ctx->m_rx->vlan_tci != cmp_ctx->m_tx->vlan_tci)
        {
            ERROR("rx_burst[%u]: VLAN TCI mismatch", cmp_ctx->rx_idx);
            return TE_ETADNOTMATCH;
        }
    }
    else
    {
        if ((cmp_ctx->m_rx->ol_flags & rx_vlan_strip) != 0)
        {
            ERROR("rx_burst[%u]: VLAN offload flags mismatch", cmp_ctx->rx_idx);
            return TE_ETADNOTMATCH;
        }
    }

    return 0;
}

/*
 * The caller must make sure that cmp_ctx->m_rx is not multi-seg.
 * At the same time, cmp_ctx->m_tx can be multi-seg.
 */
static te_errno
rte_mbuf_match_tx_rx_cmp_pld(const struct rte_mbuf_parse_ctx *parse_ctx,
                             struct rte_mbuf_cmp_ctx         *cmp_ctx)
{
    const struct rte_mbuf *m_tx = cmp_ctx->m_tx;
    const struct rte_mbuf *m_rx = cmp_ctx->m_rx;
    size_t                 cmp_ofst_tx = parse_ctx->header_size +
                                         cmp_ctx->m_tx_pld_cur_pos;
    size_t                 cmp_ofst_rx = parse_ctx->header_size;
    size_t                 cmp_size_rem = cmp_ctx->m_rx_pld_size;

    while (cmp_size_rem > 0)
    {
        size_t      cmp_size_part;
        uint64_t    bounce_buf_tx;
        const void *m_tx_pld_part;
        const void *m_rx_pld_part;

        cmp_size_part = MIN(cmp_size_rem, sizeof(bounce_buf_tx));

        m_tx_pld_part = rte_pktmbuf_read(m_tx, cmp_ofst_tx, cmp_size_part,
                                         &bounce_buf_tx);
        m_rx_pld_part = rte_pktmbuf_mtod_offset(m_rx, const void *,
                                                cmp_ofst_rx);

        if (memcmp(m_tx_pld_part, m_rx_pld_part, cmp_size_part) != 0)
        {
            ERROR("rx_burst[%u]: payload mismatch", cmp_ctx->rx_idx);
            return TE_ETADNOTMATCH;
        }

        cmp_size_rem -= cmp_size_part;
        cmp_ofst_tx += cmp_size_part;
        cmp_ofst_rx += cmp_size_part;
    }

    cmp_ctx->m_tx_pld_cur_pos += cmp_ctx->m_rx_pld_size;

    return 0;
}

static void
rte_mbuf_recompute_cksums(const struct rte_mbuf_parse_ctx    *parse_ctx,
                          struct rte_mbuf_cmp_ctx            *cmp_ctx,
                          const struct tarpc_rte_mbuf_report *report)
{
    const struct rte_mbuf     *m_rx = cmp_ctx->m_rx;
    const struct rte_ipv4_hdr *ipv4h;
    const struct rte_ipv6_hdr *ipv6h;
    const void                *l4h;

    ipv4h = rte_pktmbuf_mtod_offset(m_rx, const struct rte_ipv4_hdr *,
                                    parse_ctx->outer_l3_ofst);
    ipv6h = rte_pktmbuf_mtod_offset(m_rx, const struct rte_ipv6_hdr *,
                                    parse_ctx->outer_l3_ofst);

    if (report->ol_outer_ip_cksum == TARPC_RTE_MBUF_OL_DONE)
        cmp_ctx->outer_ip_cksum = rte_ipv4_cksum(ipv4h);

    l4h = rte_pktmbuf_mtod_offset(m_rx, const void *, parse_ctx->outer_l4_ofst);

    if (report->ol_outer_udp_cksum == TARPC_RTE_MBUF_OL_DONE)
    {
        if (report->ol_outer_ip_cksum != TARPC_RTE_MBUF_OL_NA)
            cmp_ctx->outer_udp_cksum = rte_ipv4_udptcp_cksum(ipv4h, l4h);
        else
            cmp_ctx->outer_udp_cksum = rte_ipv6_udptcp_cksum(ipv6h, l4h);
    }

    ipv4h = rte_pktmbuf_mtod_offset(m_rx, const struct rte_ipv4_hdr *,
                                    parse_ctx->innermost_l3_ofst);
    ipv6h = rte_pktmbuf_mtod_offset(m_rx, const struct rte_ipv6_hdr *,
                                    parse_ctx->innermost_l3_ofst);

    if (report->ol_innermost_ip_cksum == TARPC_RTE_MBUF_OL_DONE)
        cmp_ctx->innermost_ip_cksum = rte_ipv4_cksum(ipv4h);

    l4h = rte_pktmbuf_mtod_offset(m_rx, const void *,
                                  parse_ctx->innermost_l4_ofst);

    if (report->ol_innermost_l4_cksum == TARPC_RTE_MBUF_OL_DONE)
    {
        if (report->ol_innermost_ip_cksum != TARPC_RTE_MBUF_OL_NA)
            cmp_ctx->innermost_l4_cksum = rte_ipv4_udptcp_cksum(ipv4h, l4h);
        else
            cmp_ctx->innermost_l4_cksum = rte_ipv6_udptcp_cksum(ipv6h, l4h);
    }
}

static void
rte_mbuf_apply_edits(const struct rte_mbuf_parse_ctx    *parse_ctx,
                     const struct rte_mbuf_cmp_ctx      *cmp_ctx,
                     const struct tarpc_rte_mbuf_report *report)
{
    struct rte_mbuf     *m_tx = cmp_ctx->m_tx;
    struct rte_mbuf     *m_rx = cmp_ctx->m_rx;
    uint16_t             ipv4h_packet_id;
    uint32_t             tcph_sent_seq;
    struct rte_ipv4_hdr *ipv4h;
    struct rte_ipv6_hdr *ipv6h;
    struct rte_udp_hdr  *udph;
    struct rte_tcp_hdr  *tcph;

    switch (parse_ctx->outer_layers & RTE_PTYPE_L3_MASK)
    {
        case RTE_PTYPE_L3_IPV4:
        case RTE_PTYPE_L3_IPV4_EXT:
        case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN:
            ipv4h = rte_pktmbuf_mtod_offset(m_rx, struct rte_ipv4_hdr *,
                                            parse_ctx->outer_l3_ofst);

            if (report->ol_outer_ip_cksum == TARPC_RTE_MBUF_OL_DONE)
                ipv4h->hdr_checksum = cmp_ctx->outer_ip_cksum;

            ipv4h = rte_pktmbuf_mtod_offset(m_tx, struct rte_ipv4_hdr *,
                                            parse_ctx->outer_l3_ofst);
            ipv4h_packet_id = rte_be_to_cpu_16(ipv4h->packet_id);
            ipv4h_packet_id += (cmp_ctx->rx_idx != 0) ? 1 : 0;
            ipv4h->packet_id = rte_cpu_to_be_16(ipv4h_packet_id);
            ipv4h->total_length = rte_cpu_to_be_16(parse_ctx->header_size -
                                                   parse_ctx->outer_l3_ofst +
                                                   cmp_ctx->m_rx_pld_size);
            break;
        case RTE_PTYPE_L3_IPV6:
        case RTE_PTYPE_L3_IPV6_EXT:
        case RTE_PTYPE_L3_IPV6_EXT_UNKNOWN:
            ipv6h = rte_pktmbuf_mtod_offset(m_tx, struct rte_ipv6_hdr *,
                                            parse_ctx->outer_l3_ofst);
            ipv6h->payload_len = rte_cpu_to_be_16(parse_ctx->header_size -
                                                  parse_ctx->outer_l3_ofst -
                                                  sizeof(*ipv6h) +
                                                  cmp_ctx->m_rx_pld_size);
            break;
        default:
            break;
    }

    switch (parse_ctx->outer_layers & RTE_PTYPE_L4_MASK)
    {
        case RTE_PTYPE_L4_UDP:
            udph = rte_pktmbuf_mtod_offset(m_rx, struct rte_udp_hdr *,
                                           parse_ctx->outer_l4_ofst);
            if (report->ol_outer_udp_cksum == TARPC_RTE_MBUF_OL_DONE)
                udph->dgram_cksum = cmp_ctx->outer_udp_cksum;

            udph = rte_pktmbuf_mtod_offset(m_tx, struct rte_udp_hdr *,
                                           parse_ctx->outer_l4_ofst);
            udph->dgram_len = rte_cpu_to_be_16(parse_ctx->header_size -
                                               parse_ctx->outer_l4_ofst +
                                               cmp_ctx->m_rx_pld_size);
            break;
        default:
            break;
    }

    switch (parse_ctx->innermost_layers & RTE_PTYPE_L3_MASK)
    {
        case RTE_PTYPE_L3_IPV4:
        case RTE_PTYPE_L3_IPV4_EXT:
        case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN:
            ipv4h = rte_pktmbuf_mtod_offset(m_rx, struct rte_ipv4_hdr *,
                                            parse_ctx->innermost_l3_ofst);

            if (report->ol_innermost_ip_cksum == TARPC_RTE_MBUF_OL_DONE)
                ipv4h->hdr_checksum = cmp_ctx->innermost_ip_cksum;

            ipv4h = rte_pktmbuf_mtod_offset(m_tx, struct rte_ipv4_hdr *,
                                            parse_ctx->innermost_l3_ofst);
            ipv4h_packet_id = rte_be_to_cpu_16(ipv4h->packet_id);
            ipv4h_packet_id += (cmp_ctx->rx_idx != 0) ? 1 : 0;
            ipv4h->packet_id = rte_cpu_to_be_16(ipv4h_packet_id);
            ipv4h->total_length =
                rte_cpu_to_be_16(parse_ctx->header_size -
                                 parse_ctx->innermost_l3_ofst +
                                 cmp_ctx->m_rx_pld_size);
            break;
        case RTE_PTYPE_L3_IPV6:
        case RTE_PTYPE_L3_IPV6_EXT:
        case RTE_PTYPE_L3_IPV6_EXT_UNKNOWN:
            ipv6h = rte_pktmbuf_mtod_offset(m_tx, struct rte_ipv6_hdr *,
                                            parse_ctx->innermost_l3_ofst);
            ipv6h->payload_len = rte_cpu_to_be_16(parse_ctx->header_size -
                                                  parse_ctx->innermost_l3_ofst -
                                                  sizeof(*ipv6h) +
                                                  cmp_ctx->m_rx_pld_size);
            break;
        default:
            break;
    }

    switch (parse_ctx->innermost_layers & RTE_PTYPE_L4_MASK)
    {
        case RTE_PTYPE_L4_TCP:
            tcph = rte_pktmbuf_mtod_offset(m_rx, struct rte_tcp_hdr *,
                                           parse_ctx->innermost_l4_ofst);
            if (report->ol_innermost_l4_cksum == TARPC_RTE_MBUF_OL_DONE)
                tcph->cksum = cmp_ctx->innermost_l4_cksum;

            tcph = rte_pktmbuf_mtod_offset(m_tx, struct rte_tcp_hdr *,
                                           parse_ctx->innermost_l4_ofst);
            tcph_sent_seq = rte_be_to_cpu_32(tcph->sent_seq);
            tcph_sent_seq += cmp_ctx->prev_m_rx_pld_size;
            tcph->sent_seq = rte_cpu_to_be_32(tcph_sent_seq);

            tcph->tcp_flags = parse_ctx->tcp_flags;

            if (cmp_ctx->rx_idx != 0)
                tcph->tcp_flags &= ~RTE_TCP_CWR_FLAG;

            if (cmp_ctx->rx_idx + 1 != cmp_ctx->nb_rx)
                tcph->tcp_flags &= ~(RTE_TCP_FIN_FLAG | RTE_TCP_PSH_FLAG);
            break;
        case RTE_PTYPE_L4_UDP:
            udph = rte_pktmbuf_mtod_offset(m_rx, struct rte_udp_hdr *,
                                           parse_ctx->innermost_l4_ofst);
            if (report->ol_innermost_l4_cksum == TARPC_RTE_MBUF_OL_DONE)
                udph->dgram_cksum = cmp_ctx->innermost_l4_cksum;

            udph = rte_pktmbuf_mtod_offset(m_tx, struct rte_udp_hdr *,
                                           parse_ctx->innermost_l4_ofst);
            udph->dgram_len = rte_cpu_to_be_16(parse_ctx->header_size -
                                               parse_ctx->innermost_l4_ofst +
                                               cmp_ctx->m_rx_pld_size);
            break;
        default:
            break;
    }
}

/*
 * The caller must make sure that cmp_ctx->m_tx
 * and cmp_ctx->m_rx have contiguous headers.
 */
static te_errno
rte_mbuf_match_tx_rx_cmp_headers(const struct rte_mbuf_parse_ctx *parse_ctx,
                                 const struct rte_mbuf_cmp_ctx   *cmp_ctx)
{
    const struct rte_mbuf *m_tx = cmp_ctx->m_tx;
    const struct rte_mbuf *m_rx = cmp_ctx->m_rx;

    if (memcmp(rte_pktmbuf_mtod(m_tx, const void *),
               rte_pktmbuf_mtod(m_rx, const void *),
               parse_ctx->header_size) != 0)
    {
        unsigned int i;

        for (i = 0; i < parse_ctx->header_size; ++i)
        {
            uint8_t *m_byte_txp = rte_pktmbuf_mtod_offset(m_tx, uint8_t *, i);
            uint8_t *m_byte_rxp = rte_pktmbuf_mtod_offset(m_rx, uint8_t *, i);

            if (*m_byte_txp != *m_byte_rxp)
            {
                ERROR("rx_burst[%u]: header mismatch on byte %u",
                      cmp_ctx->rx_idx, i);
            }
        }

        return TE_ETADNOTMATCH;
    }

    return 0;
}

static te_errno
rte_mbuf_match_tx_rx_cmp(const struct rte_mbuf_parse_ctx *parse_ctx,
                         struct rte_mbuf_cmp_ctx         *cmp_ctx,
                         struct tarpc_rte_mbuf_report    *report)
{
    const struct rte_mbuf *m_tx = cmp_ctx->m_tx;
    const struct rte_mbuf *m_rx = cmp_ctx->m_rx;
    size_t                 m_rx_pld_size_min;
    size_t                 m_rx_pld_size_exp;
    te_errno               rc;

    rc = rte_mbuf_match_tx_rx_cmp_vlan(cmp_ctx, report);
    if (rc != 0)
        return rc;

    if (m_rx->nb_segs != 1)
    {
        ERROR("rx_burst[%u]: multi-seg (unsupported)", cmp_ctx->rx_idx);
        return TE_EOPNOTSUPP;
    }

    if (m_tx->tso_segsz != 0)
    {
        m_rx_pld_size_min = (parse_ctx->pld_size != 0) ? 1 : 0;
        m_rx_pld_size_exp = MIN(parse_ctx->pld_size - cmp_ctx->m_tx_pld_cur_pos,
                                m_tx->tso_segsz);
    }
    else
    {
        m_rx_pld_size_min = parse_ctx->pld_size - cmp_ctx->m_tx_pld_cur_pos;
        m_rx_pld_size_exp = parse_ctx->pld_size - cmp_ctx->m_tx_pld_cur_pos;
    }

    if (m_rx_pld_size_exp == 0)
    {
        ERROR("rx_burst[%u]: unexpected (excess) packet", cmp_ctx->rx_idx);
        return TE_ETADNOTMATCH;
    }

    if (m_rx->pkt_len < parse_ctx->header_size + m_rx_pld_size_min)
    {
        ERROR("rx_burst[%u]: insufficient data count (%u bytes); must be%s%zu bytes",
              cmp_ctx->rx_idx, m_rx->pkt_len,
              (m_tx->tso_segsz != 0) ? " at least " : " ",
              parse_ctx->header_size + m_rx_pld_size_min);
        return TE_ETADLESSDATA;
    }

    cmp_ctx->prev_m_rx_pld_size = cmp_ctx->m_rx_pld_size;
    cmp_ctx->m_rx_pld_size = MIN(m_rx->pkt_len - parse_ctx->header_size,
                                 m_rx_pld_size_exp);

    if (cmp_ctx->m_rx_pld_size != m_rx_pld_size_exp)
    {
        size_t next_m_tx_data_pos = parse_ctx->header_size +
                                    cmp_ctx->m_tx_pld_cur_pos +
                                    cmp_ctx->m_rx_pld_size;

        if (report->tso_cutoff_barrier == 0)
        {
            report->tso_cutoff_barrier = next_m_tx_data_pos;
        }
        else if (next_m_tx_data_pos % report->tso_cutoff_barrier != 0)
        {
            ERROR("rx_burst[%u]: inconsistent repeating TSO cutoff barrier",
                  cmp_ctx->rx_idx);
            return TE_ETADNOTMATCH;
        }
    }

    /* Compare the two payloads. */
    rc = rte_mbuf_match_tx_rx_cmp_pld(parse_ctx, cmp_ctx);
    if (rc != 0)
        return rc;

    /*
     * Recompute checksums (if need be) in the received frame.
     * If a checksum is correct, the new value will be either
     * 0xffff or 0x0, depending on the particular header type.
     */
    rte_mbuf_recompute_cksums(parse_ctx, cmp_ctx, report);

    /*
     * Insert new checksum values (if need be) to the Rx mbuf.
     * Apply required TSO edits to the comparison (Tx) header.
     */
    rte_mbuf_apply_edits(parse_ctx, cmp_ctx, report);

    /* Compare the two headers. */
    rc = rte_mbuf_match_tx_rx_cmp_headers(parse_ctx, cmp_ctx);
    if (rc != 0)
        return rc;

    return 0;
}

static int
rte_mbuf_match_tx_rx(struct tarpc_rte_mbuf_match_tx_rx_in  *in,
                     struct tarpc_rte_mbuf_match_tx_rx_out *out)

{
    struct rte_mbuf_parse_ctx     parse_ctx = { 0 };
    struct rte_mbuf_cmp_ctx       cmp_ctx = { 0 };
    struct tarpc_rte_mbuf_report  report = { 0 };
    unsigned int                  nb_rx_min;
    struct rte_mbuf              *m_tx;
    struct rte_mbuf              *m_rx;
    te_errno                      rc;
    unsigned int                  i;

    if (in->rx_burst.rx_burst_len == 0)
    {
        ERROR("rx_burst: empty");
        return -TE_RC(TE_RPCS, TE_EINVAL);
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
        m_rx = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->rx_burst.rx_burst_val[0], ns);
        m_tx = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->m_tx, ns);
    });

    /* This also makes sure the mbuf is not multi-seg. */
    rc = rte_mbuf_detect_layers(&parse_ctx, m_tx);
    if (rc != 0)
        return -TE_RC(TE_RPCS, rc);

    if (m_tx->tso_segsz != 0)
        nb_rx_min = TE_DIV_ROUND_UP(parse_ctx.pld_size, m_tx->tso_segsz);
    else
        nb_rx_min = 1;

    nb_rx_min = MAX(nb_rx_min, 1);

    if (in->rx_burst.rx_burst_len < nb_rx_min)
    {
        ERROR("rx_burst: wrong packet count (%u); must be at least %u",
              in->rx_burst.rx_burst_len, nb_rx_min);
        return -TE_RC(TE_RPCS, TE_ETADNOTMATCH);
    }

    cmp_ctx.nb_rx = in->rx_burst.rx_burst_len;

    cmp_ctx.m_tx = m_tx;
    cmp_ctx.m_rx = m_rx;

    /*
     * Find out whether Tx VLAN and checksum offloads have happened. To do this,
     * consider the first mbuf in the Rx burst. A checksum offload is deemed to
     * have been done if the checksum value is different from the "spoiled" one.
     * If this is the case, rewrite the corresponding field in the Tx header so
     * that it will match its counterpart in the Rx header after the latter has
     * been recomputed over a buffer containing correct checksum value in place.
     */
    rc = rte_mbuf_match_tx_rx_learn(&parse_ctx, &cmp_ctx, &report);
    if (rc != 0)
        return -TE_RC(TE_RPCS, rc);

    /* Conduct the comparison. TSO edits are taken care of internally. */
    for (i = 0; i < cmp_ctx.nb_rx; ++i)
    {
        struct rte_mbuf *m;

        RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_RTE_MBUF, {
            m = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->rx_burst.rx_burst_val[i], ns);
        });

        cmp_ctx.rx_idx = i;
        cmp_ctx.m_rx = m;

        /* This also conducts necessary checks on the Rx mbuf internally. */
        rc = rte_mbuf_match_tx_rx_cmp(&parse_ctx, &cmp_ctx, &report);
        if (rc != 0)
            return -TE_RC(TE_RPCS, rc);
    }

    memcpy(&out->report, &report, sizeof(report));

    return 0;
}

TARPC_FUNC_STATIC(rte_mbuf_match_tx_rx, {},
{
    MAKE_CALL(out->retval = func(in, out));
}
)
