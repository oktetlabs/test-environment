/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IPv6 CSAP layer-related callbacks.
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
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
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD IPv6"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#include <netinet/ip6.h>

#include "te_defs.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"
#include "tad_ipstack_impl.h"

/* TODO: Move these defines somewhere in generic place */
#define IP6_HDR_LEN         40
#define IP6_HDR_PLEN_OFFSET 4

#define IF_RC_RETURN(_call) \
    if ((rc = (_call)) != 0) return rc
#define IF_NULL_RETURN(_call) \
    if ((_call) == NULL) return TE_RC(TE_TAD_CSAP, TE_ENOMEM)

/**
 * IPv6 layer specific data
 */

/** Structure to keep default values for parts of IPv6 header. */
typedef struct tad_ip6_proto_data {
    /** Default values for IPv6 Header fields */
    tad_bps_pkt_frag_def    hdr;
    /** Default values for Options Header (Hop-By-Hop and Destination) */
    tad_bps_pkt_frag_def    opts_hdr;

    /** Default values for PAD1 option */
    tad_bps_pkt_frag_def    opt_pad1;
    /** Default values for generic TLV option */
    tad_bps_pkt_frag_def    opt_tlv;
    /** DEfault values for Router Alert option */
    tad_bps_pkt_frag_def    opt_ra;

    /**
     * The value for the last "next-header" field in the list
     * of extension headers.
     */
    uint8_t                 upper_protocol;
} tad_ip6_proto_data;

/**
 * Structure to keep information about an Option specified
 * in one of extension headers (in template PDU).
 */
typedef struct tad_ip6_ext_hdr_opt_data {
    /** Option-specific values obtained from layer PDU template */
    tad_bps_pkt_frag_data  opt;
    /** Pointer to the default values for this option */
    tad_bps_pkt_frag_def  *opt_def;
} tad_ip6_ext_hdr_opt_data;

/** Structure to keep information about Extension Header */
typedef struct tad_ip6_ext_hdr_data {
    /** Actual values for Extension header */
    tad_bps_pkt_frag_data     hdr;
    /** Pointer to the default values for this header */
    tad_bps_pkt_frag_def     *hdr_def;
    /** The number of options in this Extension header */
    uint32_t                  opts_num;
    /** An array of options */
    tad_ip6_ext_hdr_opt_data *opts;
    /** The number of bytes used for options in this Extension header */
    uint32_t                  opts_len;
} tad_ip6_ext_hdr_data;

/**
 * IPv6 layer specific data for PDU processing (both send and receive).
 */
typedef struct tad_ip6_proto_pdu_data {
    /** Data for IPv6 header */
    tad_bps_pkt_frag_data   hdr;
    /** An array of extension headers */
    tad_ip6_ext_hdr_data   *ext_hdrs;
    /** The number of extension headers in 'ext_hdrs' array */
    uint32_t                ext_hdrs_num;
    /** Length of all IPv6 extension headers in bytes */
    uint32_t                ext_hdrs_len;
} tad_ip6_proto_pdu_data;

/**
 * Definition of Internet Protocol version 6 (IPv6) header (see RFC 2460).
 */
static const tad_bps_pkt_frag tad_ip6_bps_hdr[] =
{
    { "version",          4, BPS_FLD_CONST(6),
      TAD_DU_I32, FALSE },
    { "traffic-class",    8, BPS_FLD_CONST_DEF(NDN_TAG_IP6_TCL, 0),
      TAD_DU_I32, TRUE },
    { "flow-label",      20, BPS_FLD_CONST_DEF(NDN_TAG_IP6_FLAB, 0),
      TAD_DU_I32, FALSE },
    { "payload-length",  16, BPS_FLD_CONST_DEF(NDN_TAG_IP6_LEN, 0),
      TAD_DU_I32, TRUE },
    { "next-header",      8, BPS_FLD_SIMPLE(NDN_TAG_IP6_NEXT_HEADER),
      TAD_DU_I32, FALSE },
    { "hop-limit",        8, BPS_FLD_CONST_DEF(NDN_TAG_IP6_HLIM, 64),
      TAD_DU_I32, FALSE },
    { "src-addr",       128, NDN_TAG_IP6_SRC_ADDR,
      NDN_TAG_IP6_LOCAL_ADDR, NDN_TAG_IP6_REMOTE_ADDR, 0,
      TAD_DU_OCTS, FALSE },
    { "dst-addr",       128, NDN_TAG_IP6_DST_ADDR,
      NDN_TAG_IP6_REMOTE_ADDR, NDN_TAG_IP6_LOCAL_ADDR, 0,
      TAD_DU_OCTS, FALSE },
};

/**
 * Definition of Options Header type:
 * - Hop-by-Hop Options Header (RFC2460, section 4.3)
 * - Destination Options Header (RFC2460, section 4.6)
 */
static const tad_bps_pkt_frag tad_ip6_ext_hdr_opts_bps_hdr[] =
{
    { "next-header", 8, BPS_FLD_NO_DEF(NDN_TAG_IP6_NEXT_HEADER),
      TAD_DU_I32, FALSE },
    { "length", 8, BPS_FLD_NO_DEF(NDN_TAG_IP6_EXT_HEADER_LEN),
      TAD_DU_I32, FALSE },
};

/* Generic TLV Option */
static const tad_bps_pkt_frag tad_ip6_tlv_option[] =
{
    { "type",   8, BPS_FLD_NO_DEF(NDN_TAG_IP6_EXT_HEADER_OPT_TYPE),
      TAD_DU_I32, FALSE },
    { "length", 8, BPS_FLD_NO_DEF(NDN_TAG_IP6_EXT_HEADER_OPT_LEN),
      TAD_DU_I32, FALSE },
    { "data",   0,
      BPS_FLD_CONST_DEF(NDN_TAG_IP6_EXT_HEADER_OPT_DATA, 0),
      TAD_DU_OCTS, FALSE },
};

