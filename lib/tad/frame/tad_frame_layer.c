/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Frame
 *
 * Traffic Application Domain Command Handler.
 * Frame layer-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD Frame"

#include "te_config.h"

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_frame_impl.h"


/** Frame layer pattern data */
typedef struct tad_frame_ptrn_data {
    uint32_t        rest_len;
    tad_recv_pkt   *reassemble_pkt;
} tad_frame_ptrn_data;


/**
 * Fill in frame length field.
 *
 * This function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_frame_set_len(tad_pkt *pkt, void *opaque)
{
    tad_pkt_seg    *frame_hdr = tad_pkt_first_seg(pkt);
    uint32_t        frame_len = htonl(tad_pkt_len(pkt));

    UNUSED(opaque);

    assert(frame_hdr->data_len == sizeof(frame_len));
    memcpy(frame_hdr->data_ptr, &frame_len, sizeof(frame_len));

    return 0;
}


/* See description in tad_frame_impl.h */
te_errno
tad_frame_gen_bin_cb(csap_p                csap,
                     unsigned int          layer,
                     const asn_value      *tmpl_pdu,
                     void                 *opaque,
                     const tad_tmpl_arg_t *args,
                     size_t                arg_num,
                     tad_pkts             *sdus,
                     tad_pkts             *pdus)
{
    te_errno        rc;

    UNUSED(opaque);

    assert(csap != NULL);
    F_ENTRY("(%d:%u) tmpl_pdu=%p args=%p arg_num=%u sdus=%p pdus=%p",
            csap->id, layer, (void *)tmpl_pdu, (void *)args,
            (unsigned)arg_num, sdus, pdus);

    /* Move all SDUs to PDUs */
    tad_pkts_move(pdus, sdus);

    /* Add header segment to each PDU. */
    rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, sizeof(uint32_t), NULL);
    if (rc != 0)
    {
        return rc;
    }

    /* Fill in frame-length field in each PDU */
    rc = tad_pkt_enumerate(pdus, tad_frame_set_len, NULL);
    if (rc != 0)
    {
        ERROR("Failed to set length for all PDUs-frames: %r", rc);
        return rc;
    }

    return 0;
}


/* See description in tad_frame_impl.h */
te_errno
tad_frame_confirm_ptrn_cb(csap_p csap, unsigned int  layer,
                          asn_value *layer_pdu, void **p_opaque)
{
    tad_frame_ptrn_data    *ptrn_data;

    UNUSED(csap);
    UNUSED(layer);
    UNUSED(layer_pdu);

    ptrn_data = malloc(sizeof(*ptrn_data));
    if (ptrn_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    *p_opaque = ptrn_data;

    ptrn_data->rest_len = 0;
    ptrn_data->reassemble_pkt = NULL;

    return 0;
}

/* See description in tad_frame_impl.h */
void
tad_frame_release_ptrn_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_frame_ptrn_data *ptrn_data = opaque;

    UNUSED(layer);

    if (ptrn_data == NULL)
        return;

    if (ptrn_data->reassemble_pkt != NULL)
    {
        tad_recv_pkt_free(csap, ptrn_data->reassemble_pkt);
        WARN("Incompletely reassembled frame destructed.\n"
             "Possibly garbage remains in frame layer media.\n");
    }

    free(ptrn_data);
}

/* See description in tad_frame_impl.h */
te_errno
tad_frame_match_do_cb(csap_p           csap,
                      unsigned int     layer,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{
    tad_frame_ptrn_data *ptrn_data = ptrn_opaque;

    UNUSED(csap);
    UNUSED(layer);
    UNUSED(ptrn_pdu);
    UNUSED(meta_pkt);
    UNUSED(pdu);
    UNUSED(sdu);

    UNUSED(ptrn_data);

    return TE_RC(TE_TAD_CSAP, TE_ENOSYS);
}
