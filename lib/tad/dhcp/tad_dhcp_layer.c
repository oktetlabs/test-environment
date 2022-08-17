/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD DHCP
 *
 * Traffic Application Domain Command Handler.
 * DHCP CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#define TE_LGR_USER     "TAD DHCP"

#include "te_config.h"


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

#include "te_defs.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "tad_bps.h"
#include "tad_dhcp_impl.h"

/**
 * DHCP layer specific data
 */
typedef struct tad_dhcp_proto_data {
    tad_bps_pkt_frag_def    hdr;
} tad_dhcp_proto_data;

/**
 * DHCP layer specific data for PDU processing (both send and
 * receive).
 */
typedef struct tad_dhcp_proto_pdu_data {
    tad_bps_pkt_frag_data   hdr;
} tad_dhcp_proto_pdu_data;

/**
 * Definition of Dynamic Host Configuration Protocol (DHCP) header.
 */
static const tad_bps_pkt_frag tad_dhcp_bps_hdr[] =
{
    { "op",        8, BPS_FLD_NO_DEF(NDN_DHCP_OP),
      TAD_DU_I32, FALSE },
    { "htype",     8, BPS_FLD_NO_DEF(NDN_DHCP_HTYPE),
      TAD_DU_I32, FALSE },
    { "hlen",      8, BPS_FLD_NO_DEF(NDN_DHCP_HLEN),
      TAD_DU_I32, TRUE },
    { "hops",      8, BPS_FLD_CONST_DEF(NDN_DHCP_HOPS, 0),
      TAD_DU_I32, FALSE },
    { "xid",      32, BPS_FLD_CONST_DEF(NDN_DHCP_XID, 0),
      TAD_DU_I32, FALSE },
    { "secs",     16, BPS_FLD_CONST_DEF(NDN_DHCP_SECS, 0),
      TAD_DU_I32, FALSE },
    { "flags" ,   16, BPS_FLD_NO_DEF(NDN_DHCP_FLAGS),
      TAD_DU_I32, FALSE },
    { "ciaddr",   32, BPS_FLD_CONST_DEF(NDN_DHCP_CIADDR, 0),
      TAD_DU_OCTS, FALSE },
    { "yiaddr",   32, BPS_FLD_CONST_DEF(NDN_DHCP_YIADDR, 0),
      TAD_DU_OCTS, FALSE },
    { "siaddr",   32, BPS_FLD_CONST_DEF(NDN_DHCP_SIADDR, 0),
      TAD_DU_OCTS, FALSE },
    { "giaddr",   32, BPS_FLD_CONST_DEF(NDN_DHCP_GIADDR, 0),
      TAD_DU_OCTS, FALSE },
    { "chaddr",  128, BPS_FLD_NO_DEF(NDN_DHCP_CHADDR),
      TAD_DU_OCTS, FALSE },
    { "sname",   512, BPS_FLD_CONST_DEF(NDN_DHCP_SNAME, 0),
      TAD_DU_OCTS, FALSE },
    { "file",   1024, BPS_FLD_CONST_DEF(NDN_DHCP_FILE, 0),
      TAD_DU_OCTS, FALSE },
};

/**
 * Definition of DHCPv6 message header
 */
static const tad_bps_pkt_frag tad_dhcp6_bps_hdr[] =
{
    { "msg-type", 8, BPS_FLD_NO_DEF(NDN_DHCP6_TYPE),
      TAD_DU_I32, FALSE },
    { "transaction-id", 24, BPS_FLD_CONST_DEF(NDN_DHCP6_TID, 0),
      TAD_DU_I32, FALSE },
};

#if 0
/**
 * Definition of DHCPv6 relay message header
 */
static const tad_bps_pkt_frag tad_dhcp6_bps_hdr[] =
{
    { "msg-type", 8, BPS_FLD_NO_DEF(NDN_DHCP6_TYPE),
      TAD_DU_I32, FALSE },
    { "hop-count", 8, BPS_FLD_CONST_DEF(NDN_DHCP6_HOPCNT, 0),
      TAD_DU_I32, FALSE },
    { "link-addr", 128, BPS_FLD_CONST_DEF(NDN_DHCP6_LADDR, 0),
      TAD_DU_OCTS, FALSE},
    { "peer-addr", 128, BPS_FLD_CONST_DEF(NDN_DHCP6_PADDR, 0),
      TAD_DU_OCTS, FALSE},
};
#endif

#define TAD_DHCP_OPTIONS

#ifdef TAD_DHCP_OPTIONS
/**
 * The first four octets of the 'options' field of the DHCP message
 * RFC 2131 section 3.
 */
static unsigned char magic_dhcp[] = { 99, 130, 83, 99 };

static te_errno dhcp_calculate_options_data(const asn_value *options,
                                            size_t *len);
static te_errno fill_dhcp_options(void *buf, const asn_value *options);
#endif

static te_errno fill_dhcp6_options(void *buf, const asn_value *options);

static te_errno dhcp6_calculate_options_data(const asn_value *options,
                                             size_t *len);

/* See description tad_ipstack_impl.h */
#define TAD_DHCP_INIT_CB(_dhcp_name) \
te_errno                                                            \
tad_##_dhcp_name##_init_cb(csap_p csap, unsigned int layer)         \
{                                                                   \
    te_errno                rc;                                     \
    tad_dhcp_proto_data    *proto_data;                             \
    const asn_value        *layer_nds;                              \
                                                                    \
    if ((proto_data = TE_ALLOC(sizeof(*proto_data))) == NULL)       \
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);                       \
                                                                    \
    csap_set_proto_spec_data(csap, layer, proto_data);              \
                                                                    \
    layer_nds = csap->layers[layer].nds;                            \
                                                                    \
    if ((rc = tad_bps_pkt_frag_init(tad_##_dhcp_name##_bps_hdr,     \
                        TE_ARRAY_LEN(tad_##_dhcp_name##_bps_hdr),   \
                        layer_nds, &proto_data->hdr)) != 0)         \
        return rc;                                                  \
                                                                    \
    return 0;                                                       \
}
TAD_DHCP_INIT_CB(dhcp)
TAD_DHCP_INIT_CB(dhcp6)
#undef TAD_DHCP_INIT_CB

/* See description tad_ipstack_impl.h */
te_errno
tad_dhcp_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_dhcp_proto_data *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->hdr);

    free(proto_data);

    return 0;
}

