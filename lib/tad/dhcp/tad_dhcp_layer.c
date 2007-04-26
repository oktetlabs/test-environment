/** @file
 * @brief TAD DHCP
 *
 * Traffic Application Domain Command Handler.
 * DHCP CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD DHCP"

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


/* See description tad_ipstack_impl.h */
te_errno
tad_dhcp_init_cb(csap_p csap, unsigned int layer)
{ 
    te_errno                rc;
    tad_dhcp_proto_data    *proto_data;
    const asn_value        *layer_nds;

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    rc = tad_bps_pkt_frag_init(tad_dhcp_bps_hdr,
                               TE_ARRAY_LEN(tad_dhcp_bps_hdr),
                               layer_nds, &proto_data->hdr);
    if (rc != 0)
        return rc;

    return 0; 
}

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

    int     xid;
    size_t  len = sizeof (xid);

    rc = asn_read_value_field(layer_pdu, &xid, &len, "xid.#plain");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        xid = random();
        rc = asn_write_int32(layer_pdu, xid, "xid.#plain");
        if (rc != 0) 
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
tad_dhcp_gen_bin_cb(csap_p csap, unsigned int layer,
                    const asn_value *tmpl_pdu, void *opaque,
                    const tad_tmpl_arg_t *args, size_t arg_num, 
                    tad_pkts *sdus, tad_pkts *pdus)
{
    tad_dhcp_proto_data                 *proto_data;
    tad_dhcp_proto_pdu_data             *tmpl_data = opaque;

    te_errno        rc;
    size_t          bitlen;
    unsigned int    bitoff;
    uint8_t        *msg;

#ifdef TAD_DHCP_OPTIONS
    const asn_value    *options;
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
            if (opt_type == 255 || opt_type == 0)
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

#endif /* TAD_DHCP_OPTIONS */

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