/* PAD1 Option */
static const tad_bps_pkt_frag tad_ip6_pad1_option[] =
{
    { "type", 8, BPS_FLD_CONST(IP6OPT_PAD1), TAD_DU_I32, FALSE },
};

/* FIXME: This constant should be defined in the proper place. */
#ifndef IP6OPT_ROUTER_ALERT
#define IP6OPT_ROUTER_ALERT 0x05
#endif

/* Router Alert Option (see RFC 2711) */
static const tad_bps_pkt_frag tad_ip6_ra_option[] =
{
    { "type",   8, BPS_FLD_CONST(IP6OPT_ROUTER_ALERT), TAD_DU_I32, FALSE },
    { "length", 8, BPS_FLD_CONST(2), TAD_DU_I32, FALSE },
    { "value", 16, BPS_FLD_NO_DEF(NDN_TAG_IP6_EXT_HEADER_OPT_VALUE),
      TAD_DU_I32, FALSE },
};

static te_errno
tad_bps_pkt_frag_data_get_oct_str(tad_bps_pkt_frag_def *def,
                                  tad_bps_pkt_frag_data *data,
                                  asn_tag_value tag,
                                  size_t len, const uint8_t **ptr)
{
    unsigned int i;

    for (i = 0; i < def->fields; i++)
    {
        if (def->descr[i].tag == tag)
        {
            tad_data_unit_t *du;

            if (data->dus[i].du_type != TAD_DU_UNDEF)
                du = data->dus + i;
            else if (def->tx_def[i].du_type != TAD_DU_UNDEF)
                du = def->tx_def + i;
            else
            {
                ERROR("%s(): Missing specification for '%s' to get data",
                      __FUNCTION__, def->descr[i].name);
                return TE_RC(TE_TAD_CSAP, TE_ETADMISSNDS);
            }
            if (du->du_type != TAD_DU_OCTS)
            {
                ERROR("Field %s is not OCTET STRING", def->descr[i].name);
                return TE_RC(TE_TAD_CSAP, TE_EINVAL);
            }
            if (du->val_data.len < len)
            {
                ERROR("The length of %s field value is %d, not %d",
                      def->descr[i].name,
                      du->val_data.len, len);
                return TE_RC(TE_TAD_CSAP, TE_EINVAL);
            }
            *ptr = du->val_data.oct_str;
            return 0;
        }
    }
    return TE_RC(TE_TAD_CSAP, TE_ENOENT);
}

/**
 * Convert te_tad_protocols_t into IANA protocol numbers.
 *
 * @param  te_proto TE enumeration for protocol value
 *
 * @return IANA protocol number.
 */
static uint8_t
tad_te_proto2ip_proto(te_tad_protocols_t te_proto)
{
    switch (te_proto)
    {
#define TE_PROTO2IP_PROTO(te_val_, ip_val_) \
        case TE_PROTO_ ## te_val_: return IPPROTO_ ## ip_val_

#ifdef IPPROTO_IPIP
        TE_PROTO2IP_PROTO(IP4, IPIP);
#elif defined(IPPROTO_ENCAP)
        TE_PROTO2IP_PROTO(IP4, ENCAP);
#endif
        TE_PROTO2IP_PROTO(UDP, UDP);
        TE_PROTO2IP_PROTO(TCP, TCP);
        TE_PROTO2IP_PROTO(ICMP4, ICMP);
        TE_PROTO2IP_PROTO(IGMP, IGMP);
        TE_PROTO2IP_PROTO(IP6, IPV6);
        TE_PROTO2IP_PROTO(ICMP6, ICMPV6);

#undef TE_PROTO2IP_PROTO

        default:
            return IPPROTO_NONE;
    }
}

/* See description tad_ipstack_impl.h */
te_errno
tad_ip6_init_cb(csap_p csap, unsigned int layer)
{
    UNUSED(csap);
    UNUSED(layer);
    te_errno            rc;
    tad_ip6_proto_data *proto_data;
    const asn_value    *layer_nds;

    IF_NULL_RETURN(proto_data = TE_ALLOC(sizeof(*proto_data)));

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    IF_RC_RETURN(tad_bps_pkt_frag_init(tad_ip6_bps_hdr,
                               TE_ARRAY_LEN(tad_ip6_bps_hdr),
                               layer_nds, &proto_data->hdr));

    IF_RC_RETURN(tad_bps_pkt_frag_init(tad_ip6_ext_hdr_opts_bps_hdr,
                               TE_ARRAY_LEN(tad_ip6_ext_hdr_opts_bps_hdr),
                               NULL, &proto_data->opts_hdr));

    IF_RC_RETURN(tad_bps_pkt_frag_init(tad_ip6_tlv_option,
                               TE_ARRAY_LEN(tad_ip6_tlv_option),
                               NULL, &proto_data->opt_tlv));

    IF_RC_RETURN(tad_bps_pkt_frag_init(tad_ip6_pad1_option,
                               TE_ARRAY_LEN(tad_ip6_pad1_option),
                               NULL, &proto_data->opt_pad1));

    IF_RC_RETURN(tad_bps_pkt_frag_init(tad_ip6_ra_option,
                               TE_ARRAY_LEN(tad_ip6_ra_option),
                               NULL, &proto_data->opt_ra));

    if (layer > 0)
        proto_data->upper_protocol =
                tad_te_proto2ip_proto(csap->layers[layer - 1].proto_tag);
    else
        proto_data->upper_protocol = IPPROTO_NONE;

    return 0;
}


/* See description tad_ipstack_impl.h */
te_errno
tad_ip6_destroy_cb(csap_p csap, unsigned int layer)
{
    UNUSED(csap);
    UNUSED(layer);
    tad_ip6_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);
    tad_bps_pkt_frag_free(&proto_data->opts_hdr);
    tad_bps_pkt_frag_free(&proto_data->opt_tlv);
    tad_bps_pkt_frag_free(&proto_data->opt_pad1);
    tad_bps_pkt_frag_free(&proto_data->opt_ra);

    free(proto_data);
    return 0;
}

