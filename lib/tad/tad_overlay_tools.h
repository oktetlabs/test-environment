/** @file
 * @brief TAD Overlay Auxiliary Tools
 *
 * Traffic Application Domain Command Handler
 * Overlay Auxiliary Tools definitions
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
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
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_TAD_OVERLAY_TOOLS_H__
#define __TE_TAD_OVERLAY_TOOLS_H__

#include "config.h"

#include "te_defs.h"
#include "te_errno.h"

#include "tad_csap_support.h"
#include "tad_bps.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Guess and fill in encap. protocol number within an overlay header default BPS
 *
 * @param csap      CSAP pointer
 * @param layer_idx Index of the overlay layer
 * @param def       Overlay header default BPS
 * @param du_idx    Protocol number DU index within the BPS
 *
 * @return Status code
 */
extern te_errno tad_overlay_guess_def_protocol(csap_p                csap,
                                               unsigned int          layer_idx,
                                               tad_bps_pkt_frag_def *def,
                                               unsigned int          du_idx);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_OVERLAY_TOOLS_H__ */
