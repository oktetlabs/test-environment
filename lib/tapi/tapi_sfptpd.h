/** @file
 * @brief Test API to manage Solarflare PTP daemon
 *
 * @defgroup tapi_conf_sfptpd Solarflare PTP daemon configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to manage sfptpd daemon.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_SFPTPD_H__
#define __TE_TAPI_SFPTPD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_errno.h"
#include "rcf_rpc.h"

/**
 * Start sfptpd daemon.
 *
 * @param ta    Test agent name
 */
static inline void
tapi_sfptpd_enable(const char *ta)
{
    CHECK_RC(cfg_set_instance_fmt(CVT_INTEGER, (void *)1,
                                  "/agent:%s/sfptpd:/enable:", ta));
    TAPI_WAIT_NETWORK;
}

/**
 * Stop sfptpd daemon.
 *
 * @param ta    Test agent name
 */
static inline void
tapi_sfptpd_disable(const char *ta)
{
    CHECK_RC(cfg_set_instance_fmt(CVT_INTEGER, (void *)0,
                                  "/agent:%s/sfptpd:/enable:", ta));
}

/**
 * sfptpd daemon status.
 *
 * @param ta    Test agent name
 *
 * @return @c TRUE if daemon is enabled
 */
static inline te_bool
tapi_sfptpd_status(const char *ta)
{
    int val;

    CHECK_RC(cfg_get_instance_int_fmt(&val, "/agent:%s/sfptpd:/enable:", ta));

    return val == 0 ? FALSE : TRUE;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_TAPI_SFPTPD_H__ */

/**@} <!-- END tapi_conf_sfptpd --> */
