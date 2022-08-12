/** @file 
 * @brief Common library for test agents which is very useful
 * in supporting read-create instances implementing "commit" operation.
 *
 * Test Agent configuration library
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#ifndef __TE_RCF_PCH_TA_CFG_H__
#define __TE_RCF_PCH_TA_CFG_H__

#if HAVE_NET_IF_H && !defined(__CYGWIN__)
#include <net/if.h>
#endif

#ifndef IF_NAMESIZE
#ifdef IFNAMSIZ
#define IF_NAMESIZE IFNAMSIZ
#elif defined(__CYGWIN__)
#define IF_NAMESIZE 16
#else
#error Neither IF_NAMESIZE nor IFNAMSIZ defined
#endif
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_queue.h"
#include "rcf_common.h"


/*
 * The main goal of this library is to simplify implementation of
 * read-create/commit instances.
 *
 * Usually read-create instances have a set of children, which appears
 * together with a new parent instance.
 * There are two possible ways to create such parent instances:
 * 1. Perform ordinary ADD operation, which in turn (after completion)
 *    reads out the list of children with their values from Test Agent
 *    (in term of Configurator this operation called "synchronization").
 *    Such approach is not so flexible in cases where we need to create
 *    a new instance whose children have not default values but some
 *    particular ones.
 *    Of course we can perform SET operations for just created children,
 *    but this might not be what we want (sequence of ADD, SET operations
 *    is not the same as atomic ADD with all specified values).
 *    In this circumstance we should consider the second way of creating
 *    object instances;
 * 2. Perform local ADD operation, a set of local SET operations,
 *    and finally COMMIT operation.
 *    In this case we accumulate all the values in so called cache, and
 *    install it as a bulk in COMMIT.
 *
 * Which way to choose depends on the implementation of Test Agent,
 * because from programmer's point of view the only difference in these
 * two ways is that to support the second approach they should provide 
 * callback function for COMMIT operation, and save all the information 
 * obtained in ADD and SET callbacks in some local data structures.
 *
 * This library plays a role of such local data structure where all the
 * information can be locally stored (in ADD, or SET callbacks) and 
 * then extracted (in COMMIT callback).
 *
 * How to work with it:
 * The library uses term "object", which has no in common with term object
 * defined in Configurator context.
 * Each object is associated with a particular type (a constant string)
 * and a name (also a constant string). These two strings is a key for
 * objects, which means that you can identify each object with this pair.
 * Each object can keep a set of attributes with their values.
 * Attribute is identified by name (a constant string), and its value is
 * also a constant string.
 * In addition, each object keeps "action" value, which defines the type of
 * operation that should be performed with it (in most of the cases this
 * value is used in commit callback to define what to do - whether to add
 * a new instance, delete an existing instance, or just update some 
 * attributes of an existing instance).
 */
