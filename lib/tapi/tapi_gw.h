/** @file
 * @brief Test GateWay network configuring API
 *
 * @defgroup ts_tapi_gw Macros to get test parameters on a gateway configuration
 * @ingroup te_ts_tapi
 * @{
 *
 * Macros to be used in tests. The header must be included from test
 * sources only. It is allowed to use the macros only from @b main()
 * function of the test.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Konstantin Abramenko <Konstantin Abramenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_GW_H__
#define __TE_TAPI_GW_H__

#include "conf_api.h"
#include "tapi_cfg.h"

/**
 * Get tagged network address from Configurator.
 *
 * @param _addr   variable for address.
 * @param _tag    name in CS of address.
 */
#define TEST_GET_TAG_ADDR(_addr, _tag) \
    do {                                                        \
        te_errno rc;                                            \
        rc = cfg_get_instance_fmt(NULL, &(_addr),               \
                      "/local:/addr:/entry:%s/address:", #_tag);\
        if (rc != 0)                                            \
            TEST_FAIL("Failed to get tagged address: %r", rc);  \
    } while (0)



/**
 * Get name of tagged network interface from Configurator.
 *
 * @param _if     variable for interface name.
 * @param _tag    name in CS of interface.
 */
#define TEST_GET_TST_IF(_if, _tag)  \
    do {                                                                \
        char *string;                                                   \
        te_errno rc = cfg_get_instance_fmt(NULL, &(string),             \
                                           "/local:/tst_if:" # _tag);   \
        if (rc != 0)                                                    \
            TEST_FAIL("Failed to get name of tester "                   \
                      "interface: %r", rc);                             \
        if (tapi_is_cfg_link(string))                                   \
        {                                                               \
            (_if) = strdup(strrchr(string, ':') + 1);                   \
        }                                                               \
        else                                                            \
            (_if) = string;                                             \
    } while (0)

#endif /* !__TE_TAPI_GW_H__ */

/**@} <!-- END ts_tapi_gw --> */