/**
 * Convert traffic template NDS to BPS internal data and
 * check the result for completeness.
 *
 * @param def   default values for packet fragment
 * @param nds   ASN value for this packet fragment
 * @param data  BPS internal data to fill and check
 *
 * @return Status of the operation
 */
static te_errno
tad_ip6_nds_to_data_and_confirm(tad_bps_pkt_frag_def *def, asn_value *nds,
                                tad_bps_pkt_frag_data *data)
{
    te_errno rc;

    IF_RC_RETURN(tad_bps_nds_to_data_units(def, nds, data));

    return tad_bps_confirm_send(def, data);
}

/**
 * Process options of IPv6 Options Extension header.
 *
 * @param proto_data  layer-specific context
 * @param hdr_data    options header context to be filled in
 *                    with option-specific values
 * @parma opts        ASN sequence of options
 *
 * @return Status of the operation
 */
static te_errno
opts_hdr_process_opts(tad_ip6_proto_data *proto_data,
                      tad_ip6_ext_hdr_data *hdr_data, asn_value *opts)
{
    asn_value *opt;
    int        opts_num;
    int        opt_len;
    int        i;
    int        rc;

    hdr_data->opts = NULL;
    hdr_data->opts_num = 0;
    hdr_data->opts_len = 0;

    opts_num = asn_get_length(opts, "");
    if (opts_num <= 0)
        return 0;

    IF_NULL_RETURN(
        hdr_data->opts = TE_ALLOC(opts_num * sizeof(*hdr_data->opts)));

    hdr_data->opts_num = opts_num;

    for (i = 0; i < opts_num; i++)
    {
        rc = asn_get_indexed(opts, &opt, i, "");
        if (rc == 0)
        {
            asn_tag_class t_cl;
            asn_tag_value t_val;
            int           val;

            asn_get_choice_value(opt, &opt, &t_cl, &t_val);
            switch (t_val)
            {
                case NDN_TAG_IP6_EXT_HEADER_OPT_PAD1:
                    /* PAD1 option is 1 byte length option. */
                    IF_RC_RETURN(tad_ip6_nds_to_data_and_confirm(
                                                &proto_data->opt_pad1, opt,
                                                &hdr_data->opts[i].opt));

                    hdr_data->opts[i].opt_def = &proto_data->opt_pad1;
                    hdr_data->opts_len += 1;

                    INFO("Option PAD1");
                    break;

                case NDN_TAG_IP6_EXT_HEADER_OPT_TLV:
                    /*
                     * Check if we need to detect the value
                     * for Length field
                     */
                    if ((rc = asn_read_int32(opt, &val, "length")) != 0)
                    {
                        if ((opt_len = asn_get_length(opt, "data")) >= 0)
                        {
                            rc = asn_write_int32(opt, opt_len,
                                                 "length.#plain");
                            if (rc != 0)
                            {
                                ERROR("Failed to write 'length' "
                                      "field for TLV option, %r", rc);
                                return rc;
                            }
                        }
                    }
                    IF_RC_RETURN(tad_ip6_nds_to_data_and_confirm(
                                                &proto_data->opt_tlv, opt,
                                                &hdr_data->opts[i].opt));

                    INFO("Option TLV");

                    hdr_data->opts[i].opt_def = &proto_data->opt_tlv;
                    hdr_data->opts_len += (2 + asn_get_length(opt, "data"));
                    break;

                case NDN_TAG_IP6_EXT_HEADER_OPT_ROUTER_ALERT:
                    IF_RC_RETURN(tad_ip6_nds_to_data_and_confirm(
                                                &proto_data->opt_ra, opt,
                                                &hdr_data->opts[i].opt));

                    INFO("Option Router-Alert");

                    hdr_data->opts[i].opt_def = &proto_data->opt_ra;
                    /*
                     * Router Alert Option (see RFC 2711):
                     * It is fixed length option (type, length, value)
                     * where type and length are 1 byte fields and value
                     * is two bytes field.
                     */
                    hdr_data->opts_len += (2 + 2);
                    break;


                default:
                    ERROR("Unsupported option type");
                    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
            }
        }
    }
    return 0;
}

/**
 * Convert ASN TAG value of IPv6 Extension Header type to
 * IANA constant for Next-Header value.
 *
 * @param tag  ASN TAG of an extension header
 *
 * @return IANA value for this extension header
 */
static unsigned int
next_hdr_tag2bin(asn_tag_value tag)
{
    switch (tag)
    {
        case NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP:
            return IPPROTO_HOPOPTS;
        case NDN_TAG_IP6_EXT_HEADER_DESTINATION:
            return IPPROTO_DSTOPTS;
        default:
            ERROR("%s() Unsupported TAG %d specified", tag);
            return 0xff;
    }
}