/**
 * Convert traffic template/pattern NDS to BPS internal data.
 *
 * @param csap          CSAP instance
 * @param proto_data    Protocol data prepared during CSAP creation
 * @param layer_pdu     Layer NDS
 * @param p_pdu_data    Location for PDU data pointer (updated in any
 *                      case and should be released by caller even in
 *                      the case of failure)
 *
 * @return Status code.
 */
static te_errno
tad_dhcp_nds_to_pdu_data(csap_p csap, tad_dhcp_proto_data *proto_data,
                         const asn_value *layer_pdu,
                         tad_dhcp_proto_pdu_data **p_pdu_data)
{
    te_errno                    rc;
    tad_dhcp_proto_pdu_data    *pdu_data;

    UNUSED(csap);

    assert(proto_data != NULL);
    assert(layer_pdu != NULL);
    assert(p_pdu_data != NULL);

    *p_pdu_data = pdu_data = TE_ALLOC(sizeof(*pdu_data));
    if (pdu_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->hdr, layer_pdu,
                                   &pdu_data->hdr);

    return rc;
}

/* See description in tad_ipstack_impl.h */
void
tad_dhcp_release_pdu_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_dhcp_proto_data     *proto_data;
    tad_dhcp_proto_pdu_data *pdu_data = opaque;

    proto_data = csap_get_proto_spec_data(csap, layer);
    assert(proto_data != NULL);

    if (pdu_data != NULL)
    {
        tad_bps_free_pkt_frag_data(&proto_data->hdr, &pdu_data->hdr);
        free(pdu_data);
    }
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                         asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *tmpl_data;
    int                         xid;
    size_t                      len = sizeof (xid);

    rc = asn_read_value_field(layer_pdu, &xid, &len, "xid.#plain");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        xid = random();
        if ((rc = asn_write_int32(layer_pdu, xid, "xid.#plain")) != 0)
            return rc;
    }

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_dhcp_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp6_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *tmpl_data;
    int                         type;
    int                         trid;
    size_t                      len = sizeof (type);
#if 0
    uint8_t                     hop_count;
#endif

    if ((rc = asn_read_value_field(layer_pdu, &type,
                                   &len, "msg-type.#plain")) != 0)
        return rc;

    assert(type >= DHCP6_MSG_SOLICIT && type <= DHCP6_MSG_RELAY_REPL);

#if 0
    if (type != DHCP6_MSG_RELAY_FORW && type != DHCP6_MSG_RELAY_REPL)
    { /* client/server message */
#endif
        rc = asn_read_value_field(layer_pdu, &trid,
                                  &len, "transaction-id.#plain");
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            trid = random();
            if ((rc = asn_write_value_field(layer_pdu, &trid, 24,
                                            "transaction-id.#plain")) != 0)
                return rc;
        }
#if 0
    }
    else
    { /* relay/server message */
        rc = asn_read_value_field(layer_pdu, &trid,
                                  &len, "hop-count.#plain");
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            hop_count = 255;
            if ((rc = asn_write_value_field(layer_pdu, &hop_count,
                                            sizeof(hop_count),
                                            "hop-count.#plain")) != 0)
                return rc;
        }
    }
#endif

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_dhcp_nds_to_pdu_data(csap, proto_data, layer_pdu, &tmpl_data);
    *p_opaque = tmpl_data;
    if (rc != 0)
        return rc;

    tmpl_data = *p_opaque;

    rc = tad_bps_confirm_send(&proto_data->hdr, &tmpl_data->hdr);

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp_gen_bin_cb(csap_p csap, unsigned int layer,
                    const asn_value *tmpl_pdu, void *opaque,
                    const tad_tmpl_arg_t *args, size_t arg_num,
                    tad_pkts *sdus, tad_pkts *pdus)
{
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *tmpl_data = opaque;
    te_errno                    rc;
    size_t                      bitlen;
    unsigned int                bitoff;
    uint8_t                    *msg;
#ifdef TAD_DHCP_OPTIONS
    const asn_value            *options;
#endif

    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Calculate length of the header */
    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr,
                                          &tmpl_data->hdr);
    assert((bitlen & 7) == 0);

#ifdef TAD_DHCP_OPTIONS
    if (asn_get_child_value(tmpl_pdu, &options,
                            PRIVATE, NDN_DHCP_OPTIONS) == 0)
    {
        size_t len;

        rc = dhcp_calculate_options_data(options, &len);
        if (rc != 0)
            return rc;

        bitlen += (sizeof(magic_dhcp) + len) << 3;
    }
#endif

    /* Allocate memory for binary template of the header */
    msg = malloc(bitlen >> 3);
    if (msg == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Generate binary template of the DHCP message */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, msg, &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        free(msg);
        return rc;
    }
    assert((bitoff & 7) == 0);

#ifdef TAD_DHCP_OPTIONS
    if (options != NULL)
    {
        uint8_t    *p = msg + (bitoff >> 3);

        memcpy(p, magic_dhcp, sizeof(magic_dhcp));
        p += sizeof(magic_dhcp);

        if ((rc = fill_dhcp_options(p, options)) != 0)
        {
            free(msg);
            return rc;
        }
    }
