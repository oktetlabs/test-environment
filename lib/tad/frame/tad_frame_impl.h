/** @file
 * @brief TAD Frame
 *
 * Traffic Application Domain Command Handler.
 * Frame layer support internal declarations.
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

#ifndef __TE_TAD_FRAME_IMPL_H__
#define __TE_TAD_FRAME_IMPL_H__ 

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_frame_gen_bin_cb(csap_p                csap,
                                     unsigned int          layer,
                                     const asn_value      *tmpl_pdu,
                                     void                 *opaque,
                                     const tad_tmpl_arg_t *args,
                                     size_t                arg_num,
                                     tad_pkts             *sdus,
                                     tad_pkts             *pdus);


/**
 * Callback for initialize pattern opaque data.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_frame_confirm_ptrn_cb(csap_p         csap,
                                          unsigned int   layer, 
                                          asn_value     *layer_pdu,
                                          void         **p_opaque); 

/**
 * Callback to release pattern opaque data.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_frame_release_ptrn_cb(csap_p csap, unsigned int layer,
                                      void *opaque);

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_frame_match_do_cb(csap_p           csap,
                                      unsigned int     layer,
                                      const asn_value *ptrn_pdu,
                                      void            *ptrn_opaque,
                                      tad_recv_pkt    *meta_pkt,
                                      tad_pkt         *pdu,
                                      tad_pkt         *sdu);

#endif /* !__TE_TAD_FRAME_IMPL_H__ */
