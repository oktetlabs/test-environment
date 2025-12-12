/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Network namespaces configuration test API
 *
 * Implementation of test API for network namespaces configuration.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "NETNS TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "te_kvpair.h"
#include "conf_api.h"
#include "logger_api.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_l4_port.h"
#include "tapi_cfg_rcf.h"
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
    return cfg_del_instance_fmt(false, "/agent:%s/rsrc:netns_%s",
                                ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add(const char *ta, const char *ns_name)
{
    te_errno rc;

    rc = tapi_netns_add_rsrc(ta, ns_name);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/namespace:/net:%s", ta, ns_name);
    if (rc != 0)
    {
        (void)tapi_netns_del_rsrc(ta, ns_name);
        return rc;
    }

    return 0;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_del(const char *ta, const char *ns_name)
{
    te_errno rc;

    rc = cfg_del_instance_fmt(true, "/agent:%s/namespace:/net:%s", ta,
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
    return cfg_del_instance_fmt(true,
                                "/agent:%s/namespace:/net:%s/interface:%s",
                                ta, ns_name, if_name);
}

/**
 * Configure created TA @p ta_name in basic aspects similar to @p base_ta.
 */
static te_errno
configure_ta_by_ta(const char *base_ta, const char *ta_name)
{
    te_errno rc;
    char *str;

    /*
     * Loopback interface should be UP in the namespace
     * for logging from RPC servers to work correctly.
     */
    rc = tapi_cfg_base_if_add_rsrc(ta_name, "lo");
    if (rc == 0)
        rc = tapi_cfg_base_if_up(ta_name, "lo");
    if (rc == 0)
        rc = tapi_cfg_base_if_del_rsrc(ta_name, "lo");

    if (rc != 0)
    {
        ERROR("Failed to bring loopback up on TA '%s': %r", ta_name, rc);
        return rc;
    }

    rc = cfg_get_string(&str, "/agent:%s/rpcprovider:", base_ta);
    if (rc == 0)
    {
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, str),
                                  "/agent:%s/rpcprovider:", ta_name);
        free(str);
        if (rc != 0)
        {
            ERROR("Failed to set TA '%s' RPC provider: %r", ta_name, rc);
            return rc;
        }
    }
    else if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        ERROR("Failed to get TA '%s' RPC provider: %r", base_ta, rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add_ta_by_ta(const char *base_ta, const char *ns_name,
                        const char *ta_name)
{
    const char *base_shell;
    unsigned int ta_flags;
    te_kvpair_h ta_conf;
    uint16_t rcf_port;
    char *ta_type;
    char *ta_rcflib;
    char *ta_confstr;
    char *shell = NULL;
    te_errno rc;

    rc = rcf_get_ta(base_ta, &ta_type, &ta_rcflib, &ta_confstr, &ta_flags);
    if (rc != 0)
    {
        ERROR("Failed to get TA '%s' configuration: %r", base_ta, rc);
        return rc;
    }

    te_kvpair_init(&ta_conf);
    rc = te_kvpair_from_str(ta_confstr, &ta_conf);
    if (rc != 0)
    {
        ERROR("Cannot parse TA '%s' confstr '%s': %r", base_ta, ta_confstr, rc);
        goto fail_ta_conf;
    }

    rc = te_kvpairs_del(&ta_conf, "port");
    if (rc != 0)
    {
        ERROR("Failed to delete port from base TA config: %r", rc);
        goto fail_del_port;
    }

    rc = tapi_cfg_l4_port_alloc(base_ta, AF_UNSPEC, SOCK_STREAM, &rcf_port);
    if (rc != 0)
    {
        ERROR("Failed to allocate TCP port on base TA '%s': %r", base_ta, rc);
        goto fail_alloc_port;
    }

    rc = te_kvpair_add(&ta_conf, "port", "%u", rcf_port);
    if (rc != 0)
    {
        ERROR("Failed to add port to TA config: %r", rc);
        goto fail_add_port;
    }

    /* Use format string with empty value to cope -Wformat-zero-length */
    rc = te_kvpair_add(&ta_conf, "ext_rcf_listener", "%s", "");
    if (rc != 0)
    {
        ERROR("Failed to add ext_rcf_listener to TA config: %r", rc);
        goto fail_add_ext_rcf_listener;
    }

    base_shell = te_kvpairs_get(&ta_conf, "shell");
    if (base_shell != NULL)
    {
        shell = TE_STRDUP(base_shell);
        te_kvpairs_del(&ta_conf, "shell");
    }

    rc = te_kvpair_add(&ta_conf, "shell", "ip netns exec %s%s%s",
                       ns_name, shell != NULL ? " ": "",
                       shell != NULL ? shell : "");
    if (rc != 0)
    {
        ERROR("Failed to add shell to TA config: %r", rc);
        goto fail_add_shell;
    }

    /* No point to sync time since TA is already running on the host */
    ta_flags |= RCF_TA_NO_SYNC_TIME;
    /* Safer to disable reboot since another agent is running nearby */
    ta_flags &= ~RCF_TA_REBOOTABLE;

    rc = tapi_cfg_rcf_add_ta(ta_name, ta_type, ta_rcflib, &ta_conf, ta_flags);
    if (rc != 0)
    {
        ERROR("Failed to add TA '%s' of type '%s' using '%s': %r",
              ta_name, ta_type, ta_rcflib, rc);
        goto fail_add_ta;
    }

    rc = configure_ta_by_ta(base_ta, ta_name);
    if (rc != 0)
    {
        ERROR("Failed to configure TA '%s': %r", ta_name, rc);
        (void)tapi_cfg_rcf_del_ta(ta_name);
    }

    /* fallthrough to free resources */

fail_add_ta:
fail_add_shell:
    free(shell);

fail_add_ext_rcf_listener:
fail_add_port:
fail_alloc_port:
fail_del_port:
    te_kvpair_fini(&ta_conf);

fail_ta_conf:
    free(ta_confstr);
    free(ta_rcflib);
    free(ta_type);
    return rc;
}