static te_errno
fill_tmpl_addr(asn_value *tmpl,
               tad_bps_pkt_frag_def *def, tad_bps_pkt_frag_data *data,
               asn_tag_value tag, const char *label)
{
    const uint8_t *ip6_addr;
    void          *tmp_ptr;
    te_errno       rc;

    /* Check if we have an address specified */
    if (asn_get_field_data(tmpl, (void *)&tmp_ptr, label) == 0)
        return 0;

    IF_RC_RETURN(tad_bps_pkt_frag_data_get_oct_str(def, data, tag,
                                           IP6_ADDR_LEN, &ip6_addr));

    if ((rc = asn_write_value_field(tmpl, ip6_addr,
                                    IP6_ADDR_LEN, label)) != 0)
    {
        ERROR("Failed to set '%s' field, %r", label, rc);
    }
    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip6_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    tad_ip6_proto_data     *proto_data;
    tad_ip6_proto_pdu_data *tmpl_data;
    tad_bps_pkt_frag_def   *ext_hdr_def = NULL;
    uint32_t                ext_hdr_id = 0;
    asn_value              *prev_hdr = layer_pdu;
    asn_value              *hdrs;
    void                   *tmp_ptr;
    int                     val;
    te_errno                rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    tmpl_data = TE_ALLOC(sizeof(*tmpl_data));
    *p_opaque = tmpl_data;
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    tmpl_data->ext_hdrs = NULL;
    tmpl_data->ext_hdrs_num = 0;
    tmpl_data->ext_hdrs_len = 0;

    rc = asn_get_descendent(layer_pdu, &hdrs, "ext-headers");
    if (rc == 0)
    {
        asn_value *hdr;
        asn_value *opts;
        int        hdr_num;
        int        i;

        hdr_num = asn_get_length(hdrs, "");
        if (hdr_num <= 0)
            goto ext_hdr_end;

        tmpl_data->ext_hdrs =
                    TE_ALLOC(hdr_num * sizeof(*tmpl_data->ext_hdrs));

        if (tmpl_data->ext_hdrs == NULL)
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

        tmpl_data->ext_hdrs_num = hdr_num;

        for (i = 0; i < hdr_num; i++, prev_hdr = hdr)
        {
            rc = asn_get_indexed(hdrs, &hdr, i, "");
            if (rc == 0)
            {
                asn_tag_class t_cl;
                asn_tag_value t_val;

                rc = asn_get_choice_value(hdr, &hdr, &t_cl, &t_val);
                if (rc != 0)
                    return rc;

                /*
                 * Update "Next-Header" field of IPv6 header or Extension
                 * Headers in case it is not specified in layer PDU.
                 */
                if ((rc = asn_read_int32(prev_hdr, &val,
                                         "next-header")) != 0)
                {
                    rc = asn_write_int32(prev_hdr,
                                         next_hdr_tag2bin(t_val),
                                         "next-header.#plain");
                    if (rc != 0)
                        return rc;

                    /*
                     * Convert and check only Extension headers.
                     * IPv6 Header will be validated and converted
                     * in the end.
                     */
                    if (ext_hdr_def != NULL)
                    {
                        rc = tad_ip6_nds_to_data_and_confirm(
                                    ext_hdr_def, prev_hdr,
                                    &tmpl_data->ext_hdrs[ext_hdr_id].hdr);
                        if (rc != 0)
                            return rc;
                    }
                }

                switch (t_val)
                {
                    case NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP:
                    case NDN_TAG_IP6_EXT_HEADER_DESTINATION:
                        INFO("Header type %s",
                             t_val == NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP ?
                                 "Hop-by-Hop" : "Destination");
                        rc = asn_get_descendent(hdr, &opts, "options");
                        if (rc != 0)
                            return rc;
                        rc = opts_hdr_process_opts(
                                            proto_data,
                                            &tmpl_data->ext_hdrs[i], opts);
                        if (rc != 0)
                            return rc;

                        if ((rc = asn_read_int32(hdr, &val, "length")) != 0)
                        {
                            if (tmpl_data->ext_hdrs[i].opts_len == 0 ||
                                (tmpl_data->ext_hdrs[i].opts_len
                                                            + 2) % 8 != 0)
                            {
                                ERROR("Total length of options is "
                                      "not correct %d",
                                      tmpl_data->ext_hdrs[i].opts_len);
                                return TE_RC(TE_TAD_CSAP, TE_ETADCSAPSTATE);
                            }
                            rc = asn_write_int32(
                                    hdr,
                                    (tmpl_data->ext_hdrs[i].opts_len
                                                            + 2) / 8  - 1,
                                    "length.#plain");
                            if (rc != 0)
                                return rc;
                        }
                        tmpl_data->ext_hdrs_len +=
                                    (2 + tmpl_data->ext_hdrs[i].opts_len);
                        tmpl_data->ext_hdrs[i].hdr_def =
                                    &proto_data->opts_hdr;
                        ext_hdr_def = &proto_data->opts_hdr;
                        ext_hdr_id = i;
                        break;

                    default:
                        ERROR("Not supported IPv6 Extension header");
                        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPSTATE);
                }
            }
        }
    }
ext_hdr_end:

    /*
     * Set the last "next-header" field (either the field of IPv6 header or
     * the field of the last extension header) to upper layer protocol
     */
    if ((rc = asn_read_int32(prev_hdr, &val, "next-header")) != 0)
    {
        rc = asn_write_int32(prev_hdr, proto_data->upper_protocol,
                             "next-header.#plain");
        if (rc != 0)
            return rc;
    }

    /* Convert the last Extension Header */
    if (ext_hdr_def != NULL)
    {
        rc = tad_ip6_nds_to_data_and_confirm(
                                    ext_hdr_def, prev_hdr,
                                    &tmpl_data->ext_hdrs[ext_hdr_id].hdr);
        if (rc != 0)
            return rc;
    }

    /* Check IPv6 Header */
    rc = tad_ip6_nds_to_data_and_confirm(&proto_data->hdr, layer_pdu,
                                         &tmpl_data->hdr);
    if (rc != 0)
        return rc;

    /*
     * In case destination IPv6 address is multicast and there is no
     * MAC address specified in Ethernet layer template, then map IPv6
     * address to Ethernet multicast address according to RFC2464.
     */
    if ((layer + 1) < csap->depth &&
        csap->layers[layer + 1].proto_tag == TE_PROTO_ETH &&
        csap->layers[layer + 1].pdu != NULL &&
        asn_get_field_data(csap->layers[layer + 1].pdu,
                   (void *)&tmp_ptr, "dst-addr.#plain") == TE_EASNINCOMPLVAL)
    {
        const uint8_t *ip6_dst;

        rc = tad_bps_pkt_frag_data_get_oct_str(&proto_data->hdr,
                                               &tmpl_data->hdr,
                                               NDN_TAG_IP6_DST_ADDR,
                                               IP6_ADDR_LEN, &ip6_dst);
        if (rc != 0)
            return rc;

        if (IN6_IS_ADDR_MULTICAST((struct in6_addr *)ip6_dst))
        {
            uint8_t mcast_mac[6] = { 0x33, 0x33, };

            mcast_mac[2] = ip6_dst[12];
            mcast_mac[3] = ip6_dst[13];
            mcast_mac[4] = ip6_dst[14];
            mcast_mac[5] = ip6_dst[15];
            rc = asn_write_value_field(csap->layers[layer + 1].pdu,
                                       mcast_mac, sizeof(mcast_mac),
                                       "dst-addr.#plain");
            if (rc != 0)
            {
                ERROR("Failed to set Ethernet 'dst-addr' to "
                      "IPv6 multicast mapped value, %r", rc);
                return rc;
            }
        }
    }

    /*
     * Set IPv6 SRC and DST addresses in template in order to let
     * upper layers to build pseudo header for checksum calculation.
     */
    rc = fill_tmpl_addr(layer_pdu, &proto_data->hdr, &tmpl_data->hdr,
                        NDN_TAG_IP6_DST_ADDR, "dst-addr.#plain");
    if (rc != 0)
        return rc;
    rc = fill_tmpl_addr(layer_pdu, &proto_data->hdr, &tmpl_data->hdr,
                        NDN_TAG_IP6_SRC_ADDR, "src-addr.#plain");

    return rc;
}

