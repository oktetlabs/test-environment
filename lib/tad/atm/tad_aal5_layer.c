/** @file
 * @brief TAD AAL5
 *
 * Traffic Application Domain Command Handler.
 * AAL5 CSAP layer-related callbacks.
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

#define TE_LGR_USER     "TAD AAL5"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_bps.h"

#include "ndn_atm.h"
#include "tad_atm_impl.h"


/**
 * AAL5 layer specific data
 */
typedef struct tad_aal5_proto_data {
    tad_bps_pkt_frag_def    trailer;
} tad_aal5_proto_data;

/**
 * AAL5 layer specific data for send processing
 */
typedef struct tad_aal5_proto_tmpl_data {
    tad_bps_pkt_frag_data   trailer;
} tad_aal5_proto_tmpl_data;


/**
 * Definition of AAL5 CPCS PDU trailer.
 */
static const tad_bps_pkt_frag tad_all5_bps_cpcs_trailer[] =
{
    { "cpcs-uu",    8,  BPS_FLD_CONST_DEF(NDN_TAG_AAL5_CPCS_UU, 0),
                        TAD_DU_I32, FALSE },
    { "cpi",        8,  BPS_FLD_CONST_DEF(NDN_TAG_AAL5_CPI, 0),
                        TAD_DU_I32, FALSE },
    { "length",     16, NDN_TAG_AAL5_LENGTH,
                        ASN_TAG_CONST, ASN_TAG_USER, 0, TAD_DU_I32, TRUE },
    { "crc",        32, NDN_TAG_AAL5_CRC,
                        ASN_TAG_CONST, ASN_TAG_USER, 0, TAD_DU_I32, TRUE },
};

/** Array filled in with zeros to be used as padding */
static uint8_t  tad_all5_pad[ATM_PAYLOAD_LEN - 1] = { 0, };




/**
 * Calculate product of a(x) by x^k in 
 * the residue-class ring of polynomials by modulo G(x) 
 *
 * G(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + 
 *             + x^8  + x^7  + x^5  + x^4  + x^2  + x    + 1
 *
 * Note, that bitwise XOR is åddition for polynomials over Z_2 field. 
 *
 * @param a     Bitmask, representing coefficients of polynomial over 
 *              Z_2 field with degree, less then 32.
 * @param k     Degree of monome, with which product should be obtained.
 *
 * @return bitmask, represenging coefficients of production result.
 */
static inline uint32_t
product_in_ring_to_power(uint32_t a, int k)
{
    const uint32_t g_defect = 0x04c11db7; /* G(x) - x^32 */
    uint32_t r = 0;

    for (; k > 0; k--)
    {
        /* perform a(x) := (a(x) * x) mod G(x) */
        r = a << 1;
        if (a & 0x80000000)
            a = r ^ g_defect;
        else 
            a = r;
    }

    return a;
}

/* See description in tad_atm_impl.h */
uint32_t
calculate_crc32(uint32_t previous_value, 
                uint8_t *next_pkt,
                size_t   next_len)
{ 
    uint32_t result = previous_value;

    if (next_pkt == NULL) 
        return 0;

    for (result = previous_value; next_len > 0; next_pkt++, next_len--)
        result = product_in_ring_to_power(result, 8) ^ 
                 product_in_ring_to_power(next_pkt[0], 32);

    return result;
}