#ifdef __cplusplus
extern "C" {
#endif


/** Object attribute data structure */
typedef struct ta_cfg_obj_attr {
    struct ta_cfg_obj_attr *next; /**< Pointer to the next attribute */

    char name[RCF_MAX_ID]; /**< Attribute name */
    char value[128]; /**< Attribute value */
} ta_cfg_obj_attr_t;

/**
 * Action that should be performed with the object.
 */
typedef enum ta_cfg_obj_action {
    TA_CFG_OBJ_CREATE, /**< Create a new entry in the system */
    TA_CFG_OBJ_DELETE, /**< Delete an existing entry */
    TA_CFG_OBJ_SET, /**< Change some attribute of an existing entry */
    TA_CFG_OBJ_GET, /**< Find/get an entry */
} ta_cfg_obj_action_e;

/** Type of the function used to free user-provided data */
typedef void (ta_cfg_obj_data_free)(void *data);

/** Object data structure, which is inserted into collection */
typedef struct ta_cfg_obj {
    te_bool  in_use;    /**< Whether this entry is in use */
    char    *type;      /**< Object type in string representation */
    char    *name;      /**< Object name - actually, instance name */
    char    *value;     /**< Object value - actually, instance value */

    void    *user_data; /**< Pointer to user-provided data */
    ta_cfg_obj_data_free *user_free; /**< Function for releasing
                                          user-provided data */

    unsigned int         gid;    /**< Group ID */
    ta_cfg_obj_action_e  action; /**< Object action */
    ta_cfg_obj_attr_t   *attrs;  /**< List of object's attributes */
} ta_cfg_obj_t;

/**
 * Release all the objects which are still marked as being in use.
 */
extern void ta_obj_cleanup(void);

/**
 * Set (or add if there is no such attribute) specified value to
 * the particular attribute.
 *
 * @param obj    Object where to update attibute list
 * @param name   Attribute name to update (or to add)
 * @param value  Attribute value
 *
 * @return Error code or 0
 */
extern int ta_obj_attr_set(ta_cfg_obj_t *obj,
                           const char *name, const char *value);

/**
 * Find specified attribute in object.
 *
 * @param obj   Object where we want to find an attribute
 * @param name  Attribute name
 *
 * @return Pointer to the attribute if found, and NULL otherwise
 */
extern ta_cfg_obj_attr_t *ta_obj_attr_find(ta_cfg_obj_t *obj,
                                           const char *name);

/**
 * Free object.
 *
 * @param obj  Object to be freed
 */
extern void ta_obj_free(ta_cfg_obj_t *obj);

/**
 * Finds an object of specified type whose name is the same as @p name
 * parameter.
 *
 * @param type  Object type - user-defined constant
 * @param name  Object name - actually, instance name
 * @param gid   Request group ID
 *
 * @return Pointer to object in case of success, @c NULL otherwise.
 */
extern ta_cfg_obj_t *ta_obj_find(const char *type, const char *name,
                                 unsigned int gid);

/**
 * Prototype for callback function used to fill created
 * object attributes()
 */
typedef te_errno (* ta_obj_cb)(ta_cfg_obj_t *obj);

/**
 * Find an object of specified type whose name is the same as @p name
 * parameter. Creates an object if it does not exist.
 *
 * @param type        Object type - user-defined constant.
 * @param name        Object name - instance name.
 * @param gid         Request group ID.
 * @param cb_func     Callback function for filling object attributes
 *                    if it is created.
 * @param obj         Where to save pointer to object.
 * @param created     Will be set to @c TRUE if not @c NULL and object
 *                    is created.
 *
 * @return Status code.
 */
extern te_errno ta_obj_find_create(const char *type, const char *name,
                                   unsigned int gid, ta_obj_cb cb_func,
                                   ta_cfg_obj_t **obj, te_bool *created);

/**
 * Create an object of specified type with particular value.
 *
 * @param type        Object type - user-defined constant
 * @param name        Object name - actually, instance name
 * @param value       Object value
 * @param gid         Request group ID
 * @param user_data   Some user-data value associated with this object
 * @param user_free   Function that can be used to release @p user_data
 * @param new_obj     Object entry (OUT)
 *
 * @return Error code or 0
 */
extern int ta_obj_add(const char *type, const char *name,
                      const char *value, unsigned int gid,
                      void *user_data, ta_cfg_obj_data_free *user_free,
                      ta_cfg_obj_t **new_obj);

/**
 * Set the value of the object.
 *
 * @param type       Object type - user-defined constant
 * @param name       Object name
 * @param value      Object value
 * @param gid        Request group ID
 * @param cb_func    Callback function for filling object attributes
 *                   if it is created.
 *
 * @return Status code.
 */
extern te_errno ta_obj_value_set(const char *type, const char *name,
                                 const char *value, unsigned int gid,
                                 ta_obj_cb cb_func);

/**
 * Create or update object of specified type.
 * In case object with specified name has been already added it
 * just updates (adds) the value of specified attribute.
 * In case there is no object with specified name and type, it creates it
 * with SET action and calls callback function func.
 *
 * Callback function should be used to fill in all the attributes with
 * their current values if necessary.
 *
 * @param type        Object type - user-defined constant
 * @param name        Object name - actually, instance name
 * @param attr_name   Attribute name to update in object
 * @param attr_value  Attribute value
 * @param gid         Request group ID
 * @param cb_func     Callback function to be called if SET operation
 *                    leads to creating a new object in collection
 *
 * @return Error code or 0
 */
extern int ta_obj_set(const char *type, const char *name,
                      const char *attr_name, const char *attr_value,
                      unsigned int gid, ta_obj_cb cb_func);

/**
 * Create an object of specified type and mark it as deleted.
 *
 * @param type        Object type - user-defined constant
 * @param name        Object name - actually, instance name
 * @param user_data   Some user-data value associated with this object
 * @param user_free   Function that can be used to release @p user_data
 * @param gid         Request group ID
 * @param cb_func     Callback function to be called for created object
 *
 * @return Error code or 0
 */
extern int ta_obj_del(const char *type, const char *name, void *user_data,
                      ta_cfg_obj_data_free *user_free,
                      unsigned int gid, ta_obj_cb cb_func);

/**
 * Log TA configuration object using RING logging level.
 *
 * @param obj       An object
 */
extern void ta_cfg_obj_log(const ta_cfg_obj_t *obj);


/* Route-specific definitions */

/** Name for object of type route */
#define TA_OBJ_TYPE_ROUTE "route"

/** Name for object of type sniffer */
#define TA_OBJ_TYPE_SNIFFER "sniffer"

/**
 * Name for the object type of network interface interrupt coalescing
 * settings
 */
#define TA_OBJ_TYPE_IF_COALESCE "if_coalesce"

/**
 * Name for the object type of network interface pause parameters
 */
#define TA_OBJ_TYPE_IF_PAUSE "if_pause"

/**
 * Name for the object type of network interface link settings
 */
#define TA_OBJ_TYPE_IF_LINK_SETS "if_link_sets"

/**
 * Name for the object type containing set of strings related
 * to network interface (like list of names of Ethernet features).
 */
#define TA_OBJ_TYPE_IF_STRINGS "if_strings"

/**
 * Name for the object type containing RSS hash key and indirection
 * table.
 */
#define TA_OBJ_TYPE_IF_RSSH "if_rssh"

/** System-independent route types */
typedef enum ta_route_type {
    TA_RT_TYPE_UNSPECIFIED,
    TA_RT_TYPE_UNICAST,
    TA_RT_TYPE_LOCAL,
    TA_RT_TYPE_BROADCAST,
    TA_RT_TYPE_ANYCAST,
    TA_RT_TYPE_MULTICAST,
    TA_RT_TYPE_BLACKHOLE,
    TA_RT_TYPE_UNREACHABLE,
    TA_RT_TYPE_PROHIBIT,
    TA_RT_TYPE_THROW,
    TA_RT_TYPE_NAT,
    TA_RT_TYPE_MAX_VALUE
} ta_route_type;    

/**
 * Converts a route type id to a string representation 
 *
 * @param   Route type id
 *
 * @return  A pointer to static buffer holding the type name
 */
extern const char *ta_rt_type2name(ta_route_type type);

/** Flag which is set if gateway is specified for route nexthop. */
#define TA_RT_NEXTHOP_FLG_GW 0x1
/** Flag which is set if interface is specified for route nexthop. */
#define TA_RT_NEXTHOP_FLG_OIF 0x2

/** Nexthop of a multipath route. */
typedef struct ta_rt_nexthop_t {
    TAILQ_ENTRY(ta_rt_nexthop_t) links;           /**< Queue links. */

    uint16_t                flags;                /**< Flags (see above). */
    struct sockaddr_storage gw;                   /**< Gateway address. */
    char                    ifname[IF_NAMESIZE];  /**< Interface name. */
    unsigned int            weight;               /**< Weight. */
    unsigned int            id;                   /**< Internal ID, used
                                                       when editing existing
                                                       route. */
} ta_rt_nexthop_t;

/** Queue head type for nexthops of a multipath route. */
typedef TAILQ_HEAD(ta_rt_nexthops_t, ta_rt_nexthop_t) ta_rt_nexthops_t;

/**
 * Remove all elements from a queue of route nexthops, release memory
 * occupied by them.
 *
 * @param hops      Head of nexthops queue.
 */
extern void ta_rt_nexthops_clean(ta_rt_nexthops_t *hops);

/** Structure that keeps system independent representation of the route */
typedef struct ta_rt_info_t {
    struct sockaddr_storage dst; /**< Route destination address */
    struct sockaddr_storage src; /**< Route source address */
    uint32_t                prefix; /**< Route destination address prefix */
    /** Gateway address - for indirect routes */
    struct sockaddr_storage gw;

    ta_rt_nexthops_t        nexthops; /**< Nexthops of a multipath route. */

    /** Interface name - for direct routes */
    char                    ifname[IF_NAMESIZE];
    uint16_t                flags;      /**< Route flags */
    uint32_t                metric;     /**< Route metric */
    uint32_t                mtu;        /**< Route MTU */
    uint32_t                win;        /**< Route window size */
    uint32_t                irtt;       /**< Route transfer time */
    uint32_t                hoplimit;   /**< Route hoplimit */
    uint32_t                tos;        /**< Route type of service */
    ta_route_type           type;       /**< Route type (e.g. unicast) */
    uint32_t                table;      /**< Route table id */
} ta_rt_info_t;

/**
 * Release dynamic memory allocated for fields of ta_rt_info_t structure,
 * clear its fields.
 *
 * @param rt_info     Pointer to ta_rt_info_t structure.
 */
extern void ta_rt_info_clean(ta_rt_info_t *rt_info);

/** Gateway address is specified for the route */
#define TA_RT_INFO_FLG_GW     0x0001
/** Interface name is specified for the route */
#define TA_RT_INFO_FLG_IF     0x0002
/** Metric is specified for the route */
#define TA_RT_INFO_FLG_METRIC 0x0004
/** MTU is specified for the route */
#define TA_RT_INFO_FLG_MTU    0x0008
/** Window size is specified for the route */
#define TA_RT_INFO_FLG_WIN    0x0010
/** IRTT is specified for the route */
#define TA_RT_INFO_FLG_IRTT   0x0020
/** TOS is specified for the route */
#define TA_RT_INFO_FLG_TOS    0x0040
/** Source address is specified for the route */
#define TA_RT_INFO_FLG_SRC    0x0080
/** Table ID is specified for the route */
#define TA_RT_INFO_FLG_TABLE  0x0100
/** Route is multipath */
#define TA_RT_INFO_FLG_MULTIPATH  0x0200
/** hoplimit is specified for the route */
#define TA_RT_INFO_FLG_HOPLIMIT   0x0400

/**
 * Initialize ta_rt_info_t data structure.
 *
 * @param type     Which route type to set for @p rt_info
 * @param rt_info  Routing information data structure to initialize
 *
 * @return N/A
 */
static inline void
ta_rt_info_init(ta_route_type type, ta_rt_info_t *rt_info)
{
    memset(rt_info, 0, sizeof(*rt_info));
    rt_info->type = type;
}

/**
 * Parses route instance name and fills in a part of rt_info 
 * data structure.
 *
 * @param name     Route instance name
 * @param rt_info  Routing information data structure (OUT)
 */
extern int ta_rt_parse_inst_name(const char *name, ta_rt_info_t *rt_info);

/**
 * Parses route instance value to rt_info data structure.
 *
 * @param value    Value of the instance
 * @param rt_info  Routing information data structure (OUT)
 */
extern int ta_rt_parse_inst_value(const char *value, ta_rt_info_t *rt_info);

/**
 * Parses route object attributes and fills in a part of rt_info 
 * data structure.
 *
 * @param attrs    Route attributes
 * @param rt_info  Routing information data structure (OUT)
 */
extern int ta_rt_parse_attrs(ta_cfg_obj_attr_t *attrs,
                             ta_rt_info_t *rt_info);

/**
 * Parses route object info to rt_info data structure.
 *
 * @param obj      Route object
 * @param rt_info  Routing information data structure (OUT)
 */
extern int ta_rt_parse_obj(ta_cfg_obj_t *obj, ta_rt_info_t *rt_info);


/* Resource handling */

/*
 * Create a lock for the resource with specified name.
 *
 * @param name                  resource name
 * @param[in,out] shared        create lock in shared (@c TRUE) or exclusive
 *                              (@c FALSE) mode. On success contains actual
 *                              mode of the acquired lock.
 * @param fallback_shared       Should the attempt to acquire lock in shared
 *                              mode be taken after unsuccessful attempts
 *                              to acquire the lock in exclusive mode time out.
 * @param attempts_timeout_ms   Timeout of attempts to acquire the lock in
 *                              milliseconds.
 *
 * @return Status code
 */
extern te_errno ta_rsrc_create_lock(const char *name, te_bool *shared,
                                    te_bool fallback_shared,
                                    unsigned int attempts_timeout_ms);

/*
 * Delete a lock for the resource with specified name.
 *
 * @param name     resource name
 */
void ta_rsrc_delete_lock(const char *name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_TA_CFG_H__ */
