/** @file
 * @brief TAD GRE
 *
 * Traffic Application Domain Command Handler
 * GRE CSAP layer-related callbacks
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAD GRE"

#include "te_config.h"


#include "te_alloc.h"

#include "ndn_gre.h"
#include "tad_overlay_tools.h"
#include "tad_bps.h"
#include "tad_gre_impl.h"

#define TAD_GRE_HEADER_MIN_LEN 4
#define TAD_GRE_OPT_FIELD_LEN 4
#define TAD_GRE_HEADER_BPS_DU_PROTOCOL_IDX 6
#define TAD_GRE_HEADER_CKSUM_PRESENT_OFFSET 0
#define TAD_GRE_HEADER_KEY_PRESENT_OFFSET 2
#define TAD_GRE_HEADER_SEQN_PRESENT_OFFSET 3

/** GRE layer specific data */
typedef struct tad_gre_proto_data {
    tad_bps_pkt_frag_def header;
    tad_bps_pkt_frag_def opt_cksum;
    tad_bps_pkt_frag_def opt_key_nvgre;
    tad_bps_pkt_frag_def opt_seqn;
} tad_gre_proto_data_t;

/**
 * GRE layer specific data for PDU processing
 * (both send and receive)
 */
typedef struct tad_gre_proto_pdu_data {
    tad_bps_pkt_frag_data header;

    tad_bps_pkt_frag_data opt_cksum;
    te_bool               opt_cksum_valid;

    tad_bps_pkt_frag_data opt_key_nvgre;
    te_bool               opt_key_nvgre_valid;

    tad_bps_pkt_frag_data opt_seqn;
    te_bool               opt_seqn_valid;
} tad_gre_proto_pdu_data_t;

/** GRE header BPS representation (RFC 2784 updated by RFC 2890) */
static const tad_bps_pkt_frag tad_gre_bps_header[] =
{
    { "cksum-present",     1,  BPS_FLD_CONST_DEF(NDN_TAG_GRE_CKSUM_PRESENT, 0),
      TAD_DU_I32, FALSE },
    { "flags-reserved-1",  1,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "key-present",       1,  BPS_FLD_CONST_DEF(NDN_TAG_GRE_KEY_PRESENT, 0),
      TAD_DU_I32, FALSE },
    { "seqn-present",      1,  BPS_FLD_CONST_DEF(NDN_TAG_GRE_SEQN_PRESENT, 0),
      TAD_DU_I32, FALSE },
    { "flags-reserved-2",  9,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "version",           3,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "protocol",          16, BPS_FLD_SIMPLE(NDN_TAG_GRE_PROTOCOL),
      TAD_DU_I32, FALSE },
};

