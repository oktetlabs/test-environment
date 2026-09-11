/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Declarative typed configuration trees for Test Agents
 *
 * An alternative to the RCF_PCH_CFG_NODE_* macros: a configuration
 * subtree is a single static const literal written top-down, value
 * conversion between strings and C types is done by the framework,
 * and handlers receive a context object instead of positional
 * varargs.  Registered trees are mirrored onto plain
 * rcf_pch_cfg_object nodes, so they coexist with subtrees declared
 * the old way.
 */

#ifndef __TE_RCF_PCH_TREE_H__
#define __TE_RCF_PCH_TREE_H__

#include <inttypes.h>
#include <sys/socket.h>

#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "te_vector.h"
#include "te_enum.h"
#include "conf_val_type.h"
#include "rcf_ch_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A node says what kind of value it holds with cfg_val_type, the very
 * type set Configurator declares its objects with (conf_val_type.h),
 * so that a node and the object it implements cannot disagree:
 *
 *   @c CVT_NONE      no value: NA nodes, unvalued collections
 *   @c CVT_STRING    free-form string, or -- when the node carries a
 *                    map -- an int mapped through it, since a mapped
 *                    value is a name on the wire
 *   @c CVT_BOOL      boolean, "0"/"1" on the wire
 *   @c CVT_INT8 .. @c CVT_UINT64
 *                    integer of exactly that width, decimal on the
 *                    wire, accessed through the matching accessor
 *   @c CVT_ADDRESS   network address (IPv4, IPv6, a link-layer
 *                    address or @c AF_UNSPEC), spelled the way
 *                    te_netaddr2te_str() spells it
 *   @c CVT_DOUBLE    floating point, printed with "%g".  Read-only:
 *                    an object of that type must be declared
 *                    read_only, so such a node has no set or add
 *
 * @c CVT_UNSPECIFIED is not a value type and is rejected at
 * registration.
 */

/** Opaque per-request context passed to every handler. */
typedef struct ta_conf_ctx ta_conf_ctx;

/**
 * Instance name of the nearest enclosing level with sub-identifier
 * @p sub_id (searched from the leaf up).  Unknown @p sub_id is a
 * programming error and aborts the agent via TE_FATAL_ERROR().
 *
 * The lookup never reaches the agent level: "agent" is not a valid
 * @p sub_id here and passing it is a programming error (it aborts,
 * same as any other unknown @p sub_id).  The agent name is not part
 * of the ta_conf OID; on the Unix agent it is available separately
 * as the global ta_name.
 *
 * @param ctx           request context
 * @param sub_id        sub-identifier of the enclosing level to look up
 *
 * @return Instance name of that level (never NULL)
 */
extern const char *ta_conf_ctx_inst(const ta_conf_ctx *ctx,
                                    const char *sub_id);

/**
 * Configuration group identifier of the request.
 *
 * @param ctx           request context
 *
 * @return Group identifier
 */
extern unsigned int ta_conf_ctx_gid(const ta_conf_ctx *ctx);

/**
 * Full instance OID of the request, or NULL in a commit handler
 * (commits get a truncated parsed OID internally).  In a list
 * handler (see ta_conf_list_fn) this is the PARENT instance OID,
 * not an OID of the collection being listed.
 *
 * @param ctx           request context
 *
 * @return Instance OID string, or NULL
 */
extern const char *ta_conf_ctx_oid(const ta_conf_ctx *ctx);

/**
 * Get handler; the member must match the node's type.
 *
 * For as_str, @p val is a te_string bound to a fixed-capacity
 * external buffer of RCF_MAX_VAL bytes supplied by the framework.
 * A value that does not fit is a programming error: unlike the
 * legacy RCF_PCH_CFG_NODE_* handlers, which silently truncate via
 * snprintf(), overflowing a te_string external buffer is fatal and
 * aborts the agent.
 *
 * @param ctx           request context
 * @param val           location for the retrieved value
 *
 * @return Status code
 */
