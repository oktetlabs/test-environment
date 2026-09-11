/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Declarative typed configuration trees: implementation
 */

#define TE_LGR_USER "RCF PCH TREE"

#include "te_config.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#include "te_errno.h"
#include "te_defs.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "te_vector.h"
#include "te_enum.h"
#include "te_queue.h"
#include "te_sockaddr.h"
#include "logger_api.h"
#include "conf_oid.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"

/* Per-request context (opaque in the header) */
struct ta_conf_ctx {
    unsigned int gid;          /* configuration group id */
    const char *oid_str;       /* full instance OID or NULL (commit) */
    const cfg_oid *oid;        /* parsed instance OID */
    const ta_conf_node *node;  /* node the request targets */
};

/* Mirrored PCH node */
typedef struct ta_conf_mirror {
    rcf_pch_cfg_object pch;
    const ta_conf_node *node;
} ta_conf_mirror;

/* One ta_conf_register() call */
typedef struct ta_conf_reg {
    SLIST_ENTRY(ta_conf_reg) links;
    const ta_conf_node *root;
    cfg_oid *father;           /* parsed object OID of the father */
} ta_conf_reg;

static SLIST_HEAD(, ta_conf_reg) regs = SLIST_HEAD_INITIALIZER(regs);

/*
 * All union members are function pointers with identical
 * representation, so testing any member against NULL tells whether
 * the handler is set at all.
 */
static bool
has_get(const ta_conf_node *n)
{
    return n->get.as_str != NULL;
}

static bool
has_set(const ta_conf_node *n)
{
    return n->set.as_str != NULL;
}

static bool
has_add(const ta_conf_node *n)
{
    return n->add.as_str != NULL;
}

/* See description in rcf_pch_tree.h */
const char *
ta_conf_ctx_inst(const ta_conf_ctx *ctx, const char *sub_id)
{
    const cfg_inst_subid *ids = (const cfg_inst_subid *)ctx->oid->ids;
    int i;

    for (i = (int)ctx->oid->len - 1; i >= 2; i--)
    {
        if (strcmp(ids[i].subid, sub_id) == 0)
            return ids[i].name;
    }

    TE_FATAL_ERROR("no level '%s' in OID", sub_id);
    return NULL;
}

/* See description in rcf_pch_tree.h */
unsigned int
ta_conf_ctx_gid(const ta_conf_ctx *ctx)
{
    return ctx->gid;
}

/* See description in rcf_pch_tree.h */
const char *
ta_conf_ctx_oid(const ta_conf_ctx *ctx)
{
    return ctx->oid_str;
}

/*
 * s_ is the accessor suffix (int8), t_ the C type (int8_t): they
 * differ, so both are passed rather than pasted from one another.
 */
#define CODEC_GET_INT(s_, t_, fmt_)                             \
    do {                                                        \
        t_ v;                                                   \
        te_errno rc = n->get.as_##s_(ctx, &v);                  \
                                                                \
        if (rc != 0)                                            \
            return rc;                                          \
        snprintf(value, RCF_MAX_VAL, fmt_, v);                  \
        return 0;                                               \
    } while (0)

