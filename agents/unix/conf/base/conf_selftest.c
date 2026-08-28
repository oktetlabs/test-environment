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
#include "te_queue.h"
#include "te_alloc.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch_tree.h"
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
                    bool first)
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
    return commit_obj_prop_get(gid, oid, value, true);
}

static te_errno
commit_obj_prop_b_get(unsigned int gid, const char *oid, char *value)
{
    return commit_obj_prop_get(gid, oid, value, false);
}

static te_errno
commit_obj_prop_set(unsigned int gid, const char *oid, const char *value,
                    bool first)
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
    return commit_obj_prop_set(gid, oid, value, true);
}

static te_errno
commit_obj_prop_b_set(unsigned int gid, const char *oid, const char *value)
{
    return commit_obj_prop_set(gid, oid, value, false);
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
                  bool first)
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
    return incr_obj_prop_get(gid, oid, value, true);
}

static te_errno
incr_obj_prop_b_get(unsigned int gid, const char *oid, char *value)
{
    return incr_obj_prop_get(gid, oid, value, false);
}

static te_errno
incr_obj_prop_set(unsigned int gid, const char *oid, const char *value,
                  bool first)
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
    return incr_obj_prop_set(gid, oid, value, true);
}

static te_errno
incr_obj_prop_b_set(unsigned int gid, const char *oid, const char *value)
{
    return incr_obj_prop_set(gid, oid, value, false);
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

/*
 * Demo subtree for the declarative ta_conf tree framework
 * (lib/rcfpch/rcf_pch_tree.h). See description of the objects in
 * doc/cm/cm_selftest.yml.
 */

static char *demo_str;               /* TE_STRDUP'd, starts NULL = "" */
static int32_t demo_int;
static uint32_t demo_uint;
static int32_t demo_i32;
static uint16_t demo_u16;
static uint8_t demo_u8;
static uint8_t demo_narrowing_u8;
static int8_t demo_narrowing_i8;
static bool demo_bool;
static int demo_color = 1;

typedef struct demo_item {
    SLIST_ENTRY(demo_item) links;
    char *name;
    char *value;
} demo_item;
static SLIST_HEAD(, demo_item) demo_items =
    SLIST_HEAD_INITIALIZER(demo_items);

static uint32_t demo_grp_a, demo_grp_b;          /* committed */
static uint32_t demo_grp_a_new, demo_grp_b_new;  /* staged */
static uint32_t demo_commits;

static te_errno
demo_ro_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    te_string_append(val, "ro");
    return 0;
}

static te_errno
demo_str_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    te_string_append(val, "%s", demo_str == NULL ? "" : demo_str);
    return 0;
}

static te_errno
demo_str_set(const ta_conf_ctx *ctx, const char *val)
{
    UNUSED(ctx);
    free(demo_str);
    demo_str = TE_STRDUP(val);
    return 0;
}

static te_errno
demo_int_get(const ta_conf_ctx *ctx, int32_t *val)
{
    UNUSED(ctx);
    *val = demo_int;
    return 0;
}

static te_errno
demo_int_set(const ta_conf_ctx *ctx, int32_t val)
{
    UNUSED(ctx);
    demo_int = val;
    return 0;
}

static te_errno
demo_uint_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    UNUSED(ctx);
    *val = demo_uint;
    return 0;
}

static te_errno
demo_uint_set(const ta_conf_ctx *ctx, uint32_t val)
{
    UNUSED(ctx);
    demo_uint = val;
    return 0;
}

static te_errno
demo_i32_get(const ta_conf_ctx *ctx, int32_t *val)
{
    UNUSED(ctx);
    *val = demo_i32;
    return 0;
}

static te_errno
demo_i32_set(const ta_conf_ctx *ctx, int32_t val)
{
    UNUSED(ctx);
    demo_i32 = val;
    return 0;
}

static te_errno
demo_u16_get(const ta_conf_ctx *ctx, uint16_t *val)
{
    UNUSED(ctx);
    *val = demo_u16;
    return 0;
}

static te_errno
demo_u16_set(const ta_conf_ctx *ctx, uint16_t val)
{
    UNUSED(ctx);
    demo_u16 = val;
    return 0;
}

static te_errno
demo_u8_get(const ta_conf_ctx *ctx, uint8_t *val)
{
    UNUSED(ctx);
    *val = demo_u8;
    return 0;
}

