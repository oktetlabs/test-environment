/** @file
 * @brief TAD Geneve
 *
 * Traffic Application Domain Command Handler
 * Geneve CSAP layer-related callbacks
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#define TE_LGR_USER "TAD Geneve"

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_alloc.h"

#include "ndn_geneve.h"
#include "tad_overlay_tools.h"
#include "tad_bps.h"
#include "tad_geneve_impl.h"

#define TAD_GENEVE_HEADER_LEN 8
#define TAD_GENEVE_HEADER_OPTIONS_LEN_OFFSET_BITS 2
#define TAD_GENEVE_HEADER_OPTIONS_LEN_NB_BITS 6
#define TAD_GENEVE_OPTION_LEN_MIN 4
#define TAD_GENEVE_OPTION_LEN_OFFSET_BITS 27
#define TAD_GENEVE_OPTION_LEN_NB_BITS 5
#define TAD_GENEVE_HEADER_BPS_DU_PROTOCOL_IDX 5
#define TAD_GENEVE_OPTION_BPS_DU_DATA_IDX 4

/** Geneve layer specific data */
typedef struct tad_geneve_proto_data {
    tad_bps_pkt_frag_def header;
    tad_bps_pkt_frag_def option;
} tad_geneve_proto_data_t;

/** Geneve option information */
typedef struct tad_geneve_option {
    tad_bps_pkt_frag_data  option;
    uint8_t                len_32bit_words;
} tad_geneve_option_t;

/**
 * Geneve layer specific data for PDU processing
 * (both send and receive)
 */
typedef struct tad_geneve_proto_pdu_data {
    tad_bps_pkt_frag_data  header;
    tad_geneve_option_t   *options;
    unsigned int           nb_options;
    uint8_t                options_len_32bit_words;
} tad_geneve_proto_pdu_data_t;

