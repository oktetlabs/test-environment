/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Overlay Auxiliary Tools
 *
 * Traffic Application Domain Command Handler
 * Overlay Auxiliary Tools definitions
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_OVERLAY_TOOLS_H__
#define __TE_TAD_OVERLAY_TOOLS_H__


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