static te_errno
demo_u8_set(const ta_conf_ctx *ctx, uint8_t val)
{
    UNUSED(ctx);
    demo_u8 = val;
    return 0;
}

/*
 * "narrowing_u8"/"narrowing_i8": nodes deliberately narrower than
 * the objects they implement (uint8 behind uint16, int8 behind
 * int16), so that a value the node cannot hold can actually reach
 * the framework's codec -- an object of the node's own width would
 * be range-checked by the engine first and the agent would never see
 * the value.  They exist to check that the codec refuses it with
 * TE_ERANGE instead of truncating; see doc/cm/cm_selftest.yml.
 */
static te_errno
demo_narrowing_u8_get(const ta_conf_ctx *ctx, uint8_t *val)
{
    UNUSED(ctx);
    *val = demo_narrowing_u8;
    return 0;
}

static te_errno
demo_narrowing_u8_set(const ta_conf_ctx *ctx, uint8_t val)
{
    UNUSED(ctx);
    demo_narrowing_u8 = val;
    return 0;
}

static te_errno
demo_narrowing_i8_get(const ta_conf_ctx *ctx, int8_t *val)
{
    UNUSED(ctx);
    *val = demo_narrowing_i8;
    return 0;
}

static te_errno
demo_narrowing_i8_set(const ta_conf_ctx *ctx, int8_t val)
{
    UNUSED(ctx);
    demo_narrowing_i8 = val;
    return 0;
}

static te_errno
demo_bool_get(const ta_conf_ctx *ctx, bool *val)
{
    UNUSED(ctx);
    *val = demo_bool;
    return 0;
}

static te_errno
demo_bool_set(const ta_conf_ctx *ctx, bool val)
{
    UNUSED(ctx);
    demo_bool = val;
    return 0;
}

static te_errno
demo_color_get(const ta_conf_ctx *ctx, int *val)
{
    UNUSED(ctx);
    *val = demo_color;
    return 0;
}

static te_errno
demo_color_set(const ta_conf_ctx *ctx, int val)
{
    UNUSED(ctx);
    demo_color = val;
    return 0;
}

static demo_item *
demo_item_find(const char *name)
{
    demo_item *p;

    SLIST_FOREACH(p, &demo_items, links)
    {
        if (strcmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

static te_errno
demo_item_add(const ta_conf_ctx *ctx, const char *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "item");
    demo_item *p;

    if (demo_item_find(name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = TE_ALLOC(sizeof(*p));
    p->name = TE_STRDUP(name);
    p->value = TE_STRDUP(val);
    SLIST_INSERT_HEAD(&demo_items, p, links);
    return 0;
}

static te_errno
demo_item_del(const ta_conf_ctx *ctx)
{
    const char *name = ta_conf_ctx_inst(ctx, "item");
    demo_item *p = demo_item_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&demo_items, p, demo_item, links);
    free(p->name);
    free(p->value);
    free(p);
    return 0;
}

static te_errno
demo_item_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "item");
    demo_item *p = demo_item_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", p->value);
    return 0;
}

static te_errno
demo_item_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "item");
    demo_item *p = demo_item_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free(p->value);
    p->value = TE_STRDUP(val);
    return 0;
}

static te_errno
demo_item_list(const ta_conf_ctx *ctx, te_vec *names)
{
    demo_item *p;

    UNUSED(ctx);
    SLIST_FOREACH(p, &demo_items, links)
    {
        char *name = TE_STRDUP(p->name);

        TE_VEC_APPEND(names, name);
    }
    return 0;
}

static te_errno
demo_item_depth_get(const ta_conf_ctx *ctx, te_string *val)
{
    te_string_append(val, "%s", ta_conf_ctx_inst(ctx, "item"));
    return 0;
}

/*
 * "fixed": TA_CONF_LIST -- three statically named instances, no
 * value of their own, with a RO_UINT32 child echoing the instance's
 * position in the fixed set (instance lookup via ta_conf_ctx_inst()
 * from a child, same as "item/depth" above).
 */
static const char *const demo_fixed_names[] = { "one", "two", "three" };
#define DEMO_FIXED_N \
    (sizeof(demo_fixed_names) / sizeof(demo_fixed_names[0]))

