/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unix Test Agent
 *
 * Configuration objects used when testing Configurator itself.
 */

#define TE_LGR_USER     "Unix Conf Selftest"

#include "te_config.h"
#include "config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "te_string.h"

/** Data for object with two properties */
typedef struct two_props_data {
    /** The first property */
    unsigned int a;
    /** The second property */
    unsigned int b;
} two_props_data;

/* See description of mentioned objects in doc/cm/cm_selftest.yml */

/* Current state of commit_obj instance */
static two_props_data commit_obj_state = {0, 0};
/* New state of commit_obj instance (to be committed) */
static two_props_data commit_obj_new_state = {0, 0};
/*
 * GID used when the current commit was initiated by
 * the first set operation for commit_obj.
 */
uint64_t last_commit_gid = UINT64_MAX;

/* State of commit_obj_dep instance */
static unsigned int commit_obj_dep_state = 0;

/* State of incr_obj instance */
static two_props_data incr_obj_state = {0, 0};

static te_errno
commit_obj_prop_get(unsigned int gid, const char *oid, char *value,
                    te_bool first)
{
    te_string value_str = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    UNUSED(gid);
    UNUSED(oid);

    te_string_append(&value_str, "%u",
                     first ? commit_obj_state.a : commit_obj_state.b);

    return 0;
}

static te_errno
commit_obj_prop_a_get(unsigned int gid, const char *oid, char *value)
{
    return commit_obj_prop_get(gid, oid, value, TRUE);
}

static te_errno
commit_obj_prop_b_get(unsigned int gid, const char *oid, char *value)
{
    return commit_obj_prop_get(gid, oid, value, FALSE);
}

static te_errno
commit_obj_prop_set(unsigned int gid, const char *oid, const char *value,
                    te_bool first)
{
    unsigned int value_uint;
    te_errno rc;

    UNUSED(oid);

    if (gid != last_commit_gid)
    {
        memcpy(&commit_obj_new_state, &commit_obj_state,
               sizeof(commit_obj_state));
        last_commit_gid = gid;
    }

    rc = te_strtoui(value, 10, &value_uint);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TA_UNIX, rc);

    if (first)
        commit_obj_new_state.a = value_uint;
    else
        commit_obj_new_state.b = value_uint;

    return 0;
}

static te_errno
commit_obj_prop_a_set(unsigned int gid, const char *oid, const char *value)
{
    return commit_obj_prop_set(gid, oid, value, TRUE);
}

static te_errno
commit_obj_prop_b_set(unsigned int gid, const char *oid, const char *value)
{
    return commit_obj_prop_set(gid, oid, value, FALSE);
}

static te_errno
commit_obj_commit(unsigned int gid, const cfg_oid *p_oid)
{
    UNUSED(p_oid);

    if (commit_obj_new_state.a != commit_obj_new_state.b)
    {
        ERROR("%s(): a != b", __FUNCTION__);
        memcpy(&commit_obj_new_state, &commit_obj_state,
               sizeof(commit_obj_state));
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (last_commit_gid == gid)
    {
        memcpy(&commit_obj_state, &commit_obj_new_state,
               sizeof(commit_obj_state));
    }
    return 0;
}

static te_errno
incr_obj_prop_get(unsigned int gid, const char *oid, char *value,
                  te_bool first)
{
    te_string value_str = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    UNUSED(gid);
    UNUSED(oid);

    te_string_append(&value_str, "%u",
                     first ? incr_obj_state.a : incr_obj_state.b);

    return 0;
}

static te_errno
incr_obj_prop_a_get(unsigned int gid, const char *oid, char *value)
{
    return incr_obj_prop_get(gid, oid, value, TRUE);
}

static te_errno
incr_obj_prop_b_get(unsigned int gid, const char *oid, char *value)
{
    return incr_obj_prop_get(gid, oid, value, FALSE);
}

static te_errno
incr_obj_prop_set(unsigned int gid, const char *oid, const char *value,
                  te_bool first)
{
    unsigned int value_uint;
    te_errno rc;
    unsigned int *prop;
    unsigned int *other_prop;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strtoui(value, 10, &value_uint);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TA_UNIX, rc);

    if (first)
    {
        prop = &incr_obj_state.a;
        other_prop = &incr_obj_state.b;
    }
    else
    {
        prop = &incr_obj_state.b;
        other_prop = &incr_obj_state.a;
    }

    if ((value_uint > *other_prop && value_uint != *other_prop + 1) ||
        (value_uint < *other_prop && value_uint != *other_prop - 1))
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    *prop = value_uint;

    return 0;
}

