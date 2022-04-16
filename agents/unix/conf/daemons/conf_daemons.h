/** @file
 * @brief Unix Test Agent
 *
 * Unix daemons initialization/release functions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 */

#ifndef __TE_TA_UNIX_CONF_DAEMONS_H__
#define __TE_TA_UNIX_CONF_DAEMONS_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef CFG_UNIX_DAEMONS
/**
 * Initializes conf_daemons support.
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno ta_unix_conf_daemons_init(void);

/** Release resources allocated for the configuration support. */
extern void ta_unix_conf_daemons_release(void);
#endif

#ifdef WITH_SFPTPD
/** Init sfptpd subtree. */
extern te_errno ta_unix_conf_sfptpd_init(void);

/** Release resources allocated for sfptpd management. */
extern void ta_unix_conf_sfptpd_release(void);
#endif

#ifdef WITH_NTPD
/** Init NTP daemon subtree. */
extern te_errno ta_unix_conf_ntpd_init(void);
#endif

#endif /* __TE_TA_UNIX_CONF_DAEMONS_H__ */