static te_errno
demo_fixed_list(const ta_conf_ctx *ctx, te_vec *names)
{
    size_t i;

    UNUSED(ctx);
    for (i = 0; i < DEMO_FIXED_N; i++)
    {
        char *name = TE_STRDUP(demo_fixed_names[i]);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
demo_fixed_index_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "fixed");
    size_t i;

    for (i = 0; i < DEMO_FIXED_N; i++)
    {
        if (strcmp(demo_fixed_names[i], name) == 0)
        {
            *val = i;
            return 0;
        }
    }
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/*
 * "cell": TA_CONF_RO_COLL_INT32 -- fixed instances with signed values,
 * proving the signed get/list path of a read-only collection.
 */
typedef struct demo_cell {
    const char *name;
    int32_t value;
} demo_cell;

static const demo_cell demo_cells[] = {
    { "neg",  -5 },
    { "zero",  0 },
    { "pos",   7 },
};
#define DEMO_CELLS_N (sizeof(demo_cells) / sizeof(demo_cells[0]))

static te_errno
demo_cell_list(const ta_conf_ctx *ctx, te_vec *names)
{
    size_t i;

    UNUSED(ctx);
    for (i = 0; i < DEMO_CELLS_N; i++)
    {
        char *name = TE_STRDUP(demo_cells[i].name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
demo_cell_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "cell");
    size_t i;

    for (i = 0; i < DEMO_CELLS_N; i++)
    {
        if (strcmp(demo_cells[i].name, name) == 0)
        {
            *val = demo_cells[i].value;
            return 0;
        }
    }
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/*
 * "dial": TA_CONF_RW_COLL_UINT32 -- a fixed 2-entry table of settable
 * values; instances are enumerable but not created/destroyed through
 * this node (no add/del).
 */
static uint32_t demo_dial_x, demo_dial_y;

static uint32_t *
demo_dial_find(const char *name)
{
    if (strcmp(name, "x") == 0)
        return &demo_dial_x;
    if (strcmp(name, "y") == 0)
        return &demo_dial_y;
    return NULL;
}

static te_errno
demo_dial_list(const ta_conf_ctx *ctx, te_vec *names)
{
    char *x_name = TE_STRDUP("x");
    char *y_name = TE_STRDUP("y");

    UNUSED(ctx);
    TE_VEC_APPEND(names, x_name);
    TE_VEC_APPEND(names, y_name);
    return 0;
}

static te_errno
demo_dial_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "dial");
    uint32_t *p = demo_dial_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = *p;
    return 0;
}

static te_errno
demo_dial_set(const ta_conf_ctx *ctx, uint32_t val)
{
    const char *name = ta_conf_ctx_inst(ctx, "dial");
    uint32_t *p = demo_dial_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *p = val;
    return 0;
}

/*
 * "citem": TA_CONF_COLL_STR_RW_COMMIT -- like "item", but set()
 * stages the new value per instance instead of applying it; get()
 * always returns the last APPLIED value.  The subtree commit handler
 * applies every instance's staged value (if any) and bumps a shared
 * counter, read back through the sibling leaf "citem_commits" (a
 * collection has no value of its own to hang a counter off, so the
 * counter lives next to it, same idea as "grp/commits" one level
 * up).  The counter only counts commits that actually applied a
 * staged value: rcf_pch_conf.c fires the commit handler after every
 * add/set/del too (see key_commit() in conf_key.c), not only after
 * an explicit cfg_commit(); an add has nothing staged yet, so that
 * extra call must be a no-op for the counter to mean anything.
 */
typedef struct demo_citem {
    SLIST_ENTRY(demo_citem) links;
    char *name;
    char *value;    /* applied */
    char *staged;   /* pending, NULL if nothing staged */
} demo_citem;
static SLIST_HEAD(, demo_citem) demo_citems =
    SLIST_HEAD_INITIALIZER(demo_citems);
static uint32_t demo_citem_commits;

static demo_citem *
demo_citem_find(const char *name)
{
    demo_citem *p;

    SLIST_FOREACH(p, &demo_citems, links)
    {
        if (strcmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

static te_errno
demo_citem_add(const ta_conf_ctx *ctx, const char *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "citem");
    demo_citem *p;

    if (demo_citem_find(name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = TE_ALLOC(sizeof(*p));
    p->name = TE_STRDUP(name);
    p->value = TE_STRDUP(val);
    SLIST_INSERT_HEAD(&demo_citems, p, links);
    return 0;
}

static te_errno
demo_citem_del(const ta_conf_ctx *ctx)
{
    const char *name = ta_conf_ctx_inst(ctx, "citem");
    demo_citem *p = demo_citem_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&demo_citems, p, demo_citem, links);
    free(p->name);
    free(p->value);
    free(p->staged);
    free(p);
    return 0;
}

static te_errno
demo_citem_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "citem");
    demo_citem *p = demo_citem_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", p->value);
    return 0;
}

static te_errno
demo_citem_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "citem");
    demo_citem *p = demo_citem_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free(p->staged);
    p->staged = TE_STRDUP(val);
    return 0;
}