typedef struct tad_ip6_gen_bin_cb_per_pdu_data {
    uint8_t    *hdr;
    te_bool     use_phdr;
    uint32_t    init_checksum;
    int         upper_checksum_offset;
} tad_ip6_gen_bin_cb_per_pdu_data;

/**
 * Callback to set up the correct value of Payload-Length
 * field in IPv6 Header.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_ip6_gen_bin_cb_per_pdu(tad_pkt *pdu, void *opaque)
{
    size_t                              len = tad_pkt_len(pdu);
    uint16_t                            tmp;
    tad_pkt_seg                        *seg = tad_pkt_first_seg(pdu);
    tad_ip6_gen_bin_cb_per_pdu_data    *data = opaque;

    if (len > 0xffff)
    {
        ERROR("PDU is too big to be IP6 PDU");
        return TE_RC(TE_TAD_CSAP, TE_E2BIG);
    }

    assert(seg->data_ptr != NULL);
    assert(seg->data_len >= IP6_HDR_LEN);

    /* Copy IPv6 Header with extension headers */
    memcpy(seg->data_ptr, data->hdr, seg->data_len);

    /* Set correct Payload-Length in the header template */
    tmp = htons(len - IP6_HDR_LEN);
    memcpy(seg->data_ptr + IP6_HDR_PLEN_OFFSET, &tmp, sizeof(tmp));

    return 0;
}

typedef struct tad_ip6_upper_checksum_seg_cb_data {
    uint32_t    checksum;   /**< Accumulated checksum */
} tad_ip6_upper_checksum_seg_cb_data;

static te_errno
tad_ip6_upper_checksum_seg_cb(const tad_pkt *pkt, tad_pkt_seg *seg,
                              unsigned int seg_num, void *opaque)
{
    tad_ip6_upper_checksum_seg_cb_data *data = opaque;

    /* Data length is even or it is the last segument */
    assert(((seg->data_len & 1) == 0) ||
           (seg_num == tad_pkt_seg_num(pkt) - 1));
    data->checksum += calculate_checksum(seg->data_ptr, seg->data_len);

    return 0;
}