typedef te_errno ta_conf_get_str_fn(const ta_conf_ctx *ctx, te_string *val);
typedef te_errno ta_conf_get_bool_fn(const ta_conf_ctx *ctx, bool *val);
typedef te_errno ta_conf_get_int8_fn(const ta_conf_ctx *ctx, int8_t *val);
typedef te_errno ta_conf_get_uint8_fn(const ta_conf_ctx *ctx, uint8_t *val);
typedef te_errno ta_conf_get_int16_fn(const ta_conf_ctx *ctx, int16_t *val);
typedef te_errno ta_conf_get_uint16_fn(const ta_conf_ctx *ctx,
                                       uint16_t *val);
typedef te_errno ta_conf_get_int32_fn(const ta_conf_ctx *ctx, int32_t *val);
typedef te_errno ta_conf_get_uint32_fn(const ta_conf_ctx *ctx,
                                       uint32_t *val);
typedef te_errno ta_conf_get_int64_fn(const ta_conf_ctx *ctx, int64_t *val);
typedef te_errno ta_conf_get_uint64_fn(const ta_conf_ctx *ctx,
                                       uint64_t *val);
typedef te_errno ta_conf_get_double_fn(const ta_conf_ctx *ctx, double *val);
typedef te_errno ta_conf_get_addr_fn(const ta_conf_ctx *ctx,
                                     struct sockaddr_storage *val);
typedef te_errno ta_conf_get_enum_fn(const ta_conf_ctx *ctx, int *val);

typedef union ta_conf_get_fn {
    ta_conf_get_str_fn       *as_str;
    ta_conf_get_bool_fn      *as_bool;
    ta_conf_get_int8_fn      *as_int8;
    ta_conf_get_uint8_fn     *as_uint8;
    ta_conf_get_int16_fn     *as_int16;
    ta_conf_get_uint16_fn    *as_uint16;
    ta_conf_get_int32_fn     *as_int32;
    ta_conf_get_uint32_fn    *as_uint32;
    ta_conf_get_int64_fn     *as_int64;
    ta_conf_get_uint64_fn    *as_uint64;
    ta_conf_get_double_fn    *as_double;
    ta_conf_get_addr_fn      *as_addr;
    ta_conf_get_enum_fn      *as_enum;
} ta_conf_get_fn;

/**
 * Set handler; the member must match the node's type.  There is no
 * as_double: Configurator only registers an object of type double as
 * read_only, so a node of that type is never written.
 *
 * @param ctx           request context
 * @param val           new value
 *
 * @return Status code
 */
typedef te_errno ta_conf_val_str_fn(const ta_conf_ctx *ctx,
                                    const char *val);
typedef te_errno ta_conf_val_bool_fn(const ta_conf_ctx *ctx, bool val);
typedef te_errno ta_conf_val_int8_fn(const ta_conf_ctx *ctx, int8_t val);
typedef te_errno ta_conf_val_uint8_fn(const ta_conf_ctx *ctx, uint8_t val);
typedef te_errno ta_conf_val_int16_fn(const ta_conf_ctx *ctx, int16_t val);
typedef te_errno ta_conf_val_uint16_fn(const ta_conf_ctx *ctx,
                                       uint16_t val);
typedef te_errno ta_conf_val_int32_fn(const ta_conf_ctx *ctx, int32_t val);
typedef te_errno ta_conf_val_uint32_fn(const ta_conf_ctx *ctx,
                                       uint32_t val);
typedef te_errno ta_conf_val_int64_fn(const ta_conf_ctx *ctx, int64_t val);
typedef te_errno ta_conf_val_uint64_fn(const ta_conf_ctx *ctx,
                                       uint64_t val);
typedef te_errno ta_conf_val_addr_fn(const ta_conf_ctx *ctx,
                                     const struct sockaddr *val);
typedef te_errno ta_conf_val_enum_fn(const ta_conf_ctx *ctx, int val);

