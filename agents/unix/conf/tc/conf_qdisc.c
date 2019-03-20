/** @file
 * @brief Unix TA Traffic Control qdisc configuration support
 *
 * Implementation of get/set methods for qdisc node
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "te_defs.h"
#include "rcf_common.h"

#include "conf_tc_internal.h"

#include <netlink/route/qdisc.h>

#define ROOT            "root"
#define DEFAULT_HANDLE  "1:0"

/* See the description in conf_qdisc.h */
te_errno
conf_qdisc_enabled_set(unsigned int gid, const char *oid,
                       const char *value, const char *if_name)
{
    UNUSED(gid);
    UNUSED(oid);

    if (strcmp(value, "1") == 0)
        return conf_tc_internal_qdisc_enable(if_name);
    else if (strcmp(value, "0") == 0)
        return conf_tc_internal_qdisc_disable(if_name);

    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/* See the description in conf_qdisc.h */
te_errno
conf_qdisc_enabled_get(unsigned int gid, const char *oid,
                       char *value, const char *if_name)
{
    uint32_t handle;
    struct rtnl_qdisc *qdisc = conf_tc_internal_get_qdisc(if_name);

    UNUSED(gid);
    UNUSED(oid);

    if (qdisc == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    handle = rtnl_tc_get_handle(TC_CAST(qdisc));
    strcpy(value, handle == 0 ? "0" : "1");

    return 0;
}

/* See the description in conf_qdisc.h */
te_errno
conf_qdisc_parent_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    /* TODO: add support parent */
    strcpy(value, ROOT);
    return 0;
}

/* See the description in conf_qdisc.h */
te_errno
conf_qdisc_handle_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    /* TODO: add support handle */
    strcpy(value, DEFAULT_HANDLE);
    return 0;
}

/* See the description in conf_qdisc.h */
te_errno
conf_qdics_kind_get(unsigned int gid, const char *oid, char *value,
                    const char *if_name)
{
    const char *kind;
    struct rtnl_qdisc *qdisc = conf_tc_internal_get_qdisc(if_name);

    UNUSED(gid);
    UNUSED(oid);

    if (qdisc == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    kind = rtnl_tc_get_kind(TC_CAST(qdisc));
    snprintf(value, RCF_MAX_VAL, "%s", kind != NULL ? kind: "");

    return 0;
}

/* See the description in conf_qdisc.h */
te_errno
conf_qdics_kind_set(unsigned int gid, const char *oid,
                    const char *value, const char *if_name)
{
    int rc;
    struct rtnl_qdisc *qdisc = conf_tc_internal_get_qdisc(if_name);

    UNUSED(gid);
    UNUSED(oid);

    if (qdisc == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = rtnl_tc_set_kind(TC_CAST(qdisc), value);
    if (rc == -NLE_EXIST)
        rc = 0;

    return conf_tc_internal_nl_error2te_errno(rc);
}
