/** @file
 * @brief Test API to configure local subtree.
 *
 * Definition of API to configure local subtree.
 *
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_LOCAL_H__
#define __TE_TAPI_CFG_LOCAL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "tapi_cfg.h"
#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_local Local subtree access
 * @ingroup tapi_conf
 * @{
 */

/**
 * Disable reuse_pco mode for the next test iteration.
 *
 * @return 0 on success or error code
 */
static inline te_errno
tapi_no_reuse_pco_disable_once(void)
{
    te_errno rc;

    rc = tapi_cfg_set_int_str(1, NULL, "/local:/no_reuse_pco:");

    return rc;
}

/**
 * Reset @b no_reuse_pco local value to default
 *
 * @return 0 on success or error code
 */
static inline te_errno
tapi_no_reuse_pco_reset(void)
{
    te_errno rc;

    rc = tapi_cfg_set_int_str(0, NULL, "/local:/no_reuse_pco:");

    return rc;
}

/**
 * Get @b no_reuse_pco local value
 *
 * @param    no_reuse_pco [OUT] pointer to value of no_reuse_pco local variable
 *
 * @return @return 0 on success or error code
 */
static inline te_errno
tapi_no_reuse_pco_get(te_bool *no_reuse_pco)
{
    int      no_reuse_pco_tmp;
    te_errno rc;

    rc = tapi_cfg_get_int_str(&no_reuse_pco_tmp, "/local:/no_reuse_pco:");
    if (rc == TE_RC(TE_CS, TE_ENOENT))
    {
      *no_reuse_pco = FALSE;
      return 0;
    }
    if (rc == 0)
      *no_reuse_pco = (no_reuse_pco_tmp == 0) ? FALSE : TRUE;

    return rc;
}


/**@} <!-- END tapi_conf_local --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_LOCAL_H__ */