typedef union ta_conf_set_fn {
    ta_conf_val_str_fn       *as_str;
    ta_conf_val_bool_fn      *as_bool;
    ta_conf_val_int8_fn      *as_int8;
    ta_conf_val_uint8_fn     *as_uint8;
    ta_conf_val_int16_fn     *as_int16;
    ta_conf_val_uint16_fn    *as_uint16;
    ta_conf_val_int32_fn     *as_int32;
    ta_conf_val_uint32_fn    *as_uint32;
    ta_conf_val_int64_fn     *as_int64;
    ta_conf_val_uint64_fn    *as_uint64;
    ta_conf_val_addr_fn      *as_addr;
    ta_conf_val_enum_fn      *as_enum;
} ta_conf_set_fn;

/**
 * Add handler for collections: as_none for unvalued collections,
 * otherwise the member matching the node's type.  The new instance's
 * own name is ta_conf_ctx_inst(ctx, "<this node's name>").
 *
 * @param ctx           request context
 * @param val           value of the new instance (typed members only)
 *
 * @return Status code
 */
typedef te_errno ta_conf_none_fn(const ta_conf_ctx *ctx);

typedef union ta_conf_add_fn {
    ta_conf_none_fn       *as_none;
    ta_conf_val_str_fn    *as_str;
    ta_conf_val_bool_fn   *as_bool;
    ta_conf_val_int8_fn   *as_int8;
    ta_conf_val_uint8_fn  *as_uint8;
    ta_conf_val_int16_fn  *as_int16;
    ta_conf_val_uint16_fn *as_uint16;
    ta_conf_val_int32_fn  *as_int32;
    ta_conf_val_uint32_fn *as_uint32;
    ta_conf_val_int64_fn  *as_int64;
    ta_conf_val_uint64_fn *as_uint64;
    ta_conf_val_addr_fn   *as_addr;
    ta_conf_val_enum_fn   *as_enum;
} ta_conf_add_fn;

/**
 * Delete handler for collections.
 *
 * @param ctx           request context
 *
 * @return Status code
 */
typedef te_errno ta_conf_del_fn(const ta_conf_ctx *ctx);

/**
 * List handler: append each instance name to @p names as a
 * heap-allocated char * (the framework owns them afterwards).
 *
 * Unlike add/del/get/set handlers of the collection's own instances,
 * a list handler runs one level up: @p ctx carries the PARENT
 * instance OID, not an instance OID of the collection being listed.
 * The collection's own sub-identifier is therefore not yet an
 * enclosing level, and ta_conf_ctx_inst(ctx, "<own sub_id>") is not
 * available here -- it aborts the agent, same as any other unknown
 * @p sub_id.  Look up the parent's instance names instead; the
 * names being listed are exactly what this handler produces.
 *
 * On the wire, the mirrored node joins @p names using the legacy
 * PCH list encoding: every name is emitted followed by a single
 * space, including an empty name -- so a single instance named ""
 * is distinguishable from an empty list, which the more common
 * "separator between names" join cannot represent.
 *
 * @param ctx           request context
 * @param names         vector of heap-allocated char * to append to
 *
 * @return Status code
 */
typedef te_errno ta_conf_list_fn(const ta_conf_ctx *ctx,
                                 te_vec *names);

/**
 * Commit handler; covers the node's whole subtree.
 *
 * @param ctx           request context
 *
 * @return Status code
 */
typedef te_errno ta_conf_commit_fn(const ta_conf_ctx *ctx);

