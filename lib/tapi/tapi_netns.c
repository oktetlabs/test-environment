/** @file
 * @brief Network namespaces configuration test API
 *
 * Implementation of test API for network namespaces configuration.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "NETNS TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include "conf_api.h"
#include "tapi_cfg_base.h"
#include "tapi_namespaces.h"


/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add_rsrc(const char *ta, const char *ns_name)
{
    char ns_oid[CFG_OID_MAX];

    snprintf(ns_oid, CFG_OID_MAX, "/agent:%s/namespace:/net:%s", ta, ns_name);
    return cfg_add_instance_fmt(NULL, CVT_STRING, ns_oid,
                                "/agent:%s/rsrc:netns_%s", ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_del_rsrc(const char *ta, const char *ns_name)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/rsrc:netns_%s",
                                ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add(const char *ta, const char *ns_name)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/namespace:/net:%s", ta, ns_name);
    if (rc != 0)
        return rc;

    return tapi_netns_add_rsrc(ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_del(const char *ta, const char *ns_name)
{
    te_errno rc;

    rc = cfg_del_instance_fmt(TRUE, "/agent:%s/namespace:/net:%s", ta,
                              ns_name);
    if (rc != 0)
        return rc;

    return tapi_netns_del_rsrc(ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_if_set(const char *ta, const char *ns_name, const char *if_name)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/namespace:/net:%s/interface:%s",
                              ta, ns_name, if_name);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_del_rsrc(ta, if_name);
    if (rc == TE_RC(TE_CS, TE_ENOENT))
        return 0;

    return rc;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_if_unset(const char *ta, const char *ns_name, const char *if_name)
{
    return cfg_del_instance_fmt(TRUE,
                                "/agent:%s/namespace:/net:%s/interface:%s",
                                ta, ns_name, if_name);
}