static te_errno
incr_obj_prop_a_set(unsigned int gid, const char *oid, const char *value)
{
    return incr_obj_prop_set(gid, oid, value, TRUE);
}

static te_errno
incr_obj_prop_b_set(unsigned int gid, const char *oid, const char *value)
{
    return incr_obj_prop_set(gid, oid, value, FALSE);
}

static te_errno
commit_obj_dep_get(unsigned int gid, const char *oid, char *value)
{
    te_string value_str = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);

    UNUSED(gid);
    UNUSED(oid);

    if (commit_obj_state.a == 0 || commit_obj_state.b == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(&value_str, "%u", commit_obj_dep_state);
    return 0;
}

static te_errno
commit_obj_dep_set(unsigned int gid, const char *oid, const char *value)
{
    unsigned int value_uint;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if (commit_obj_state.a == 0 || commit_obj_state.b == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(value, 10, &value_uint);
    if (rc != 0)
    {
        return TE_RC_UPSTREAM(TE_TA_UNIX, rc);
    }

    commit_obj_dep_state = value_uint;
    return 0;
}

static rcf_pch_cfg_object node_incr_obj_a = {
    .sub_id = "a",
    .get = (rcf_ch_cfg_get)incr_obj_prop_a_get,
    .set = (rcf_ch_cfg_set)incr_obj_prop_a_set,
};

static rcf_pch_cfg_object node_incr_obj_b = {
    .sub_id = "b",
    .brother = &node_incr_obj_a,
    .get = (rcf_ch_cfg_get)incr_obj_prop_b_get,
    .set = (rcf_ch_cfg_set)incr_obj_prop_b_set,
};

static rcf_pch_cfg_object node_incr_obj = {
    .sub_id = "incr_obj",
    .son = &node_incr_obj_b,
};

static rcf_pch_cfg_object node_commit_obj_dep = {
    .sub_id = "commit_obj_dep",
    .brother = &node_incr_obj,
    .get = (rcf_ch_cfg_get)commit_obj_dep_get,
    .set = (rcf_ch_cfg_set)commit_obj_dep_set,
};

static rcf_pch_cfg_object node_commit_obj;

static rcf_pch_cfg_object node_commit_obj_a = {
    .sub_id = "a",
    .get = (rcf_ch_cfg_get)commit_obj_prop_a_get,
    .set = (rcf_ch_cfg_set)commit_obj_prop_a_set,
    .commit_parent = &node_commit_obj,
};

static rcf_pch_cfg_object node_commit_obj_b = {
    .sub_id = "b",
    .brother = &node_commit_obj_a,
    .get = (rcf_ch_cfg_get)commit_obj_prop_b_get,
    .set = (rcf_ch_cfg_set)commit_obj_prop_b_set,
    .commit_parent = &node_commit_obj,
};

static rcf_pch_cfg_object node_commit_obj = {
    .sub_id = "commit_obj",
    .son = &node_commit_obj_b,
    .brother = &node_commit_obj_dep,
    .commit = (rcf_ch_cfg_commit)commit_obj_commit,
};

static rcf_pch_cfg_object node_selftest = {
    .sub_id = "selftest",
    .son = &node_commit_obj,
};

extern te_errno
ta_unix_conf_selftest_init(void)
{
    return rcf_pch_add_node("/agent", &node_selftest);
}