/** One configuration tree node.  Normally built with the macros. */
typedef struct ta_conf_node {
    const char *name;       /**< Configurator sub-identifier */
    cfg_val_type type;      /**< Value type of get/set/add */
    const te_enum_map *map; /**< Name/value map, @c CVT_STRING only.
                                 Its presence is what makes the node
                                 an enumeration: the value travels as
                                 a name, and the accessors used are
                                 as_enum rather than as_str */

    ta_conf_get_fn get;     /**< Get accessor or unset */
    ta_conf_set_fn set;     /**< Set accessor or unset */
    ta_conf_add_fn add;     /**< Collection add or unset */
    ta_conf_del_fn *del;     /**< Collection delete or unset */
    ta_conf_list_fn *list;   /**< Collection list or unset */
    ta_conf_commit_fn *commit; /**< Commit covering the subtree */

    /** Substitutions passed through to the mirrored node */
    const rcf_pch_cfg_substitution *subst;

    /**
     * Children in sibling order.  Arrays built by TA_CONF_CHILDREN()
     * carry a leading NULL pad and a NULL terminator; iteration
     * starts at index 1.
     */
    const struct ta_conf_node *const *children;
} ta_conf_node;

/**
 * Validate @p root and graft it (and its subtree) under the object
 * OID @p father (e.g. "/agent"), like rcf_pch_add_node().  Must be
 * called during rcf_ch_conf_init(), i.e. from an agent conf init
 * function.  Returns TE_EINVAL (with an ERROR naming the node) on an
 * inconsistent tree.
 *
 * @param father        object OID to graft the subtree under
 * @param root          root of the subtree to register
 *
 * @return Status code
 */
extern te_errno ta_conf_register(const char *father,
                                 const ta_conf_node *root);

/**
 * Child node list.
 *
 * The expansion is a file-scope compound literal array of node
 * pointers with a leading NULL pad (so that the GNU ", ##" comma
 * deletion works for empty lists) and a NULL terminator.
 *
 * By convention, children of a node macro are indented one 4-space
 * level per tree depth, so that the source layout mirrors the tree.
 *
 * The node kind macros may be nested arbitrarily as children
 * arguments, including the same macro inside itself: argument
 * prescan fully expands nested invocations.
 *
 * Children must be node expressions written in place.  Do not keep a
 * subtree in a separate "static const ta_conf_node *const" variable
 * and pass the variable as a child: before C23 such a variable is not
 * a constant expression, and compilers older than GCC 13 reject it in
 * a static initializer.
 *
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_CHILDREN(...) \
    (const ta_conf_node *const []){ NULL, ## __VA_ARGS__, NULL }

/** @cond internal */
#define TA_CONF__NODE(...) (&(const ta_conf_node){ __VA_ARGS__ })
/** @endcond */

/**
 * Non-accessible intermediate node; extra args are children.
 *
 * @param name_         node's Configurator sub-identifier
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_NA(name_, ...) \
    TA_CONF__NODE(.name = (name_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Non-accessible node whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_NA_COMMIT(name_, commit_, ...) \
    TA_CONF__NODE(.name = (name_), .commit = (commit_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-only string leaf; extra args are children.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_STR(name_, get_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-write string leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param set_          set handler (as_str)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_STR(name_, get_, set_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, \
                  .set = { .as_str = (set_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/** @cond INTERNAL */
