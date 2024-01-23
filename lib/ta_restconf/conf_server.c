/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RESTCONF agent library
 *
 * Implementation of /config/server configuration tree.
 */

#define TE_LGR_USER "TA RESTCONF Conf Server"

#include "te_config.h"

#include "te_str.h"
#include "te_string.h"
#include "rcf_pch.h"
#include "ta_restconf.h"
#include "ta_restconf_internal.h"


static te_errno
password_get(unsigned int gid, const char *oid, char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", restconf.password);
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
password_set(unsigned int gid, const char *oid, const char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strlcpy_safe((char *)&restconf.password,
                         value, sizeof(restconf.password));
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
username_get(unsigned int gid, const char *oid, char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", restconf.username);
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
username_set(unsigned int gid, const char *oid, const char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strlcpy_safe((char *)&restconf.username,
                         value, sizeof(restconf.username));
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
https_get(unsigned int gid, const char *oid, char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", restconf.https ? "1" : "0");
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
https_set(unsigned int gid, const char *oid, const char *value)
{
    te_bool tmp;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strtol_bool(value, &tmp);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TA, rc);

    restconf.https = tmp;
    return 0;
}

static te_errno
port_get(unsigned int gid, const char *oid, char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_snprintf(value, RCF_MAX_VAL, "%u", (unsigned int)restconf.port);
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
port_set(unsigned int gid, const char *oid, const char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strtou_size(value, 0, &restconf.port,
                        sizeof(restconf.port));
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
host_get(unsigned int gid, const char *oid, char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_snprintf(value, RCF_MAX_VAL, "%s", restconf.host);
    return TE_RC_UPSTREAM(TE_TA, rc);
}

static te_errno
host_set(unsigned int gid, const char *oid, const char *value)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strlcpy_safe((char *)&restconf.host,
                         value, sizeof(restconf.host));
    return TE_RC_UPSTREAM(TE_TA, rc);
}

RCF_PCH_CFG_NODE_RW(node_password, "password", NULL, NULL,
                    password_get, password_set);

RCF_PCH_CFG_NODE_RW(node_username, "username", NULL, &node_password,
                    username_get, username_set);

RCF_PCH_CFG_NODE_RW(node_https, "https", NULL, &node_username,
                    https_get, https_set);

RCF_PCH_CFG_NODE_RW(node_port, "port", NULL, &node_https,
                    port_get, port_set);

RCF_PCH_CFG_NODE_RW(node_host, "host", NULL, &node_port,
                    host_get, host_set);

RCF_PCH_CFG_NODE_RO(node_server, "server", &node_host, NULL, NULL);


te_errno
ta_restconf_conf_server_init(void)
{
    return rcf_pch_add_node("/agent/restconf/config", &node_server);
}