static te_errno
tad_ip6_upper_checksum_cb(tad_pkt *pdu, void *opaque)
{
    tad_ip6_gen_bin_cb_per_pdu_data    *data = opaque;
    tad_pkt_seg                        *seg = tad_pkt_first_seg(pdu);
    size_t                              len = tad_pkt_len(pdu);
    uint16_t                            tmp;
    tad_ip6_upper_checksum_seg_cb_data  seg_data;

    if (data->upper_checksum_offset == -1)
    {
        return 0;
    }

    if (len > 0xffff)
    {
        ERROR("PDU is too big to be IP6 PDU");
        return TE_RC(TE_TAD_CSAP, TE_E2BIG);
    }

    if (seg == NULL ||
        seg->data_len < (size_t)(data->upper_checksum_offset) + 2)
    {
        ERROR("Skip calculation of upper layer checksum, since "
              "the first segment of IPv6 PDU is too short");
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    seg_data.checksum = data->init_checksum;

    if (data->use_phdr)
    {
        tmp = htons(len);
        seg_data.checksum += calculate_checksum(&tmp, sizeof(tmp));
    }

    /* Preset checksum field by zeros */
    memset((uint8_t *)seg->data_ptr +
           data->upper_checksum_offset, 0, 2);

    /* Upper layer data checksum */
    (void)tad_pkt_enumerate_seg(pdu,
                                tad_ip6_upper_checksum_seg_cb,
                                &seg_data);

    /* Finalize checksum calculation */
    tmp = ~((seg_data.checksum & 0xffff) +
            (seg_data.checksum >> 16));

    /* Write calculcated checksum to packet */
    memcpy((uint8_t *)seg->data_ptr +
           data->upper_checksum_offset, &tmp, sizeof(tmp));

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip6_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num,
                   tad_pkts *sdus, tad_pkts *pdus)
{
    tad_ip6_proto_data                 *proto_data;
    tad_ip6_proto_pdu_data             *tmpl_data = opaque;
    tad_ip6_gen_bin_cb_per_pdu_data     cb_data;
    uint32_t                            hdrlen;
    te_errno                            rc;
    size_t                              bitlen;
    unsigned int                        bitoff;
    unsigned int                        prev_bitoff;
    unsigned int                        dst_addr_offset;
    uint32_t                            i, j;
    uint16_t                            tmp;
    const asn_value                    *pld_checksum;
    int32_t                             pld_checksum_val;
    asn_tag_value                       tv;
    uint8_t                             next_header;

    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Calculate IPv6 header length */
    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr,
                                          &tmpl_data->hdr);

    /* Add length of all IPv6 extension headers */
    bitlen += tmpl_data->ext_hdrs_len * 8;

    assert((bitlen & 7) == 0);

    hdrlen = (((bitlen >> 3) + 3) >> 2) << 2;

    /* Allocate memory for binary template of the header */
    if ((cb_data.hdr = TE_ALLOC(hdrlen)) == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;
    if ((rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                       args, arg_num, cb_data.hdr,
                                       &bitoff, bitlen)) != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        goto cleanup;
    }

#define IP6_ADDRLEN                 16
#define IP6_SRC_OFFSET              8
#define IP6_DST_OFFSET              IP6_SRC_OFFSET + IP6_ADDRLEN
#define IP6_NEXT_HEADER_OFFSET      6
#define IP6_NEXT_HEADER(_ptr) \
    *((uint8_t *)(_ptr) + IP6_NEXT_HEADER_OFFSET)
#define IP6_ROUTING_HEADER          43
#define IP6_EXT_NEXT_HEADER_OFFSET  0
#define IP6_HDR_EXT_LEN_OFFSET      1
#define IP6_ROUTING_TYPE_OFFSET 1
#define IP6_EXT_NEXT_HEADER(_ptr) \
    *((uint8_t *)(_ptr) + IP6_EXT_NEXT_HEADER_OFFSET)
#define IP6_HDR_EXT_LEN(_ptr) \
    *((uint8_t *)(_ptr) + IP6_HDR_EXT_LEN_OFFSET)
#define IP6_ROUTING_TYPE(_ptr) \
    *((uint8_t *)(_ptr) + IP6_ROUTING_TYPE_OFFSET)
#define IP6_HDR_EXT_LEN_MULTIPLE    8

    dst_addr_offset = IP6_DST_OFFSET;

    next_header = IP6_NEXT_HEADER(cb_data.hdr);
    for (i = 0; i < tmpl_data->ext_hdrs_num; i++)
    {
        prev_bitoff = bitoff;

        if ((rc = tad_bps_pkt_frag_gen_bin(tmpl_data->ext_hdrs[i].hdr_def,
                                      &tmpl_data->ext_hdrs[i].hdr,
                                      args, arg_num, cb_data.hdr,
                                      &bitoff, bitlen)) != 0)
            goto cleanup;

        for (j = 0; j < tmpl_data->ext_hdrs[i].opts_num; j++)
        {
            if ((rc = tad_bps_pkt_frag_gen_bin(
                            tmpl_data->ext_hdrs[i].opts[j].opt_def,
                            &tmpl_data->ext_hdrs[i].opts[j].opt,
                            args, arg_num, cb_data.hdr,
                            &bitoff, bitlen)) != 0)
                goto cleanup;
        }

        /*
         * prev_bitoff points to the origin of generated extension header.
         */
        if (next_header == IP6_ROUTING_HEADER &&
            IP6_ROUTING_TYPE(cb_data.hdr + prev_bitoff) == 0)
        {
            /* Calculate last IP displacement (RFC 2460) */
            dst_addr_offset =
                /* Base offset value is beginning of routing header */
                prev_bitoff +
                    /*
                     * Displacement forward to the end of Routing header.
                     * Multiple - eight octets. First eight octets are
                     * not calculetad in hdr_ext_len field
                     */
                    IP6_HDR_EXT_LEN_MULTIPLE *
                        (IP6_HDR_EXT_LEN(cb_data.hdr + prev_bitoff) + 1) -
                        /*
                         * Displacement back to the origin of last IP
                         * in the list
                         */
                        IP6_ADDRLEN;
        }

        next_header = IP6_EXT_NEXT_HEADER(cb_data.hdr + prev_bitoff);
    }

    assert(bitoff == bitlen);

    /* Move all fragments to IPv6 PDUs */
    tad_pkts_move(pdus, sdus);

    /* Calculate upper layer checksum */
    tmp = htons((uint16_t)(proto_data->upper_protocol));
    cb_data.init_checksum = calculate_checksum(&tmp, sizeof(tmp));
    cb_data.use_phdr = TRUE;
    switch(proto_data->upper_protocol)
    {
        case IPPROTO_TCP:
            cb_data.upper_checksum_offset = 16;
            break;
        case IPPROTO_UDP:
            cb_data.upper_checksum_offset = 6;
            break;
        default:
            cb_data.init_checksum = 0;
            cb_data.use_phdr = FALSE;
            cb_data.upper_checksum_offset = -1;
            break;
    }

    rc = asn_get_child_value(tmpl_pdu, &pld_checksum,
                             PRIVATE, NDN_TAG_IP6_PLD_CHECKSUM);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        /* Not specified. Do nothing. */
    }
    else if (rc != 0)
    {
        ERROR("%s(): asn_get_child_value() failed for 'pld-checksum': "
              "%r", __FUNCTION__, rc);
        rc = TE_RC(TE_TAD_CSAP, rc);
        goto cleanup;
    }
    else
    {
        if ((rc = asn_get_choice_value(pld_checksum,
                                       (asn_value **)&pld_checksum,
                                       NULL, &tv)) != 0)
        {
            ERROR("%s(): asn_get_choice_value() failed for "
                  "'pld-checksum': %r", __FUNCTION__, rc);
            goto cleanup;
        }

        switch (tv)
        {
            case NDN_TAG_IP6_PLD_CH_DISABLE:
                cb_data.upper_checksum_offset = -1;
                break;

            case NDN_TAG_IP6_PLD_CH_OFFSET:
                if ((rc = asn_read_int32(pld_checksum,
                                         &pld_checksum_val,
                                         NULL)) != 0)
                {
                    ERROR("%s(): asn_read_int32() failed for "
                          "'pld-checksum.#offset': %r", __FUNCTION__, rc);
                          goto cleanup;
                }
                cb_data.upper_checksum_offset = pld_checksum_val;
                break;

            case NDN_TAG_IP6_PLD_CH_DIFF:
                if ((rc = asn_read_int32(pld_checksum,
                                         &pld_checksum_val,
                                         NULL)) != 0)
                {
                    ERROR("%s(): asn_read_int32() failed for "
                          "'pld-checksum.#diff': %r", __FUNCTION__, rc);
                          goto cleanup;
                }
                cb_data.init_checksum += pld_checksum_val;
                break;

            default:
                ERROR("%s(): Unexpected choice tag value for "
                      "'pld-checksum'", __FUNCTION__);
                rc = TE_RC(TE_TAD_CSAP, TE_EASNOTHERCHOICE);
                goto cleanup;
        }
    }

    if (cb_data.upper_checksum_offset != -1)
    {
        if (cb_data.use_phdr)
        {
            cb_data.init_checksum += calculate_checksum(
                                            cb_data.hdr + IP6_SRC_OFFSET,
                                            IP6_ADDRLEN);

            cb_data.init_checksum += calculate_checksum(
                                            cb_data.hdr + dst_addr_offset,
                                            IP6_ADDRLEN);
        }

        tad_pkt_enumerate(pdus, tad_ip6_upper_checksum_cb,
                          &cb_data);
    }

    /*
     * Prepend each packet with space necessary for
     * IPv6 Header together with all extension headers.
    */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, hdrlen, NULL);
    if (rc != 0)
        goto cleanup;

    /*
     * Per-PDU processing - set correct Payload
     * Length value of IPv6 Header
     */
    rc = tad_pkt_enumerate(pdus, tad_ip6_gen_bin_cb_per_pdu, &cb_data);