/** GRE header optional checksum BPS representation (RFC 2784) */
static const tad_bps_pkt_frag tad_gre_bps_header_opt_cksum[] =
{
    { "value",             16,
      BPS_FLD_CONST_DEF(NDN_TAG_GRE_OPT_CKSUM_VALUE, 0),
      TAD_DU_I32, TRUE },
    { "reserved",          16, BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
};

/**
 * NVGRE-specific implementation of GRE key optional field (RFC 7637)
 * BPS representation
 */
static const tad_bps_pkt_frag tad_gre_bps_header_opt_key_nvgre[] =
{
    { "vsid",              24,
      BPS_FLD_CONST_DEF(NDN_TAG_GRE_OPT_KEY_NVGRE_VSID, 0),
      TAD_DU_I32, FALSE },
    { "flowid",            8,
      BPS_FLD_CONST_DEF(NDN_TAG_GRE_OPT_KEY_NVGRE_FLOWID, 0),
      TAD_DU_I32, FALSE },
};

/** GRE header optional sequence number BPS representation (RFC 2890) */
static const tad_bps_pkt_frag tad_gre_bps_header_opt_seqn[] =
{
    { "value",             32,
      BPS_FLD_CONST_DEF(NDN_TAG_GRE_OPT_SEQN_VALUE, 0),
      TAD_DU_I32, FALSE },
};

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_init_cb(csap_p       csap,
                unsigned int layer_idx)
{
    tad_gre_proto_data_t *proto_data = NULL;
    te_errno              rc;

    if ((csap == NULL) || (csap->layers == NULL) || (layer_idx >= csap->depth))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_init(tad_gre_bps_header,
                               TE_ARRAY_LEN(tad_gre_bps_header),
                               csap->layers[layer_idx].nds,
                               &proto_data->header);
    if (rc != 0)
        goto fail;

    rc = tad_overlay_guess_def_protocol(csap, layer_idx, &proto_data->header,
                                        TAD_GRE_HEADER_BPS_DU_PROTOCOL_IDX);
    if (rc != 0)
        goto fail;

    rc = tad_bps_pkt_frag_init(tad_gre_bps_header_opt_cksum,
                               TE_ARRAY_LEN(tad_gre_bps_header_opt_cksum),
                               NULL, &proto_data->opt_cksum);
    if (rc != 0)
        goto fail;

    rc = tad_bps_pkt_frag_init(tad_gre_bps_header_opt_key_nvgre,
                               TE_ARRAY_LEN(tad_gre_bps_header_opt_key_nvgre),
                               NULL, &proto_data->opt_key_nvgre);
    if (rc != 0)
        goto fail;

    rc = tad_bps_pkt_frag_init(tad_gre_bps_header_opt_seqn,
                               TE_ARRAY_LEN(tad_gre_bps_header_opt_seqn),
                               NULL, &proto_data->opt_seqn);
    if (rc != 0)
        goto fail;

    csap_set_proto_spec_data(csap, layer_idx, proto_data);
    return 0;

fail:
   tad_bps_pkt_frag_free(&proto_data->opt_seqn);
   tad_bps_pkt_frag_free(&proto_data->opt_key_nvgre);
   tad_bps_pkt_frag_free(&proto_data->opt_cksum);
   tad_bps_pkt_frag_free(&proto_data->header);
   free(proto_data);

   return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_destroy_cb(csap_p       csap,
                   unsigned int layer_idx)
{
    tad_gre_proto_data_t *proto_data;

    if (csap == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    csap_set_proto_spec_data(csap, layer_idx, NULL);
    tad_bps_pkt_frag_free(&proto_data->header);
    tad_bps_pkt_frag_free(&proto_data->opt_cksum);
    tad_bps_pkt_frag_free(&proto_data->opt_key_nvgre);
    tad_bps_pkt_frag_free(&proto_data->opt_seqn);
    free(proto_data);

    return 0;
}

/* See description in 'tad_gre_impl.h' */
void
tad_gre_release_pdu_cb(csap_p        csap,
                       unsigned int  layer_idx,
                       void         *opaque)
{
    tad_gre_proto_data_t     *proto_data;
    tad_gre_proto_pdu_data_t *pdu_data = opaque;

    if ((csap == NULL) || (pdu_data == NULL))
        return;

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return;

    tad_bps_free_pkt_frag_data(&proto_data->header, &pdu_data->header);
    tad_bps_free_pkt_frag_data(&proto_data->opt_cksum, &pdu_data->opt_cksum);
    tad_bps_free_pkt_frag_data(&proto_data->opt_seqn, &pdu_data->opt_seqn);
    tad_bps_free_pkt_frag_data(&proto_data->opt_key_nvgre,
                               &pdu_data->opt_key_nvgre);

    free(pdu_data);
}

static te_errno
tad_gre_mk_data_from_nds_and_confirm(tad_bps_pkt_frag_def  *def,
                                     asn_value             *nds,
                                     tad_bps_pkt_frag_data *data,
                                     te_bool                confirm)
{
    te_errno rc;

    /* NOTE: Arguments check is omitted in static function */

    rc = tad_bps_nds_to_data_units(def, nds, data);
    if (rc != 0)
        return rc;

   return (confirm) ? tad_bps_confirm_send(def, data) : 0;
}

static te_errno
tad_gre_process_opt_fields(tad_gre_proto_data_t        *proto_data,
                           asn_value                   *layer_pdu,
                           tad_gre_proto_pdu_data_t    *pdu_data,
                           te_bool                      confirm)
{
    asn_value *opt_cksum = NULL;
    asn_value *opt_seqn = NULL;
    asn_value *opt_key = NULL;
    asn_value *opt_key_nvgre = NULL;
    te_errno   rc;

    /* NOTE: Arguments check is omitted in static function */

    rc = asn_get_descendent(layer_pdu, &opt_cksum, "opt-cksum");
    rc = (rc == TE_EASNINCOMPLVAL) ? 0 : rc;
    if (rc != 0)
        return rc;

    rc = asn_get_descendent(layer_pdu, &opt_seqn, "opt-seqn");
    rc = (rc == TE_EASNINCOMPLVAL) ? 0 : rc;
    if (rc != 0)
        return rc;

    rc = asn_get_descendent(layer_pdu, &opt_key, "opt-key");
    rc = (rc == TE_EASNINCOMPLVAL) ? 0 : rc;
    if (rc != 0)
        return rc;

    if (opt_key != NULL)
    {
        rc = asn_get_choice_value(opt_key, &opt_key_nvgre, NULL, NULL);
        if (rc != 0)
            return rc;

        if (asn_get_tag(opt_key_nvgre) != NDN_TAG_GRE_OPT_KEY_NVGRE)
            opt_key_nvgre = NULL;
    }

#define TAD_GRE_PROCESS_OPT_FIELD(_name) \
    if (_name != NULL)                                                         \
    {                                                                          \
        rc = tad_gre_mk_data_from_nds_and_confirm(&proto_data->_name, _name,   \
                                                  &pdu_data->_name,            \
                                                  confirm);                    \
        if (rc != 0)                                                           \
            goto fail;                                                         \
                                                                               \
        pdu_data->_name##_valid = TRUE;                                        \
    }

    TAD_GRE_PROCESS_OPT_FIELD(opt_cksum);
    TAD_GRE_PROCESS_OPT_FIELD(opt_seqn);
    TAD_GRE_PROCESS_OPT_FIELD(opt_key_nvgre);

#undef TAD_GRE_PROCESS_OPT_FIELD

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->opt_cksum, &pdu_data->opt_cksum);
    tad_bps_free_pkt_frag_data(&proto_data->opt_seqn, &pdu_data->opt_seqn);
    tad_bps_free_pkt_frag_data(&proto_data->opt_key_nvgre,
                               &pdu_data->opt_key_nvgre);

    return rc;
}

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_confirm_tmpl_cb(csap_p         csap,
                        unsigned int   layer_idx,
                        asn_value     *layer_pdu,
                        void         **p_opaque)
{
    tad_gre_proto_data_t     *proto_data;
    tad_gre_proto_pdu_data_t *tmpl_data = NULL;
    te_errno                  rc = 0;

    if ((csap == NULL) || (layer_pdu == NULL) || (p_opaque == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    tmpl_data = TE_ALLOC(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_gre_mk_data_from_nds_and_confirm(&proto_data->header, layer_pdu,
                                              &tmpl_data->header, TRUE);
    if (rc != 0)
        goto fail;

    rc = tad_gre_process_opt_fields(proto_data, layer_pdu, tmpl_data, TRUE);
    if (rc != 0)
        goto fail;

    *p_opaque = tmpl_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &tmpl_data->header);
    free(tmpl_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_gen_bin_cb(csap_p                csap,
                   unsigned int          layer_idx,
                   const asn_value      *tmpl_pdu,
                   void                 *opaque,
                   const tad_tmpl_arg_t *args,
                   size_t                arg_num,
                   tad_pkts             *sdus,
                   tad_pkts             *pdus)
{
    tad_gre_proto_data_t     *proto_data;
    tad_gre_proto_pdu_data_t *tmpl_data = opaque;
    unsigned int              bitoff;
    uint8_t                  *binary = NULL;
    size_t                    binary_len;
    te_errno                  rc = 0;

    UNUSED(tmpl_pdu);

    if ((csap == NULL) || (tmpl_data == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    binary_len = TAD_GRE_HEADER_MIN_LEN;
    if (tmpl_data->opt_cksum_valid)
        binary_len += TAD_GRE_OPT_FIELD_LEN;
    if (tmpl_data->opt_seqn_valid)
        binary_len += TAD_GRE_OPT_FIELD_LEN;
    if (tmpl_data->opt_key_nvgre_valid)
        binary_len += TAD_GRE_OPT_FIELD_LEN;

    binary = TE_ALLOC(binary_len);
    if (binary == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->header, &tmpl_data->header,
                                  args, arg_num, binary, &bitoff,
                                  binary_len << 3);
    if (rc != 0)
        goto fail;

    if (bitoff != (TAD_GRE_HEADER_MIN_LEN << 3))
    {
        rc = TE_EINVAL;
        goto fail;
    }

#define TAD_GRE_GEN_BIN_OPT_FIELD(_name) \
    if (tmpl_data->_name##_valid)                                              \
    {                                                                          \
        unsigned int bitoff_old = bitoff;                                      \
                                                                               \
        rc = tad_bps_pkt_frag_gen_bin(&proto_data->_name, &tmpl_data->_name,   \
                                      args, arg_num, binary, &bitoff,          \
                                      binary_len << 3);                        \
        if (rc != 0)                                                           \
            goto fail;                                                         \
                                                                               \
        if ((bitoff - bitoff_old) != (TAD_GRE_OPT_FIELD_LEN << 3))             \
        {                                                                      \
           rc = TE_EINVAL;                                                     \
           goto fail;                                                          \
        }                                                                      \
    }

    /* At first approximation, it's important to keep the specified order */
    TAD_GRE_GEN_BIN_OPT_FIELD(opt_cksum);
    TAD_GRE_GEN_BIN_OPT_FIELD(opt_key_nvgre);
    TAD_GRE_GEN_BIN_OPT_FIELD(opt_seqn);

#undef TAD_GRE_GEN_BIN_OPT_FIELD

    tad_pkts_move(pdus, sdus);
    rc = tad_pkts_add_new_seg(pdus, TRUE, binary, binary_len,
                              tad_pkt_seg_data_free);
    if (rc != 0)
        goto fail;

    return 0;

fail:
    free(binary);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_confirm_ptrn_cb(csap_p         csap,
                        unsigned int   layer_idx,
                        asn_value     *layer_pdu,
                        void         **p_opaque)
{
    tad_gre_proto_data_t     *proto_data;
    tad_gre_proto_pdu_data_t *ptrn_data = NULL;
    te_errno                  rc = 0;

    if ((csap == NULL) || (layer_pdu == NULL) || (p_opaque == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    ptrn_data = TE_ALLOC(sizeof(*ptrn_data));
    if (ptrn_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_nds_to_data_units(&proto_data->header, layer_pdu,
                                   &ptrn_data->header);
    if (rc != 0)
        goto fail;

    rc = tad_gre_process_opt_fields(proto_data, layer_pdu, ptrn_data, FALSE);
    if (rc != 0)
        goto fail;

    *p_opaque = ptrn_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &ptrn_data->header);
    free(ptrn_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_match_post_cb(csap_p              csap,
                      unsigned int        layer_idx,
                      tad_recv_pkt_layer *meta_pkt_layer)
{
    asn_value                   *meta_pkt_layer_nds = NULL;
    tad_gre_proto_data_t        *proto_data;
    tad_gre_proto_pdu_data_t    *pkt_data;
    tad_pkt                     *pkt;
    unsigned int                 bitoff = 0;
    te_errno                     rc = 0;

    if (csap == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    if (~csap->state & CSAP_STATE_RESULTS)
        return 0;

    if (meta_pkt_layer == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    pkt = tad_pkts_first_pkt(&meta_pkt_layer->pkts);
    pkt_data = meta_pkt_layer->opaque;
    if ((proto_data == NULL) || (pkt == NULL) || (pkt_data == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    meta_pkt_layer_nds = asn_init_value(ndn_gre_header);
    if (meta_pkt_layer_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_match_post(&proto_data->header, &pkt_data->header,
                                     pkt, &bitoff, meta_pkt_layer_nds);
    if (rc != 0)
        goto out;

    if (pkt_data->opt_cksum_valid)
    {
        asn_value *opt_cksum_nds;

        opt_cksum_nds = asn_init_value(ndn_gre_header_opt_cksum);
        if (opt_cksum_nds == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_put_child_value_by_label(meta_pkt_layer_nds, opt_cksum_nds,
                                          "opt-cksum");
        if (rc != 0)
        {
            asn_free_value(opt_cksum_nds);
            goto out;
        }

        rc = tad_bps_pkt_frag_match_post(&proto_data->opt_cksum, &pkt_data->opt_cksum,
                                         pkt, &bitoff, opt_cksum_nds);
        if (rc != 0)
            goto out;
    }

    if (pkt_data->opt_key_nvgre_valid)
    {
        asn_value *opt_key_nds;
        asn_value *opt_key_nvgre_nds;

        opt_key_nds = asn_init_value(ndn_gre_header_opt_key);
        if (opt_key_nds == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_put_child_value_by_label(meta_pkt_layer_nds, opt_key_nds,
                                          "opt-key");
        if (rc != 0)
        {
            asn_free_value(opt_key_nds);
            goto out;
        }

        opt_key_nvgre_nds = asn_init_value(ndn_gre_header_opt_key_nvgre);
        if (opt_key_nvgre_nds == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_put_choice(opt_key_nds, opt_key_nvgre_nds);
        if (rc != 0)
        {
            asn_free_value(opt_key_nvgre_nds);
            goto out;
        }

        rc = tad_bps_pkt_frag_match_post(&proto_data->opt_key_nvgre,
                                         &pkt_data->opt_key_nvgre,
                                         pkt, &bitoff, opt_key_nvgre_nds);
        if (rc != 0)
            goto out;
    }

    if (pkt_data->opt_seqn_valid)
    {
        asn_value *opt_seqn_nds;

        opt_seqn_nds = asn_init_value(ndn_gre_header_opt_seqn);
        if (opt_seqn_nds == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_put_child_value_by_label(meta_pkt_layer_nds, opt_seqn_nds,
                                          "opt-seqn");
        if (rc != 0)
        {
            asn_free_value(opt_seqn_nds);
            goto out;
        }

        rc = tad_bps_pkt_frag_match_post(&proto_data->opt_seqn,
                                         &pkt_data->opt_seqn,
                                         pkt, &bitoff, opt_seqn_nds);
        if (rc != 0)
            goto out;
    }

out:
    if (rc == 0)
        meta_pkt_layer->nds = meta_pkt_layer_nds;
    else
        asn_free_value(meta_pkt_layer_nds);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_gre_impl.h' */
te_errno
tad_gre_match_do_cb(csap_p           csap,
                    unsigned int     layer_idx,
                    const asn_value *ptrn_pdu,
                    void            *ptrn_opaque,
                    tad_recv_pkt    *meta_pkt,
                    tad_pkt         *pdu,
                    tad_pkt         *sdu)
{
    size_t                    pdu_len;
    tad_gre_proto_data_t     *proto_data;
    tad_gre_proto_pdu_data_t *ptrn_data = ptrn_opaque;
    tad_gre_proto_pdu_data_t *pkt_data = NULL;
    unsigned int              bitoff = 0;
    te_errno                  rc = 0;

    UNUSED(ptrn_pdu);

    if ((csap == NULL) || (ptrn_data == NULL) || (meta_pkt == NULL) ||
        (meta_pkt->layers == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    pdu_len = tad_pkt_len(pdu);

    if (pdu_len < TAD_GRE_HEADER_MIN_LEN)
        return TE_RC(TE_TAD_CSAP, TE_ETADNOTMATCH);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    pkt_data = TE_ALLOC(sizeof(*pkt_data));
    if (pkt_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_match_pre(&proto_data->header, &pkt_data->header);
    if (rc != 0)
        goto fail;

    rc = tad_bps_pkt_frag_match_do(&proto_data->header, &ptrn_data->header,
                                   &pkt_data->header, pdu, &bitoff);
    if (rc != 0)
        goto fail;

    if (tad_pkt_read_bit(pdu, TAD_GRE_HEADER_CKSUM_PRESENT_OFFSET))
    {
        if (((pdu_len << 3) - bitoff) < (TAD_GRE_OPT_FIELD_LEN << 3))
        {
            rc = TE_EINVAL;
            goto fail;
        }

        pkt_data->opt_cksum_valid = TRUE;

        rc = tad_bps_pkt_frag_match_pre(&proto_data->opt_cksum,
                                        &pkt_data->opt_cksum);
        if (rc != 0)
            goto fail;

        if (ptrn_data->opt_cksum_valid)
        {
            tad_data_unit_t    *opt_cksum_value_du;
            tad_cksum_str_code  cksum_str_code;

            if (ptrn_data->opt_cksum.dus == NULL)
            {
                rc = TE_EINVAL;
                goto fail;
            }

            opt_cksum_value_du = &ptrn_data->opt_cksum.dus[0];
            cksum_str_code = tad_du_get_cksum_str_code(opt_cksum_value_du);

            if (cksum_str_code != TAD_CKSUM_STR_CODE_NONE)
                tad_data_unit_clear(opt_cksum_value_du);

            rc = tad_bps_pkt_frag_match_do(&proto_data->opt_cksum,
                                           &ptrn_data->opt_cksum,
                                           &pkt_data->opt_cksum,
                                           pdu, &bitoff);
            if (rc != 0)
                goto fail;

            if (cksum_str_code != TAD_CKSUM_STR_CODE_NONE)
            {
                uint8_t  *pdu_binary;
                uint16_t  cksum;

                pdu_binary = TE_ALLOC(pdu_len);
                if (pdu_binary == NULL)
                {
                    rc = TE_ENOMEM;
                    goto fail;
                }

                tad_pkt_read_bits(pdu, 0, pdu_len << 3, pdu_binary);

                /*
                 * We don't care about 16-bit word padding here and rely on
                 * calculate_checksum() behaviour which should keep an eye on it
                 */
                cksum = ~calculate_checksum((void *)pdu_binary, pdu_len);
                free(pdu_binary);
                if ((cksum_str_code == TAD_CKSUM_STR_CODE_CORRECT) !=
                    (cksum == CKSUM_CMP_CORRECT))
                {
                    rc = TE_ETADNOTMATCH;
                    goto fail;
                }
            }
        }
        else
        {
            bitoff += (TAD_GRE_OPT_FIELD_LEN << 3);
        }
    }

    if (tad_pkt_read_bit(pdu, TAD_GRE_HEADER_KEY_PRESENT_OFFSET))
    {
        if (((pdu_len << 3) - bitoff) < (TAD_GRE_OPT_FIELD_LEN << 3))
        {
            rc = TE_EINVAL;
            goto fail;
        }

        pkt_data->opt_key_nvgre_valid = TRUE;

        rc = tad_bps_pkt_frag_match_pre(&proto_data->opt_key_nvgre,
                                        &pkt_data->opt_key_nvgre);
        if (rc != 0)
            goto fail;

        if (ptrn_data->opt_key_nvgre_valid)
        {
            rc = tad_bps_pkt_frag_match_do(&proto_data->opt_key_nvgre,
                                           &ptrn_data->opt_key_nvgre,
                                           &pkt_data->opt_key_nvgre,
                                           pdu, &bitoff);
            if (rc != 0)
                goto fail;
        }
        else
        {
            bitoff += (TAD_GRE_OPT_FIELD_LEN << 3);
        }
    }

    if (tad_pkt_read_bit(pdu, TAD_GRE_HEADER_SEQN_PRESENT_OFFSET))
    {
        if (((pdu_len << 3) - bitoff) < (TAD_GRE_OPT_FIELD_LEN << 3))
        {
            rc = TE_EINVAL;
            goto fail;
        }

        pkt_data->opt_seqn_valid = TRUE;

        rc = tad_bps_pkt_frag_match_pre(&proto_data->opt_seqn,
                                        &pkt_data->opt_seqn);
        if (rc != 0)
            goto fail;

        if (ptrn_data->opt_seqn_valid)
        {
            rc = tad_bps_pkt_frag_match_do(&proto_data->opt_seqn,
                                           &ptrn_data->opt_seqn,
                                           &pkt_data->opt_seqn,
                                           pdu, &bitoff);
            if (rc != 0)
                goto fail;
        }
        else
        {
            bitoff += (TAD_GRE_OPT_FIELD_LEN << 3);
        }
    }

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          pdu_len - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
        goto fail;

    meta_pkt->layers[layer_idx].opaque = pkt_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &pkt_data->header);
    tad_bps_free_pkt_frag_data(&proto_data->opt_cksum, &pkt_data->opt_cksum);
    tad_bps_free_pkt_frag_data(&proto_data->opt_seqn, &pkt_data->opt_seqn);
    tad_bps_free_pkt_frag_data(&proto_data->opt_key_nvgre,
                               &pkt_data->opt_key_nvgre);
    free(pkt_data);

    return TE_RC(TE_TAD_CSAP, rc);
}