#endif

    /* Move SDUs to PDUs and add DHCP message header */
    tad_pkts_move(pdus, sdus);
    rc = tad_pkts_add_new_seg(pdus, TRUE,
                              msg, bitlen >> 3, tad_pkt_seg_data_free);
    if (rc != 0)
    {
        free(msg);
        return rc;
    }

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp6_gen_bin_cb(csap_p csap, unsigned int layer,
                     const asn_value *tmpl_pdu, void *opaque,
                     const tad_tmpl_arg_t *args, size_t arg_num,
                     tad_pkts *sdus, tad_pkts *pdus)
{
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *tmpl_data = opaque;
    te_errno                    rc;
    size_t                      bitlen;
    unsigned int                bitoff;
    uint8_t                    *msg;
    const asn_value            *options;
    size_t                      len;

    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Calculate length of the header */
    bitlen = tad_bps_pkt_frag_data_bitlen(&proto_data->hdr,
                                          &tmpl_data->hdr);
    assert((bitlen & 7) == 0);

    if (asn_get_child_value(tmpl_pdu, &options,
                            PRIVATE, NDN_DHCP6_OPTIONS) == 0)
    {
        if ((rc = dhcp6_calculate_options_data(options, &len)) != 0)
            return rc;

        bitlen += len << 3;
    }

    /* Allocate memory for binary template of the header */
    msg = malloc(bitlen >> 3);
    if (msg == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    /* Generate binary template of the DHCP message */
    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->hdr, &tmpl_data->hdr,
                                  args, arg_num, msg, &bitoff, bitlen);
    if (rc != 0)
    {
        ERROR("%s(): tad_bps_pkt_frag_gen_bin failed for header: %r",
              __FUNCTION__, rc);
        free(msg);
        return rc;
    }
    assert((bitoff & 7) == 0);

    if (options != NULL)
    {
        uint8_t    *p = msg + (bitoff >> 3);

        if ((rc = fill_dhcp6_options(p, options)) != 0)
        {
            free(msg);
            return rc;
        }
    }

    /* Move SDUs to PDUs and add DHCP message header */
    tad_pkts_move(pdus, sdus);
    rc = tad_pkts_add_new_seg(pdus, TRUE,
                              msg, bitlen >> 3, tad_pkt_seg_data_free);
    if (rc != 0)
    {
        free(msg);
        return rc;
    }

    return 0;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                         asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *ptrn_data;

    F_ENTRY("(%d:%u) layer_pdu=%p", csap->id, layer,
            (void *)layer_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    rc = tad_dhcp_nds_to_pdu_data(csap, proto_data, layer_pdu, &ptrn_data);
    *p_opaque = ptrn_data;

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp_match_pre_cb(csap_p              csap,
                      unsigned int        layer,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *pkt_data;
    te_errno                    rc;

    proto_data = csap_get_proto_spec_data(csap, layer);

    pkt_data = malloc(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    meta_pkt_layer->opaque = pkt_data;

    rc = tad_bps_pkt_frag_match_pre(&proto_data->hdr, &pkt_data->hdr);

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp_match_post_cb(csap_p              csap,
                       unsigned int        layer,
                       tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                    *pkt;
    te_errno                    rc;
    unsigned int                bitoff = 0;

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_dhcpv4_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_dhcpv4_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);

    rc = tad_bps_pkt_frag_match_post(&proto_data->hdr, &pkt_data->hdr,
                                     pkt, &bitoff, meta_pkt_layer->nds);
    if (rc != 0)
        return rc;

#ifdef TAD_DHCP_OPTIONS
    uint8_t    *data_ptr;
    uint8_t    *data;
    size_t      data_len;
    asn_value  *opt_list;

    assert(tad_pkt_seg_num(pkt) == 1);
    assert(tad_pkt_first_seg(pkt) != NULL);
    data_ptr = data = tad_pkt_first_seg(pkt)->data_ptr;
    data_len = tad_pkt_first_seg(pkt)->data_len;

    data += 236;

    /* check for magic DHCP cookie, see RFC2131, section 3. */
    if ((data + sizeof(magic_dhcp)) > (data_ptr + data_len) ||
        memcmp(magic_dhcp, data, sizeof(magic_dhcp)) != 0)
    {
        VERB("DHCP magic does not match: "
             "it is pure BOOTP message without options");
    }
    else
    {
        data += sizeof(magic_dhcp);

        opt_list = asn_init_value(ndn_dhcpv4_options);

        while (data < (data_ptr + data_len))
        {
            asn_value  *opt = asn_init_value(ndn_dhcpv4_option);
            uint8_t     opt_len;
            uint8_t     opt_type;

#define FILL_DHCP_OPT_FIELD(_obj, _label, _size) \
            do {                                            \
                asn_write_value_field(_obj, data, _size,    \
                                      _label ".#plain");    \
                data += _size;                              \
            } while(0);

            opt_type = *data;
            FILL_DHCP_OPT_FIELD(opt, "type",  1);

            /* Do not add padding zeros to ASN value*/
            if (opt_type == 0)
            {
                asn_free_value(opt);
                continue;
            }

            if (opt_type == 255)
            {
                /* END and PAD options don't have length and value */
                asn_insert_indexed(opt_list, opt, -1, "");
                continue;
            }

            opt_len = *data;
            FILL_DHCP_OPT_FIELD(opt, "length", 1);
            FILL_DHCP_OPT_FIELD(opt, "value", opt_len);

            /* possible suboptions.  */
            switch (opt_type)
            {
                case 43:
                {
                    asn_value   *sub_opt_list;
                    uint8_t      sub_opt_len;
                    asn_value   *sub_opt;
                    void        *start_opt_value;

                    /* Set pointer to the beginning of the Option data */
                    data -= opt_len;
                    start_opt_value = data;
                    sub_opt_list = asn_init_value(ndn_dhcpv4_options);
                    while (((void *)data) < (start_opt_value + opt_len))
                    {
                        sub_opt = asn_init_value(ndn_dhcpv4_option);

                        FILL_DHCP_OPT_FIELD(sub_opt, "type",  1);
                        sub_opt_len = *data;
                        FILL_DHCP_OPT_FIELD(sub_opt, "length", 1);
                        FILL_DHCP_OPT_FIELD(sub_opt, "value", sub_opt_len);

                        asn_insert_indexed(sub_opt_list, sub_opt, -1, "");
                    }
                    asn_put_child_value(opt, sub_opt_list,
                                        PRIVATE, NDN_DHCP_OPTIONS);
                    break;
                }
            }
            asn_insert_indexed(opt_list, opt, -1, "");
        }

        asn_put_child_value(meta_pkt_layer->nds, opt_list,
                            PRIVATE, NDN_DHCP_OPTIONS);
    }

    VERB("MATCH CALLBACK OK\n");
#undef FILL_DHCP_OPT_FIELD
#endif /* TAD_DHCP_OPTIONS */

    return rc;
}

static void
process_dhcp6_options(asn_value *opt_list, uint8_t **data_p,
                      uint8_t *data_ptr, size_t data_len)
{
    asn_value                  *opt;
    asn_value                  *sub_opt_list;
    asn_value                  *sub_opt;
    asn_value                  *option_body;
    asn_value                  *class_data_list;
    uint16_t                    type16;
    uint16_t                    opt_type;
    uint16_t                    opt_len;
    size_t                      opt_data_len;
    uint8_t                     msg_type;
    uint16_t                    u16_val;
    uint32_t                    u32_val;

/*
 * Need to translate data notation order from network to host
 */
#define FILL_DHCP_OPT_FIELD_INT(_obj, _label, _size) \
    do {                                                    \
        if (_size == sizeof(uint32_t))                      \
        {                                                   \
            u32_val = ntohl(*((uint32_t *)(*data_p)));      \
            asn_write_value_field(_obj, &u32_val, _size,    \
                                  _label ".#plain");        \
        }                                                   \
        else if (_size == sizeof(uint16_t))                 \
        {                                                   \
            u16_val = ntohs(*((uint16_t *)(*data_p)));      \
            asn_write_value_field(_obj, &u16_val, _size,    \
                                  _label ".#plain");        \
        }                                                   \
        else                                                \
            /* uint8_t or else */                           \
            asn_write_value_field(_obj, *data_p, _size,     \
                              _label ".#plain");            \
                                                            \
        *data_p += _size;                                   \
        opt_data_len -= _size;                              \
    } while(0);

#define FILL_DHCP_OPT_FIELD(_obj, _label, _size) \
    do {                                            \
        asn_write_value_field(_obj, *data_p, _size, \
                              _label ".#plain");    \
        *data_p += _size;                           \
        opt_data_len -= _size;                      \
    } while(0);

    while (*data_p < (data_ptr + data_len))
    {
        opt = asn_init_value(ndn_dhcpv6_option);

        opt_type = ntohs(*((uint16_t *)(*data_p)));
        opt_len = ntohs(*((uint16_t *)(*data_p + sizeof(uint16_t))));
        /* Trick to process lists of suboptions correctly */
        opt_data_len = (size_t)opt_len +
                            sizeof(opt_type) +
                                sizeof(opt_len);

        FILL_DHCP_OPT_FIELD_INT(opt, "type",  2);
        FILL_DHCP_OPT_FIELD_INT(opt, "length", 2);

        if (opt_type == DHCP6_OPT_CLIENTID ||
            opt_type == DHCP6_OPT_SERVERID)
        {
            type16 = ntohs(*((uint16_t *)(*data_p)));

            if (type16 != DUID_LL && type16 != DUID_LLT && type16 != DUID_EN)
            {
                ERROR("Wrong DUID type field in option");
                FILL_DHCP_OPT_FIELD(opt, "value", opt_len);
            }
            else
            {
                option_body = asn_init_value(ndn_dhcpv6_duid);
                FILL_DHCP_OPT_FIELD_INT(option_body, "type", 2);
                if (type16 == DUID_EN)
                {
                    FILL_DHCP_OPT_FIELD_INT(option_body,
                                            "enterprise-number", 4);
                    FILL_DHCP_OPT_FIELD(option_body, "identifier",
                                        opt_len - 6);
                }
                else if (type16 == DUID_LLT)
                {
                    FILL_DHCP_OPT_FIELD_INT(option_body,
                                            "hardware-type", 2);
                    FILL_DHCP_OPT_FIELD_INT(option_body, "time", 4);
                    FILL_DHCP_OPT_FIELD(option_body, "link-layer-address",
                                        opt_len - 8);
                }
                else /* DUID_LL */
                {
                    FILL_DHCP_OPT_FIELD_INT(option_body,
                                            "hardware-type", 2);
                    FILL_DHCP_OPT_FIELD(option_body, "link-layer-address",
                                        opt_len - 4);
                }

                asn_put_child_value(opt, option_body,
                                    PRIVATE, NDN_DHCP6_DUID);
            }
        }
        else if (opt_type == DHCP6_OPT_RELAY_MSG)
        { /* DHCP6 relayed message */
            msg_type = (*data_p)[0];

            option_body = asn_init_value(ndn_dhcpv6_message);
            FILL_DHCP_OPT_FIELD(option_body, "msg-type", 1);
            FILL_DHCP_OPT_FIELD(option_body, "transaction-id", 3);

            if (msg_type == DHCP6_MSG_RELAY_FORW ||
                msg_type == DHCP6_MSG_RELAY_REPL)
            { /* relay/server forward/reply message */
                FILL_DHCP_OPT_FIELD(option_body, "link-address", 16);
                FILL_DHCP_OPT_FIELD(option_body, "peer-address", 16);
            }

            sub_opt_list = asn_init_value(ndn_dhcpv6_options);

            process_dhcp6_options(sub_opt_list, data_p,
                                  *data_p, opt_data_len);

            asn_put_child_value(option_body, sub_opt_list,
                                PRIVATE, NDN_DHCP6_OPTIONS);

            asn_put_child_value(opt, option_body,
                                PRIVATE, NDN_DHCP6_RELAY_MESSAGE);
        }
        else if (opt_type == DHCP6_OPT_IA_NA    ||
                 opt_type == DHCP6_OPT_IA_TA    ||
                 opt_type == DHCP6_OPT_IAADDR   ||
                 opt_type == DHCP6_OPT_IA_PD    ||
                 opt_type == DHCP6_OPT_IA_PREFIX)
        { /* IA_NA, IA_TA, IA_ADDR, IA_PD and IA_PREFIX options */
            /* I like Mayakovski poems */
            option_body =
                asn_init_value(
                    (opt_type == DHCP6_OPT_IA_NA) ?
                        ndn_dhcpv6_ia_na :
                            (opt_type == DHCP6_OPT_IA_TA) ?
                                ndn_dhcpv6_ia_ta :
                                    (opt_type == DHCP6_OPT_IAADDR) ?
                                        ndn_dhcpv6_ia_addr :
                                            (opt_type == DHCP6_OPT_IA_PD) ?
                                                ndn_dhcpv6_ia_pd :
                                                    ndn_dhcpv6_ia_prefix);

            if (opt_type == DHCP6_OPT_IA_NA ||
                opt_type == DHCP6_OPT_IA_TA ||
                opt_type == DHCP6_OPT_IA_PD)
            {
                FILL_DHCP_OPT_FIELD_INT(option_body, "iaid", 4);
            }

            if (opt_type == DHCP6_OPT_IA_NA ||
                opt_type == DHCP6_OPT_IA_PD)
            {
                FILL_DHCP_OPT_FIELD_INT(option_body, "t1", 4);
                FILL_DHCP_OPT_FIELD_INT(option_body, "t2", 4);
            }

            if (opt_type == DHCP6_OPT_IAADDR)
            {
                FILL_DHCP_OPT_FIELD(option_body, "ipv6-address", 16);
                FILL_DHCP_OPT_FIELD_INT(option_body,
                                        "preferred-lifetime", 4);
                FILL_DHCP_OPT_FIELD_INT(option_body, "valid-lifetime", 4);
            }

            if (opt_type == DHCP6_OPT_IA_PREFIX)
            {
                FILL_DHCP_OPT_FIELD_INT(option_body,
                                        "preferred-lifetime", 4);
                FILL_DHCP_OPT_FIELD_INT(option_body, "valid-lifetime", 4);
                FILL_DHCP_OPT_FIELD(option_body, "prefix-length", 1);
                FILL_DHCP_OPT_FIELD(option_body, "prefix-address", 16);
            }

            sub_opt_list = asn_init_value(ndn_dhcpv6_options);

            process_dhcp6_options(sub_opt_list, data_p,
                                  *data_p, opt_data_len);

            asn_put_child_value(option_body, sub_opt_list,
                                PRIVATE, NDN_DHCP6_OPTIONS);

            /* I like Mayakovski poems */
            asn_put_child_value(
                opt, option_body, PRIVATE,
                    (opt_type == DHCP6_OPT_IA_NA) ?
                        NDN_DHCP6_IA_NA :
                            (opt_type == DHCP6_OPT_IA_TA) ?
                                NDN_DHCP6_IA_TA :
                                    (opt_type == DHCP6_OPT_IAADDR) ?
                                        NDN_DHCP6_IA_ADDR :
                                            (opt_type == DHCP6_OPT_IA_PD) ?
                                                NDN_DHCP6_IA_PD :
                                                    NDN_DHCP6_IA_PREFIX);
        }
        else if (opt_type == DHCP6_OPT_ORO)
        {
            if ((opt_len % 2) != 0)
            {
                ERROR("Opion OPTION_ORO has wrong option-len field");
                FILL_DHCP_OPT_FIELD(opt, "value", opt_len);
            }
            else
            {
                option_body = asn_init_value(ndn_dhcpv6_oro);

                do {
                    sub_opt = asn_init_value(ndn_dhcpv6_opcode);
                    FILL_DHCP_OPT_FIELD_INT(sub_opt, "opcode", 2);
                    asn_insert_indexed(option_body, sub_opt, -1, ""),
                    opt_len -= 2;
                } while (opt_len > 0);

                asn_put_child_value(opt, option_body,
                                    PRIVATE, NDN_DHCP6_ORO);
            }
        }
        else if (opt_type == DHCP6_OPT_ELAPSED_TIME)
        {
            FILL_DHCP_OPT_FIELD_INT(opt, "elapsed-time", 2);
        }
        else if (opt_type == DHCP6_OPT_AUTH)
        {
            option_body = asn_init_value(ndn_dhcpv6_auth);

            FILL_DHCP_OPT_FIELD(option_body, "protocol", 1);
            FILL_DHCP_OPT_FIELD(option_body, "algorithm", 1);
            FILL_DHCP_OPT_FIELD(option_body, "rdm", 1);
            FILL_DHCP_OPT_FIELD(option_body, "relay-detection", 8);
            FILL_DHCP_OPT_FIELD(option_body, "auth-info", opt_len - 11);

            asn_put_child_value(opt, option_body, PRIVATE, NDN_DHCP6_AUTH);
        }
        else if (opt_type == DHCP6_OPT_UNICAST)
        {
            FILL_DHCP_OPT_FIELD(opt, "servaddr", opt_len);
        }
        else if (opt_type == DHCP6_OPT_STATUS_CODE)
        {
            option_body = asn_init_value(ndn_dhcpv6_status);

            FILL_DHCP_OPT_FIELD_INT(option_body, "status-code", 2);
            FILL_DHCP_OPT_FIELD(option_body, "status-message", opt_len - 2);

            asn_put_child_value(opt, option_body, PRIVATE, NDN_DHCP6_STATUS);
        }
        else if (opt_type == DHCP6_OPT_USER_CLASS)
        {
            option_body = asn_init_value(ndn_dhcpv6_class_data_list);

            do {
                sub_opt = asn_init_value(ndn_dhcpv6_class_data);
                type16 = ntohs(*((uint16_t *)(*data_p)));
                /* type16 is used to save class-data-len value !*/

                FILL_DHCP_OPT_FIELD_INT(sub_opt, "class-data-len", 2);
                opt_len -= 2;

                FILL_DHCP_OPT_FIELD(sub_opt, "class-data-opaque", type16);
                opt_len -= type16;

                asn_insert_indexed(option_body, sub_opt, -1, "");
            } while (opt_len > 0);

            asn_put_child_value(opt, option_body, PRIVATE,
                                NDN_DHCP6_USER_CLASS);
        }
        else if (opt_type == DHCP6_OPT_VENDOR_CLASS)
        {
            option_body = asn_init_value(ndn_dhcpv6_vendor_class);

            FILL_DHCP_OPT_FIELD_INT(option_body, "enterprise-number", 4);
            opt_len -= 4;

            class_data_list = asn_init_value(ndn_dhcpv6_class_data_list);

            do {
                sub_opt = asn_init_value(ndn_dhcpv6_class_data);
                type16 = ntohs(*((uint16_t *)(*data_p)));
                /* type16 is used to save class-data-len value !*/

                FILL_DHCP_OPT_FIELD_INT(sub_opt, "class-data-len", 2);
                opt_len -= 2;

                FILL_DHCP_OPT_FIELD(sub_opt, "class-data-opaque", type16);
                opt_len -= type16;

                asn_insert_indexed(class_data_list, sub_opt, -1, "");
            } while (opt_len > 0);

            asn_put_child_value(option_body, class_data_list, PRIVATE,
                                NDN_DHCP6_VENDOR_CLASS_DATA);

            asn_put_child_value(opt, option_body, PRIVATE,
                                NDN_DHCP6_VENDOR_CLASS);
        }
        else if (opt_type == DHCP6_OPT_VENDOR_OPTS)
        {
            option_body = asn_init_value(ndn_dhcpv6_vendor_specific);

            FILL_DHCP_OPT_FIELD_INT(option_body, "enterprise-number", 4);
            opt_len -= 4;

            /* Do not process vendor-specific options here !*/
            FILL_DHCP_OPT_FIELD(option_body, "option-data", opt_len);

            asn_put_child_value(opt, option_body, PRIVATE,
                                NDN_DHCP6_VENDOR_SPECIFIC);
        }
        else if (opt_type == DHCP6_OPT_RAPID_COMMIT ||
                 opt_type == DHCP6_OPT_RECONF_ACCEPT)
        {
            RING("DHCPv6 option %s has no value field",
                 (opt_type == DHCP6_OPT_RAPID_COMMIT) ?
                    "RAPID_COMMIT" : "RECONF_ACCEPT");
        }
        else
        {
            ERROR("Can not recognize DHCPv6 option type in "
                  "opt-type field value");
            FILL_DHCP_OPT_FIELD(opt, "value", opt_len);
        }

        asn_insert_indexed(opt_list, opt, -1, "");
    }
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp6_match_post_cb(csap_p              csap,
                        unsigned int        layer,
                        tad_recv_pkt_layer *meta_pkt_layer)
{
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *pkt_data = meta_pkt_layer->opaque;
    tad_pkt                    *pkt;
    te_errno                    rc;
    unsigned int                bitoff = 0;
    uint8_t                    *data;
    uint8_t                    *data_ptr;
    size_t                      data_len;
    asn_value                  *opt_list;
#if 0
    uint8_t                     msg_type;
#endif

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if ((meta_pkt_layer->nds = asn_init_value(ndn_dhcpv6_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_dhcpv6_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);

    if ((rc = tad_bps_pkt_frag_match_post(&proto_data->hdr, &pkt_data->hdr,
                                          pkt, &bitoff,
                                          meta_pkt_layer->nds)) != 0)
        return rc;

    assert(tad_pkt_seg_num(pkt) == 1);
    assert(tad_pkt_first_seg(pkt) != NULL);
    data_ptr = data = tad_pkt_first_seg(pkt)->data_ptr;
    data_len = tad_pkt_first_seg(pkt)->data_len;

#if 0
    msg_type = data[0];

    if (msg_type != DHCP6_MSG_RELAY_FORW &&
        msg_type != DHCP6_MSG_RELAY_REPL)
    { /* client/server message */
#endif
        data += 4; /* msg-type (1 octet) + transaction-id (3 octets) */
#if 0
    }
    else
    { /* relay/server forward/reply message */
        data += 34; /* msg-type (1 octet) + hop-count (1 octet) +
                       link-address (16 octets) + peer-address (16 octets) */
    }
#endif

    opt_list = asn_init_value(ndn_dhcpv6_options);

    process_dhcp6_options(opt_list, &data, data_ptr, data_len);

    asn_put_child_value(meta_pkt_layer->nds, opt_list,
                        PRIVATE, NDN_DHCP6_OPTIONS);

    VERB("MATCH CALLBACK OK\n");

    return rc;
}

/* See description in tad_ipstack_impl.h */
te_errno
tad_dhcp_match_do_cb(csap_p           csap,
                     unsigned int     layer,
                     const asn_value *ptrn_pdu,
                     void            *ptrn_opaque,
                     tad_recv_pkt    *meta_pkt,
                     tad_pkt         *pdu,
                     tad_pkt         *sdu)
{
    tad_dhcp_proto_data        *proto_data;
    tad_dhcp_proto_pdu_data    *ptrn_data = ptrn_opaque;
    tad_dhcp_proto_pdu_data    *pkt_data = meta_pkt->layers[layer].opaque;
    te_errno                    rc;
    unsigned int                bitoff = 0;

    UNUSED(ptrn_pdu);

    if (tad_pkt_len(pdu) < 20)
    {
        F_VERB(CSAP_LOG_FMT "PDU is too small to be DHCP packet",
               CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);
    }

    proto_data = csap_get_proto_spec_data(csap, layer);

    assert(proto_data != NULL);
    assert(ptrn_data != NULL);
    assert(pkt_data != NULL);

    rc = tad_bps_pkt_frag_match_do(&proto_data->hdr, &ptrn_data->hdr,
                                   &pkt_data->hdr, pdu, &bitoff);
    if (rc != 0)
    {
        F_VERB(CSAP_LOG_FMT "Match PDU vs DHCP header failed on bit "
               "offset %u: %r", CSAP_LOG_ARGS(csap), (unsigned)bitoff, rc);
        return rc;
    }

    /* TODO: DHCP options processing */

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare DHCP SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    EXIT(CSAP_LOG_FMT "OK", CSAP_LOG_ARGS(csap));

    return 0;
}

/* See description in tad_dhcp_impl.h */
te_errno
tad_dhcp_gen_pattern_cb(csap_p csap, unsigned int layer,
                        const asn_value *tmpl_pdu,
                        asn_value **ptrn_pdu)
{
    te_errno    rc;
    int         xid;
    size_t      len = sizeof(xid);

    UNUSED(csap);
    UNUSED(layer);

    if ((*ptrn_pdu = asn_init_value(ndn_dhcpv4_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_dhcpv4_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    rc = asn_read_value_field(tmpl_pdu, &xid, &len, "xid.#plain");
    if (rc == 0)
    {
        rc = asn_write_int32(*ptrn_pdu, xid, "xid.#plain");
    }
    /* TODO: DHCP options to be inserted into pattern */

    return rc;
}

/* See description in tad_dhcp_impl.h */
te_errno
tad_dhcp6_gen_pattern_cb(csap_p csap, unsigned int layer,
                        const asn_value *tmpl_pdu,
                        asn_value **ptrn_pdu)
{
    te_errno    rc;
    int         tid;
    size_t      len = sizeof(tid);

    UNUSED(csap);
    UNUSED(layer);

    if ((*ptrn_pdu = asn_init_value(ndn_dhcpv6_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_dhcpv6_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    if ((rc = asn_read_value_field(tmpl_pdu, &tid, &len,
                                   "transaction-id.#plain")) == 0)
    {
        rc = asn_write_value_field(*ptrn_pdu, &tid, 24,
                                   "transaction-id.#plain");
    }

    return rc;
}

/* See description in tad_dhcp_impl.h */
char *
tad_dhcp_get_param_cb(csap_p csap, unsigned int layer, const char *param)
{
    dhcp_csap_specific_data_t *spec_data;

    UNUSED(layer);

    spec_data = csap_get_rw_data(csap);

    if (strcmp (param, "ipaddr") == 0)
    {
        return spec_data->ipaddr;
    }

    return NULL;
}

#ifdef TAD_DHCP_OPTIONS
/**
 * Calculate amount of data necessary for all options in DHCP message.
 *
 * @param options       asn_value with sequence of DHCPv4-Option
 * @param len           calculated number of octets is written here
 *
 * @return 0 if success or error code if failed.
 */
static te_errno
dhcp_calculate_options_data(const asn_value *options, size_t *len)
{
    size_t              data_len = 0;
    int                 n_opts = asn_get_length(options, "");
    int                 i;
    asn_value          *opt;
    uint8_t             opt_type;
    size_t              l;
    const asn_value    *sub_opts;
    te_errno            rc;

    for (i = 0; i < n_opts; i++)
    {
        rc = asn_get_indexed(options, &opt, i, "");
        if (rc != 0)
            return rc;

        l = sizeof(opt_type);
        rc = asn_read_value_field(opt, &opt_type, &l, "type.#plain");
        if (rc != 0)
            return rc;

        /* Options 255 and 0 don't have 'length' and 'value' parts */
        if (opt_type != 255 && opt_type != 0)
        {
            data_len += 2;  /* octets for 'type' and 'length' */

            if (asn_get_child_value(opt, &sub_opts,
                                    PRIVATE, NDN_DHCP_OPTIONS) == 0)
            {
                rc = dhcp_calculate_options_data(sub_opts, &l);
                if (rc != 0)
                    return rc;

                data_len += l;
            }
            else
            {
                data_len += asn_get_length(opt, "value");
            }
        }
        else
        {
            data_len += 1; /* octets for 'type' only (255 and 0 options) */
        }
    }

    *len = data_len;

    return 0;
}

static te_errno
fill_dhcp_options(void *buf, const asn_value *options)
{
    asn_value  *opt;
    int         i;
    size_t      len;
    int         n_opts;
    te_errno    rc = 0;
    uint8_t     tmp;

    if (options == NULL)
        return 0;

    n_opts = asn_get_length(options, "");
    for (i = 0; i < n_opts; i++)
    {
        rc = asn_get_indexed(options, &opt, i, "");
        if (rc != 0)
            break;

        len = sizeof(tmp);
        if ((rc = asn_read_value_field(opt, &tmp, &len,
                                       "type.#plain")) != 0)
            break;

        memcpy(buf, &tmp, len);
        buf += len;
        /* Options 255 and 0 don't have length and value parts */
        if (tmp == 255 || tmp == 0)
            continue;

        len = sizeof(tmp);
        if ((rc = asn_read_value_field(opt, buf, &len, "length.#plain")) != 0)
            break;

        buf += len;
        if (asn_get_length(opt, "options") > 0)
        {
            const asn_value *sub_opts;

            if ((rc = asn_get_child_value(opt, &sub_opts,
                                          PRIVATE, NDN_DHCP_OPTIONS)) != 0)
                break;
            rc = fill_dhcp_options(buf, sub_opts);
            if (rc != 0)
                break;
        }
        else
        {
            len = asn_get_length(opt, "value.#plain");
            if ((rc = asn_read_value_field(opt, buf, &len,
                                           "value.#plain")) != 0)
                break;
            buf += len;
        }
    }
    return rc;
}
#endif /* TAD_DHCP_OPTIONS */

/**
 * Calculate amount of data necessary for all options in DHCP6 message.
 *
 * @param options       asn_value with sequence of DHCPv4-Option
 * @param len           calculated number of octets is written here
 *
 * @return 0 if success or error code if failed.
 */
static te_errno
dhcp6_calculate_options_data(const asn_value *options, size_t *len)
{
    size_t              data_len = 0;
    int                 n_opts = asn_get_length(options, "");
    int                 i;
    asn_value          *opt;
    uint16_t            opt_len;
    size_t              l;
    te_errno            rc;

    for (i = 0; i < n_opts; i++)
    {
        if ((rc = asn_get_indexed(options, &opt, i, "")) != 0)
            return rc;

        l = sizeof(opt_len);
        if ((rc = asn_read_value_field(opt, &opt_len, &l, "length.#plain"))!= 0)
            return rc;

        data_len += 4;
        data_len += opt_len;
    }

    *len = data_len;

    return 0;
}

static te_errno
fill_dhcp6_options(void *buf, const asn_value *options)
{
    asn_value  *opt;
    const asn_value  *opt_body;
    const asn_value  *sub_opt;
    const asn_value  *sub_sub_opt;
    int         i, j;
    size_t      len;
    int         n_opts;
    int         n_oro;
    te_errno    rc = 0;
    uint8_t     uint8_val;
    uint16_t    uint16_val;
    uint32_t    uint32_val;
    uint8_t     uint8_val_net;
    uint16_t    uint16_val_net;
    uint32_t    uint32_val_net;
    uint16_t    opt_type;
    uint16_t    duid_type;

    if (options == NULL)
        return 0;

    n_opts = asn_get_length(options, "");
    for (i = 0; i < n_opts; i++)
    {
        if ((rc = asn_get_indexed(options, &opt, i, "")) != 0)
            break;
#define READ_FIELD_VALUE(_asn_value, _type, _name) \
    do {                                                                    \
        len = sizeof(_type##_t);                                            \
        if ((rc = asn_read_value_field(_asn_value, &_type##_val, &len,      \
                                       _name ".#plain")) != 0)              \
            break;                                                          \
                                                                            \
        /* We need to write values in the network byte order */             \
        if (strcmp(#_type, "uint8") == 0)                                   \
            _type##_val_net = _type##_val; /* the same as host byte order */\
        else if (strcmp(#_type, "uint16") == 0)                             \
            _type##_val_net = htons(_type##_val);                           \
        else /* uint32 */                                                   \
            _type##_val_net = htonl(_type##_val);                           \
                                                                            \
        memcpy(buf, &_type##_val_net, len);                                 \
        buf += len;                                                         \
    } while (0)

#define READ_FIELD_OCTETS(_asn_value, _name) \
    do {                                                                    \
        len = asn_get_length(_asn_value, _name ".#plain");                  \
                                                                            \
        if ((rc = asn_read_value_field(_asn_value, buf, &len,               \
                                       _name ".#plain")) != 0)              \
                break;                                                      \
                                                                            \
        buf += len;                                                         \
    } while (0)

        READ_FIELD_VALUE(opt, uint16, "type");

        opt_type = uint16_val;

        READ_FIELD_VALUE(opt, uint16, "length");

        if (opt_type == DHCP6_OPT_RAPID_COMMIT ||
            opt_type == DHCP6_OPT_RECONF_ACCEPT)
        {   /* nothing else */
            continue;
        }
        else if (opt_type == DHCP6_OPT_CLIENTID ||
                 opt_type == DHCP6_OPT_SERVERID)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_DUID)) != 0)
                break;

            READ_FIELD_VALUE(opt_body, uint16, "type");

            duid_type = uint16_val;

            if (duid_type != DUID_LLT &&
                duid_type != DUID_LL &&
                duid_type != DUID_EN)
            {
                ERROR("Incorrect DUID type value ");
                rc = -1;
                break;
            }

            if (duid_type == DUID_LLT)
            {
                READ_FIELD_VALUE(opt_body, uint16, "hardware-type");
                READ_FIELD_VALUE(opt_body, uint32, "time");
                READ_FIELD_OCTETS(opt_body, "link-layer-address");
            }
            else if (duid_type == DUID_LL)
            {
                READ_FIELD_VALUE(opt_body, uint16, "hardware-type");
                READ_FIELD_OCTETS(opt_body, "link-layer-address");
            }
            else
            {   /* DUID_EN */
                READ_FIELD_VALUE(opt_body, uint32, "enterprise-number");
                READ_FIELD_OCTETS(opt_body, "identifier");
            }
        }
        else if (opt_type == DHCP6_OPT_IA_NA)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_IA_NA)) != 0)
                break;
            READ_FIELD_VALUE(opt_body, uint32, "iaid");
            READ_FIELD_VALUE(opt_body, uint32, "t1");
            READ_FIELD_VALUE(opt_body, uint32, "t2");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_OPTIONS)) == 0)
            {
                if ((rc = fill_dhcp6_options(buf, sub_opt)) != 0)
                    break;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                rc = 0;
            else
                break;
        }
        else if (opt_type == DHCP6_OPT_IA_TA)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_IA_TA)) != 0)
                break;
            READ_FIELD_VALUE(opt_body, uint32, "iaid");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_OPTIONS)) == 0)
            {
                if ((rc = fill_dhcp6_options(buf, sub_opt)) != 0)
                    break;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                rc = 0;
            else
                break;
        }
        else if (opt_type == DHCP6_OPT_IAADDR)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_IA_ADDR)) != 0)
                break;
            READ_FIELD_OCTETS(opt_body, "ipv6-address");
            READ_FIELD_VALUE(opt_body, uint32, "preferred-lifetime");
            READ_FIELD_VALUE(opt_body, uint32, "valid-lifetime");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_OPTIONS)) == 0)
            {
                if ((rc = fill_dhcp6_options(buf, sub_opt)) != 0)
                    break;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                rc = 0;
            else
                break;
        }
        else if (opt_type == DHCP6_OPT_ORO)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_ORO)) != 0)
                break;

            n_oro = asn_get_length(opt_body, "");
            for (j = 0; j < n_oro; j++)
            {
                if ((rc = asn_get_indexed(opt_body, (asn_value **)&sub_opt,
                                          j, "")) != 0)
                    break;

                READ_FIELD_VALUE(sub_opt, uint16, "opcode");
            }
        }
        else if (opt_type == DHCP6_OPT_ELAPSED_TIME)
        {
            READ_FIELD_VALUE(opt, uint16, "elapsed-time");
        }
        else if (opt_type == DHCP6_OPT_RELAY_MSG)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_RELAY_MESSAGE)) != 0)
                break;

            READ_FIELD_VALUE(opt_body, uint8, "msg-type");
            READ_FIELD_VALUE(opt_body, uint32, "transaction-id");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_OPTIONS)) == 0)
            {
                if ((rc = fill_dhcp6_options(buf, sub_opt)) != 0)
                    break;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                rc = 0;
            else
                break;
        }
        else if (opt_type == DHCP6_OPT_AUTH)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_AUTH)) != 0)
                break;

            READ_FIELD_VALUE(opt_body, uint8, "protocol");
            READ_FIELD_VALUE(opt_body, uint8, "algorithm");
            READ_FIELD_VALUE(opt_body, uint8, "rdm");
            READ_FIELD_OCTETS(opt_body, "replay-detection");
            READ_FIELD_OCTETS(opt_body, "auth-info");
        }
        else if (opt_type == DHCP6_OPT_UNICAST)
        {
            READ_FIELD_OCTETS(opt, "server-address");
        }
        else if (opt_type == DHCP6_OPT_STATUS_CODE)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_STATUS)) != 0)
                break;

            READ_FIELD_VALUE(opt_body, uint16, "status-code");
            READ_FIELD_OCTETS(opt_body, "status-message");
        }
        else if (opt_type == DHCP6_OPT_USER_CLASS)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_USER_CLASS)) != 0)
                break;

            n_oro = asn_get_length(opt_body, "");
            for (j = 0; j < n_oro; j++)
            {
                if ((rc = asn_get_indexed(opt_body, (asn_value **)&sub_opt,
                                          j, "")) != 0)
                    break;

                READ_FIELD_VALUE(sub_opt, uint16, "class-data-len");
                READ_FIELD_OCTETS(sub_opt, "opaque-data");
            }
        }
        else if (opt_type == DHCP6_OPT_VENDOR_CLASS)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_VENDOR_CLASS)) != 0)
                break;

            READ_FIELD_VALUE(opt_body, uint32, "enterprise-number");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_VENDOR_CLASS)) != 0)
                break;

            n_oro = asn_get_length(sub_opt, "");
            for (j = 0; j < n_oro; j++)
            {
                if ((rc = asn_get_indexed(sub_opt, (asn_value **)&sub_sub_opt,
                                          j, "")) != 0)
                    break;

                READ_FIELD_VALUE(sub_sub_opt, uint16, "class-data-len");
                READ_FIELD_OCTETS(sub_sub_opt, "opaque-data");
            }
        }
        else if (opt_type == DHCP6_OPT_VENDOR_OPTS)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_VENDOR_SPECIFIC)) != 0)
                break;

            READ_FIELD_VALUE(opt_body, uint32, "enterprise-number");
            READ_FIELD_OCTETS(opt_body, "option-data");
        }
        else if (opt_type == DHCP6_OPT_IA_PD)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_IA_PD)) != 0)
                break;
            READ_FIELD_VALUE(opt_body, uint32, "iaid");
            READ_FIELD_VALUE(opt_body, uint32, "t1");
            READ_FIELD_VALUE(opt_body, uint32, "t2");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_OPTIONS)) == 0)
            {
                if ((rc = fill_dhcp6_options(buf, sub_opt)) != 0)
                    break;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                rc = 0;
            else
                break;
        }
        else if (opt_type == DHCP6_OPT_IA_PREFIX)
        {
            if ((rc = asn_get_child_value(opt, &opt_body, PRIVATE,
                                          NDN_DHCP6_IA_PREFIX)) != 0)
                break;
            READ_FIELD_VALUE(opt_body, uint32, "preferred-lifetime");
            READ_FIELD_VALUE(opt_body, uint32, "valid-lifetime");
            READ_FIELD_VALUE(opt_body, uint8, "prefix-length");
            READ_FIELD_OCTETS(opt_body, "prefix-address");

            if ((rc = asn_get_child_value(opt_body, &sub_opt, PRIVATE,
                                          NDN_DHCP6_OPTIONS)) == 0)
            {
                if ((rc = fill_dhcp6_options(buf, sub_opt)) != 0)
                    break;
            }
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
                rc = 0;
            else
                break;
        }
        else
        {   /* Read 'value' field directly */
            len = asn_get_length(opt, "value.#plain");
            if ((rc = asn_read_value_field(opt, buf, &len,
                                       "value.#plain")) != 0)
                break;

            buf += len;

            continue;
        }
    }
    return rc;
}