/* Print the result of the node's get handler into value[RCF_MAX_VAL] */
static te_errno
codec_get(const ta_conf_ctx *ctx, char *value)
{
    const ta_conf_node *n = ctx->node;

    switch (n->type)
    {
        case CVT_STRING:
        {
            te_string s = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
            const char *mapped;
            int enum_val;
            te_errno rc;

            /*
             * A map is what makes a string-valued node an
             * enumeration: the value travels as a name, so the
             * object is CVT_STRING either way and only the map tells
             * the two readings apart.
             */
            if (n->map == NULL)
                return n->get.as_str(ctx, &s);

            rc = n->get.as_enum(ctx, &enum_val);
            if (rc != 0)
                return rc;

            mapped = te_enum_map_from_any_value(n->map, enum_val, NULL);
            if (mapped == NULL)
            {
                ERROR("node '%s': get handler returned a value not "
                      "present in the map", n->name);
                return TE_RC(TE_RCF_PCH, TE_EINVAL);
            }
            TE_STRLCPY(value, mapped, RCF_MAX_VAL);
            return 0;
        }

        case CVT_INT8:   CODEC_GET_INT(int8, int8_t, "%" PRId8);
        case CVT_UINT8:  CODEC_GET_INT(uint8, uint8_t, "%" PRIu8);
        case CVT_INT16:  CODEC_GET_INT(int16, int16_t, "%" PRId16);
        case CVT_UINT16: CODEC_GET_INT(uint16, uint16_t, "%" PRIu16);
        case CVT_INT32:  CODEC_GET_INT(int32, int32_t, "%" PRId32);
        case CVT_UINT32: CODEC_GET_INT(uint32, uint32_t, "%" PRIu32);
        case CVT_INT64:  CODEC_GET_INT(int64, int64_t, "%" PRId64);
        case CVT_UINT64: CODEC_GET_INT(uint64, uint64_t, "%" PRIu64);

        case CVT_BOOL:
        {
            bool v;
            te_errno rc = n->get.as_bool(ctx, &v);

            if (rc != 0)
                return rc;
            snprintf(value, RCF_MAX_VAL, "%s", v ? "1" : "0");
            return 0;
        }

        case CVT_ADDRESS:
        {
            struct sockaddr_storage v;
            te_string s = TE_STRING_EXT_BUF_INIT(value, RCF_MAX_VAL);
            te_errno rc = n->get.as_addr(ctx, &v);

            if (rc != 0)
                return rc;
            return te_netaddr2te_str(SA(&v), &s);
        }

        case CVT_DOUBLE:
        {
            double v;
            te_errno rc = n->get.as_double(ctx, &v);

            if (rc != 0)
                return rc;
            snprintf(value, RCF_MAX_VAL, "%g", v);
            return 0;
        }

        case CVT_NONE:
        default:
            TE_FATAL_ERROR("node '%s': get called on a valueless node",
                           n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
}

#undef CODEC_GET_INT

/*
 * strtoumax()-based parsing (as used by te_strtoumax()) accepts a
 * leading '-' and silently wraps a negative value into the unsigned
 * range instead of rejecting it.  An unsigned node value must never
 * be parsed that way, so callers check for a leading '-' (after
 * skipping optional whitespace, as strtoumax() itself would) before
 * invoking te_strtoumax().
 */
static te_errno
reject_negative(const char *value)
{
    const char *p = value;

    while (isspace((unsigned char)*p))
        p++;

    if (*p == '-')
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    return 0;
}

/* Node value parsed from its wire representation */
typedef struct codec_value {
    const char *as_str;
    bool        as_bool;
    int8_t      as_int8;
    uint8_t     as_uint8;
    int16_t     as_int16;
    uint16_t    as_uint16;
    int32_t     as_int32;
    uint32_t    as_uint32;
    int64_t     as_int64;
    uint64_t    as_uint64;
    int         as_enum;
    struct sockaddr_storage as_addr;
} codec_value;

/*
 * Both parse into the widest temporary of their signedness and check
 * the value against the target width's bounds before narrowing, so
 * that a value the node cannot hold is refused instead of being
 * silently truncated by the framework.
 */
#define CODEC_PARSE_INT(s_, min_, max_)                         \
    do {                                                        \
        intmax_t raw;                                           \
        te_errno rc = te_strtoimax(value, 10, &raw);            \
                                                                \
        if (rc != 0)                                            \
            return TE_RC(TE_RCF_PCH, rc);                       \
        if (raw < (intmax_t)(min_) || raw > (intmax_t)(max_))   \
            return TE_RC(TE_RCF_PCH, TE_ERANGE);                \
        v->as_##s_ = raw;                                       \
        return 0;                                               \
    } while (0)

#define CODEC_PARSE_UINT(s_, max_)                              \
    do {                                                        \
        uintmax_t raw;                                          \
        te_errno rc = reject_negative(value);                   \
                                                                \
        if (rc != 0)                                            \
            return rc;                                          \
        rc = te_strtoumax(value, 10, &raw);                     \
        if (rc != 0)                                            \
            return TE_RC(TE_RCF_PCH, rc);                       \
        if (raw > (uintmax_t)(max_))                            \
            return TE_RC(TE_RCF_PCH, TE_ERANGE);                \
        v->as_##s_ = raw;                                       \
        return 0;                                               \
    } while (0)

/*
 * Parse value into the member of @p v matching the node's type.
 * Shared by set and add so that both accept exactly the same
 * representation of every type.
 */
static te_errno
codec_parse(const ta_conf_node *n, const char *value, codec_value *v)
{
    switch (n->type)
    {
        case CVT_STRING:
            /* A map means the wire value is a name; see codec_get() */
            if (n->map == NULL)
            {
                v->as_str = value;
                return 0;
            }
            v->as_enum = te_enum_map_from_str(n->map, value, INT_MIN);
            if (v->as_enum == INT_MIN)
                return TE_RC(TE_RCF_PCH, TE_EINVAL);
            return 0;

        case CVT_INT8:   CODEC_PARSE_INT(int8, INT8_MIN, INT8_MAX);
        case CVT_UINT8:  CODEC_PARSE_UINT(uint8, UINT8_MAX);
        case CVT_INT16:  CODEC_PARSE_INT(int16, INT16_MIN, INT16_MAX);
        case CVT_UINT16: CODEC_PARSE_UINT(uint16, UINT16_MAX);
        case CVT_INT32:  CODEC_PARSE_INT(int32, INT32_MIN, INT32_MAX);
        case CVT_UINT32: CODEC_PARSE_UINT(uint32, UINT32_MAX);
        case CVT_INT64:  CODEC_PARSE_INT(int64, INT64_MIN, INT64_MAX);
        case CVT_UINT64: CODEC_PARSE_UINT(uint64, UINT64_MAX);

        case CVT_BOOL:
        {
            uintmax_t raw;
            te_errno rc = te_strtoumax(value, 10, &raw);

            if (rc != 0)
                return TE_RC(TE_RCF_PCH, rc);
            if (raw > 1)
                return TE_RC(TE_RCF_PCH, TE_EINVAL);
            v->as_bool = raw != 0;
            return 0;
        }

        case CVT_ADDRESS:
        {
            te_errno rc = te_netaddr_from_te_str(value, &v->as_addr);

            if (rc != 0)
                return TE_RC(TE_RCF_PCH, rc);
            return 0;
        }

        default:
            TE_FATAL_ERROR("node '%s': value parsed for an unknown type",
                           n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
}

#undef CODEC_PARSE_INT
#undef CODEC_PARSE_UINT

/* Parse value and call the node's set handler */
static te_errno
codec_set(const ta_conf_ctx *ctx, const char *value)
{
    const ta_conf_node *n = ctx->node;
    codec_value v;
    te_errno rc;

    if (n->type == CVT_NONE)
    {
        TE_FATAL_ERROR("node '%s': set called on a valueless node",
                       n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    rc = codec_parse(n, value, &v);
    if (rc != 0)
        return rc;

    switch (n->type)
    {
        case CVT_STRING:
            if (n->map == NULL)
                return n->set.as_str(ctx, v.as_str);
            return n->set.as_enum(ctx, v.as_enum);

        case CVT_INT8:
            return n->set.as_int8(ctx, v.as_int8);

        case CVT_UINT8:
            return n->set.as_uint8(ctx, v.as_uint8);

        case CVT_INT16:
            return n->set.as_int16(ctx, v.as_int16);

        case CVT_UINT16:
            return n->set.as_uint16(ctx, v.as_uint16);

        case CVT_INT32:
            return n->set.as_int32(ctx, v.as_int32);

        case CVT_UINT32:
            return n->set.as_uint32(ctx, v.as_uint32);

        case CVT_INT64:
            return n->set.as_int64(ctx, v.as_int64);

        case CVT_UINT64:
            return n->set.as_uint64(ctx, v.as_uint64);

        case CVT_BOOL:
            return n->set.as_bool(ctx, v.as_bool);

        case CVT_ADDRESS:
            return n->set.as_addr(ctx, SA(&v.as_addr));

        default:
            TE_FATAL_ERROR("node '%s': set called with an unknown type",
                           n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
}

/* Parse value (NULL treated as "") and call the add handler */
static te_errno
codec_add(const ta_conf_ctx *ctx, const char *value)
{
    const ta_conf_node *n = ctx->node;
    codec_value v;
    te_errno rc;

    if (value == NULL)
        value = "";

    if (n->type == CVT_NONE)
        return n->add.as_none(ctx);

    rc = codec_parse(n, value, &v);
    if (rc != 0)
        return rc;

    switch (n->type)
    {
        case CVT_STRING:
            if (n->map == NULL)
                return n->add.as_str(ctx, v.as_str);
            return n->add.as_enum(ctx, v.as_enum);

        case CVT_INT8:
            return n->add.as_int8(ctx, v.as_int8);

        case CVT_UINT8:
            return n->add.as_uint8(ctx, v.as_uint8);

        case CVT_INT16:
            return n->add.as_int16(ctx, v.as_int16);

        case CVT_UINT16:
            return n->add.as_uint16(ctx, v.as_uint16);

        case CVT_INT32:
            return n->add.as_int32(ctx, v.as_int32);

        case CVT_UINT32:
            return n->add.as_uint32(ctx, v.as_uint32);

        case CVT_INT64:
            return n->add.as_int64(ctx, v.as_int64);

        case CVT_UINT64:
            return n->add.as_uint64(ctx, v.as_uint64);

        case CVT_BOOL:
            return n->add.as_bool(ctx, v.as_bool);

        case CVT_ADDRESS:
            return n->add.as_addr(ctx, SA(&v.as_addr));

        default:
            TE_FATAL_ERROR("node '%s': add called with an unknown type",
                           n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
}

/*
 * Check that ids[1 .. father->len) of an instance OID match the
 * object OID of a registration's father.
 */
static bool
father_matches(const cfg_inst_subid *p_ids, unsigned int p_len,
               const cfg_oid *father)
{
    const cfg_object_subid *f_ids = (const cfg_object_subid *)father->ids;
    unsigned int flen = father->len;
    unsigned int i;

    if (p_len < flen)
        return false;

    for (i = 1; i < flen; i++)
    {
        if (strcmp(p_ids[i].subid, f_ids[i].subid) != 0)
            return false;
    }

    return true;
}

/*
 * Descend from node @p n by consecutive levels p_ids[from .. p_len),
 * matching each subid against the current node's children.  Returns
 * the node reached, or NULL if any level does not match.
 */
static const ta_conf_node *
descend(const ta_conf_node *n, const cfg_inst_subid *p_ids,
        unsigned int from, unsigned int p_len)
{
    unsigned int i;

    for (i = from; i < p_len; i++)
    {
        const ta_conf_node *const *c;
        const ta_conf_node *next = NULL;

        if (n->children != NULL)
        {
            for (c = n->children + 1; *c != NULL; c++)
            {
                if (strcmp((*c)->name, p_ids[i].subid) == 0)
                {
                    next = *c;
                    break;
                }
            }
        }

        if (next == NULL)
            return NULL;
        n = next;
    }

    return n;
}

/*
 * Find the ta_conf node addressed by instance OID p_oid.
 * Returns NULL when p_oid does not fall inside any registered tree.
 */
static const ta_conf_node *
lookup(const cfg_oid *p_oid)
{
    ta_conf_reg *reg;
    const cfg_inst_subid *p_ids = (const cfg_inst_subid *)p_oid->ids;

    SLIST_FOREACH(reg, &regs, links)
    {
        unsigned int flen = reg->father->len;
        const ta_conf_node *n;

        if (!father_matches(p_ids, p_oid->len, reg->father))
            continue;
        if (p_oid->len <= flen)
            continue;
        if (strcmp(p_ids[flen].subid, reg->root->name) != 0)
            continue;

        n = descend(reg->root, p_ids, flen + 1, p_oid->len);
        if (n != NULL)
            return n;
    }

    return NULL;
}

/* Find the collection node: parent OID string + child sub_id */
static const ta_conf_node *
lookup_child(const cfg_oid *parent_oid, const char *sub_id)
{
    ta_conf_reg *reg;
    const cfg_inst_subid *p_ids;

    if (parent_oid == NULL)
        return NULL;

    p_ids = (const cfg_inst_subid *)parent_oid->ids;

    SLIST_FOREACH(reg, &regs, links)
    {
        unsigned int flen = reg->father->len;
        const ta_conf_node *parent;
        const ta_conf_node *const *c;

        if (!father_matches(p_ids, parent_oid->len, reg->father))
            continue;

        if (parent_oid->len == flen)
        {
            /* parent_oid addresses the father itself */
            if (strcmp(reg->root->name, sub_id) == 0)
                return reg->root;
            continue;
        }

        if (strcmp(p_ids[flen].subid, reg->root->name) != 0)
            continue;

        parent = descend(reg->root, p_ids, flen + 1, parent_oid->len);
        if (parent == NULL || parent->children == NULL)
            continue;

        for (c = parent->children + 1; *c != NULL; c++)
        {
            if (strcmp((*c)->name, sub_id) == 0)
                return *c;
        }
    }

    return NULL;
}

static te_errno
tramp_get(unsigned int gid, const char *oid, char *value, ...)
{
    cfg_oid *p_oid = cfg_convert_oid_str(oid);
    const ta_conf_node *n;
    ta_conf_ctx ctx;
    te_errno rc;

    if (p_oid == NULL)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    n = lookup(p_oid);
    if (n == NULL)
    {
        cfg_free_oid(p_oid);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    ctx = (ta_conf_ctx){ .gid = gid, .oid_str = oid,
                         .oid = p_oid, .node = n };
    rc = codec_get(&ctx, value);
    cfg_free_oid(p_oid);

    return rc;
}

static te_errno
tramp_set(unsigned int gid, const char *oid, const char *value, ...)
{
    cfg_oid *p_oid = cfg_convert_oid_str(oid);
    const ta_conf_node *n;
    ta_conf_ctx ctx;
    te_errno rc;

    if (p_oid == NULL)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    n = lookup(p_oid);
    if (n == NULL)
    {
        cfg_free_oid(p_oid);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    ctx = (ta_conf_ctx){ .gid = gid, .oid_str = oid,
                         .oid = p_oid, .node = n };
    rc = codec_set(&ctx, value);
    cfg_free_oid(p_oid);

    return rc;
}

static te_errno
tramp_add(unsigned int gid, const char *oid, const char *value, ...)
{
    cfg_oid *p_oid = cfg_convert_oid_str(oid);
    const ta_conf_node *n;
    ta_conf_ctx ctx;
    te_errno rc;

    if (p_oid == NULL)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    n = lookup(p_oid);
    if (n == NULL)
    {
        cfg_free_oid(p_oid);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    ctx = (ta_conf_ctx){ .gid = gid, .oid_str = oid,
                         .oid = p_oid, .node = n };
    rc = codec_add(&ctx, value);
    cfg_free_oid(p_oid);

    return rc;
}

static te_errno
tramp_del(unsigned int gid, const char *oid, ...)
{
    cfg_oid *p_oid = cfg_convert_oid_str(oid);
    const ta_conf_node *n;
    ta_conf_ctx ctx;
    te_errno rc;

    if (p_oid == NULL)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    n = lookup(p_oid);
    if (n == NULL)
    {
        cfg_free_oid(p_oid);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    ctx = (ta_conf_ctx){ .gid = gid, .oid_str = oid,
                         .oid = p_oid, .node = n };
    rc = n->del(&ctx);
    cfg_free_oid(p_oid);

    return rc;
}

static te_errno
tramp_list(unsigned int gid, const char *oid, const char *sub_id,
           char **list, ...)
{
    cfg_oid *p_oid = (oid != NULL) ? cfg_convert_oid_str(oid) : NULL;
    const ta_conf_node *n;
    ta_conf_ctx ctx;
    te_errno rc;
    te_vec names = TE_VEC_INIT_AUTOPTR(char *);
    te_string joined = TE_STRING_INIT;
    char **item;

    if (oid != NULL && p_oid == NULL)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    n = lookup_child(p_oid, sub_id);
    if (n == NULL || n->list == NULL)
    {
        cfg_free_oid(p_oid);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    ctx = (ta_conf_ctx){ .gid = gid, .oid_str = oid,
                         .oid = p_oid, .node = n };

    rc = n->list(&ctx, &names);
    if (rc == 0)
    {
        /*
         * Legacy PCH list encoding: every name is followed by a
         * single space, including an empty name (so a single
         * instance named "" becomes " ", not ""). This is what
         * every legacy list handler produced and what the engine
         * tokenizer (rcf_pch_conf.c) consumes; it is the only
         * encoding of this pair that can represent an empty
         * instance name.
         */
        TE_VEC_FOREACH(&names, item)
            te_string_append(&joined, "%s ", *item);

        *list = joined.ptr != NULL ? joined.ptr : TE_STRDUP("");
    }
    else
    {
        te_string_free(&joined);
    }
    te_vec_free(&names);

    cfg_free_oid(p_oid);

    return rc;
}

static te_errno
tramp_commit(unsigned int gid, const cfg_oid *p_oid)
{
    const ta_conf_node *n = lookup(p_oid);
    ta_conf_ctx ctx;

    if (n == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOENT);

    ctx = (ta_conf_ctx){ .gid = gid, .oid_str = NULL,
                         .oid = p_oid, .node = n };

    return n->commit(&ctx);
}

/* Recursively validate a subtree; see rcf_pch_tree.h for the rules. */
static te_errno
validate(const ta_conf_node *n)
{
    const ta_conf_node *const *c;

    if (n->name == NULL || n->name[0] == '\0')
    {
        ERROR("node '%s': name must be non-empty",
              n->name == NULL ? "(null)" : n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }
    if (strlen(n->name) >= RCF_MAX_NAME)
    {
        ERROR("node '%s': name is too long", n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    /* CVT_UNSPECIFIED says a value type is unknown, it is not one */
    if (n->type >= CVT_UNSPECIFIED)
    {
        ERROR("node '%s': %u is not a value type", n->name,
              (unsigned int)n->type);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    if (n->type == CVT_NONE)
    {
        if (has_get(n) || has_set(n))
        {
            ERROR("node '%s': a valueless node cannot have get/set",
                  n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
        }
    }
    else if (!has_get(n))
    {
        ERROR("node '%s': a typed node must have a get handler",
              n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    /*
     * A mapped value travels as a name, so an enumerated node is a
     * string node carrying a map, and the map is the only thing that
     * tells the two apart: the codecs read the accessors as as_enum
     * exactly when it is present.  Hence a map on any other type is
     * an error, and a map that maps nothing is one too -- every
     * value the get handler could return would be missing from it.
     */
    if (n->map != NULL)
    {
        if (n->type != CVT_STRING)
        {
            ERROR("node '%s': map set on a node that is not a string",
                  n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
        }
        if (n->map[0].name == NULL)
        {
            ERROR("node '%s': map without a single name", n->name);
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
        }
    }

    /*
     * Configurator refuses to register an object of type double with
     * any access but read_only, so such a node can only be read.
     */
    if (n->type == CVT_DOUBLE && (has_set(n) || has_add(n)))
    {
        ERROR("node '%s': a DOUBLE node cannot have set or add", n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    if ((has_add(n) || n->del != NULL) && n->list == NULL)
    {
        ERROR("node '%s': add/del without a list handler", n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    if (has_set(n) && !has_get(n))
    {
        ERROR("node '%s': set without get", n->name);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    if (n->children != NULL)
    {
        for (c = n->children + 1; *c != NULL; c++)
        {
            const ta_conf_node *const *c2;
            te_errno rc;

            for (c2 = c + 1; *c2 != NULL; c2++)
            {
                if (strcmp((*c)->name, (*c2)->name) == 0)
                {
                    ERROR("node '%s': duplicate child name '%s'",
                          n->name, (*c)->name);
                    return TE_RC(TE_RCF_PCH, TE_EINVAL);
                }
            }

            rc = validate(*c);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/* Number of nodes in the subtree rooted at n (n included) */
static size_t
count_nodes(const ta_conf_node *n)
{
    size_t total = 1;
    const ta_conf_node *const *c;

    if (n->children != NULL)
    {
        for (c = n->children + 1; *c != NULL; c++)
            total += count_nodes(*c);
    }

    return total;
}

/*
 * Fill in the mirror for node @p n (taken from *cursor, which is
 * advanced past it) and recursively for its children.
 * @p commit_anc is the nearest ancestor mirror whose node has a
 * commit handler (or NULL); it is used to wire up commit_parent the
 * same way the engine's dispatch derives commit_obj (see
 * rcf_pch_conf.c: commit_obj = obj->commit_parent ?: obj).
 */
static ta_conf_mirror *
build(const ta_conf_node *n, ta_conf_mirror **cursor,
      rcf_pch_cfg_object *commit_anc)
{
    ta_conf_mirror *m = (*cursor)++;
    rcf_pch_cfg_object *my_anc = commit_anc;
    ta_conf_mirror *prev_son = NULL;
    const ta_conf_node *const *c;

    m->node = n;
    TE_STRLCPY(m->pch.sub_id, n->name, RCF_MAX_NAME);

    if (has_get(n))
        m->pch.get = tramp_get;
    if (has_set(n))
        m->pch.set = tramp_set;
    if (has_add(n))
        m->pch.add = tramp_add;
    if (n->del != NULL)
        m->pch.del = tramp_del;
    if (n->list != NULL)
        m->pch.list = tramp_list;
    if (n->commit != NULL)
    {
        m->pch.commit = tramp_commit;
        my_anc = &m->pch;
    }

    if ((has_set(n) || has_add(n) || n->del != NULL) &&
        my_anc != NULL && my_anc != &m->pch)
    {
        m->pch.commit_parent = my_anc;
    }

    m->pch.subst = n->subst;

    if (n->children != NULL)
    {
        for (c = n->children + 1; *c != NULL; c++)
        {
            ta_conf_mirror *son = build(*c, cursor, my_anc);

            if (prev_son == NULL)
                m->pch.son = &son->pch;
            else
                prev_son->pch.brother = &son->pch;
            prev_son = son;
        }
    }

    return m;
}

/* See description in rcf_pch_tree.h */
te_errno
ta_conf_register(const char *father, const ta_conf_node *root)
{
    cfg_oid *p;
    ta_conf_reg *reg;
    ta_conf_mirror *arr;
    ta_conf_mirror *cursor;
    ta_conf_mirror *mirror_root;
    size_t count;
    te_errno rc;

    rc = validate(root);
    if (rc != 0)
        return rc;

    p = cfg_convert_oid_str(father);
    if (p == NULL || p->inst)
    {
        ERROR("ta_conf_register(): '%s' is not a valid object OID",
              father);
        cfg_free_oid(p);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    reg = TE_ALLOC(sizeof(*reg));
    reg->root = root;
    reg->father = p;
    /*
     * Insert before grafting the mirror onto the PCH tree: the mirror
     * becomes reachable by trampolines the moment rcf_pch_add_node()
     * returns, and lookup()/lookup_child() must already see this
     * registration by then.
     */
    SLIST_INSERT_HEAD(&regs, reg, links);

    count = count_nodes(root);
    arr = TE_ALLOC(count * sizeof(*arr));
    cursor = arr;
    mirror_root = build(root, &cursor, NULL);

    /*
     * ta_conf_register() must be called during rcf_ch_conf_init():
     * rcf_pch_cfg_subtree_init() fills in each mirrored node's
     * oid_len only after rcf_ch_conf_init() returns, and commit OID
     * truncation (rcf_pch_conf.c, commit()) relies on that oid_len
     * being correct by the time a commit is dispatched.
     */
    return rcf_pch_add_node(father, &mirror_root->pch);
}
