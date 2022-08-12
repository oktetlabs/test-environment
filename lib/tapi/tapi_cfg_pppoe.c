/** @file
 * @brief Test API to configure PPPoE.
 *
 * Implementation of API to configure PPPoE.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAPI CFG PPPOE"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "conf_api.h"
#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_pppoe.h"


/** Format string for DHCP server object instance */
#define TE_CFG_TA_PPPOE_SERVER_FMT   "/agent:%s/pppoeserver:"

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_if_add(const char *ta, const char *ifname)
{
    cfg_handle      handle;

    /* Add subnet configuration entry */
    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_PPPOE_SERVER_FMT "/interface:%s",
                                ta, ifname);
}

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_if_del(const char *ta, const char *ifname)
{
    /* Delete subnet configuration entry */
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_PPPOE_SERVER_FMT
                                "/interface:%s", ta, ifname);
}

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_subnet_set(const char *ta, const char *subnet)
{
    if (ta == NULL || subnet == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Add subnet configuration entry */
    return cfg_set_instance_fmt(CFG_VAL(STRING, subnet),
                                TE_CFG_TA_PPPOE_SERVER_FMT "/subnet:", ta);
}

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_subnet_get(const char *ta, char **subnet_p)
{
    int rc;

    /* Add subnet configuration entry */
    if ((rc = cfg_get_instance_string_fmt(subnet_p,
                                          TE_CFG_TA_PPPOE_SERVER_FMT "/subnet:",
                                          ta)) != 0)
    {
        ERROR("Failed to get pppoe server subnet: %r", rc);
    }

    return rc;
}

/* See description in tapi_cfg_pppoe.h */
void
tapi_cfg_pppoe_server_laddr_get(const char *ta, struct sockaddr **addr)
{
    CHECK_RC(cfg_get_instance_fmt(NULL, addr,
                                  TE_CFG_TA_PPPOE_SERVER_FMT "/laddr:", ta));
}

/* See description in tapi_cfg_pppoe.h */
void
tapi_cfg_pppoe_server_raddr_get(const char *ta, struct sockaddr **addr)
{
    CHECK_RC(cfg_get_instance_fmt(NULL, addr,
                                  TE_CFG_TA_PPPOE_SERVER_FMT "/raddr:", ta));
}