/* See description in tad_atm_impl.h */
te_errno
tad_aal5_init_cb(csap_p csap, unsigned int layer)
{
    te_errno                rc;
    tad_aal5_proto_data     *proto_data;
    const asn_value        *layer_nds;

    proto_data = calloc(1, sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_set_proto_spec_data(csap, layer, proto_data);

    layer_nds = csap->layers[layer].nds;

    /* Initialize AAL5 cell header binary support */
    rc = tad_bps_pkt_frag_init(tad_all5_bps_cpcs_trailer,
                               TE_ARRAY_LEN(tad_all5_bps_cpcs_trailer),
                               layer_nds, &proto_data->trailer);
    if (rc != 0)
        return rc;

    if (tad_bps_pkt_frag_data_bitlen(&proto_data->trailer, NULL) !=
            (AAL5_TRAILER_LEN << 3))
    {
        ERROR(CSAP_LOG_FMT "Unexpected AAL5 cell header length",
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CH, TE_EINVAL);
    }

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_aal5_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_aal5_proto_data    *proto_data;

    proto_data = csap_get_proto_spec_data(csap, layer);
    csap_set_proto_spec_data(csap, layer, NULL);

    tad_bps_pkt_frag_free(&proto_data->trailer);

    free(proto_data);

    return 0;
}


/* See description in tad_atm_impl.h */
te_errno
tad_aal5_confirm_tmpl_cb(csap_p csap, unsigned int  layer, 
                         asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_aal5_proto_data         *proto_data;
    tad_aal5_proto_tmpl_data    *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* FIXME: Use malloc() and initialize data units at first */
    tmpl_data = calloc(1, sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    /* Get template values for AAL5 cell header fields */
    rc = tad_bps_nds_to_data_units(&proto_data->trailer, layer_pdu,
                                   &tmpl_data->trailer);
    if (rc != 0)
        return rc;

    rc = tad_bps_confirm_send(&proto_data->trailer, &tmpl_data->trailer);
    if (rc != 0)
        return rc;

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_aal5_confirm_ptrn_cb(csap_p csap, unsigned int  layer, 
                         asn_value *layer_pdu, void **p_opaque)
{
    te_errno                    rc;
    tad_aal5_proto_data         *proto_data;
    tad_aal5_proto_tmpl_data    *tmpl_data;

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* FIXME: Use malloc() and initialize data units at first */
    tmpl_data = calloc(1, sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = tmpl_data;

    /* Get template values for AAL5 cell header fields */
    rc = tad_bps_nds_to_data_units(&proto_data->trailer, layer_pdu,
                                   &tmpl_data->trailer);
    if (rc != 0)
        return rc;

    return rc;
}


/**
 * Calculate CRC-32 of the segment data.
 *
 * This function complies with tad_pkt_seg_enum_cb prototype.
 */
static te_errno
tad_all5_crc32(const tad_pkt *pkt, tad_pkt_seg *seg,
               unsigned int seg_num, void *opaque)
{
    uint32_t   *crc32 = opaque;

    if (seg_num < tad_pkt_seg_num(pkt) - 1)
    {
        *crc32 = calculate_crc32(*crc32, seg->data_ptr, seg->data_len);
    }
    else
    {
        assert(seg->data_len == AAL5_TRAILER_LEN);
        /* Exclude CRC placeholder from the calculations */
        *crc32 = calculate_crc32(*crc32,
                                 seg->data_ptr,
                                 seg->data_len - sizeof(uint32_t));
    }

    return 0;
}

/**
 * Data for callback to prepare AAL5 PDUs.
 */
typedef struct tad_aal5_prepare_pdus_data {
    csap_p      csap;       /**< CSAP */
    tad_pkts   *pdus;       /**< List to put PDUs */
    uint8_t    *trailer;    /**< CPCS PDU trailer template */
    te_bool     write_crc;  /**< Calculate and write CRC to trailer */
    te_bool     write_len;  /**< Calculate and write payload length
                                 to trailer */
} tad_aal5_prepare_pdus_data;

/**
 * Fix length of the padding segment. Calculate CRC.
 * Write length of the payload and CRC in CPCS PDU trailer.
 *
 * @a opaque parameter refers to tad_aal5_prepare_pdus_data.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_aal5_prepare_pdus(tad_pkt *pkt, void *opaque)
{
    tad_aal5_prepare_pdus_data *data = opaque;

    tad_pkt_seg            *trailer = tad_pkt_last_seg(pkt);
    tad_pkt_seg            *padding = tad_pkt_prev_seg(pkt, trailer);
    size_t                  pld_len;
    size_t                  pad_len;
    te_errno                rc;
    tad_atm_cell_ctrl_data *cell_ctrl;
    uint16_t                len;
    uint32_t                crc = CRC32_INIT;

    /* Remember actual length of the payload */
    assert(tad_pkt_len(pkt) >= AAL5_TRAILER_LEN);
    pld_len = tad_pkt_len(pkt) - AAL5_TRAILER_LEN;

    /*
     * Calculate padding length
     */
    assert(padding->data_len == 0);

    pad_len = tad_pkt_len(pkt) % ATM_PAYLOAD_LEN;
    if (pad_len > 0)
        pad_len = ATM_PAYLOAD_LEN - pad_len;
    tad_pkt_set_seg_data_len(pkt, padding, pad_len);

    assert(tad_pkt_len(pkt) % ATM_PAYLOAD_LEN == 0);

    /* 
     * Copy template of the trailer
     */
    assert(trailer->data_len == AAL5_TRAILER_LEN);
    memcpy(trailer->data_ptr, data->trailer, AAL5_TRAILER_LEN);

    /*
     * Write length of the payload to trailer if requested
     */
    if (data->write_len)
    {
        len = htons(pld_len);
        memcpy((uint8_t *)trailer->data_ptr + 2, &len, sizeof(len));
    }

    /*
     * Calculate CRC and write it to trailer if requested
     */
    if (data->write_crc)
    {
        (void)tad_pkt_enumerate_seg(pkt, tad_all5_crc32, &crc);
        crc = htonl(~crc);
        memcpy((uint8_t *)trailer->data_ptr + 4, &crc, sizeof(crc));
    }

    /*
     * CPCS-PDU is ready for 'segmentation'
     */
    rc = tad_pkt_fragment(pkt, ATM_PAYLOAD_LEN,
                          -1 /* no additional segment */,
                          FALSE /* meaningless with -1 */,
                          data->pdus);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Segmentation of CPCS-PDU to ATM cells "
              "payload failed: %r", CSAP_LOG_ARGS(data->csap), rc);
    }
    else
    {
        /* Set ATM-User-to-ATM-User indication to 1 in the last cell */
        cell_ctrl = malloc(sizeof(*cell_ctrl));
        if (cell_ctrl == NULL)
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
        cell_ctrl->indication = TRUE;
        tad_pkt_set_opaque(data->pdus->pkts.cqh_last, cell_ctrl, free);
    }

    return rc;
}

/* See description in tad_atm_impl.h */
te_errno
tad_aal5_gen_bin_cb(csap_p                csap,
                    unsigned int          layer,
                    const asn_value      *tmpl_pdu,
                    void                 *opaque,
                    const tad_tmpl_arg_t *args,
                    size_t                arg_num,
                    tad_pkts             *sdus,
                    tad_pkts             *pdus)
{
    tad_aal5_proto_data        *proto_data;
    tad_aal5_proto_tmpl_data   *tmpl_data = opaque;
    tad_aal5_prepare_pdus_data  cb_data;

    te_errno        rc;
    uint8_t         trailer[AAL5_TRAILER_LEN];
    unsigned int    bitoff = 0;

    UNUSED(tmpl_pdu);

    proto_data = csap_get_proto_spec_data(csap, layer);

    /* Prepare AAL5 CPCS PDU trailer template */
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->trailer, &tmpl_data->trailer,
                                  args, arg_num, trailer, &bitoff,
                                  sizeof(trailer) << 3);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare AAL5 cell header: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }
    assert(bitoff == (sizeof(trailer) << 3));

    /* Add space for AAL5 padding segment to each PDU */
    rc = tad_pkts_add_new_seg(sdus, FALSE, tad_all5_pad, 0, NULL);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to add AAL5 padding segment: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }
    /* Add space for AAL5 CPCS PDU trailer segment to each PDU */
    rc = tad_pkts_add_new_seg(sdus, FALSE, NULL, AAL5_TRAILER_LEN, NULL);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to add AAL5 CPCS PDU trailer "
              "segment: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* Check each packet and fill in its header */
    cb_data.csap = csap;
    cb_data.pdus = pdus;
    cb_data.trailer = trailer;
    /*
     * Don't calculate and write payload length
     * if it is already present in the template.
     */
    cb_data.write_len = tmpl_data->trailer.dus[2].du_type == TAD_DU_UNDEF;

    /*
     * Don't calculate and write CRC if it
     * is already present in the template.
     */
    cb_data.write_crc = tmpl_data->trailer.dus[3].du_type == TAD_DU_UNDEF;

    rc = tad_pkt_enumerate(sdus, tad_aal5_prepare_pdus, &cb_data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare AAL5 PDUs: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}


/* See description in tad_atm_impl.h */
te_errno
tad_aal5_match_bin_cb(csap_p           csap,
                      unsigned int     layer,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(ptrn_pdu);
    UNUSED(ptrn_opaque);
    UNUSED(meta_pkt);
    UNUSED(pdu);
    UNUSED(sdu);

    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