static te_errno
demo_citem_list(const ta_conf_ctx *ctx, te_vec *names)
{
    demo_citem *p;

    UNUSED(ctx);
    SLIST_FOREACH(p, &demo_citems, links)
    {
        char *name = TE_STRDUP(p->name);

        TE_VEC_APPEND(names, name);
    }
    return 0;
}

static te_errno
demo_citem_commits_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    UNUSED(ctx);
    *val = demo_citem_commits;
    return 0;
}

static te_errno
demo_citem_commit(const ta_conf_ctx *ctx)
{
    demo_citem *p;
    bool applied = false;

    UNUSED(ctx);
    SLIST_FOREACH(p, &demo_citems, links)
    {
        if (p->staged != NULL)
        {
            free(p->value);
            p->value = p->staged;
            p->staged = NULL;
            applied = true;
        }
    }
    /*
     * rcf_pch_conf.c auto-commits after every successful add/set/del
     * on a node with a commit handler (immediately for a standalone
     * RPC, or once per group otherwise -- see key_commit()'s comment
     * in conf_key.c for the same rule applied to another
     * TA_CONF_COLL_STR_RW_COMMIT node), so this fires once right
     * after add() too, with nothing staged yet. Only count commits
     * that actually applied something, so the counter reflects real
     * value changes, not every dispatch that happens to pass through
     * a commit-bearing node.
     */
    if (applied)
        demo_citem_commits++;
    return 0;
}

/*
 * "batch": generic TA_CONF_NODE() -- unvalued collection (add/del/
 * list, no get/set of its own), same shape as rx_rules's "rule"
 * node.  Unlike "citem" there is nothing to stage per instance, so
 * every commit dispatch is real work and the handler counts them
 * all unconditionally; a caller that wants a delta attributable to
 * its own explicit cfg_commit() (rather than the add/del auto-commit
 * described for "citem" above) must stage the add locally first, the
 * same way rx_rules stages a rule before committing it to hardware.
 * The counter is exposed the same way as "citem_commits", through
 * the sibling leaf "batch_commits".
 */
typedef struct demo_batch {
    SLIST_ENTRY(demo_batch) links;
    char *name;
} demo_batch;
static SLIST_HEAD(, demo_batch) demo_batches =
    SLIST_HEAD_INITIALIZER(demo_batches);
static uint32_t demo_batch_commits;

