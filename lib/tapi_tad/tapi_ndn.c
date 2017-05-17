/** @file
 * @brief Tester API for NDN
 *
 * Implementation of Tester API for NDN.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
#include "te_errno.h"
#include "logger_api.h"
#include "asn_impl.h"
#include "ndn.h"
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

            if (pdu_field == NULL)
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
tapi_tad_set_cksum_script_correct(asn_value  *proto_pdu,
                                  char       *du_cksum_label)
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

    err = asn_write_string(proto_pdu, "correct", du_cksum_label_choice);
    if (err != 0)
        goto out;

out:
    free(du_cksum_label_choice);

    return TE_RC(TE_TAPI, err);
}

static te_errno
tapi_tad_request_correct_cksums(uint32_t           hw_flags,
                                asn_child_desc_t  *ip4_pdus,
                                unsigned int       nb_ip4_pdus,
                                asn_child_desc_t  *ip6_pdus,
                                unsigned int       nb_ip6_pdus,
                                asn_child_desc_t  *udp_pdus,
                                unsigned int       nb_udp_pdus,
                                asn_value         *tcp_pdu)
{
    te_errno err = 0;
    te_bool  ip4_inner = FALSE;
    te_bool  ip4_outer = FALSE;

    if (nb_ip4_pdus + nb_ip6_pdus > TMPL_NB_IP_PDUS_MAX)
    {
        err = TE_EINVAL;
        goto out;
    }

    if ((nb_ip4_pdus == 1) && (nb_ip6_pdus == 1))
    {
        ip4_inner = (ip4_pdus[0].index < ip6_pdus[0].index);
        ip4_outer = !ip4_inner;
    }
    else
    {
        ip4_inner = (nb_ip4_pdus > 0);
        ip4_outer = (nb_ip4_pdus > 1);
    }


    if (((hw_flags & SEND_COND_HW_OFFL_IP_CKSUM) ==
         SEND_COND_HW_OFFL_IP_CKSUM) && ip4_inner)
    {
        err = tapi_tad_set_cksum_script_correct(ip4_pdus[0].value,
                                                "h-checksum");
        if (err != 0)
            goto out;
    }

    if (((hw_flags & SEND_COND_HW_OFFL_OUTER_IP_CKSUM) ==
         SEND_COND_HW_OFFL_OUTER_IP_CKSUM) && ip4_outer)
    {
        err = tapi_tad_set_cksum_script_correct(ip4_pdus[1].value,
                                                "h-checksum");
        if (err != 0)
            goto out;
    }

    if ((hw_flags & SEND_COND_HW_OFFL_L4_CKSUM) ==
        SEND_COND_HW_OFFL_L4_CKSUM)
    {
        unsigned int i;

        if (tcp_pdu != NULL)
        {
            err = tapi_tad_set_cksum_script_correct(tcp_pdu, "checksum");
            if (err != 0)
                goto out;
        }

        for (i = 0; i < nb_udp_pdus; ++i)
        {
            err = tapi_tad_set_cksum_script_correct(udp_pdus[i].value,
                                                    "checksum");
            if (err != 0)
                goto out;
        }
    }

out:
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
    asn_child_desc_t  *udp_pdus = NULL;
    unsigned int       nb_udp_pdus;
    asn_value         *tcp_pdu = NULL;
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

    err = asn_find_child_choice_values(pdus_copy, TE_PROTO_UDP,
                                       &udp_pdus, &nb_udp_pdus);
    if (err != 0)
        goto out;

    tcp_pdu = asn_find_child_choice_value(pdus_copy, TE_PROTO_TCP);

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
    }

    err = asn_write_value_field(pattern_unit, payload_data + *data_offset,
                                (is_tso) ? seg_len : payload_len,
                                "payload.#bytes");
    if (err != 0)
        goto out;

    *data_offset += (is_tso) ? seg_len : payload_len;

    if (transform != NULL)
    {
        err = tapi_tad_request_correct_cksums(transform->hw_flags,
                                              ip4_pdus, nb_ip4_pdus,
                                              ip6_pdus, nb_ip6_pdus,
                                              udp_pdus, nb_udp_pdus,
                                              tcp_pdu);
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
    int             payload_len;
    uint8_t        *payload_data = NULL;
    asn_value      *tcp_pdu = NULL;
    size_t          data_offset = 0;
    asn_value     **pattern_units = NULL;
    unsigned int    n_pattern_units = 0;

    err = asn_get_subvalue(packet, &pdus, "pdus");
    if (err != 0)
        goto out;

    payload_len = asn_get_length(packet, "payload.#bytes");
    if (payload_len < 0)
    {
        err = TE_EINVAL;
        goto out;
    }

    payload_data = malloc(payload_len);
    if (payload_data == NULL)
    {
        err = TE_ENOMEM;
        goto out;
    }

    err = asn_read_value_field(packet, (void *)payload_data,
                               (size_t *)&payload_len, "payload.#bytes");
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
