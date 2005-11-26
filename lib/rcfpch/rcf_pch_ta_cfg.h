/** @file 
 * @brief Common library for test agents which is very useful
 * in supporting read-create instances implementing "commit" operation.
 *
 * Test Agent configuration library
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_PCH_TA_CFG_H__
#define __TE_RCF_PCH_TA_CFG_H__

#include "te_defs.h"
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
 *    but this might not be what we want (sequense of ADD, SET operations
 *    is not the same as atomic ADD with all specified values).
 *    In this circumstance we should consider the second way of creating
 *    object instances;
 * 2. Perform local ADD operation, a set of local SET operations,
 *    and finally COMMIT operation.
 *    In this case we accumulate all the values in so called cache, and
 *    install it as a bulk in COMMIT.
 *
 * Which way to chose depends on the implementation of Test Agent,
 * because from programmer's point of view the only difference in this 
 * to ways is that to support the second approach they should provide 
 * callback function for COMMIT operation, and save all the information 
 * obtained in ADD and SET callbacks in some local data structures.
 *
 * This library plays a roule of such local data structure where all the
 * information can be locally stored (in ADD, or SET callbacks) and 
 * then extracted (in COMMIT callback).
 *
 * How to work with it:
 * The library uses term "object", which has no in common with term object
 * defined in Configurator context.
 * Each object is associated with a patricular type (a constant string)
 * and a name (also a constant string). These two strings is a key for
 * objects, which means that you can identify each object with this pair.
 * Each object can keep a set of attributes with their values.
 * Attribute is identified by name (a constant string), and its value is
 * also a constant string.
 * In addition, each object keeps "action" value, which defines the type of
 * operation that should be performed with it (in most of the cases this
 * value is used in commit callback to define what to do - whether to add
 * a new instnace, delete an existing instance, or just update some 
 * attributes of an existing instance).
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef IF_NAMESIZE
#define IF_NAMESIZE     16
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
} ta_cfg_obj_action_e;

/** Object data structure, which is inserted into collection */
typedef struct ta_cfg_obj {
    te_bool  in_use; /**< Whether this entry is in use */
    char    *type; /**< Object type in string representation */
    char    *name; /**< Object name - actually, instance name */
    char    *value; /**< Object value - actually, instance value */
    void    *user_data; /**< Pointer to user-provided data */

    ta_cfg_obj_action_e  action; /**< Object action */
    ta_cfg_obj_attr_t   *attrs; /**< List of object's attributes */
} ta_cfg_obj_t;

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
 *
 * @return Error code or 0
 */
extern ta_cfg_obj_t *ta_obj_find(const char *type, const char *name);

/**
 * Create an object of specified type with particular value.
 *
 * @param type        Object type - user-defined constant
 * @param name        Object name - actually, instance name
 * @param value       Object value
 * @param user_data   Some user-data value associated with this object
 * @param new_obj     Object entry (OUT)
 *
 * @return Error code or 0
 */
extern int ta_obj_add(const char *type,
                      const char *name, const char *value,
                      void *user_data, ta_cfg_obj_t **new_obj);

/**
 * Set the value of the object.
 *
 * @param type       Object type - user-defined constant
 * @param name       Object name
 * @param value      Object value
 */
extern int ta_obj_value_set(const char *type, const char *name,
                            const char *value); 

/** Prototype for callback function used in ta_obj_set() */
typedef int (* ta_obj_set_cb)(ta_cfg_obj_t *obj);

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
 * @param cb_func     Callback function to be called if SET operation
 *                    leads to creating a new object in collection
 *
 * @return Error code or 0
 */
extern int ta_obj_set(const char *type, const char *name,
                      const char *attr_name, const char *attr_value,
                      ta_obj_set_cb cb_func);

/**
 * Create an object of specified type and mark it as deleted.
 *
 * @param type        Object type - user-defined constant
 * @param name        Object name - actually, instance name
 * @param user_data   Some user-data value associated with this object
 *
 * @return Error code or 0
 */
extern int ta_obj_del(const char *type, const char *name, void *user_data);

/* Route-specific definitions */

/** Name for object of type route */
#define TA_OBJ_TYPE_ROUTE "route"

/** Structure that keeps system independent representation of the route */
typedef struct ta_rt_info {
    struct sockaddr_storage dst; /**< Route destination address */
    uint32_t                prefix; /**< Route destination address prefix */
    /** Gateway address - for indirect routes */
    struct sockaddr_storage gw;
    /** Interface name - for direct routes */
    char                    ifname[IF_NAMESIZE];
    uint16_t                flags;  /**< Route flags */
    uint32_t                metric; /**< Route metric */
    uint32_t                mtu;    /**< Route MTU */
    uint32_t                win;    /**< Route window size */
    uint32_t                irtt;   /**< Route transfer time */
    uint32_t                tos;    /**< Route type of service */
} ta_rt_info_t;

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_TA_CFG_H__ */
