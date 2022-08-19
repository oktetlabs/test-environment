/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to configure VTund.
 *
 * Implementation of API to configure VTund.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI CFG VTund"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "te_sleep.h"
#include "te_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_vtund.h"


/** Default port for VTund server */
#define TAPI_CFG_VTUND_PORT_DEF     5000


/* See description in tapi_cfg_vtund.h */
te_errno
tapi_cfg_vtund_create_tunnel(const char            *ta_srv,
                             const char            *ta_clnt,
                             const struct sockaddr *srv_addr,
                             cfg_handle            *ta_srv_if,
                             cfg_handle            *ta_clnt_if)
{
    te_errno        rc;
    uint16_t        srv_port;
    cfg_handle      tmp;
    char           *str;

    srv_port = ntohs(te_sockaddr_get_port(srv_addr));
    if (srv_port == 0)
        srv_port = TAPI_CFG_VTUND_PORT_DEF;

    /* Configure tunnel */

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 0),
                              "/agent:%s/vtund:/server:%u",
                              ta_srv, srv_port);
    if (rc != 0)
        return rc;
    te_sleep(1); /* Wait for server setup. */

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                              "/agent:%s/vtund:/server:%u/session:%s-%s",
                              ta_srv, srv_port, ta_srv, ta_clnt);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 0),
                              "/agent:%s/vtund:/client:%s-%s",
                              ta_clnt, ta_srv, ta_clnt);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(ADDRESS, srv_addr),
                              "/agent:%s/vtund:/client:%s-%s/server:",
                              ta_clnt, ta_srv, ta_clnt);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, srv_port),
                              "/agent:%s/vtund:/client:%s-%s/port:",
                              ta_clnt, ta_srv, ta_clnt);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                              "/agent:%s/vtund:/server:%u",
                              ta_srv, srv_port);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                              "/agent:%s/vtund:/client:%s-%s",
                              ta_clnt, ta_srv, ta_clnt);
    if (rc != 0)
        return rc;

    te_sleep(10);

    /*
     * Synchronize configuration trees and get assigned interfaces
     */

    rc = cfg_synchronize_fmt(TRUE, "/agent:%s", ta_srv);
    if (rc != 0)
        return rc;

    rc = cfg_get_instance_string_fmt(&str,
                         "/agent:%s/vtund:/server:%u/session:%s-%s/interface:",
                         ta_srv, srv_port, ta_srv, ta_clnt);
    if (rc != 0)
    {
        ERROR("Failed to get name of the network interface created "
              "by the tunnel on server side: %r", rc);
        return rc;
    }

    {
        char if_oid[100];

        snprintf(if_oid, sizeof(if_oid), "/agent:%s/interface:%s",
                 ta_srv, str);
        rc = cfg_add_instance_fmt(NULL, CVT_STRING, if_oid,
                                  "/agent:%s/rsrc:%s",
                                  ta_srv, str);
        if (rc != 0)
        {
            ERROR("Failed to add resource for a new PPP interface '%s'"
                  "on TA '%s': %r", str, ta_srv, rc);
            return rc;
        }
    }

    rc = cfg_find_fmt(&tmp, "/agent:%s/interface:%s", ta_srv, str);
    if (rc != 0)
    {
        ERROR("Failed to find interface '%s' on TA '%s': %r",
              str, ta_srv, rc);
        free(str);
        return rc;
    }

    rc = tapi_cfg_base_if_up(ta_srv, str);
    if (rc != 0)
    {
        ERROR("Failed to UP interface '%s' on TA '%s': %r",
              str, ta_srv, rc);
        free(str);
        return rc;
    }

    free(str);

    if (ta_srv_if != NULL)
        *ta_srv_if = tmp;


    rc = cfg_synchronize_fmt(TRUE, "/agent:%s", ta_clnt);
    if (rc != 0)
        return rc;

    rc = cfg_get_instance_string_fmt(&str,
                                     "/agent:%s/vtund:/client:%s-%s/interface:",
                                     ta_clnt, ta_srv, ta_clnt);
    if (rc != 0)
    {
        ERROR("Failed to get name of the network interface created "
              "by the tunnel on client side: %r", rc);
        return rc;
    }

    {
        char if_oid[100];

        snprintf(if_oid, sizeof(if_oid), "/agent:%s/interface:%s",
                 ta_clnt, str);
        rc = cfg_add_instance_fmt(NULL, CVT_STRING, if_oid,
                                  "/agent:%s/rsrc:%s",
                                  ta_clnt, str);
        if (rc != 0)
        {
            ERROR("Failed to add resource for a new PPP interface '%s'"
                  "on TA '%s': %r", str, ta_clnt, rc);
            return rc;
        }
    }

    rc = cfg_find_fmt(&tmp, "/agent:%s/interface:%s", ta_clnt, str);
    if (rc != 0)
    {
        ERROR("Failed to find interface '%s' on TA '%s': %r",
              str, ta_clnt, rc);
        free(str);
        return rc;
    }

    rc = tapi_cfg_base_if_up(ta_clnt, str);
    if (rc != 0)
    {
        ERROR("Failed to UP interface '%s' on TA '%s': %r",
              str, ta_clnt, rc);
        free(str);
        return rc;
    }

    free(str);

    if (ta_clnt_if != NULL)
        *ta_clnt_if = tmp;

    return 0;
}
