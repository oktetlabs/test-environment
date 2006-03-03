/** @file
 * @brief TAD Frame
 *
 * Traffic Application Domain Command Handler.
 * Frame layer-related callbacks.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Frame"

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
#include "tad_frame_impl.h"


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
tad_frame_match_do_cb(csap_p           csap,
                      unsigned int     layer,
                      const asn_value *ptrn_pdu,
                      void            *ptrn_opaque,
                      tad_recv_pkt    *meta_pkt,
                      tad_pkt         *pdu,
                      tad_pkt         *sdu)
{
    UNUSED(ptrn_pdu);
    UNUSED(ptrn_opaque);

    return 0;
}
