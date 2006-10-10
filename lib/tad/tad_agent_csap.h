/** @file
 * @brief TAD /agent/csap
 *
 * Traffic Application Domain Command Handler.
 * Implementation of /agent/csap configuration subtree.
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
 * $Id: tad_csap_inst.h 28325 2006-05-22 13:11:25Z arybchik $
 */

#ifndef __TE_TAD_AGENT_CSAP_H__
#define __TE_TAD_AGENT_CSAP_H__ 

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize TAD /agent/csap configuration subtree support.
 *
 * @return Status code.
 */
extern te_errno tad_agent_csap_init(void);

/**
 * Shutdown TAD /agent/csap configuration subtree support.
 *
 * @return Status code.
 */
extern te_errno tad_agent_csap_fini(void);

/**
 * Destroy CSAP.
 *
 * @param csap_id       ID of the CSAP to be destroyed
 *
 * @return Status code.
 */
extern te_errno tad_csap_destroy(csap_handle_t csap_id);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_AGENT_CSAP_H__ */