/** Geneve header BPS representation (draft-gross-geneve-00) */
static const tad_bps_pkt_frag tad_geneve_bps_header[] =
{
    { "version",           2,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "options-length",    6,
      BPS_FLD_CONST_DEF(NDN_TAG_GENEVE_OPTIONS_LENGTH, 0),
      TAD_DU_I32, TRUE },
    { "oam",               1,  BPS_FLD_CONST_DEF(NDN_TAG_GENEVE_OAM, 0),
      TAD_DU_I32, FALSE },
    { "critical",          1,  BPS_FLD_CONST_DEF(NDN_TAG_GENEVE_CRITICAL, 0),
      TAD_DU_I32, FALSE },
    { "reserved-1",        6,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "protocol",          16, BPS_FLD_SIMPLE(NDN_TAG_GENEVE_PROTOCOL),
      TAD_DU_I32, FALSE },
    { "vni",               24, BPS_FLD_CONST_DEF(NDN_TAG_GENEVE_VNI, 0),
      TAD_DU_I32, FALSE },
    { "reserved-2",        8,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
};

/** Geneve option BPS representation (draft-gross-geneve-00) */
static const tad_bps_pkt_frag tad_geneve_bps_option[] =
{
    { "option-class",      16, BPS_FLD_NO_DEF(NDN_TAG_GENEVE_OPTION_CLASS),
      TAD_DU_I32, FALSE },
    { "type",              8,  BPS_FLD_NO_DEF(NDN_TAG_GENEVE_OPTION_TYPE),
      TAD_DU_I32, FALSE },
    { "flags-reserved",    3,  BPS_FLD_CONST(0),
      TAD_DU_I32, FALSE },
    { "length",            5,  BPS_FLD_CONST_DEF(NDN_TAG_GENEVE_OPTION_LENGTH, 0),
      TAD_DU_I32, TRUE },
    { "data",              0,
      BPS_FLD_CONST_DEF(NDN_TAG_GENEVE_OPTION_DATA, 0),
      TAD_DU_OCTS, FALSE },
};

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_init_cb(csap_p       csap,
                   unsigned int layer_idx)
{
    tad_geneve_proto_data_t *proto_data = NULL;
    te_errno                 rc;

    if ((csap == NULL) || (csap->layers == NULL) || (layer_idx >= csap->depth))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = TE_ALLOC(sizeof(*proto_data));
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_init(tad_geneve_bps_header,
                               TE_ARRAY_LEN(tad_geneve_bps_header),
                               csap->layers[layer_idx].nds,
                               &proto_data->header);
    if (rc != 0)
        goto fail;

    rc = tad_overlay_guess_def_protocol(csap, layer_idx, &proto_data->header,
                                        TAD_GENEVE_HEADER_BPS_DU_PROTOCOL_IDX);
    if (rc != 0)
        goto fail;

    rc = tad_bps_pkt_frag_init(tad_geneve_bps_option,
                               TE_ARRAY_LEN(tad_geneve_bps_option),
                               NULL, &proto_data->option);
    if (rc != 0)
        goto fail;

    csap_set_proto_spec_data(csap, layer_idx, proto_data);
    return 0;

fail:
   /*
    * FIXME: It should be responsibility of tad_bps_pkt_frag_init()
    *        to perform cleanup for a fragment in case of failure
    */
   tad_bps_pkt_frag_free(&proto_data->option);

   tad_bps_pkt_frag_free(&proto_data->header);
   free(proto_data);

   return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_destroy_cb(csap_p       csap,
                      unsigned int layer_idx)
{
    tad_geneve_proto_data_t *proto_data;

    if ((csap == NULL) || (csap->layers == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    csap_set_proto_spec_data(csap, layer_idx, NULL);
    tad_bps_pkt_frag_free(&proto_data->header);
    tad_bps_pkt_frag_free(&proto_data->option);
    free(proto_data);

    return 0;
}

/* See description in 'tad_geneve_impl.h' */
void
tad_geneve_release_pdu_cb(csap_p        csap,
                          unsigned int  layer_idx,
                          void         *opaque)
{
    tad_geneve_proto_data_t     *proto_data;
    tad_geneve_proto_pdu_data_t *pdu_data = opaque;
    unsigned int                 i;

    if ((csap == NULL) || (pdu_data == NULL))
        return;

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return;

    tad_bps_free_pkt_frag_data(&proto_data->header, &pdu_data->header);

    if (pdu_data->options != NULL)
    {
        for (i = 0; i < pdu_data->nb_options; ++i)
        {
            tad_bps_free_pkt_frag_data(&proto_data->option,
                                       &pdu_data->options[i].option);
        }

        free(pdu_data->options);
    }

    free(pdu_data);
}

static te_errno
tad_geneve_mk_data_from_nds_and_confirm(tad_bps_pkt_frag_def  *def,
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
tad_geneve_process_options(tad_geneve_proto_data_t     *proto_data,
                           asn_value                   *layer_pdu,
                           tad_geneve_proto_pdu_data_t *pdu_data,
                           te_bool                      confirm)
{
    tad_geneve_option_t *options = NULL;
    int                  nb_options = 0;
    asn_value           *du_options;
    uint8_t              options_len_32bit_words = 0;
    te_errno             rc = 0;
    unsigned int         i;

    /* NOTE: Arguments check is omitted in static function */

    rc = asn_get_descendent(layer_pdu, &du_options, "options");
    if (rc == TE_EASNINCOMPLVAL)
        goto options_summary;
    else if (rc != 0)
        return rc;

    nb_options = asn_get_length(du_options, "");
    if (nb_options < 0)
        return TE_EINVAL;
    else if (nb_options == 0)
        goto options_summary;

    options = TE_ALLOC(nb_options * sizeof(*options));
    if (options == NULL)
        return TE_ENOMEM;

    for (i = 0; i < (unsigned int)nb_options; ++i)
    {
        asn_value *du_option;
        asn_value *du_option_data;
        uint8_t    option_data_len_32bit_words = 0;

        rc = asn_get_indexed(du_options, &du_option, i, "");
        if (rc != 0)
            goto fail;

        rc = asn_get_descendent(du_option, &du_option_data, "data");
        if (rc == 0)
        {
            int option_data_len_bytes;

            option_data_len_bytes = asn_get_length(du_option, "data");
            if ((option_data_len_bytes < 0) ||
                ((option_data_len_bytes % WORD_4BYTE) != 0))
            {
                rc = TE_EINVAL;
                goto fail;
            }

            option_data_len_32bit_words = option_data_len_bytes / WORD_4BYTE;
        }
        else if (rc != TE_EASNINCOMPLVAL)
        {
            goto fail;
        }

        options[i].len_32bit_words = (TAD_GENEVE_OPTION_LEN_MIN >> 2) +
                                     option_data_len_32bit_words;
        options_len_32bit_words += options[i].len_32bit_words;

        rc = tad_geneve_mk_data_from_nds_and_confirm(&proto_data->option,
                                                     du_option,
                                                     &options[i].option,
                                                     confirm);
        if (rc != 0)
            goto fail;
    }

options_summary:
    pdu_data->options = options;
    pdu_data->nb_options = nb_options;
    pdu_data->options_len_32bit_words = options_len_32bit_words;

    return 0;

fail:
    for (i = 0; i < (unsigned int)nb_options; ++i)
        tad_bps_free_pkt_frag_data(&proto_data->option, &options[i].option);

    free(options);

    return rc;
}

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_confirm_tmpl_cb(csap_p         csap,
                           unsigned int   layer_idx,
                           asn_value     *layer_pdu,
                           void         **p_opaque)
{
    tad_geneve_proto_data_t     *proto_data;
    tad_geneve_proto_pdu_data_t *tmpl_data = NULL;
    te_errno                     rc = 0;

    if ((csap == NULL) || (layer_pdu == NULL) || (p_opaque == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    tmpl_data = TE_ALLOC(sizeof(*tmpl_data));
    if (tmpl_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_geneve_mk_data_from_nds_and_confirm(&proto_data->header, layer_pdu,
                                                 &tmpl_data->header, TRUE);
    if (rc != 0)
        goto fail;

    rc = tad_geneve_process_options(proto_data, layer_pdu, tmpl_data, TRUE);
    if (rc != 0)
        goto fail;

    *p_opaque = tmpl_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &tmpl_data->header);
    free(tmpl_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_gen_bin_cb(csap_p                csap,
                      unsigned int          layer_idx,
                      const asn_value      *tmpl_pdu,
                      void                 *opaque,
                      const tad_tmpl_arg_t *args,
                      size_t                arg_num,
                      tad_pkts             *sdus,
                      tad_pkts             *pdus)
{
    tad_geneve_proto_data_t     *proto_data;
    tad_geneve_proto_pdu_data_t *tmpl_data = opaque;
    unsigned int                 bitoff;
    uint8_t                     *binary = NULL;
    size_t                       binary_len;
    unsigned int                 i;
    te_errno                     rc = 0;

    UNUSED(tmpl_pdu);

    if ((csap == NULL) || (tmpl_data == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    proto_data = csap_get_proto_spec_data(csap, layer_idx);
    if (proto_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    binary_len = TAD_GENEVE_HEADER_LEN +
                 (tmpl_data->options_len_32bit_words << 2);

    binary = TE_ALLOC(binary_len);
    if (binary == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    bitoff = 0;
    rc = tad_bps_pkt_frag_gen_bin(&proto_data->header, &tmpl_data->header,
                                  args, arg_num, binary, &bitoff,
                                  binary_len << 3);
    if (rc != 0)
        goto fail;

    if (bitoff != (TAD_GENEVE_HEADER_LEN << 3))
    {
        rc = TE_EINVAL;
        goto fail;
    }

    for (i = 0; i < tmpl_data->nb_options; ++i)
    {
        unsigned int bitoff_old = bitoff;

        rc = tad_bps_pkt_frag_gen_bin(&proto_data->option,
                                      &tmpl_data->options[i].option,
                                      args, arg_num, binary, &bitoff,
                                      binary_len << 3);
        if (rc != 0)
            goto fail;

        if ((bitoff - bitoff_old) !=
            (unsigned int)(tmpl_data->options[i].len_32bit_words << 5))
        {
            rc = TE_EINVAL;
            goto fail;
        }
    }

    tad_pkts_move(pdus, sdus);
    rc = tad_pkts_add_new_seg(pdus, TRUE, binary, binary_len,
                              (tad_pkt_seg_free)free);
    if (rc != 0)
        goto fail;

    return 0;

fail:
    free(binary);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_confirm_ptrn_cb(csap_p         csap,
                           unsigned int   layer_idx,
                           asn_value     *layer_pdu,
                           void         **p_opaque)
{
    tad_geneve_proto_data_t     *proto_data;
    tad_geneve_proto_pdu_data_t *ptrn_data = NULL;
    te_errno                     rc = 0;

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

    rc = tad_geneve_process_options(proto_data, layer_pdu, ptrn_data, FALSE);
    if (rc != 0)
        goto fail;

    *p_opaque = ptrn_data;

    return 0;

fail:
    tad_bps_free_pkt_frag_data(&proto_data->header, &ptrn_data->header);
    free(ptrn_data);

    return TE_RC(TE_TAD_CSAP, rc);
}

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_match_post_cb(csap_p              csap,
                         unsigned int        layer_idx,
                         tad_recv_pkt_layer *meta_pkt_layer)
{
    asn_value                   *meta_pkt_layer_nds = NULL;
    tad_geneve_proto_data_t     *proto_data;
    tad_geneve_proto_pdu_data_t *pkt_data;
    tad_pkt                     *pkt;
    unsigned int                 bitoff = 0;
    asn_value                   *options_nds;
    unsigned int                 i;
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

    meta_pkt_layer_nds = asn_init_value(ndn_geneve_header);
    if (meta_pkt_layer_nds == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_bps_pkt_frag_match_post(&proto_data->header, &pkt_data->header,
                                     pkt, &bitoff, meta_pkt_layer_nds);
    if ((rc != 0) || (pkt_data->nb_options == 0))
        goto out;

    options_nds = asn_init_value(ndn_geneve_options);
    if (options_nds == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rc = asn_put_child_value_by_label(meta_pkt_layer_nds, options_nds,
                                      "options");
    if (rc != 0)
    {
        asn_free_value(options_nds);
        goto out;
    }

    for (i = 0; i < pkt_data->nb_options; ++i)
    {
        asn_value    *option_nds;
        unsigned int  option_data_nb_bits;

        option_nds = asn_init_value(ndn_geneve_option);
        if (option_nds == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }

        rc = asn_insert_indexed(options_nds, option_nds, -1, "");
        if (rc != 0)
        {
            asn_free_value(option_nds);
            goto out;
        }

        rc = tad_bps_pkt_frag_match_post(&proto_data->option,
                                         &pkt_data->options[i].option,
                                         pkt, &bitoff, option_nds);
        if (rc != 0)
            goto out;

        option_data_nb_bits = (pkt_data->options[i].len_32bit_words << 5) -
                              (TAD_GENEVE_OPTION_LEN_MIN << 3);
        if (option_data_nb_bits != 0)
            continue;

        rc = asn_free_child(option_nds, PRIVATE, NDN_TAG_GENEVE_OPTION_DATA);
        rc = (rc == TE_EASNWRONGLABEL) ? 0 : rc;
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

/* See description in 'tad_geneve_impl.h' */
te_errno
tad_geneve_match_do_cb(csap_p           csap,
                       unsigned int     layer_idx,
                       const asn_value *ptrn_pdu,
                       void            *ptrn_opaque,
                       tad_recv_pkt    *meta_pkt,
                       tad_pkt         *pdu,
                       tad_pkt         *sdu)
{
    tad_geneve_proto_data_t     *proto_data;
    tad_geneve_proto_pdu_data_t *ptrn_data = ptrn_opaque;
    tad_geneve_proto_pdu_data_t *pkt_data = NULL;
    unsigned int                 bitoff = 0;
    tad_geneve_option_t         *options = NULL;
    unsigned int                 nb_options = 0;
    uint8_t                      options_len_32bit_words = 0;
    unsigned int                 nb_bits_total;
    unsigned int                 i;
    te_errno                     rc = 0;

    UNUSED(ptrn_pdu);

    if ((csap == NULL) || (ptrn_data == NULL) || (meta_pkt == NULL) ||
        (meta_pkt->layers == NULL))
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);

    if (tad_pkt_len(pdu) < TAD_GENEVE_HEADER_LEN)
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

    tad_pkt_read_bits(pdu, TAD_GENEVE_HEADER_OPTIONS_LEN_OFFSET_BITS,
                      TAD_GENEVE_HEADER_OPTIONS_LEN_NB_BITS,
                      &options_len_32bit_words);

    nb_bits_total = ((TAD_GENEVE_HEADER_LEN << 3) +
                     (options_len_32bit_words << 5));

    if ((tad_pkt_len(pdu) << 3) < nb_bits_total)
    {
        rc = TE_EINVAL;
        goto fail;
    }

    while(bitoff < nb_bits_total)
    {
        tad_geneve_option_t *options_new;
        unsigned int         nb_bits_remaining = nb_bits_total - bitoff;
        uint8_t              option_data_len_32bit_words;

        if (nb_bits_remaining < (TAD_GENEVE_OPTION_LEN_MIN << 3))
        {
            rc = TE_EINVAL;
            goto fail;
        }

        options_new = realloc(options, (nb_options + 1) * sizeof(*options));
        if (options_new == NULL)
        {
            rc = TE_ENOMEM;
            goto fail;
        }
        memset(&options_new[nb_options], 0, sizeof(*options));

        options = options_new;
        ++nb_options;

        rc = tad_bps_pkt_frag_match_pre(&proto_data->option,
                                        &options[nb_options - 1].option);
        if (rc != 0)
            goto fail;

        tad_pkt_read_bits(pdu, bitoff + TAD_GENEVE_OPTION_LEN_OFFSET_BITS,
                          TAD_GENEVE_OPTION_LEN_NB_BITS,
                          &option_data_len_32bit_words);

        options[nb_options - 1].len_32bit_words =
            (TAD_GENEVE_OPTION_LEN_MIN >> 2) + option_data_len_32bit_words;

        if (option_data_len_32bit_words > 0)
        {
            tad_data_unit_t *dus;
            tad_data_unit_t *du_data;
            uint8_t         *oct_str;

            dus = options[nb_options - 1].option.dus;
            du_data = &dus[TAD_GENEVE_OPTION_BPS_DU_DATA_IDX];

            du_data->val_data.len = option_data_len_32bit_words << 2;

            oct_str = TE_ALLOC(option_data_len_32bit_words << 2);
            if (oct_str == NULL)
            {
                rc = TE_ENOMEM;
                goto fail;
            }
            du_data->val_data.oct_str = oct_str;
            du_data->du_type = TAD_DU_OCTS;
        }

        if (nb_options <= ptrn_data->nb_options)
        {
            tad_bps_pkt_frag_data *ptrn_data_option_p;

            ptrn_data_option_p = &ptrn_data->options[nb_options - 1].option;

            rc = tad_bps_pkt_frag_match_do(&proto_data->option,
                                           ptrn_data_option_p,
                                           &options[nb_options - 1].option,
                                           pdu, &bitoff);
            if (rc != 0)
                goto fail;
        }
        else
        {
            bitoff += ((TAD_GENEVE_OPTION_LEN_MIN << 3) +
                       (option_data_len_32bit_words << 5));
        }
    }

    rc = tad_pkt_get_frag(sdu, pdu, bitoff >> 3,
                          tad_pkt_len(pdu) - (bitoff >> 3),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
        goto fail;

    pkt_data->options = options;
    pkt_data->nb_options = nb_options;
    pkt_data->options_len_32bit_words = options_len_32bit_words;

    meta_pkt->layers[layer_idx].opaque = pkt_data;

    return 0;

fail:
    if (options != NULL)
    {
        for (i = 0; i < nb_options; ++i)
            tad_bps_free_pkt_frag_data(&proto_data->option, &options[i].option);
    }
    free(options);

    tad_bps_free_pkt_frag_data(&proto_data->header, &pkt_data->header);
    free(pkt_data);

    return TE_RC(TE_TAD_CSAP, rc);
}