#define TA_CONF__RO_NUM(name_, T_, t_, get_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_##T_, \
                  .get = { .as_##t_ = (get_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

#define TA_CONF__RW_NUM(name_, T_, t_, get_, set_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_##T_, \
                  .get = { .as_##t_ = (get_) }, \
                  .set = { .as_##t_ = (set_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

#define TA_CONF__RO_COLL_NUM(name_, T_, t_, get_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_##T_, \
                  .get = { .as_##t_ = (get_) }, .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

#define TA_CONF__RW_COLL_NUM(name_, T_, t_, get_, set_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_##T_, \
                  .get = { .as_##t_ = (get_) }, \
                  .set = { .as_##t_ = (set_) }, .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

#define TA_CONF__COLL_NUM_RW(name_, T_, t_, get_, set_, add_, del_, \
                             list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_##T_, \
                  .get = { .as_##t_ = (get_) }, \
                  .set = { .as_##t_ = (set_) }, \
                  .add = { .as_##t_ = (add_) }, .del = (del_), \
                  .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

#define TA_CONF__COLL_NUM_RW_COMMIT(name_, T_, t_, get_, set_, add_, del_, \
                                    list_, commit_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_##T_, \
                  .get = { .as_##t_ = (get_) }, \
                  .set = { .as_##t_ = (set_) }, \
                  .add = { .as_##t_ = (add_) }, .del = (del_), \
                  .list = (list_), .commit = (commit_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))
/** @endcond */

/**
 * Read-only signed 8-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int8)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_INT8(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, INT8, int8, get_, __VA_ARGS__)

/**
 * Read-write signed 8-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int8)
 * @param set_          set handler (as_int8)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_INT8(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, INT8, int8, get_, set_, __VA_ARGS__)

/**
 * Read-only signed 8-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int8)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_INT8(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, INT8, int8, get_, list_, __VA_ARGS__)

/**
 * Read-write signed 8-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int8)
 * @param set_          set handler (as_int8)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_INT8(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, INT8, int8, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write signed 8-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int8)
 * @param set_          set handler (as_int8)
 * @param add_          add handler (as_int8)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT8_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, INT8, int8, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write signed 8-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int8)
 * @param set_          set handler (as_int8)
 * @param add_          add handler (as_int8)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT8_RW_COMMIT(name_, get_, set_, add_, del_, \
                                    list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, INT8, int8, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only unsigned 8-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint8)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_UINT8(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, UINT8, uint8, get_, __VA_ARGS__)

/**
 * Read-write unsigned 8-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint8)
 * @param set_          set handler (as_uint8)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_UINT8(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, UINT8, uint8, get_, set_, __VA_ARGS__)

/**
 * Read-only unsigned 8-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint8)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_UINT8(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, UINT8, uint8, get_, list_, __VA_ARGS__)

/**
 * Read-write unsigned 8-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint8)
 * @param set_          set handler (as_uint8)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_UINT8(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, UINT8, uint8, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write unsigned 8-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint8)
 * @param set_          set handler (as_uint8)
 * @param add_          add handler (as_uint8)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT8_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, UINT8, uint8, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write unsigned 8-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint8)
 * @param set_          set handler (as_uint8)
 * @param add_          add handler (as_uint8)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT8_RW_COMMIT(name_, get_, set_, add_, del_, \
                                     list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, UINT8, uint8, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only signed 16-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int16)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_INT16(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, INT16, int16, get_, __VA_ARGS__)

/**
 * Read-write signed 16-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int16)
 * @param set_          set handler (as_int16)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_INT16(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, INT16, int16, get_, set_, __VA_ARGS__)

/**
 * Read-only signed 16-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int16)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_INT16(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, INT16, int16, get_, list_, __VA_ARGS__)

/**
 * Read-write signed 16-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int16)
 * @param set_          set handler (as_int16)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_INT16(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, INT16, int16, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write signed 16-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int16)
 * @param set_          set handler (as_int16)
 * @param add_          add handler (as_int16)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT16_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, INT16, int16, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write signed 16-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int16)
 * @param set_          set handler (as_int16)
 * @param add_          add handler (as_int16)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT16_RW_COMMIT(name_, get_, set_, add_, del_, \
                                     list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, INT16, int16, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only unsigned 16-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint16)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_UINT16(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, UINT16, uint16, get_, __VA_ARGS__)

/**
 * Read-write unsigned 16-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint16)
 * @param set_          set handler (as_uint16)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_UINT16(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, UINT16, uint16, get_, set_, __VA_ARGS__)

/**
 * Read-only unsigned 16-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint16)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_UINT16(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, UINT16, uint16, get_, list_, __VA_ARGS__)

/**
 * Read-write unsigned 16-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint16)
 * @param set_          set handler (as_uint16)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_UINT16(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, UINT16, uint16, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write unsigned 16-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint16)
 * @param set_          set handler (as_uint16)
 * @param add_          add handler (as_uint16)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT16_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, UINT16, uint16, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write unsigned 16-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint16)
 * @param set_          set handler (as_uint16)
 * @param add_          add handler (as_uint16)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT16_RW_COMMIT(name_, get_, set_, add_, del_, \
                                      list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, UINT16, uint16, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only signed 32-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int32)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_INT32(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, INT32, int32, get_, __VA_ARGS__)

/**
 * Read-write signed 32-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int32)
 * @param set_          set handler (as_int32)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_INT32(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, INT32, int32, get_, set_, __VA_ARGS__)

/**
 * Read-only signed 32-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int32)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_INT32(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, INT32, int32, get_, list_, __VA_ARGS__)

/**
 * Read-write signed 32-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int32)
 * @param set_          set handler (as_int32)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_INT32(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, INT32, int32, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write signed 32-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int32)
 * @param set_          set handler (as_int32)
 * @param add_          add handler (as_int32)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT32_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, INT32, int32, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write signed 32-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int32)
 * @param set_          set handler (as_int32)
 * @param add_          add handler (as_int32)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT32_RW_COMMIT(name_, get_, set_, add_, del_, \
                                     list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, INT32, int32, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only unsigned 32-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint32)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_UINT32(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, UINT32, uint32, get_, __VA_ARGS__)

/**
 * Read-write unsigned 32-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint32)
 * @param set_          set handler (as_uint32)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_UINT32(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, UINT32, uint32, get_, set_, __VA_ARGS__)

/**
 * Read-only unsigned 32-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint32)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_UINT32(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, UINT32, uint32, get_, list_, __VA_ARGS__)

/**
 * Read-write unsigned 32-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint32)
 * @param set_          set handler (as_uint32)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_UINT32(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, UINT32, uint32, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write unsigned 32-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint32)
 * @param set_          set handler (as_uint32)
 * @param add_          add handler (as_uint32)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT32_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, UINT32, uint32, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write unsigned 32-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint32)
 * @param set_          set handler (as_uint32)
 * @param add_          add handler (as_uint32)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT32_RW_COMMIT(name_, get_, set_, add_, del_, \
                                      list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, UINT32, uint32, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only signed 64-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int64)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_INT64(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, INT64, int64, get_, __VA_ARGS__)

/**
 * Read-write signed 64-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int64)
 * @param set_          set handler (as_int64)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_INT64(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, INT64, int64, get_, set_, __VA_ARGS__)

/**
 * Read-only signed 64-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int64)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_INT64(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, INT64, int64, get_, list_, __VA_ARGS__)

/**
 * Read-write signed 64-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int64)
 * @param set_          set handler (as_int64)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_INT64(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, INT64, int64, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write signed 64-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int64)
 * @param set_          set handler (as_int64)
 * @param add_          add handler (as_int64)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT64_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, INT64, int64, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write signed 64-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_int64)
 * @param set_          set handler (as_int64)
 * @param add_          add handler (as_int64)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_INT64_RW_COMMIT(name_, get_, set_, add_, del_, \
                                     list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, INT64, int64, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only unsigned 64-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint64)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_UINT64(name_, get_, ...) \
    TA_CONF__RO_NUM(name_, UINT64, uint64, get_, __VA_ARGS__)

/**
 * Read-write unsigned 64-bit leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint64)
 * @param set_          set handler (as_uint64)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_UINT64(name_, get_, set_, ...) \
    TA_CONF__RW_NUM(name_, UINT64, uint64, get_, set_, __VA_ARGS__)

/**
 * Read-only unsigned 64-bit valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint64)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL_UINT64(name_, get_, list_, ...) \
    TA_CONF__RO_COLL_NUM(name_, UINT64, uint64, get_, list_, __VA_ARGS__)

/**
 * Read-write unsigned 64-bit valued collection with no add/del
 * (get/set/list): instances are enumerable but not created or
 * destroyed through this node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint64)
 * @param set_          set handler (as_uint64)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_UINT64(name_, get_, set_, list_, ...) \
    TA_CONF__RW_COLL_NUM(name_, UINT64, uint64, get_, set_, list_, \
                         __VA_ARGS__)

/**
 * Fully read-write unsigned 64-bit valued collection
 * (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint64)
 * @param set_          set handler (as_uint64)
 * @param add_          add handler (as_uint64)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT64_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__COLL_NUM_RW(name_, UINT64, uint64, get_, set_, add_, del_, \
                         list_, __VA_ARGS__)

/**
 * Fully read-write unsigned 64-bit valued collection
 * (get/set/add/del/list) whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_uint64)
 * @param set_          set handler (as_uint64)
 * @param add_          add handler (as_uint64)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_UINT64_RW_COMMIT(name_, get_, set_, add_, del_, \
                                      list_, commit_, ...) \
    TA_CONF__COLL_NUM_RW_COMMIT(name_, UINT64, uint64, get_, set_, add_, \
                                del_, list_, commit_, __VA_ARGS__)

/**
 * Read-only boolean leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_bool)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_BOOL(name_, get_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_BOOL, \
                  .get = { .as_bool = (get_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-write boolean leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_bool)
 * @param set_          set handler (as_bool)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_BOOL(name_, get_, set_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_BOOL, \
                  .get = { .as_bool = (get_) }, \
                  .set = { .as_bool = (set_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-only enumerated leaf mapped through @p map_.
 *
 * @param name_         node's Configurator sub-identifier
 * @param map_          te_enum_map used to convert between the
 *                      wire string and the C int value
 * @param get_          get handler (as_enum)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_ENUM(name_, map_, get_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .map = (map_), \
                  .get = { .as_enum = (get_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-write enumerated leaf mapped through @p map_.
 *
 * @param name_         node's Configurator sub-identifier
 * @param map_          te_enum_map used to convert between the
 *                      wire string and the C int value
 * @param get_          get handler (as_enum)
 * @param set_          set handler (as_enum)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_ENUM(name_, map_, get_, set_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .map = (map_), \
                  .get = { .as_enum = (get_) }, \
                  .set = { .as_enum = (set_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Unvalued collection (add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param add_          add handler (as_none)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL(name_, add_, del_, list_, ...) \
    TA_CONF__NODE(.name = (name_), \
                  .add = { .as_none = (add_) }, .del = (del_), \
                  .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * String-valued collection (get/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param add_          add handler (as_str)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_STR(name_, get_, add_, del_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, \
                  .add = { .as_str = (add_) }, .del = (del_), \
                  .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Fully read-write string-valued collection (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param set_          set handler (as_str)
 * @param add_          add handler (as_str)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_STR_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, \
                  .set = { .as_str = (set_) }, \
                  .add = { .as_str = (add_) }, .del = (del_), \
                  .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-only address-valued leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_addr)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_ADDRESS(name_, get_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_ADDRESS, \
                  .get = { .as_addr = (get_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-write address-valued leaf.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_addr)
 * @param set_          set handler (as_addr)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_ADDRESS(name_, get_, set_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_ADDRESS, \
                  .get = { .as_addr = (get_) }, \
                  .set = { .as_addr = (set_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Fully read-write address-valued collection (get/set/add/del/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_addr)
 * @param set_          set handler (as_addr)
 * @param add_          add handler (as_addr)
 * @param del_          delete handler
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_ADDRESS_RW(name_, get_, set_, add_, del_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_ADDRESS, \
                  .get = { .as_addr = (get_) }, \
                  .set = { .as_addr = (set_) }, \
                  .add = { .as_addr = (add_) }, .del = (del_), \
                  .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Floating point leaf.  There is no read-write counterpart:
 * Configurator only registers an object of type double as read_only.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_double)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_DOUBLE(name_, get_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_DOUBLE, \
                  .get = { .as_double = (get_) }, \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Fully read-write string-valued collection (get/set/add/del/list)
 * whose commit covers its subtree.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param set_          set handler (as_str)
 * @param add_          add handler (as_str)
 * @param del_          delete handler
 * @param list_         list handler
 * @param commit_       commit handler covering the subtree
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_COLL_STR_RW_COMMIT(name_, get_, set_, add_, del_, \
                                   list_, commit_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, \
                  .set = { .as_str = (set_) }, \
                  .add = { .as_str = (add_) }, .del = (del_), \
                  .list = (list_), .commit = (commit_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-only string-valued collection (get/list).
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RO_COLL(name_, get_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-write string-valued collection with no add/del (get/set/list):
 * instances are enumerable but not created or destroyed through this
 * node (e.g. their identity comes from elsewhere), only read and
 * written.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_str)
 * @param set_          set handler (as_str)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_STR(name_, get_, set_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_STRING, \
                  .get = { .as_str = (get_) }, \
                  .set = { .as_str = (set_) }, .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * Read-write boolean-valued collection with no add/del (get/set/list):
 * instances are enumerable but not created or destroyed through this
 * node.
 *
 * @param name_         node's Configurator sub-identifier
 * @param get_          get handler (as_bool)
 * @param set_          set handler (as_bool)
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_RW_COLL_BOOL(name_, get_, set_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .type = CVT_BOOL, \
                  .get = { .as_bool = (get_) }, \
                  .set = { .as_bool = (set_) }, .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/**
 * List-only collection: no value, no add/del, just enumerable
 * instances (e.g. a set of read-only sub-identifiers with no
 * per-instance data of their own).
 *
 * @param name_         node's Configurator sub-identifier
 * @param list_         list handler
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_LIST(name_, list_, ...) \
    TA_CONF__NODE(.name = (name_), .list = (list_), \
                  .children = TA_CONF_CHILDREN(__VA_ARGS__))

/** @cond internal */
#define TA_CONF__UNPAREN(...) __VA_ARGS__
/** @endcond */

/**
 * Generic node, for shapes with no dedicated macro (odd handler
 * combinations, .subst, etc.).  Unlike a hand-written literal of the
 * form &(const ta_conf_node){ ..., .children = TA_CONF_CHILDREN(...) },
 * the children passed here through @p ... are macro arguments of
 * TA_CONF_NODE() itself, so macro-generated children (e.g. other
 * TA_CONF_* node macros) are expanded before TA_CONF_CHILDREN() ever
 * sees them, same as with every other node macro in this file.
 *
 * @attention A hand-written node states its type and its accessors
 * separately, and nothing checks that the two agree, so the enum
 * rule has to be kept by hand.  An enumerated node is @c CVT_STRING
 * with a map: handlers spelled .as_enum REQUIRE a non-NULL .map, and
 * a node whose handlers are .as_str must NOT have one.  The map is
 * what the codecs branch on, so breaking either half calls a handler
 * through a pointer of the wrong type and corrupts memory instead of
 * failing: .map on a .as_str node calls a
 * (const ta_conf_ctx *, te_string *) function as a
 * (const ta_conf_ctx *, int *) one,
 * and .as_enum without a map hands a te_string * to a handler that
 * writes an int through it.  There is no collection macro for an
 * enumerated value, so a node of that shape is exactly what this
 * macro is used for; get both halves right.
 *
 * @param fields_       designated field initializers for the node,
 *                      as a single parenthesized group, e.g.
 *                      (.name = "x", .type = CVT_STRING, ...)
 * @param ...           child node pointers, in sibling order
 */
#define TA_CONF_NODE(fields_, ...) \
    (&(const ta_conf_node){ TA_CONF__UNPAREN fields_, \
                            .children = TA_CONF_CHILDREN(__VA_ARGS__) })

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RCF_PCH_TREE_H__ */
