/** @file
 * @brief RPC for RTE mbuf CSAP layer API
 *
 * RPC routines to access RTE mbuf CSAP layer functionality
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER     "RPC RTE mbuf layer"

#include "te_config.h"
#ifdef HAVE_PACKAGE_H
#include "package.h"
#endif

#include "rte_config.h"
#include "rte_mbuf.h"
#include "rte_ring.h"

#include "asn_usr.h"
#include "asn_impl.h"
#include "ndn_rte_mbuf.h"
#include "tad_utils.h"
#include "tad_api.h"
#include "tad_csap_inst.h"

#include "logger_api.h"

#include "rpc_server.h"
#include "rpcs_dpdk.h"
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
    rc = rte_ring_dequeue_bulk(ring, (void **)mbufs, out->mbufs.mbufs_len,
                               NULL);
    neg_errno_h2rpc(&rc);
    if (rc != 0)
        goto out;

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
    rc = rte_ring_enqueue_bulk(ring, (void **)mbufs, in->mbufs.mbufs_len, NULL);
    neg_errno_h2rpc(&rc);
    if (rc != 0)
        goto out;

    /* Shove the pattern into the CSAP */
    rc = tad_recv_start_prepare(csap_instance, in->pattern, in->mbufs.mbufs_len,
                                TAD_TIMEOUT_INF, RCF_CH_TRRECV_PACKETS, &reply_ctx);
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

