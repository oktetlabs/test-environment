/** @file
 * @brief Configurator
 *
 * Object identifiers
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CONF_OID_H__
#define __TE_CONF_OID_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "te_defs.h"

#define CFG_SUBID_MAX       128  /**< Maximum length of sub-identifier
                                      including trailing 0 */
#define CFG_INST_NAME_MAX   256  /**< Instance name, including trailing 0 */

#define CFG_OID_LEN_MAX     8    /**< Maximum number of subids */

#define CFG_OID_MAX \
    ((CFG_SUBID_MAX + CFG_INST_NAME_MAX + 2) * CFG_OID_LEN_MAX)

/** Object identifier element */
typedef struct cfg_object_subid {
    char subid[CFG_SUBID_MAX]; /**< For root object "/" */
} cfg_object_subid;

/** Object instance identifier element */
typedef struct cfg_inst_subid {
    char subid[CFG_SUBID_MAX];    /**< For root instance "/" */
    char name[CFG_INST_NAME_MAX]; /**< For root instance empty */
} cfg_inst_subid;

/** Object identifier */
typedef struct cfg_oid {
    uint8_t len;  /**< Number of identifier elements */
    te_bool inst; /**< TRUE, if OID is object instance identifier */
    void   *ids;  /**< Pointer to array of identifier elements */
} cfg_oid;

/**
 * Object OID literal.
 *
 * The constructed value is a cfg_oid structure, not a pointer.
 *
 * @param ...   The list of subids. cfg_object_subid is a one-element
 *              structure, so string literals need to be wrapped in `{..}`.
 *              The first subid is implicitly set to "".
 *
 * @code{.c}
 * static cfg_oid object_oid = CFG_OBJ_OID_LITERAL({"agent"},
 *                                                 {"interface"},
 *                                                 {"status"});
 * ...
 * cfg_oid *parsed = cfg_oid_convert_str(oid_str);
 * status = cfg_oid_cmp(&object_oid, parsed);
 * cfg_free_oid(parsed);
 * @endcode
 */
#define CFG_OBJ_OID_LITERAL(...) \
    ((cfg_oid){                                                             \
        .len = 1 + TE_ARRAY_LEN(((const cfg_object_subid[]){__VA_ARGS__})), \
        .inst = FALSE,                                                      \
        .ids = (cfg_object_subid[]){{""}, __VA_ARGS__}                     \
    })

/**
 * Get instance name of the i-th sub-identifier.
 *
 * @param _p      pointer to cfg_oid
 * @param _i      index
 *
 * @return Pointer to string (char *).
 */
#define CFG_OID_GET_INST_NAME(_p, _i) \
    (((cfg_inst_subid *)(_p->ids))[(_i)].name)

/**
 * Get specified instance name from OID.
 *
 * @param oid           OID
 * @param idx           Instance index with negative values support to
 *                      count from the last subidentifier
 *                      (e.g. @c -1 to get the last instance name)
 *
 * @return Instance name allocated from heap or @c NULL.
 *
 * @sa cfg_oid_str_get_inst_name()
 */
extern char *cfg_oid_get_inst_name(const cfg_oid *oid, int idx);

/**
 * Get specified instance name from OID in string format.
 *
 * @param oid_str       OID in string format
 * @param idx           Instance index with negative values support
 *                      (@c -1 to get the last instance name)
 *
 * @return Instance name allocated from heap or @c NULL.
 *
 * @sa cfg_oid_get_inst_name()
 */
extern char *cfg_oid_str_get_inst_name(const char *oid_str, int idx);

/**
 * Get sub-ID from object OID.
 */
static inline char *
cfg_oid_object_subid(cfg_oid *oid, unsigned int i)
{
    if (oid->inst || i >= oid->len)
        return NULL;

    return ((cfg_object_subid *)(oid->ids))[i].subid;
}

/**
 * Get sub-ID from instance OID.
 */
static inline char *
cfg_oid_inst_subid(cfg_oid *oid, unsigned int i)
{
    if (!oid->inst || i >= oid->len)
        return NULL;

    return ((cfg_inst_subid *)(oid->ids))[i].subid;
}

/**
 * Allocate memory for object identifier or object instance identifier.
 *
 * @param length      number of identifier elements
 * @param inst        if TRUE - object instance identifier should be
 *                    allocated
 *
 * @return newly allocated structure pointer or NULL
 */
extern cfg_oid *cfg_allocate_oid(int length, te_bool inst);

/**
 * Convert object identifier or object instance identifier in
 * string representation to cfg_oid structure.
 *
 * @param oid         OID in string representation
 *
 * @return newly allocated structure pointer or NULL
 */
extern cfg_oid *cfg_convert_oid_str(const char *oid);

/**
 * Convert object identifier or object instance identifier in
 * structure representation to string (memory is allocated by the
 * routine using malloc()).
 *
 * @param oid         OID in structure representation
 *
 * @return newly allocated string or NULL
 */
