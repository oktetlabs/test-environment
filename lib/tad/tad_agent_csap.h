/** @file
 * @brief TAD /agent/csap
 *
 * Traffic Application Domain Command Handler.
 * Implementation of /agent/csap configuration subtree.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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
extern te_errno tad_csap_destroy_by_id(csap_handle_t csap_id);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_AGENT_CSAP_H__ */