static demo_batch *
demo_batch_find(const char *name)
{
    demo_batch *p;

    SLIST_FOREACH(p, &demo_batches, links)
    {
        if (strcmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

static te_errno
demo_batch_add(const ta_conf_ctx *ctx)
{
    const char *name = ta_conf_ctx_inst(ctx, "batch");
    demo_batch *p;

    if (demo_batch_find(name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = TE_ALLOC(sizeof(*p));
    p->name = TE_STRDUP(name);
    SLIST_INSERT_HEAD(&demo_batches, p, links);
    return 0;
}

static te_errno
demo_batch_del(const ta_conf_ctx *ctx)
{
    const char *name = ta_conf_ctx_inst(ctx, "batch");
    demo_batch *p = demo_batch_find(name);

    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&demo_batches, p, demo_batch, links);
    free(p->name);
    free(p);
    return 0;
}

static te_errno
demo_batch_list(const ta_conf_ctx *ctx, te_vec *names)
{
    demo_batch *p;

    UNUSED(ctx);
    SLIST_FOREACH(p, &demo_batches, links)
    {
        char *name = TE_STRDUP(p->name);

        TE_VEC_APPEND(names, name);
    }
    return 0;
}

static te_errno
demo_batch_commits_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    UNUSED(ctx);
    *val = demo_batch_commits;
    return 0;
}

static te_errno
demo_batch_commit(const ta_conf_ctx *ctx)
{
    UNUSED(ctx);
    demo_batch_commits++;
    return 0;
}

static te_errno
demo_grp_a_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    UNUSED(ctx);
    *val = demo_grp_a;
    return 0;
}

static te_errno
demo_grp_a_set(const ta_conf_ctx *ctx, uint32_t val)
{
    UNUSED(ctx);
    demo_grp_a_new = val;
    return 0;
}

static te_errno
demo_grp_b_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    UNUSED(ctx);
    *val = demo_grp_b;
    return 0;
}

static te_errno
demo_grp_b_set(const ta_conf_ctx *ctx, uint32_t val)
{
    UNUSED(ctx);
    demo_grp_b_new = val;
    return 0;
}

static te_errno
demo_commits_get(const ta_conf_ctx *ctx, uint32_t *val)
{
    UNUSED(ctx);
    *val = demo_commits;
    return 0;
}

static te_errno
demo_grp_commit(const ta_conf_ctx *ctx)
{
    UNUSED(ctx);
    demo_grp_a = demo_grp_a_new;
    demo_grp_b = demo_grp_b_new;
    demo_commits++;
    return 0;
}

static const te_enum_map demo_colors[] = {
    { .name = "red",   .value = 1 },
    { .name = "green", .value = 2 },
    { .name = "blue",  .value = 3 },
    TE_ENUM_MAP_END
};

static const ta_conf_node *const demo_tree =
    TA_CONF_NA("ta_conf_demo",
        TA_CONF_RO_STR("ro_val", demo_ro_get),
        TA_CONF_RW_STR("str_val", demo_str_get, demo_str_set),
        TA_CONF_RW_INT32("int_val", demo_int_get, demo_int_set),
        TA_CONF_RW_UINT32("uint_val", demo_uint_get, demo_uint_set),
        TA_CONF_RW_INT32("i32_val", demo_i32_get, demo_i32_set),
        TA_CONF_RW_UINT16("u16_val", demo_u16_get, demo_u16_set),
        TA_CONF_RW_UINT8("u8_val", demo_u8_get, demo_u8_set),
        TA_CONF_RW_UINT8("narrowing_u8", demo_narrowing_u8_get,
                         demo_narrowing_u8_set),
        TA_CONF_RW_INT8("narrowing_i8", demo_narrowing_i8_get,
                        demo_narrowing_i8_set),
        TA_CONF_RW_BOOL("bool_val", demo_bool_get, demo_bool_set),
        TA_CONF_RW_ENUM("color", demo_colors,
                        demo_color_get, demo_color_set),
        TA_CONF_COLL_STR_RW("item", demo_item_get, demo_item_set,
                            demo_item_add, demo_item_del, demo_item_list,
            TA_CONF_RO_STR("depth", demo_item_depth_get)),
        TA_CONF_LIST("fixed", demo_fixed_list,
            TA_CONF_RO_UINT32("index", demo_fixed_index_get)),
        TA_CONF_RO_COLL_INT32("cell", demo_cell_get, demo_cell_list),
        TA_CONF_RW_COLL_UINT32("dial", demo_dial_get, demo_dial_set,
                               demo_dial_list),
        TA_CONF_COLL_STR_RW_COMMIT("citem", demo_citem_get, demo_citem_set,
                                   demo_citem_add, demo_citem_del,
                                   demo_citem_list, demo_citem_commit),
        TA_CONF_RO_UINT32("citem_commits", demo_citem_commits_get),
        TA_CONF_NODE((.name = "batch",
                      .add = { .as_none = demo_batch_add },
                      .del = demo_batch_del,
                      .list = demo_batch_list,
                      .commit = demo_batch_commit)),
        TA_CONF_RO_UINT32("batch_commits", demo_batch_commits_get),
        TA_CONF_NA_COMMIT("grp", demo_grp_commit,
            TA_CONF_RW_UINT32("a", demo_grp_a_get, demo_grp_a_set),
            TA_CONF_RW_UINT32("b", demo_grp_b_get, demo_grp_b_set),
            TA_CONF_RO_UINT32("commits", demo_commits_get)));

extern te_errno
ta_unix_conf_selftest_init(void)
{
    te_errno rc = rcf_pch_add_node("/agent", &node_selftest);

    if (rc != 0)
        return rc;
    return ta_conf_register("/agent", demo_tree);
}