extern char *cfg_convert_oid(const cfg_oid *oid);

/**
 * Free memory allocated for OID stucture.
 *
 * @param oid         oid structure
 */
extern void cfg_free_oid(cfg_oid *oid);

/**
 * Compare object identifiers.
 *
 * @param o1      the first OID
 * @param o2      the second OID
 *
 * @retval 0      equal
 * @retval 1      not equal
 */
extern int cfg_oid_cmp(const cfg_oid *o1, const cfg_oid *o2);


/**
 * Checks whether @p inst_oid is an instance of @p obj_oid.
 *
 * @param inst_oid      Instance OID
 * @param obj_oid       Object OID
 * @param match_subtree If @c TRUE, @p inst_oid should be within
 *                      a subtree defined by @p obj_oid, i.e.
 *                      there should be a prefix of @p inst_oid
 *                      matching @p obj_oid. Otherwise, the whole
 *                      @p inst_oid shall match.
 * @return @c TRUE iff @p inst_oid matches @p obj_oid
 */
extern te_bool cfg_oid_match(const cfg_oid *inst_oid, const cfg_oid *obj_oid,
                             te_bool match_prefix);


/** Function type for actions for cfg_oid_rule
 *
 * @param inst_oid    Instance OID as passed to cfg_oid_dispatch()
 * @param parsed_oid  Instance OID represented as cfg_oid
 * @param ctx         User data as passed to cfg_oid_dispatch()
 */
typedef te_errno cfg_oid_action(const char *inst_oid,
                                const cfg_oid *parsed_oid,
                                void *ctx);

/** A rule entry for cfg_oid_dispatch() */
typedef struct cfg_oid_rule {
    /** Object OID */
    const cfg_oid *object_oid;
    /**
     * If @c TRUE, the prefix of an instance OID is
     * matched, otherwise the whole OID
     */
    te_bool match_prefix;
    /** Action to execute */
    cfg_oid_action *action;
} cfg_oid_rule;

/**
 * A helper macro to construct cfg_oid_rule entries.
 *
 * @param _match_prefix    See cfg_oid_rule::match_prefix
 * @param _action          Action to execute
 * @param ...              A list of subids as passed to 
 *                         CFG_OBJ_OID_LITERAL
 */
#define CFG_OID_RULE(_match_prefix, _action, ...) \
    {                                                   \
        .object_oid = &CFG_OBJ_OID_LITERAL(__VA_ARGS__),\
        .match_prefix = (_match_prefix),                \
        .action = (_action)                             \
    }

/** Terminating entry for cfg_oid_rule table */
#define CFG_OID_RULE_END {.object_oid = NULL}

/**
 * Calls an action depending on the object OID.
 *
 * The function parses @p inst_oid, then searchers @p actions
 * for an entry with a matching @a object_oid (as per cfg_oid_match).
 * Then a corresponding handler is called.
 *
 * @param rules     The array of rules, the last entry shall have
 *                  @c NULL in @a object_oid field.
 * @param inst_oid  Instance OID
 * @param ctx       User data passed to the handler
 *
 * @return Status code (may be returned from a handler)
 * @retval TE_ESRCH  No matching entry is found
 */
extern te_errno cfg_oid_dispatch(const cfg_oid_rule rules[],
                                 const char *inst_oid,
                                 void *ctx);

/**
 * Determines a common part of two OIDs. 
 * If both OIDs are object OIDs, an object OID is returned.
 * Otherwise, an instance OID is returned that starts with
 * a common part of the two, with instance names taken from
 * either 'oid1' or 'oid2' whichever is an instance OID, and
 * the rest is filled from 'oid1' with names set to '*'.
 *
 * Example:
 * - /a/b/c + /a/b/d -> /a/b
 * - /a/b/c + /a:A/b:B/d -> /a:A/b:B/c:*
 * - /a:A/b:B/c:C + /a/b/d -> /a:A/b:B/c:*
 * - /a:A/b:B/c:C + /a:A1/b:B1/d:D -> /a:A/b:B/c:*
 *
 * @param oid1  the first OID
 * @param oid2  the second OID
 *
 * @retval resulting OID
 */
extern  cfg_oid *cfg_oid_common_root(const cfg_oid *oid1, 
                                     const cfg_oid *oid2);
    

/**
 * Convert instance identifier to object identifier.
 *
 * @param inst_oid       instance identifier
 * @param obj_oid        object identifier (CFG_OID_MAX length)
 */
static inline void
cfg_oid_inst2obj(const char *inst_oid, char *obj_oid)
{
    cfg_oid *oid = cfg_convert_oid_str(inst_oid);
    
    *obj_oid = 0;
    
    if (oid != NULL && oid->inst && oid->len > 1)
    {
        int i;
        
        for (i = 1; i < oid->len; i++)
            obj_oid += sprintf(obj_oid, "/%s", 
                               ((cfg_inst_subid *)(oid->ids))[i].subid);
    }
    
    cfg_free_oid(oid);
}

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_OID_H__ */