cleanup:
    free(cb_data.hdr);

    return rc;
}

void
tad_ip6_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_ip6_proto_data     *proto_data;
    tad_ip6_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        uint32_t i, j;

        tad_bps_free_pkt_frag_data(&proto_data->hdr, &pdu_data->hdr);

        for (i = 0; i < pdu_data->ext_hdrs_num; i++)
        {
            for (j = 0; j < pdu_data->ext_hdrs[i].opts_num; j++)
            {
                if (pdu_data->ext_hdrs[i].opts[j].opt_def != NULL)
                {
                    tad_bps_free_pkt_frag_data(
                            pdu_data->ext_hdrs[i].opts[j].opt_def,
                            &pdu_data->ext_hdrs[i].opts[j].opt);
                }
            }
            free(pdu_data->ext_hdrs[i].opts);
            if (pdu_data->ext_hdrs[i].hdr_def != NULL)
                tad_bps_free_pkt_frag_data(pdu_data->ext_hdrs[i].hdr_def,
                                           &pdu_data->ext_hdrs[i].hdr);
        }
        free(pdu_data->ext_hdrs);
        free(pdu_data);
    }
}

te_errno
tad_ip6_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                        asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc = 0;
    tad_ip6_proto_data         *proto_data;
    tad_ip6_proto_pdu_data     *ptrn_data;
    tad_bps_pkt_frag_def       *ext_hdr_def = NULL;
    uint32_t                    ext_hdr_id = 0;
    asn_value                  *prev_hdr = layer_pdu;
    asn_value                  *hdrs;
    int                         val;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
                (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    IF_NULL_RETURN(*p_opaque =
                        ptrn_data =
                            TE_ALLOC(sizeof(*ptrn_data)));

    memset(ptrn_data, 0, sizeof(*ptrn_data));

    if ((rc = asn_get_descendent(layer_pdu, &hdrs, "ext-headers")) == 0)
    {/* Process extension headers */
        asn_value *hdr;
        asn_value *opts;
        int        hdr_num;
        int        i;

        if ((hdr_num = asn_get_length(hdrs, "")) <= 0)
            goto ext_hdr_end;

        IF_NULL_RETURN(
            ptrn_data->ext_hdrs =
                    TE_ALLOC(hdr_num * sizeof(*ptrn_data->ext_hdrs)));

        ptrn_data->ext_hdrs_num = hdr_num;

        for (i = 0; i < hdr_num; i++, prev_hdr = hdr)
        { /* Enumerate extension headers and process each one */
            if ((rc = asn_get_indexed(hdrs, &hdr, i, "")) == 0)
            {
                asn_tag_class t_cl;
                asn_tag_value t_val;

                IF_RC_RETURN(asn_get_choice_value(hdr, &hdr,
                                                  &t_cl, &t_val));

                /*
                 * Update "Next-Header" field of IPv6 header or Extension
                 * Headers in case it is not specified in layer PDU.
                 */
                if ((rc = asn_read_int32(prev_hdr, &val,
                                         "next-header")) != 0)
                {
                    IF_RC_RETURN(asn_write_int32(prev_hdr,
                                                 next_hdr_tag2bin(t_val),
                                                 "next-header.#plain"));

                    /*
                     * Convert and check only Extension headers.
                     * IPv6 Header will be validated and converted
                     * in the end.
                     */
                    if (ext_hdr_def != NULL)
                    {
                        IF_RC_RETURN(
                            tad_ip6_nds_to_data_and_confirm(
                                    ext_hdr_def, prev_hdr,
                                    &ptrn_data->ext_hdrs[ext_hdr_id].hdr));
                    }
                }

                switch (t_val)
                {
                    case NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP:
                    case NDN_TAG_IP6_EXT_HEADER_DESTINATION:
                        INFO("Header type %s",
                             t_val == NDN_TAG_IP6_EXT_HEADER_HOP_BY_HOP ?
                                 "Hop-by-Hop" : "Destination");

                        IF_RC_RETURN(asn_get_descendent(hdr, &opts,
                                                        "options"));

                        IF_RC_RETURN(opts_hdr_process_opts(
                                                    proto_data,
                                                    &ptrn_data->ext_hdrs[i],
                                                    opts));

                        if ((rc = asn_read_int32(hdr, &val, "length")) != 0)
                        {
                            if (ptrn_data->ext_hdrs[i].opts_len == 0 ||
                                (ptrn_data->ext_hdrs[i].opts_len
                                                            + 2) % 8 != 0)
                            {
                                ERROR("Total length of options is "
                                      "not correct %d",
                                      ptrn_data->ext_hdrs[i].opts_len);
                                return TE_RC(TE_TAD_CSAP, TE_ETADCSAPSTATE);
                            }

                            IF_RC_RETURN(
                                asn_write_int32(hdr,
                                    (ptrn_data->ext_hdrs[i].opts_len
                                                            + 2) / 8  - 1,
                                    "length.#plain"));
                        }

                        ptrn_data->ext_hdrs_len +=
                                    (2 + ptrn_data->ext_hdrs[i].opts_len);

                        ptrn_data->ext_hdrs[i].hdr_def =
                                    &proto_data->opts_hdr;

                        ext_hdr_def = &proto_data->opts_hdr;

                        ext_hdr_id = i;

                        break;

                    default:
                        ERROR("Not supported IPv6 Extension header");
                        return TE_RC(TE_TAD_CSAP, TE_ETADCSAPSTATE);
                }
            }
        } /* Enumerate extension headers and process each one */
    } /* Process extension headers */
ext_hdr_end:

    /*
     * Set the last "next-header" field (either the field of IPv6 header or
     * the field of the last extension header) to upper layer protocol
     */
    if ((rc = asn_read_int32(prev_hdr, &val, "next-header")) != 0)
    {
        IF_RC_RETURN(asn_write_int32(prev_hdr, proto_data->upper_protocol,
                                     "next-header.#plain"));
    }

    /* Convert the last Extension Header */
    if (ext_hdr_def != NULL)
    {
        IF_RC_RETURN(tad_ip6_nds_to_data_and_confirm(
                                    ext_hdr_def, prev_hdr,
                                    &ptrn_data->ext_hdrs[ext_hdr_id].hdr));
    }

    /* Check IPv6 Header */
    IF_RC_RETURN(tad_ip6_nds_to_data_and_confirm(&proto_data->hdr, layer_pdu,
                                         &ptrn_data->hdr));

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_ip6_match_pre_cb(csap_p              csap,
                     unsigned int        layer,
                     tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_ip6_proto_data     *proto_data;
    tad_ip6_proto_pdu_data *pkt_data;
    te_errno                rc = 0;

    unsigned int            i,j;
    unsigned int            n_pkts;
    unsigned int            n_segs;
    struct tad_pkt         *pkt;
    struct tad_pkt_seg     *seg;

    proto_data = csap_get_proto_spec_data(csap, layer);

    IF_NULL_RETURN(pkt_data = TE_ALLOC(sizeof(*pkt_data)));

    meta_pkt_layer->opaque = pkt_data;

    IF_RC_RETURN(tad_bps_pkt_frag_match_pre(&proto_data->hdr,
                                            &pkt_data->hdr));

    i = 0;
    pkt = meta_pkt_layer->pkts.pkts.cqh_first;
    n_pkts = meta_pkt_layer->pkts.n_pkts;
    while (i < n_pkts) {
        j = 0;
        seg = pkt->segs.cqh_first;
        n_segs = pkt->n_segs;
        while (j < n_segs) {
            seg = seg->links.cqe_next;
            j++;
        }
        pkt = pkt->links.cqe_next;
        i++;
    }

    return rc;
}

te_errno
tad_ip6_match_post_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_ip6_proto_data     *proto_data;
    tad_ip6_proto_pdu_data *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                *pkt;
    te_errno                rc;
    unsigned int            bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_ip6_header)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_ip6_header);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);

    IF_RC_RETURN(
        tad_bps_pkt_frag_match_post(&proto_data->hdr, &pkt_data->hdr,
                                    pkt, &bitoff, meta_pkt_layer->nds));

    return 0;
}

