/** @file
 * @brief Tester API for NDN
 *
 * Implementation of Tester API for NDN.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */ 

#define TE_LGR_USER     "TAPI NDN"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H 
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_stdint.h"
#include "te_string.h"
#include "te_errno.h"
#include "logger_api.h"
#include "asn_impl.h"
#include "ndn.h"
#include "ndn_eth.h"
#include "ndn_ipstack.h"

#include "tad_common.h"

#include "tapi_ndn.h"
#include "tapi_tcp.h"


/* See the description in tapi_ndn.h */
te_errno
tapi_tad_init_asn_value(asn_value **value, const asn_type *type)
{
    return ndn_init_asn_value(value, type);
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_csap_add_layer(asn_value       **csap_spec,
                        const asn_type   *layer_type,
                        const char       *layer_choice,
                        asn_value       **layer_spec)
{
    return ndn_csap_add_layer(csap_spec, layer_type, layer_choice,
                              layer_spec);
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_new_ptrn_unit(asn_value **obj_spec, asn_value **unit_spec)
{
    te_errno    rc;
    asn_value  *unit;
    asn_value  *pdus;

    rc = tapi_tad_init_asn_value(obj_spec, ndn_traffic_pattern);
    if (rc != 0)
        return rc;

    unit = asn_init_value(ndn_traffic_pattern_unit);
    if (unit == NULL)
    {
        ERROR("Failed to initialize traffic pattern unit");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = asn_insert_indexed(*obj_spec, unit, 0, "");
    if (rc != 0)
    {
        ERROR("Failed to add a new unit in traffic pattern: "
              "%r", rc);
        asn_free_value(unit);
        return TE_RC(TE_TAPI, rc);
    }

    pdus = asn_init_value(ndn_generic_pdu_sequence);
    if (pdus == NULL)
    {
        ERROR("Failed to initiaze ASN.1 value for generic PDUs "
              "sequence");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = asn_put_child_value_by_label(unit, pdus, "pdus");
    if (rc != 0)
    {
        ERROR("Failed to put 'pdus' in ASN.1 value: %r", rc);
        asn_free_value(pdus);
        return rc;
    }

    if (unit_spec != NULL)
        *unit_spec = unit;

    return 0;
}


/* See the description in tapi_ndn.h */
static te_errno
tapi_tad_tmpl_ptrn_get_unit(asn_value **obj_spec,
                            te_bool     is_pattern,
                            asn_value **unit_spec)
{
    te_errno    rc;

    assert(unit_spec != NULL);

    /*
     * Check the root object and initialize it, if it is necessary.
     */
    rc = tapi_tad_init_asn_value(obj_spec, (is_pattern) ?
                                               ndn_traffic_pattern :
                                               ndn_traffic_template);
    if (rc != 0)
        return rc;

    /*
     * Get traffic template/pattern unit or create a new
     */
    if (is_pattern)
    {
        int len = asn_get_length(*obj_spec, "");

        if (len < 0)
        {
            ERROR("%s(): asn_get_length() failed unexpectedly: %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (len == 0)
        {
            rc = tapi_tad_new_ptrn_unit(obj_spec, unit_spec);
            if (rc != 0)
                return TE_RC(TE_TAPI, rc);
        }
        else
        {
            rc = asn_get_indexed(*obj_spec, unit_spec, len - 1, NULL);
            if (rc != 0)
            {
                ERROR("Failed to get ASN.1 value by index %d: %r",
                      len - 1, rc);
                return TE_RC(TE_TAPI, rc);
            }
        }
    }
    else
    {
        *unit_spec = *obj_spec;
    }

    return 0;
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_tmpl_ptrn_add_layer(asn_value       **obj_spec,
                             te_bool           is_pattern,
                             const asn_type   *pdu_type,
                             const char       *pdu_choice,
                             asn_value       **pdu_spec)
{
    te_errno    rc;
    asn_value  *unit_spec;
    asn_value  *pdus;
    asn_value  *gen_pdu;
    asn_value  *pdu;

    if (pdu_type == NULL || pdu_choice == NULL)
    {
        ERROR("%s(): ASN.1 type of the layer have to be specified",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /*
     * Check the root object and initialize it, if it is necessary.
     */
    rc = tapi_tad_tmpl_ptrn_get_unit(obj_spec, is_pattern, &unit_spec);
    if (rc != 0)
        return rc;

    /*
     * Get or create PDUs sequence
     */
    /* FIXME: Remove type cast */
    rc = asn_get_child_value(unit_spec, (const asn_value **)&pdus,
                             PRIVATE, (is_pattern) ? NDN_PU_PDUS :
                                                     NDN_TMPL_PDUS);
    if (rc == TE_EASNINCOMPLVAL)
    {
        pdus = asn_init_value(ndn_generic_pdu_sequence);
        if (pdus == NULL)
        {
            ERROR("Failed to initiaze ASN.1 value for generic PDUs "
                  "sequence");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
        rc = asn_put_child_value_by_label(unit_spec, pdus, "pdus");
        if (rc != 0)
        {
            ERROR("Failed to put 'pdus' in ASN.1 value: %r", rc);
            asn_free_value(pdus);
            return rc;
        }
    }
    else if (rc != 0)
    {
        ERROR("Failed to get 'pdus' from ASN.1 value: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    /*
     * Create a new generic PDU and insert it in PDUs sequence as the
     * last
     */
    gen_pdu = asn_init_value(ndn_generic_pdu);
    if (gen_pdu == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for generic PDU");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = asn_insert_indexed(pdus, gen_pdu, -1, "");
    if (rc != 0)
    {
        ERROR("Failed to add a new generic PDU in sequence: %r", rc);
        asn_free_value(gen_pdu);
        return TE_RC(TE_TAPI, rc);
    }
    
    pdu = asn_init_value(pdu_type);
    if (pdu == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for PDU by type");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = asn_put_child_value_by_label(gen_pdu, pdu, pdu_choice);
    if (rc != 0)
    {
        ERROR("Failed to put PDU as choice of generic PDU: %r", rc);
        asn_free_value(pdu);
        return rc;
    }

    if (pdu_spec != NULL)
        *pdu_spec = pdu;

    return 0;
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_tmpl_ptrn_set_payload_plain(asn_value  **obj_spec,
                                     te_bool      is_pattern,
                                     const void  *payload,
                                     size_t       length)
{
    te_errno    rc;
    asn_value  *unit_spec;

    rc = tapi_tad_tmpl_ptrn_get_unit(obj_spec, is_pattern, &unit_spec);
    if (rc != 0)
        return rc;

    if (payload == NULL && length != 0)
        rc = asn_write_int32(unit_spec, length, "payload.#length");
    else
        rc = asn_write_value_field(unit_spec, payload, length,
                                   "payload.#bytes");
    if (rc != 0)
    {
        ERROR("Failed to set payload: %r", rc);
    }

    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_pdus_free_fields_by_du_tag(asn_value      *pdus,
                                asn_tag_value   du_tag)
{
    te_errno        err;
    unsigned int    i;

    if (pdus == NULL)
    {
        err = TE_EINVAL;
        goto fail;
    }

    for (i = 0; i < (unsigned int)pdus->len; ++i)
    {
        asn_value      *pdu;
        asn_value      *pdu_choice_val;
        unsigned int    j;

        err = asn_get_indexed(pdus, &pdu, i, "");
        if (err != 0)
            goto fail;

        err = asn_get_choice_value(pdu, &pdu_choice_val, NULL, NULL);
        if (err != 0)
            goto fail;

        for (j = 0; j < (unsigned int)pdu_choice_val->len; ++j)
        {
            asn_value      *pdu_field = pdu_choice_val->data.array[j];
            asn_tag_value   pdu_field_sub_tag_value;

            if ((pdu_field == NULL) || (pdu_field->syntax != CHOICE))
                continue;

            err = asn_get_choice_value(pdu_field, NULL, NULL,
                                       &pdu_field_sub_tag_value);
            if (err != 0)
                goto fail;

            if (pdu_field_sub_tag_value == du_tag)
            {
                err = asn_free_child(pdu_choice_val, PRIVATE,
                                     asn_get_tag(pdu_field));
                if (err != 0)
                    goto fail;
            }
        }
    }

    return 0;

fail:
    return TE_RC(TE_TAPI, err);
}

static const int tunnel_types[] = {
    TE_PROTO_VXLAN,
    TE_PROTO_GENEVE,
    TE_PROTO_GRE,
};

te_errno
tapi_tad_pdus_relist_outer_inner(asn_value      *pdu_seq,
                                 unsigned int   *nb_pdus_o_out,
                                 asn_value    ***pdus_o_out,
                                 unsigned int   *nb_pdus_i_out,
                                 asn_value    ***pdus_i_out)
{
    int            nb_pdus;
    int            pdu_index_tunnel;
    unsigned int   nb_pdus_o;
    asn_value    **pdus_o = NULL;
    unsigned int   nb_pdus_i;
    asn_value    **pdus_i = NULL;
    unsigned int   i, j;
    te_errno       rc;

    nb_pdus = asn_get_length(pdu_seq, "");
    if (nb_pdus < 0)
    {
        rc = TE_EINVAL;
        goto fail;
    }

    pdu_index_tunnel = -1;

    for (i = 0; i < TE_ARRAY_LEN(tunnel_types); ++i)
    {
        asn_child_desc_t *items = NULL;
        unsigned int      nb = 0;

        rc = asn_find_child_choice_values(pdu_seq, tunnel_types[i],
                                          &items, &nb);
        if (rc != 0)
            goto fail;

        if (nb > 1)
        {
            rc = TE_EINVAL;
            goto fail;
        }

        if (nb == 1)
        {
            pdu_index_tunnel = items[0].index;
            break;
        }
    }

    if (nb_pdus_i_out != NULL && pdus_i_out != NULL && pdu_index_tunnel > 0)
    {
        nb_pdus_i = pdu_index_tunnel;
        pdus_i = TE_ALLOC(nb_pdus_i * sizeof(*pdus_i));
        if (pdus_i == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        for (i = 0; i < (unsigned int)pdu_index_tunnel; ++i)
        {
            asn_value *pdu_i = NULL;

            rc = asn_get_indexed(pdu_seq, &pdu_i, i, "");
            if (rc != 0)
                goto fail;

            pdus_i[i] = pdu_i;
        }
    }
    else
    {
        nb_pdus_i = 0;
    }

    if (nb_pdus_o_out != NULL && pdus_o_out != NULL)
    {
        nb_pdus_o = nb_pdus - (pdu_index_tunnel + 1);
        pdus_o = TE_ALLOC(nb_pdus_o * sizeof(*pdus_o));
        if (pdus_o == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }

        for (i = pdu_index_tunnel + 1, j = 0; i < (unsigned int)nb_pdus; ++i, ++j)
        {
            asn_value *pdu_o = NULL;

            rc = asn_get_indexed(pdu_seq, &pdu_o, i, "");
            if (rc != 0)
                goto fail;

            pdus_o[j] = pdu_o;
        }

        *nb_pdus_o_out = nb_pdus_o;
        *pdus_o_out = pdus_o;
    }

    if (nb_pdus_i_out != NULL && pdus_i_out != NULL) {
        *nb_pdus_i_out = nb_pdus_i;
        *pdus_i_out = pdus_i;
    }

    return 0;

fail:
    free(pdus_o);
    free(pdus_i);

    return rc;
}

/* See description in 'tapi_ndn.h' */
te_errno
tapi_tad_tmpl_relist_outer_inner_pdus(asn_value      *tmpl,
                                      unsigned int   *nb_pdus_o_out,
                                      asn_value    ***pdus_o_out,
                                      unsigned int   *nb_pdus_i_out,
                                      asn_value    ***pdus_i_out)
{
    asn_value *pdu_seq = NULL;
    te_errno   rc;

    rc = asn_get_subvalue(tmpl, &pdu_seq, "pdus");
    if (rc != 0)
        return rc;

    return tapi_tad_pdus_relist_outer_inner(pdu_seq, nb_pdus_o_out, pdus_o_out,
                                            nb_pdus_i_out, pdus_i_out);
}

/* See the description in tapi_ndn.h */
asn_value *
tapi_tad_mk_pattern_from_template(asn_value  *template)
{
    te_errno          err;
    asn_value        *pattern = NULL;
    asn_value        *pattern_unit = NULL;
    asn_value        *pdus = NULL;
    asn_value        *pdus_copy = NULL;
    asn_child_desc_t *pdus_ip4 = NULL;
    unsigned int      nb_pdus_ip4;
    asn_child_desc_t *pdus_udp = NULL;
    unsigned int      nb_pdus_udp;
    asn_value        *pdu_tcp;
    unsigned int      i;

    pattern = asn_init_value(ndn_traffic_pattern);
    if (pattern == NULL)
        goto fail;

    pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    if (pattern_unit == NULL)
        goto fail;

    err = asn_insert_indexed(pattern, pattern_unit, -1, "");
    if (err != 0)
    {
        asn_free_value(pattern_unit);
        goto fail;
    }

    err = asn_get_subvalue(template, &pdus, "pdus");
    if (err != 0 || pdus == NULL)
        goto fail;

    pdus_copy = asn_copy_value(pdus);
    if (pdus_copy == NULL)
        goto fail;

    err = asn_put_child_value(pattern_unit, pdus_copy, PRIVATE, NDN_PU_PDUS);
    if (err != 0)
    {
        asn_free_value(pdus_copy);
        goto fail;
    }

    err = asn_find_child_choice_values(pdus_copy, TE_PROTO_IP4,
                                       &pdus_ip4, &nb_pdus_ip4);
    if (err != 0)
        goto fail;


    for (i = 0; i < nb_pdus_ip4; ++i)
    {
        err = asn_free_child(pdus_ip4[i].value, PRIVATE,
                             NDN_TAG_IP4_H_CHECKSUM);
        err = (err == TE_EASNWRONGLABEL) ? 0 : err;
        if (err != 0)
            goto fail;
    }

    err = asn_find_child_choice_values(pdus_copy, TE_PROTO_UDP,
                                       &pdus_udp, &nb_pdus_udp);
    for (i = 0; i < nb_pdus_udp; ++i)
    {
        err = asn_free_child(pdus_udp[i].value, PRIVATE, NDN_TAG_UDP_CHECKSUM);
        err = (err == TE_EASNWRONGLABEL) ? 0 : err;
        if (err != 0)
            goto fail;
    }

    pdu_tcp = asn_find_child_choice_value(pdus_copy, TE_PROTO_TCP);
    if (pdu_tcp != NULL)
    {
        err = asn_free_child(pdu_tcp, PRIVATE, NDN_TAG_TCP_CHECKSUM);
        err = (err == TE_EASNWRONGLABEL) ? 0 : err;
        if (err != 0)
            goto fail;
    }

    err = tapi_pdus_free_fields_by_du_tag(pdus_copy, NDN_DU_SCRIPT);
    if (err != 0)
        goto fail;

    free(pdus_ip4);
    free(pdus_udp);

    return pattern;

fail:
    asn_free_value(pattern);
    free(pdus_ip4);
    free(pdus_udp);

    return NULL;
}

static te_errno
tapi_tad_tso_seg_fix_ip4h(asn_value      *ip4_pdu,
                          size_t          payload_len,
                          size_t          seg_len,
                          unsigned int    ipid_incr)
{
    te_errno    err = 0;
    uint16_t    tot_len;
    size_t      tot_len_size = sizeof(tot_len);
    uint16_t    ip_id;
    size_t      ip_id_size = sizeof(ip_id);

    err = asn_read_value_field(ip4_pdu, &tot_len, &tot_len_size,
                               "total-length.#plain");
    if (err != 0)
        goto out;

    tot_len -= payload_len;
    tot_len += seg_len;

    err = asn_write_value_field(ip4_pdu, &tot_len, sizeof(tot_len),
                                "total-length.#plain");
    if (err != 0)
        goto out;

    err = asn_read_value_field(ip4_pdu, &ip_id, &ip_id_size, "ip-ident.#plain");
    if (err != 0)
        goto out;

    ip_id += ipid_incr;

    err = asn_write_value_field(ip4_pdu, &ip_id, sizeof(ip_id),
                                "ip-ident.#plain");
    if (err != 0)
        goto out;

out:
    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_tso_seg_fix_ip6h(asn_value  *ip6_pdu,
                          size_t      payload_len,
                          size_t      seg_len)
{
    te_errno    err = 0;
    uint16_t    ip6_p_len;
    size_t      ip6_p_len_size = sizeof(ip6_p_len);

    err = asn_read_value_field(ip6_pdu, &ip6_p_len, &ip6_p_len_size,
                               "payload-length.#plain");
    if (err != 0)
        goto out;

    ip6_p_len -= payload_len;
    ip6_p_len += seg_len;

    err = asn_write_value_field(ip6_pdu, &ip6_p_len, sizeof(ip6_p_len),
                                "payload-length.#plain");
    if (err != 0)
        goto out;

out:
    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_tso_seg_fix_tcph(asn_value      *tcp_pdu,
                          size_t          payload_len,
                          size_t          seg_len,
                          uint32_t        seg_offset)
{
    te_errno    err = 0;
    uint32_t    seqn;
    size_t      seqn_size = sizeof(seqn);

    err = asn_read_value_field(tcp_pdu, &seqn, &seqn_size, "seqn.#plain");
    if (err != 0)
        goto out;

    seqn += seg_offset;

    err = asn_write_value_field(tcp_pdu, &seqn, sizeof(seqn), "seqn.#plain");
    if (err != 0)
        goto out;

    if ((seg_offset + seg_len) != payload_len)
    {
        uint8_t     tcp_flags;
        size_t      tcp_flags_size = sizeof(tcp_flags);

        err = asn_read_value_field(tcp_pdu, &tcp_flags,
                                   &tcp_flags_size, "flags.#plain");
        if (err != 0)
            goto out;

        tcp_flags &= ~(TCP_FIN_FLAG | TCP_PSH_FLAG);

        err = asn_write_value_field(tcp_pdu, &tcp_flags,
                                    sizeof(tcp_flags), "flags.#plain");
        if (err != 0)
            goto out;
    }

out:
    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_tso_seg_fix_udph(asn_value *udp_pdu,
                          size_t     payload_len,
                          size_t     seg_len)
{
    te_errno err = 0;
    uint16_t len;
    size_t   len_size = sizeof(len);

    err = asn_read_value_field(udp_pdu, &len, &len_size, "length.#plain");
    if (err != 0)
        goto out;

    len -= payload_len;
    len += seg_len;

    err = asn_write_value_field(udp_pdu, &len, sizeof(len), "length.#plain");

out:
    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_set_cksum_script_correct(asn_value  *proto_pdu,
                                  char       *du_cksum_label,
                                  te_bool     accept_zero_cksum)
{
    te_errno    err = 0;
    int         du_cksum_index = -1;
    const char *rest_labels;
    char       *du_cksum_label_choice = NULL;
    size_t      du_cksum_label_choice_len;
    char        choice_script_postfix[] = ".#script";

    if (proto_pdu == NULL || du_cksum_label == NULL)
    {
        err = TE_EWRONGPTR;
        goto out;
    }

    err = asn_child_named_index(proto_pdu->asn_type, du_cksum_label,
                                &du_cksum_index, &rest_labels);
    if (err == 0)
    {
        if (du_cksum_index != -1)
        {
            proto_pdu->txt_len = -1;
            asn_free_value(proto_pdu->data.array[du_cksum_index]);
            proto_pdu->data.array[du_cksum_index] = NULL;
        }
    }
    else if (err != TE_EASNWRONGLABEL)
    {
        goto out;
    }

    du_cksum_label_choice_len = strlen(du_cksum_label) +
                                strlen(choice_script_postfix);

    du_cksum_label_choice = malloc(du_cksum_label_choice_len + 1);
    if (du_cksum_label_choice == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    if (snprintf(du_cksum_label_choice, du_cksum_label_choice_len + 1,
                 "%s%s", du_cksum_label, choice_script_postfix) !=
        (int)du_cksum_label_choice_len)
    {
        err = TE_ENOBUFS;
        goto out;
    }

    err = (accept_zero_cksum) ?
          asn_write_string(proto_pdu, "correct-or-zero",
                           du_cksum_label_choice) :
          asn_write_string(proto_pdu, "correct", du_cksum_label_choice);
    if (err != 0)
        goto out;

out:
    free(du_cksum_label_choice);

    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_request_correct_cksums(uint32_t   hw_flags,
                                asn_value *pdus_orig)
{
    unsigned int   nb_pdus_o;
    asn_value    **pdus_o = NULL;
    unsigned int   nb_pdus_i;
    asn_value    **pdus_i = NULL;
    unsigned int   nb_pdus;
    asn_value    **pdus;
    asn_value     *pdu_ip4 = NULL;
    asn_value     *pdu_tcp = NULL;
    asn_value     *pdu_udp = NULL;
    te_errno       err = 0;

    err = tapi_tad_pdus_relist_outer_inner(pdus_orig, &nb_pdus_o, &pdus_o,
                                           &nb_pdus_i, &pdus_i);
    if (err != 0)
        goto out;

    if (pdus_i != NULL)
    {
        asn_value *pdu_udp_outer = NULL;

        if ((hw_flags & SEND_COND_HW_OFFL_OUTER_IP_CKSUM) ==
            SEND_COND_HW_OFFL_OUTER_IP_CKSUM)
        {
            asn_value *pdu_ip4_outer = NULL;

            pdu_ip4_outer = asn_choice_array_look_up_value(nb_pdus_o, pdus_o,
                                                           TE_PROTO_IP4);
            if (pdu_ip4_outer != NULL)
            {
                err = tapi_tad_set_cksum_script_correct(pdu_ip4_outer,
                                                        "h-checksum",
                                                        FALSE);
                if (err != 0)
                    goto out;
            }
        }

        pdu_udp_outer = asn_choice_array_look_up_value(nb_pdus_o, pdus_o,
                                                       TE_PROTO_UDP);
        if (pdu_udp_outer != NULL)
        {
            err = tapi_tad_set_cksum_script_correct(pdu_udp_outer, "checksum",
                                                    TRUE);
            if (err != 0)
                goto out;
        }

        nb_pdus = nb_pdus_i;
        pdus = pdus_i;
    }
    else
    {
        nb_pdus = nb_pdus_o;
        pdus = pdus_o;
    }

    pdu_ip4 = asn_choice_array_look_up_value(nb_pdus, pdus, TE_PROTO_IP4);
    pdu_tcp = asn_choice_array_look_up_value(nb_pdus, pdus, TE_PROTO_TCP);
    pdu_udp = asn_choice_array_look_up_value(nb_pdus, pdus, TE_PROTO_UDP);

    if (((hw_flags & SEND_COND_HW_OFFL_IP_CKSUM) ==
         SEND_COND_HW_OFFL_IP_CKSUM) && (pdu_ip4 != NULL))
    {
        err = tapi_tad_set_cksum_script_correct(pdu_ip4, "h-checksum", FALSE);
        if (err != 0)
            goto out;
    }

    if ((hw_flags & SEND_COND_HW_OFFL_L4_CKSUM) ==
        SEND_COND_HW_OFFL_L4_CKSUM)
    {
        if (pdu_tcp != NULL)
        {
            err = tapi_tad_set_cksum_script_correct(pdu_tcp, "checksum", FALSE);
            if (err != 0)
                goto out;
        }

        if (pdu_udp != NULL)
        {
            err = tapi_tad_set_cksum_script_correct(pdu_udp, "checksum", FALSE);
            if (err != 0)
                goto out;
        }
    }

out:
    free(pdus_o);
    free(pdus_i);

    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_generate_pattern_unit(asn_value      *pdus,
                               uint8_t        *payload_data,
                               size_t          payload_len,
                               size_t         *data_offset,
                               send_transform *transform,
                               te_bool         is_tso,
                               asn_value    ***pattern_units,
                               unsigned int   *n_pattern_units)
{
    te_errno           err = 0;
    asn_value         *pattern_unit = NULL;
    asn_value         *pdus_copy = NULL;
    asn_child_desc_t  *ip4_pdus = NULL;
    unsigned int       nb_ip4_pdus;
    asn_child_desc_t  *ip6_pdus = NULL;
    unsigned int       nb_ip6_pdus;
    asn_value         *tcp_pdu = NULL;
    asn_child_desc_t  *udp_pdus = NULL;
    unsigned int       nb_udp_pdus;
    size_t             seg_len = 0;
    asn_value        **pattern_units_new = *pattern_units;
    unsigned int       i;

    if (pdus == NULL || payload_data == NULL || data_offset == NULL ||
        n_pattern_units == NULL || (is_tso && (transform == NULL)))
    {
        err = TE_EINVAL;
        goto out;
    }

    pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    if (pattern_unit == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    pdus_copy = asn_copy_value(pdus);
    if (pdus_copy == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    err = asn_put_child_value(pattern_unit, pdus_copy, PRIVATE, NDN_PU_PDUS);
    if (err != 0)
    {
        asn_free_value(pdus_copy);
        goto out;
    }

    err = asn_find_child_choice_values(pdus_copy, TE_PROTO_IP4,
                                       &ip4_pdus, &nb_ip4_pdus);
    if (err != 0)
        goto out;

    err = asn_find_child_choice_values(pdus_copy, TE_PROTO_IP6,
                                       &ip6_pdus, &nb_ip6_pdus);
    if (err != 0)
        goto out;

    tcp_pdu = asn_find_child_choice_value(pdus_copy, TE_PROTO_TCP);

    err = asn_find_child_choice_values(pdus_copy, TE_PROTO_UDP,
                                       &udp_pdus, &nb_udp_pdus);
    if (err != 0)
        goto out;

    if (is_tso)
    {
        if (tcp_pdu == NULL)
        {
            err = TE_EINVAL;
            goto out;
        }

        seg_len = MIN(payload_len - (*n_pattern_units * transform->tso_segsz),
                      transform->tso_segsz);

        for (i = 0; i < nb_ip4_pdus; ++i)
        {
            err = tapi_tad_tso_seg_fix_ip4h(ip4_pdus[i].value, payload_len,
                                            seg_len, *n_pattern_units);
            if (err != 0)
                goto out;
        }

        for (i = 0; i < nb_ip6_pdus; ++i)
        {
            err = tapi_tad_tso_seg_fix_ip6h(ip6_pdus[i].value,
                                            payload_len, seg_len);
            if (err != 0)
                goto out;
        }

        err = tapi_tad_tso_seg_fix_tcph(tcp_pdu, payload_len, seg_len,
                                        *n_pattern_units *
                                        transform->tso_segsz);
        if (err != 0)
            goto out;

        /*
         * Fix length field in UDP header(s).
         * This comes in handy for encapsulated frames which have outer
         * UDP header and for UDP GSO use cases.
         */
        for (i = 0; i < nb_udp_pdus; ++i)
        {
            err = tapi_tad_tso_seg_fix_udph(udp_pdus[i].value, payload_len,
                                            seg_len);
            if (err != 0)
                goto out;
        }
    }

    err = asn_write_value_field(pattern_unit, payload_data + *data_offset,
                                (is_tso) ? seg_len : payload_len,
                                "payload.#bytes");
    if (err != 0)
        goto out;

    *data_offset += (is_tso) ? seg_len : payload_len;

    if (transform != NULL)
    {
        err = tapi_tad_request_correct_cksums(transform->hw_flags, pdus_copy);
        if (err != 0)
            goto out;
    }

    for (i = 0; i < nb_ip4_pdus; ++i)
    {
        err = asn_free_child(ip4_pdus[i].value, PRIVATE, NDN_TAG_IP4_OPTIONS);
        err = (err == TE_EASNWRONGLABEL) ? 0 : err;
        if (err != 0)
            goto out;

        err = asn_free_child(ip4_pdus[i].value, PRIVATE,
                             NDN_TAG_IP4_PLD_CHECKSUM);
        err = (err == TE_EASNWRONGLABEL) ? 0 : err;
        if (err != 0)
            goto out;
    }

    for (i = 0; i < nb_ip6_pdus; ++i)
    {
        err = asn_free_child(ip6_pdus[i].value, PRIVATE,
                             NDN_TAG_IP6_PLD_CHECKSUM);
        err = (err == TE_EASNWRONGLABEL) ? 0 : err;
        if (err != 0)
            goto out;
    }

    pattern_units_new = realloc(*pattern_units, (*n_pattern_units + 1) *
                                 sizeof(**pattern_units));
    if (pattern_units_new == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    *pattern_units = pattern_units_new;
    (*pattern_units)[*n_pattern_units] = pattern_unit;
    (*n_pattern_units)++;

out:
    if ((err != 0) && (pattern_unit != NULL))
        asn_free_value(pattern_unit);

    free(ip4_pdus);
    free(ip6_pdus);
    free(udp_pdus);

    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_packet_to_pattern_units(asn_value      *packet,
                                 send_transform *transform,
                                 asn_value    ***pattern_units_out,
                                 unsigned int   *n_pattern_units_out)
{
    te_errno        err = 0;
    asn_value      *pdus = NULL;
    size_t          payload_len;
    uint8_t        *payload_data = NULL;
    asn_value      *tcp_pdu = NULL;
    size_t          data_offset = 0;
    asn_value     **pattern_units = NULL;
    unsigned int    n_pattern_units = 0;
    int             ret;

    err = asn_get_subvalue(packet, &pdus, "pdus");
    if (err != 0)
        goto out;

    ret = asn_get_length(packet, "payload.#bytes");
    if (ret < 0)
    {
        err = TE_EINVAL;
        goto out;
    }

    payload_len = ret;

    payload_data = malloc(payload_len);
    if (payload_data == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    err = asn_read_value_field(packet, (void *)payload_data,
                               &payload_len, "payload.#bytes");
    if (err != 0)
        goto out;

    tcp_pdu = asn_find_child_choice_value(pdus, TE_PROTO_TCP);
    if ((tcp_pdu != NULL) && (transform != NULL) &&
        ((transform->hw_flags & SEND_COND_HW_OFFL_TSO) ==
        SEND_COND_HW_OFFL_TSO))
    {
        while (data_offset < (size_t)payload_len)
        {
            err = tapi_tad_generate_pattern_unit(pdus, payload_data,
                                                 (size_t)payload_len,
                                                 &data_offset, transform,
                                                 TRUE, &pattern_units,
                                                 &n_pattern_units);
            if (err != 0)
            {
                unsigned int i;

                for (i = 0; i < n_pattern_units; ++i)
                    asn_free_value(pattern_units[i]);

                free(pattern_units);

                goto out;
            }
        }
    }
    else
    {
        err = tapi_tad_generate_pattern_unit(pdus, payload_data,
                                             (size_t)payload_len, &data_offset,
                                             transform, FALSE, &pattern_units,
                                             &n_pattern_units);
        if (err != 0)
            goto out;
    }

    *pattern_units_out = pattern_units;
    *n_pattern_units_out = n_pattern_units;

out:
    free(payload_data);

    return TE_RC(TE_TAPI, err);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_tad_packets_to_pattern(asn_value         **packets,
                            unsigned int        n_packets,
                            send_transform     *transform,
                            asn_value         **pattern_out)
{
    te_errno        err = 0;
    asn_value      *pattern = NULL;
    unsigned int    i;

    if ((transform != NULL) &&
        ((transform->hw_flags & SEND_COND_HW_OFFL_TSO) ==
         SEND_COND_HW_OFFL_TSO) && (transform->tso_segsz == 0))
    {
        err = TE_EINVAL;
        goto out;
    }

    pattern = asn_init_value(ndn_traffic_pattern);
    if (pattern == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < n_packets; ++i)
    {
        asn_value  **pattern_units = NULL;
        unsigned int n_pattern_units = 0;
        unsigned int j;

        err = tapi_tad_packet_to_pattern_units(packets[i], transform,
                                               &pattern_units,
                                               &n_pattern_units);
        if (err != 0)
            goto out;

        for (j = 0; j < n_pattern_units; ++j)
        {
            err = asn_insert_indexed(pattern, pattern_units[j], -1, "");
            if (err != 0)
            {
                unsigned int k;

                for (k = j; k < n_pattern_units; ++k)
                    asn_free_value(pattern_units[k]);

                free(pattern_units);

                goto out;
            }
        }

        free(pattern_units);
    }

    *pattern_out = pattern;

out:
    if ((err != 0) && (pattern != NULL))
        asn_free_value(pattern);

    return TE_RC(TE_TAPI, err);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_tad_concat_patterns(asn_value  *dst,
                         asn_value  *src)
{
    te_errno        err = 0;
    int             dst_nb_pus_old = -1;
    int             dst_nb_pus_new;
    int             src_nb_pus;
    unsigned int    i;

    if ((dst == NULL) || (src == NULL))
    {
        err = TE_EINVAL;
        goto out;
    }

    dst_nb_pus_old = asn_get_length(dst, "");
    src_nb_pus = asn_get_length(src, "");
    if ((dst_nb_pus_old < 0) || (src_nb_pus < 0))
    {
        err = TE_EINVAL;
        goto out;
    }

    for (i = 0; i < (unsigned int)src_nb_pus; ++i)
    {
        asn_value  *src_pu;
        asn_value  *src_pu_copy;

        err = asn_get_indexed(src, &src_pu, i, "");
        if (err != 0)
            goto out;

        src_pu_copy = asn_copy_value(src_pu);
        if (src_pu_copy == NULL)
        {
            err = TE_ENOMEM;
            goto out;
        }

        err = asn_insert_indexed(dst, src_pu_copy, -1, "");
        if (err != 0)
        {
            asn_free_value(src_pu_copy);
            goto out;
        }
    }

out:
    if ((dst_nb_pus_old >= 0) && (err != 0))
    {
        dst_nb_pus_new = asn_get_length(dst, "");
        if (dst_nb_pus_new > dst_nb_pus_old)
        {
            for (i = dst_nb_pus_new - 1; i >= (unsigned int)dst_nb_pus_old; --i)
                (void)asn_remove_indexed(dst, i, "");
        }
    }

    if (err == 0)
        asn_free_value(src);

    return TE_RC(TE_TAPI, err);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_tad_aggregate_patterns(asn_value     **patterns,
                            unsigned int    nb_patterns,
                            asn_value     **pattern_out)
{
    te_errno        err = 0;
    unsigned int    i;
    asn_value      *agg = NULL;

    if ((patterns == NULL) || (nb_patterns == 0) || (pattern_out == NULL))
    {
        err = TE_EINVAL;
        goto out;
    }

    agg = asn_init_value(ndn_traffic_pattern);
    if (agg == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < nb_patterns; ++i)
    {
        asn_value *pattern_copy;

        pattern_copy = asn_copy_value(patterns[i]);
        if (pattern_copy == NULL)
        {
            err = TE_ENOMEM;
            goto out;
        }

        err = tapi_tad_concat_patterns(agg, pattern_copy);
        if (err != 0)
        {
            asn_free_value(pattern_copy);
            goto out;
        }
    }

    *pattern_out = agg;

out:
    if ((err != 0) && (agg != NULL))
        asn_free_value(agg);

    return TE_RC(TE_TAPI, err);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tmpl_classify(const asn_value    *tmpl,
                       te_tad_protocols_t  hdrs[TAPI_NDN_NLEVELS])
{
    int                 nb_pdus;
    te_tad_protocols_t *l3_proto = &hdrs[TAPI_NDN_INNER_L3];
    te_tad_protocols_t *l4_proto = &hdrs[TAPI_NDN_INNER_L4];
    te_errno            rc;
    int                 i;

    assert(tmpl != NULL);
    assert(hdrs != NULL);

    for (i = 0; i < TAPI_NDN_NLEVELS; ++i)
        hdrs[i] = TE_PROTO_INVALID;

    nb_pdus = asn_get_length(tmpl, "pdus");
    if (nb_pdus < 1)
    {
        rc = TE_EINVAL;
        goto out;
    }

    for (i = 0; i < nb_pdus; ++i)
    {
        asn_value     *pdu_elt;
        asn_tag_value  pdu_choice_tag;

        rc = asn_get_indexed(tmpl, &pdu_elt, i, "pdus");
        if (rc != 0)
            goto out;

        rc = asn_get_choice_value(pdu_elt, NULL, NULL, &pdu_choice_tag);
        if (rc != 0)
            goto out;

        switch (pdu_choice_tag)
        {
            case TE_PROTO_IP4:
                /*@fallthrough@*/
            case TE_PROTO_IP6:
                *l3_proto = pdu_choice_tag;
                break;

            case TE_PROTO_TCP:
                /*@fallthrough@*/
            case TE_PROTO_UDP:
                *l4_proto = pdu_choice_tag;
                break;

            case TE_PROTO_VXLAN:
                /*@fallthrough@*/
            case TE_PROTO_GENEVE:
                /*@fallthrough@*/
            case TE_PROTO_GRE:
                hdrs[TAPI_NDN_TUNNEL] = pdu_choice_tag;
                l3_proto = &hdrs[TAPI_NDN_OUTER_L3];
                l4_proto = &hdrs[TAPI_NDN_OUTER_L4];
                break;

            case TE_PROTO_ETH:
                break;

            default:
                rc = TE_EINVAL;
                goto out;
        }
    }

out:
    return TE_RC(TE_TAPI, rc);
}

static te_errno
tapi_ndn_pdu_idx_by_proto(asn_value          *container_of_pdus,
                          te_bool             outer,
                          te_tad_protocols_t  proto,
                          int                *idx)
{
    int      nb_pdus;
    int      pdu_idx;
    int      pdu_idx_inc;
    int      pdu_idx_found = -1;
    te_errno rc;

    nb_pdus = asn_get_length(container_of_pdus, "pdus");
    if (nb_pdus < 1)
        return TE_EINVAL;

    pdu_idx = (outer) ? nb_pdus - 1 : 0;
    pdu_idx_inc = (outer) ? -1 : 1;

    while (nb_pdus-- > 0)
    {
        asn_value     *pdu_i;
        asn_tag_value  pdu_i_choice_tag;

        rc = asn_get_indexed(container_of_pdus, &pdu_i, pdu_idx, "pdus");
        if (rc != 0)
            return rc;

        rc = asn_get_choice_value(pdu_i, NULL, NULL, &pdu_i_choice_tag);
        if (rc != 0)
            return rc;

        if (pdu_i_choice_tag == proto)
        {
            pdu_idx_found = pdu_idx;
            break;
        }

        pdu_idx += pdu_idx_inc;
    }

    if (pdu_idx_found != -1)
    {
        *idx = pdu_idx_found;
        return 0;
    }

    return TE_ENOENT;
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tmpl_set_ip_cksum(asn_value        *tmpl,
                           uint16_t          cksum,
                           tapi_ndn_level_t  level)
{
    int      pdu_idx;
    te_errno rc;

    assert(tmpl != NULL);
    assert(level == TAPI_NDN_OUTER_L3 || level == TAPI_NDN_INNER_L3);

    rc = tapi_ndn_pdu_idx_by_proto(tmpl, level == TAPI_NDN_OUTER_L3,
                                   TE_PROTO_IP4, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_write_value_field_fmt(tmpl, &cksum, sizeof(cksum),
                                   "pdus.%d.#ip4.h-checksum.#plain", pdu_idx);

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tmpl_set_udp_cksum(asn_value        *tmpl,
                            uint16_t          cksum,
                            tapi_ndn_level_t  level)
{
    int      pdu_idx;
    te_errno rc;

    assert(tmpl != NULL);
    assert(level == TAPI_NDN_OUTER_L4 || level == TAPI_NDN_INNER_L4);

    rc = tapi_ndn_pdu_idx_by_proto(tmpl, level == TAPI_NDN_OUTER_L4,
                                   TE_PROTO_UDP, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_write_value_field_fmt(tmpl, &cksum, sizeof(cksum),
                                   "pdus.%d.#udp.checksum.#plain", pdu_idx);

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tmpl_set_tcp_cksum(asn_value *tmpl,
                            uint16_t   cksum)
{
    int      pdu_idx;
    te_errno rc;

    assert(tmpl != NULL);

    rc = tapi_ndn_pdu_idx_by_proto(tmpl, FALSE, TE_PROTO_TCP, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_write_value_field_fmt(tmpl, &cksum, sizeof(cksum),
                                   "pdus.%d.#tcp.checksum.#plain", pdu_idx);

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tmpl_set_tcp_flags(asn_value *tmpl,
                            uint8_t    flags)
{
    int      pdu_idx;
    te_errno rc;

    assert(tmpl != NULL);

    rc = tapi_ndn_pdu_idx_by_proto(tmpl, FALSE, TE_PROTO_TCP, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_write_value_field_fmt(tmpl, &flags, sizeof(flags),
                                   "pdus.%d.#tcp.flags.#plain", pdu_idx);

out:
    return rc;
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tmpl_set_payload_len(asn_value    *tmpl,
                              unsigned int  payload_len)
{
    te_errno rc;

    assert(tmpl != NULL);

    rc = asn_write_value_field(tmpl, &payload_len, sizeof(payload_len),
                               "payload.#length");

    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_pkt_inject_vlan_tag(asn_value *pkt,
                             uint16_t   vlan_tci)
{
    uint16_t   v;
    asn_value *provisional_du = NULL;
    int        nb_pdus;
    asn_value *du;
    te_errno   rc;

    provisional_du = asn_init_value(ndn_vlan_tag_header);
    if (provisional_du == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    v = vlan_tci & NDN_ETH_VLAN_TCI_MASK_PRIO;
    rc = asn_write_value_field(provisional_du, &v, sizeof(v),
                               "priority.#plain");
    if (rc != 0)
        goto out;

    v = vlan_tci & NDN_ETH_VLAN_TCI_MASK_CFI;
    rc = asn_write_value_field(provisional_du, &v, sizeof(v), "cfi.#plain");
    if (rc != 0)
        goto out;

    v = vlan_tci & NDN_ETH_VLAN_TCI_MASK_ID;
    rc = asn_write_value_field(provisional_du, &v, sizeof(v),
                               "vlan-id.#plain");
    if (rc != 0)
        goto out;

    assert(pkt != NULL);

    nb_pdus = asn_get_length(pkt, "pdus");
    if (nb_pdus < 1)
    {
        rc = TE_EINVAL;
        goto out;
    }

    du = asn_retrieve_descendant(pkt, NULL, "pdus.%d.#eth.tagged.#tagged",
                                 nb_pdus - 1);
    if (du == NULL)
    {
        rc = TE_EFAULT;
        goto out;
    }

    rc = asn_assign_value(du, provisional_du);

out:
    if (provisional_du != NULL)
        asn_free_value(provisional_du);

    return TE_RC(TE_TAPI, rc);
}

te_errno
tapi_ndn_pdus_inject_vlan_tags(asn_value *pdus, const uint16_t *vlan_vid,
                               const uint16_t *vlan_pri, const uint16_t *vlan_cfi,
                               size_t n_tags)
{
    te_bool has_vlan = FALSE;
    asn_value *new_vlan;
    uint16_t old_vid[2];
    uint16_t old_pri[2];
    uint16_t old_cfi[2];
    size_t n_old_tags;
    asn_value *eth;
    te_errno rc;
    int n_pdus;

    if (n_tags > 2)
    {
        ERROR("Failed to insert more than 2 VLAN tags");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (n_tags == 0)
        return 0;

    n_pdus = asn_get_length(pdus, "");
    if (n_pdus < 1)
    {
        ERROR("Failed to get PDU sequence length");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = asn_get_indexed(pdus, &eth, n_pdus - 1, "");
    if (rc != 0)
    {
        ERROR("Failed to get eth PDU");
        return rc;
    }

    n_old_tags = TE_ARRAY_LEN(old_vid);
    rc = tapi_ndn_eth_read_vlan_tci(eth, &n_old_tags, old_vid, old_pri, old_cfi);
    if (rc == 0)
    {
        has_vlan = (n_old_tags > 0);

        if (has_vlan && n_tags == 2)
        {
            ERROR("Failed to inject 2 VLAN tags in tagged PDU sequence");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
        if (n_old_tags == 2)
        {
            ERROR("Failed to inject VLAN tags in double-tagged PDU sequence");
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
    }
    else if (rc != 0)
        return rc;

    new_vlan = asn_init_value(ndn_vlan_tagged);
    if (new_vlan == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = 0;
    if (has_vlan || n_tags == 2)
    {
        unsigned int i;
        struct val_label {
            uint16_t val;
            const char *label;
        } map[] = {
            { has_vlan ? old_vid[0] : vlan_vid[0],
              "#double-tagged.inner.vid.#plain"},
            { has_vlan ? vlan_vid[0] : vlan_vid[1],
              "#double-tagged.outer.vid.#plain"},
            { has_vlan ? old_pri[0] : vlan_pri[0],
              "#double-tagged.inner.pcp.#plain"},
            { has_vlan ? vlan_pri[0] : vlan_pri[1],
              "#double-tagged.outer.pcp.#plain"},
            { has_vlan ? old_cfi[0] : vlan_cfi[0],
              "#double-tagged.inner.dei.#plain"},
            { has_vlan ? vlan_cfi[0] : vlan_cfi[1],
              "#double-tagged.outer.dei.#plain"}};

        for (i = 0; i < TE_ARRAY_LEN(map); i++)
        {
            if (rc == 0 && map[i].val != UINT16_MAX)
                rc = asn_write_value_field(new_vlan, &map[i].val,
                                           sizeof(map[i].val), map[i].label);
        }
    }
    else
    {
        unsigned int i;
        struct val_label {
            uint16_t val;
            const char *label;
        } map[] = {
            { vlan_vid[0], "#tagged.vlan-id.#plain" },
            { vlan_pri[0], "#tagged.priority.#plain" },
            { vlan_cfi[0], "#tagged.cfi.#plain" }};

        for (i = 0; i < TE_ARRAY_LEN(map); i++)
        {
            if (rc == 0 && map[i].val != UINT16_MAX)
            {
                rc = asn_write_value_field(new_vlan, &map[i].val,
                                           sizeof(map[i].val), map[i].label);
            }
        }
    }

    if (rc == 0)
        rc = asn_put_descendent(eth, new_vlan, "#eth.tagged");

    if (rc != 0)
    {
        ERROR("Failed to modify PDU sequence");
        asn_free_value(new_vlan);
        return rc;
    }

    return 0;
}

te_errno
tapi_ndn_pdus_remove_vlan_tags(asn_value *pdus, size_t n_tags)
{
    asn_value *new_vlan;
    uint16_t old_vid[2];
    uint16_t old_pri[2];
    uint16_t old_cfi[2];
    size_t n_old_tags;
    asn_value *eth;
    te_errno rc;
    int n_pdus;

    if (n_tags > 2)
    {
        ERROR("Failed to remove more than 2 VLAN tags");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (n_tags == 0)
        return 0;

    n_pdus = asn_get_length(pdus, "");
    if (n_pdus < 1)
    {
        ERROR("Failed to get PDU sequence length");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = asn_get_indexed(pdus, &eth, n_pdus - 1, "");
    if (rc != 0)
    {
        ERROR("Failed to get eth PDU");
        return rc;
    }

    n_old_tags = TE_ARRAY_LEN(old_vid);
    rc = tapi_ndn_eth_read_vlan_tci(eth, &n_old_tags, old_vid, old_pri, old_cfi);
    if (rc == 0)
    {
        if (n_tags > n_old_tags)
        {
            ERROR("Failed to remove %lu VLAN tags from %lu-tagged PDU sequence",
                  n_tags, n_old_tags);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
    }
    else if (rc != 0)
        return rc;

    new_vlan = asn_init_value(ndn_vlan_tagged);
    if (new_vlan == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    rc = 0;
    if (n_old_tags - n_tags == 1)
    {
        unsigned int i;
        struct val_label {
            uint16_t val;
            const char *label;
        } map[] = {
            { old_vid[0], "#tagged.vlan-id.#plain" },
            { old_pri[0], "#tagged.priority.#plain" },
            { old_cfi[0], "#tagged.cfi.#plain" }};

        for (i = 0; i < TE_ARRAY_LEN(map); i++)
        {
            if (rc == 0 && map[i].val != UINT16_MAX)
            {
                rc = asn_write_value_field(new_vlan, &map[i].val,
                                           sizeof(map[i].val), map[i].label);
            }
        }

        if (rc == 0)
            rc = asn_put_descendent(eth, new_vlan, "#eth.tagged");
    }
    else
    {
        rc = asn_free_subvalue(eth, "#eth.tagged");
    }

    if (rc != 0)
    {
        ERROR("Failed to modify PDU sequence");
        asn_free_value(new_vlan);
        return rc;
    }

    return 0;
}

te_errno
tapi_ndn_eth_read_vlan_tci(const asn_value *eth, size_t *n_tags,
                           uint16_t *vid, uint16_t *prio, uint16_t *cfi)
{
    asn_value *vlan_header;
    uint16_t vid_out[2] = {UINT16_MAX, UINT16_MAX};
    uint16_t prio_out[2] = {UINT16_MAX, UINT16_MAX};
    uint16_t cfi_out[2] = {UINT16_MAX, UINT16_MAX};
    size_t size = sizeof(vid_out[0]);
    size_t tags_count;
    te_errno rc;

    vlan_header = asn_find_descendant(eth, &rc, "tagged.#tagged");
    if (rc == 0)
    {
        rc = asn_read_value_field(vlan_header, &vid_out[0], &size, "vlan-id");
        if (rc == 0 || rc == TE_EASNINCOMPLVAL)
        {
            rc = asn_read_value_field(vlan_header, &prio_out[0], &size,
                                      "priority");
        }
        if (rc == 0 || rc == TE_EASNINCOMPLVAL)
        {
            rc = asn_read_value_field(vlan_header, &cfi_out[0], &size, "cfi");
        }
        if (rc != 0 && rc != TE_EASNINCOMPLVAL)
        {
            ERROR("Failed to read existing VLAN tag tci");
            return rc;
        }

        tags_count = 1;
    }
    else if (rc == TE_EASNOTHERCHOICE)
    {
        unsigned int i;
        struct ptr_label {
            uint16_t *ptr;
            const char *label;
        } map[] = {
            { &vid_out[0], "tagged.#double-tagged.inner.vid.#plain"},
            { &vid_out[1], "tagged.#double-tagged.outer.vid.#plain"},
            { &cfi_out[0], "tagged.#double-tagged.inner.dei.#plain"},
            { &cfi_out[1], "tagged.#double-tagged.outer.dei.#plain"},
            { &prio_out[0], "tagged.#double-tagged.inner.pcp.#plain"},
            { &prio_out[1], "tagged.#double-tagged.outer.pcp.#plain"}};

        rc = 0;

        for (i = 0; i < TE_ARRAY_LEN(map); i++)
        {
            if (rc == 0 || rc == TE_EASNINCOMPLVAL)
                rc = asn_read_value_field(eth, map[i].ptr, &size, map[i].label);
        }
        if (rc != 0 && rc != TE_EASNINCOMPLVAL)
        {
            ERROR("Failed to read existing double VLAN tag tci");
            return rc;
        }

        tags_count = 2;
    }
    else if (rc == TE_EASNINCOMPLVAL)
    {
        *n_tags = 0;

        return 0;
    }
    else
    {
        ERROR("Error occured during VLAN tag get");
        return rc;
    }

    if (*n_tags < tags_count)
    {
        ERROR("Not enough space to place read VLAN tags");
        return TE_RC(TE_TAPI, TE_ENOBUFS);
    }

    *n_tags = tags_count;
    memcpy(vid, vid_out, sizeof(*vid) * tags_count);
    memcpy(prio, prio_out, sizeof(*prio) * tags_count);
    memcpy(cfi, cfi_out, sizeof(*cfi) * tags_count);

    return 0;
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_pkt_demand_correct_ip_cksum(asn_value        *pkt,
                                     tapi_ndn_level_t  level)
{
    int        pdu_idx;
    asn_value *du;
    te_errno   rc;

    assert(pkt != NULL);
    assert(level == TAPI_NDN_OUTER_L3 || level == TAPI_NDN_INNER_L3);

    rc = tapi_ndn_pdu_idx_by_proto(pkt, level == TAPI_NDN_OUTER_L3,
                                   TE_PROTO_IP4, &pdu_idx);
    if (rc != 0)
       goto out;

    du = asn_find_descendant(pkt, NULL, "pdus.%d.#ip4.h-checksum", pdu_idx);
    if (du == NULL)
    {
        rc = TE_EFAULT;
        goto out;
    }

    rc = asn_free_subvalue(du, "#plain");
    if (rc != 0)
        goto out;

    rc = asn_write_string(du, "correct", "#script");

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_pkt_demand_correct_udp_cksum(asn_value        *pkt,
                                      te_bool           can_be_zero,
                                      tapi_ndn_level_t  level)
{
    int        pdu_idx;
    asn_value *du;
    te_errno   rc;

    assert(pkt != NULL);
    assert(level == TAPI_NDN_OUTER_L4 || level == TAPI_NDN_INNER_L4);

    rc = tapi_ndn_pdu_idx_by_proto(pkt, level == TAPI_NDN_OUTER_L4,
                                   TE_PROTO_UDP, &pdu_idx);
    if (rc != 0)
       goto out;

    du = asn_find_descendant(pkt, NULL, "pdus.%d.#udp.checksum", pdu_idx);
    if (du == NULL)
    {
        rc = TE_EFAULT;
        goto out;
    }

    rc = asn_free_subvalue(du, "#plain");
    if (rc != 0)
        goto out;

    rc = (can_be_zero) ? asn_write_string(du, "correct-or-zero", "#script") :
                         asn_write_string(du, "correct", "#script");

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_pkt_demand_correct_tcp_cksum(asn_value *pkt)
{
    int        pdu_idx;
    const char script[] = "correct";
    te_errno   rc;

    assert(pkt != NULL);

    rc = tapi_ndn_pdu_idx_by_proto(pkt, FALSE, TE_PROTO_TCP, &pdu_idx);
    if (rc != 0)
       goto out;

    rc = asn_free_subvalue_fmt(pkt, "pdus.%d.#tcp.checksum.#plain", pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_write_value_field_fmt(pkt, script, sizeof(script),
                                   "pdus.%d.#tcp.checksum.#script", pdu_idx);

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_superframe_gso(asn_value      *superframe,
                        size_t          seg_payload_len,
                        asn_value    ***pkts_out,
                        unsigned int   *nb_pkts_out)
{
    int            superframe_payload_len;
    unsigned int   nb_pkts = 0;
    asn_value     *provisional_frame = NULL;
    asn_value     *provisional_frame_payload = NULL;
    char          *payload_buf = NULL;
    size_t         payload_buf_len;
    asn_value    **pkts = NULL;
    te_errno       rc;
    unsigned int   i;

    assert(superframe != NULL);
    assert(seg_payload_len != 0);
    assert(pkts_out != NULL);
    assert(nb_pkts_out != NULL);

    superframe_payload_len = asn_get_length(superframe, "payload.#bytes");
    if (superframe_payload_len <= 0)
    {
        rc = TE_EINVAL;
        goto out;
    }

    nb_pkts = (size_t)superframe_payload_len / seg_payload_len;
    nb_pkts += ((size_t)superframe_payload_len % seg_payload_len) ? 1 : 0;

    provisional_frame = asn_copy_value(superframe);
    if (provisional_frame == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rc = asn_get_subvalue(provisional_frame, &provisional_frame_payload,
                          "payload");
    if (rc != 0)
        goto out;

    payload_buf_len = superframe_payload_len;
    payload_buf = TE_ALLOC(payload_buf_len);
    if (payload_buf == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rc = asn_read_value_field(provisional_frame_payload, payload_buf,
                              &payload_buf_len, "");
    if (rc != 0)
        goto out;

    pkts = TE_ALLOC(nb_pkts * sizeof(*pkts));
    if (pkts == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < nb_pkts; ++i)
    {
        size_t payload_len_remaining = payload_buf_len - i * seg_payload_len;

        pkts[i] = asn_init_value(ndn_raw_packet);
        if (pkts[i] == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_free_subvalue(provisional_frame_payload, "#bytes");
        if (rc != 0)
            goto out;

        rc = asn_write_value_field(provisional_frame_payload,
                                   payload_buf + i * seg_payload_len,
                                   MIN(payload_len_remaining, seg_payload_len),
                                   "#bytes");
        if (rc != 0)
            goto out;

        rc = asn_assign_value(pkts[i], provisional_frame);
        if (rc != 0)
            goto out;
    }

    *pkts_out = pkts;
    *nb_pkts_out = nb_pkts;

out:
    if (rc != 0 && pkts != NULL)
    {
        for (i = 0; i < nb_pkts; ++i)
        {
            if (pkts[i] != NULL)
                asn_free_value(pkts[i]);
        }

        free(pkts);
    }

    free(payload_buf);

    if (provisional_frame != NULL)
        asn_free_value(provisional_frame);

    return rc;
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_tso_pkts_edit(asn_value    **pkts,
                       unsigned int   nb_pkts)
{
    int          pdu_idx;
    uint32_t     superframe_seqn;
    size_t       superframe_seqn_size = sizeof(superframe_seqn);
    uint8_t      superframe_flags;
    size_t       superframe_flags_size = sizeof(superframe_flags);
    int          seg_payload_size;
    te_errno     rc = 0;
    unsigned int i;

    assert(pkts != NULL);
    assert(nb_pkts != 0);
    assert(pkts[0] != NULL);

    rc = tapi_ndn_pdu_idx_by_proto(pkts[0], FALSE, TE_PROTO_TCP, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_read_value_field_fmt(pkts[0], &superframe_seqn,
                                  &superframe_seqn_size,
                                  "pdus.%d.#tcp.seqn.#plain", pdu_idx);
    if (rc != 0)
        goto out;

    assert(superframe_seqn_size == sizeof(superframe_seqn));

    rc = asn_read_value_field_fmt(pkts[0], &superframe_flags,
                                  &superframe_flags_size,
                                  "pdus.%d.#tcp.flags.#plain", pdu_idx);
    if (rc != 0)
        goto out;

    assert(superframe_flags_size == sizeof(superframe_flags));

    seg_payload_size = asn_get_length(pkts[0], "payload.#bytes");
    if (seg_payload_size <= 0)
    {
        rc = TE_EINVAL;
        goto out;
    }

    for (i = 0; i < nb_pkts; ++i)
    {
        uint32_t provisional_seqn = superframe_seqn;
        uint8_t  provisional_flags = superframe_flags;

        assert(pkts[i] != NULL);

        provisional_seqn += i * seg_payload_size;
        rc = asn_write_value_field_fmt(pkts[i], &provisional_seqn,
                                       sizeof(provisional_seqn),
                                       "pdus.%d.#tcp.seqn.#plain", pdu_idx);
        if (rc != 0)
            goto out;

        if (i != 0)
            provisional_flags &= ~TCP_CWR_FLAG;

        if (i + 1 != nb_pkts)
            provisional_flags &= ~(TCP_FIN_FLAG | TCP_PSH_FLAG);

        rc = asn_write_value_field_fmt(pkts[i], &provisional_flags,
                                       sizeof(provisional_flags),
                                       "pdus.%d.#tcp.flags.#plain", pdu_idx);
        if (rc != 0)
            goto out;
    }

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_gso_pkts_ip_len_edit(asn_value          **pkts,
                              unsigned int         nb_pkts,
                              te_tad_protocols_t   ip_te_proto,
                              tapi_ndn_level_t     level)
{
    int           pdu_idx = 1;
    uint16_t      superframe_ip_len;
    size_t        superframe_ip_len_size = sizeof(superframe_ip_len);
    size_t        superframe_payload_len;
    te_errno      rc = 0;
    unsigned int  i;

    assert(pkts != NULL);
    assert(nb_pkts != 0);
    assert(ip_te_proto == TE_PROTO_IP4 || ip_te_proto == TE_PROTO_IP6);
    assert(pkts[0] != NULL);
    assert(level == TAPI_NDN_OUTER_L3 || level == TAPI_NDN_INNER_L3);

    rc = tapi_ndn_pdu_idx_by_proto(pkts[0], level == TAPI_NDN_OUTER_L3,
                                   ip_te_proto, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_read_value_field_fmt(pkts[0], &superframe_ip_len,
                                  &superframe_ip_len_size,
                                  "pdus.%d.#ip%d.%s.#plain", pdu_idx,
                                  (ip_te_proto == TE_PROTO_IP4) ? 4 : 6,
                                  (ip_te_proto == TE_PROTO_IP4) ?
                                  "total-length" : "payload-length");
    if (rc != 0)
        goto out;

    assert(superframe_ip_len_size == sizeof(superframe_ip_len));

    superframe_payload_len = 0;

    for (i = 0; i < nb_pkts; ++i)
    {
        int seg_payload_len;

        assert(pkts[i] != NULL);

        seg_payload_len = asn_get_length(pkts[i], "payload.#bytes");
        if (seg_payload_len <= 0)
        {
            rc = TE_EINVAL;
            goto out;
        }

        superframe_payload_len += seg_payload_len;
    }

    for (i = 0; i < nb_pkts; ++i)
    {
        int      seg_payload_len;
        uint16_t provisional_ip_len;

        seg_payload_len = asn_get_length(pkts[i], "payload.#bytes");
        if (seg_payload_len <= 0)
        {
            rc = TE_EINVAL;
            goto out;
        }

        provisional_ip_len = superframe_ip_len - superframe_payload_len +
                             seg_payload_len;

        rc = asn_write_value_field_fmt(pkts[i], &provisional_ip_len,
                                       sizeof(provisional_ip_len),
                                       "pdus.%d.#ip%d.%s.#plain", pdu_idx,
                                       (ip_te_proto == TE_PROTO_IP4) ? 4 : 6,
                                       (ip_te_proto == TE_PROTO_IP4) ?
                                       "total-length" : "payload-length");
        if (rc != 0)
            goto out;
    }

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_gso_pkts_ip_id_edit(asn_value        **pkts,
                             unsigned int       nb_pkts,
                             te_bool            inc_mod15,
                             tapi_ndn_level_t   level)
{
    int           pdu_idx;
    uint16_t      superframe_ip_id;
    size_t        superframe_ip_id_size = sizeof(superframe_ip_id);
    te_errno      rc = 0;
    unsigned int  i;

    assert(pkts != NULL);
    assert(nb_pkts != 0);
    assert(pkts[0] != NULL);
    assert(level == TAPI_NDN_OUTER_L3 || level == TAPI_NDN_INNER_L3);

    rc = tapi_ndn_pdu_idx_by_proto(pkts[0], level == TAPI_NDN_OUTER_L3,
                                   TE_PROTO_IP4, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_read_value_field_fmt(pkts[0], &superframe_ip_id,
                                  &superframe_ip_id_size,
                                  "pdus.%d.#ip4.ip-ident.#plain", pdu_idx);
    if (rc != 0)
        goto out;

    assert(superframe_ip_id_size == sizeof(superframe_ip_id));

    for (i = 0; i < nb_pkts; ++i)
    {
        uint16_t provisional_ip_id;

        if (inc_mod15)
        {
            provisional_ip_id = (superframe_ip_id & 0x8000) |
                                ((superframe_ip_id + i) & 0x7fff);
        }
        else
        {
            provisional_ip_id = superframe_ip_id + i;
        }

        assert(pkts[i] != NULL);

        rc = asn_write_value_field_fmt(pkts[i], &provisional_ip_id,
                                       sizeof(provisional_ip_id),
                                       "pdus.%d.#ip4.ip-ident.#plain",
                                       pdu_idx);
        if (rc != 0)
            goto out;
    }

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_gso_pkts_udp_len_edit(asn_value        **pkts,
                               unsigned int       nb_pkts,
                               tapi_ndn_level_t   level)
{
    int           pdu_idx;
    uint16_t      superframe_udp_len;
    size_t        superframe_udp_len_size = sizeof(superframe_udp_len);
    size_t        superframe_payload_len;
    te_errno      rc = 0;
    unsigned int  i;

    assert(pkts != NULL);
    assert(nb_pkts != 0);
    assert(pkts[0] != NULL);
    assert(level == TAPI_NDN_OUTER_L4 || level == TAPI_NDN_INNER_L4);

    rc = tapi_ndn_pdu_idx_by_proto(pkts[0], level == TAPI_NDN_OUTER_L4,
                                   TE_PROTO_UDP, &pdu_idx);
    if (rc != 0)
        goto out;

    rc = asn_read_value_field_fmt(pkts[0], &superframe_udp_len,
                                  &superframe_udp_len_size,
                                  "pdus.%d.#udp.length.#plain", pdu_idx);
    if (rc != 0)
        goto out;

    assert(superframe_udp_len_size == sizeof(superframe_udp_len));

    superframe_payload_len = 0;

    for (i = 0; i < nb_pkts; ++i)
    {
        int seg_payload_len;

        assert(pkts[i] != NULL);

        seg_payload_len = asn_get_length(pkts[i], "payload.#bytes");
        if (seg_payload_len <= 0)
        {
            rc = TE_EINVAL;
            goto out;
        }

        superframe_payload_len += seg_payload_len;
    }

    for (i = 0; i < nb_pkts; ++i)
    {
        int      seg_payload_len;
        uint16_t provisional_udp_len;

        seg_payload_len = asn_get_length(pkts[i], "payload.#bytes");
        if (seg_payload_len <= 0)
        {
            rc = TE_EINVAL;
            goto out;
        }

        provisional_udp_len = superframe_udp_len - superframe_payload_len +
                              seg_payload_len;

        rc = asn_write_value_field_fmt(pkts[i], &provisional_udp_len,
                                       sizeof(provisional_udp_len),
                                       "pdus.%d.#udp.length.#plain", pdu_idx);
        if (rc != 0)
            goto out;
    }

out:
    return TE_RC(TE_TAPI, rc);
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_ndn_pkts_to_ptrn(asn_value    **pkts,
                      unsigned int   nb_pkts,
                      asn_value    **ptrn_out)
{
    asn_value    *ptrn;
    te_errno      rc = 0;
    unsigned int  i;

    assert(pkts != NULL);
    assert(nb_pkts != 0);
    assert(ptrn_out != NULL);

    ptrn = asn_init_value(ndn_traffic_pattern);
    if (ptrn == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    for (i = 0; i < nb_pkts; ++i)
    {
        asn_value *ptrn_unit;
        asn_value *pdus;
        asn_value *payload;
        asn_value *pdus_copy;
        asn_value *payload_copy;

        ptrn_unit = asn_init_value(ndn_traffic_pattern_unit);
        if (ptrn_unit == NULL)
        {
             rc = TE_ENOMEM;
             goto out;
        }

        rc = asn_insert_indexed(ptrn, ptrn_unit, -1, "");
        if (rc != 0)
        {
            asn_free_value(ptrn_unit);
            goto out;
        }

        assert(pkts[i] != NULL);

        rc = asn_get_subvalue(pkts[i], &pdus, "pdus");
        if (rc != 0)
            goto out;

        rc = asn_get_subvalue(pkts[i], &payload, "payload");
        if (rc != 0)
            goto out;

        pdus_copy = asn_copy_value(pdus);
        if (pdus_copy == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        payload_copy = asn_copy_value(payload);
        if (payload_copy == NULL)
        {
            asn_free_value(pdus_copy);
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_put_child_value_by_label(ptrn_unit, pdus_copy, "pdus");
        if (rc != 0)
        {
            asn_free_value(pdus_copy);
            asn_free_value(payload_copy);
            goto out;
        }

        rc = asn_put_child_value_by_label(ptrn_unit, payload_copy,
                                          "payload");
        if (rc != 0)
        {
            asn_free_value(payload_copy);
            goto out;
        }
    }

    *ptrn_out = ptrn;

out:
    if (rc != 0 && ptrn != NULL)
        asn_free_value(ptrn);

    return TE_RC(TE_TAPI, rc);
}

/* Fill unspecified fields with zeroes */
static void
tapi_tad_vlan_zero_unspecified(size_t n_tags, uint16_t *vid, uint16_t *prio,
                               uint16_t *cfi)
{
    uint16_t *fields[] = {prio, cfi, vid};
    unsigned int i;
    unsigned int j;

    for (i = 0; i < TE_ARRAY_LEN(fields); i++)
    {
        for (j = 0; j < n_tags; j++)
        {
            if (fields[i][j] == UINT16_MAX)
                fields[i][j] = 0;
        }
    }
}

/* See the description in 'tapi_ndn.h' */
te_errno
tapi_eth_transform_ptrn_on_rx(receive_transform *rx_transform,
                              asn_value **ptrn)
{
    asn_value *new_ptrn = NULL;
    asn_value *pdus = NULL;
    asn_value *eth = NULL;
    asn_child_desc_t *eth_hdrs = NULL;
    unsigned int n_hdrs;
    size_t n_tags;
    uint16_t vid[2] = {0};
    uint16_t prio[2] = {0};
    uint16_t cfi[2] = {0};
    te_errno rc = 0;

    new_ptrn = asn_copy_value(*ptrn);
    if (new_ptrn == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    pdus = asn_find_descendant(new_ptrn, &rc, "0.pdus");
    if (rc != 0)
    {
        ERROR("Failed to get PDU sequence");
        goto out;
    }

    rc = asn_find_child_choice_values(pdus, TE_PROTO_ETH,
                                      &eth_hdrs, &n_hdrs);
    if (rc != 0 || n_hdrs < 1)
    {
        ERROR("Failed to get eth PDU");
        goto out;
    }

    eth = eth_hdrs[n_hdrs - 1].value;

    n_tags = TE_ARRAY_LEN(vid);
    rc = tapi_ndn_eth_read_vlan_tci(eth, &n_tags, vid, prio, cfi);
    if (rc != 0)
    {
        ERROR("Failed to read VLAN TCI");
        goto out;
    }

    tapi_tad_vlan_zero_unspecified(n_tags, vid, prio, cfi);

#define PACK_VLAN_TCI(_prio, _cfi, _vid) \
    (_prio << 13) | (_cfi << 12) | _vid

    if (n_tags == 1)
    {
        rx_transform->effects |= RX_XFRM_EFFECT_VLAN_TCI;
        rx_transform->vlan_tci = PACK_VLAN_TCI(prio[0], cfi[0], vid[0]);

        if (rx_transform->hw_flags & RX_XFRM_HW_OFFL_VLAN_STRIP)
        {
            rc = asn_free_descendant(eth, "#eth.tagged");
            if (rc != 0)
            {
                ERROR("Failed to free VLAN tag");
                goto out;
            }
        }
    }
    else if (n_tags == 2)
    {
        rx_transform->effects |= RX_XFRM_EFFECT_VLAN_TCI;
        rx_transform->vlan_tci = PACK_VLAN_TCI(prio[0], cfi[0], vid[0]);

        rx_transform->effects |= RX_XFRM_EFFECT_OUTER_VLAN_TCI;
        rx_transform->outer_vlan_tci = PACK_VLAN_TCI(prio[1], cfi[1], vid[1]);

        if ((rx_transform->hw_flags & RX_XFRM_HW_OFFL_VLAN_STRIP) ||
            (rx_transform->hw_flags & RX_XFRM_HW_OFFL_QINQ_STRIP))
        {
            rc = asn_free_descendant(eth, "#eth.tagged");
            if (rc != 0)
            {
                ERROR("Failed to free VLAN tag");
                goto out;
            }

            if ((~rx_transform->hw_flags & RX_XFRM_HW_OFFL_VLAN_STRIP) ||
                (~rx_transform->hw_flags & RX_XFRM_HW_OFFL_QINQ_STRIP))
            {
                te_bool outer = (rx_transform->hw_flags &
                                 RX_XFRM_HW_OFFL_VLAN_STRIP);

                rc = tapi_ndn_pdus_inject_vlan_tags(pdus,
                                                    outer ? &vid[1] : &vid[0],
                                                    outer ? &prio[1] : &prio[0],
                                                    outer ? &cfi[1] : &cfi[0],
                                                    1);

                if (rc != 0)
                {
                    ERROR("Failed to inject VLAN tag");
                    goto out;
                }
            }
        }
    }

#undef PACK_VLAN_TCI

out:
    if (rc == 0)
    {
        asn_free_value(*ptrn);
        *ptrn = new_ptrn;
    }
    else
    {
        asn_free_value(new_ptrn);
    }

    return TE_RC(TE_TAPI, rc);
}