te_errno
tad_ip6_match_do_cb(csap_p           csap,
                    unsigned int     layer,
                    const asn_value *ptrn_pdu,
                    void            *ptrn_opaque,
                    tad_recv_pkt    *meta_pkt,
                    tad_pkt         *pdu,
                    tad_pkt         *sdu)
{
    tad_ip6_proto_data     *proto_data;
    tad_ip6_proto_pdu_data *ptrn_data = ptrn_opaque;
    tad_ip6_proto_pdu_data *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                rc = 0;
    unsigned int            bitoff = 0;

    UNUSED(ptrn_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    assert(proto_data != NULL);
    assert(ptrn_data != NULL);
    assert(pkt_data != NULL);

    if (tad_pkt_len(pdu) < IP6_HDR_LEN)
    {
        F_VERB(CSAP_LOG_FMT "PDU is too small to be IPv6 packet",
               CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
    }

    if ((rc =
            tad_bps_pkt_frag_match_do(&proto_data->hdr, &ptrn_data->hdr,
                                      &pkt_data->hdr, pdu, &bitoff)) != 0)
    {
        F_VERB(CSAP_LOG_FMT "Match PDU vs IPv4 header failed on bit "
               "offset %u: %r", CSAP_LOG_ARGS(csap), (unsigned)bitoff, rc);
        return rc;
    }

    /* TODO Process extension headers */

    if((rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare IPv6 SDU: %r",
        CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    EXIT(CSAP_LOG_FMT "OK", CSAP_LOG_ARGS(csap));

    return rc;
}
