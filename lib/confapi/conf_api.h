/** @file 
 * @brief Configurator
 *
 * Exported API
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
  

#ifndef __TE_CONF_API_H__
#define __TE_CONF_API_H__

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "conf_types.h"
#include "conf_oid.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Root name for volatile objects */
#define CFG_VOLATILE    "volatile"


/** Cast to Configurator Instance Value Pointer */
#define CFG_IVP(x)     ((cfg_inst_val *)(x))

/** 
 * Macro definition to be used in cfg_{add,set}_instance_fmp() calls.
 * Here it is assumed that 'long' type has the same size as 'void *'.
 */
#define CFG_VAL(_t, _v)     CVT_##_t, ((const void *)(long)(_v))


/**
 * This macro MUST NOT be used out of the header.  It's very context
 * specific and used in inline functions defined in the header.  It
 * assumes and provides some variables.
 */
#define _CFG_HANDLE_BY_FMT \
    va_list    ap;                              \
    int        rc;                              \
    cfg_handle handle;                          \
                                                \
    va_start(ap, oid_fmt);                      \
    rc = cfg_find_vfmt(&handle, oid_fmt, ap);   \
    va_end(ap);                                 \
    if (rc != 0)                                \
        return rc;


/** Constants for primary types */
typedef enum {
    CVT_INTEGER,     /**< Value of the type 'int' */
    CVT_STRING,      /**< Value of the type 'char *' */
    CVT_ADDRESS,     /**< Value of the type 'sockaddr *' */
    CVT_NONE,        /**< The object instance has no value */
    CVT_UNSPECIFIED  /**< The type is unknown */
} cfg_val_type;


/**
 * Object and object instances handle.
 *
 * Object handle is an index in array of all objects.
 *
 * Object instance handle consists of two parts:
 * two most significant bytes contain unique number of the instance
 * and two other bytes contain index in the array of all existing
 * object instances.
 *
 * Invalid index is 0xFFFFFFFF.
 */
typedef uint32_t cfg_handle;

/** Invalid Configurator object handle */
#define CFG_HANDLE_INVALID      0xFFFFFFFF

/**
 * Is it Configurator object instance handle?
 *
 * @param _handle   Configurator handle
 */
#define CFG_IS_INST(_handle)    (_handle & 0xFFFF0000)

/** Object properties description */
typedef struct cfg_obj_descr {
    cfg_val_type type;        /**< Type of the object instance value */
    uint8_t      access;      /**< Access rights */

#define CFG_READ_ONLY   1 /**< Instance is created automatically by TA
                               and cannot be changed */
#define CFG_READ_WRITE  2 /**< Instance is created automatically by TA,
                               but its value can be changed */
#define CFG_READ_CREATE 3 /**< Instance may be added, changed and deleted */
    char        *def_val;     /**< Default value string or NULL */
} cfg_obj_descr;

/**
 * Register new object using string object identifiers.
 *
 * @param oid       object identifier in string representation
 * @param descr     object properties description
 * @param handle    location for handle of the new object
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_register_object_str(const char *oid, cfg_obj_descr *descr,
                                   cfg_handle *handle);

/**
 * Register new object using array object identifiers.
 *
 * @param oid       object identifier
 * @param descr     object properties description
 * @param handle    location for handle of the new object
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_register_object(const cfg_oid *oid, cfg_obj_descr *descr,
                               cfg_handle *handle);

/**
 * Obtain parameters configured for the object.
 *
 * @param handle    object handle
 * @param descr     OUT: location for the object properties description
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_get_object_descr(cfg_handle handle, cfg_obj_descr *descr);

/**
 * Obtain identifier of object or object instance by its handle.
 *
 * @param handle    handle of object or object instance
 * @param oid       OUT: location for the oid pointer (memory for the
 *                  string is allocated by the routine using malloc()
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
extern int cfg_get_oid_str(cfg_handle handle, char **oid);

/**
 * Obtain identifier of object or object instance by its handle.
 *
 * @param handle    handle of object or object instance
 * @param oid       OUT: location for the oid pointer (memory for the
 *                    array is allocated by the routine using malloc()
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
extern int cfg_get_oid(cfg_handle handle, cfg_oid **oid);

/**
 * Obtain sub-identifier of object by its handle.
 *
 * @param handle    handle of object
 * @param subid     OUT: location for the sub-identifier (should be
 *                  at least CFG_SUBID_MAX length)
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
extern int cfg_get_subid(cfg_handle handle, char **subid);

/**
 * Obtain name of object instance by its handle.
 *
 * @param handle    handle of object instance
 * @param name      OUT: location for the name
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
extern int cfg_get_inst_name(cfg_handle handle, char **name);

/**
 * Obtain integer name of object instance by its handle.
 *
 * @param handle    Handle of object instance
 * @param type      Type instance name
 * @param val       type == CVT_INTEGER -> (int *)
 *                  type == CVT_ADDRESS -> (struct sockaddr **)
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
extern int cfg_get_inst_name_type(cfg_handle handle, cfg_val_type type,
                                  cfg_inst_val *val);

/**
 * Obtain name of object instance by its handle.
 *
 * @param str_oid   object instance identifier in string
 * @param i         index of the instance subid
 * @param name      OUT: location for the name
 *
 * @return 0 or EINVAL if invalid handle is provided
 */
extern int cfg_get_ith_inst_name(const char *str_oid, unsigned int i,
                                 char **name);

/**
 * Find the object or object instance by its object identifier.
 * Root object identifier is "/"; root object instance identifier is "/:".
 *
 * @param oid       object identifier in string representation
 * @param p_handle  location for object or instance handle
 *
 * @return      status code
 */
extern int cfg_find_str(const char *oid, cfg_handle *p_handle);

/** The same function as cfg_find_str, but OID may be format string */
static inline int
cfg_find_vfmt(cfg_handle *p_handle, const char *oid_fmt, va_list ap)
{
    char oid[CFG_OID_MAX];

    vsnprintf(oid, sizeof(oid), oid_fmt, ap);

    return cfg_find_str(oid, p_handle);
}

/** The same function as cfg_find_str, but OID may be format string */
static inline int
cfg_find_fmt(cfg_handle *p_handle, const char *oid_fmt, ...)
{
    int     rc;
    va_list ap;

    va_start(ap, oid_fmt);
    rc = cfg_find_vfmt(p_handle, oid_fmt, ap);
    va_end(ap);

    return rc;
}

/**
 * Find the object or object instance by its object identifier.
 *
 * @param oid       object identifier
 * @param handle    location for object or instance handle
 *
 * @return status code
 */
extern int cfg_find(const cfg_oid *oid, cfg_handle *handle);

/**
 * Find object of the given instance.
 *
 * @param instance  instance handle
 * @param object    location for object handle
 *
 * @return status code.
 */
extern int cfg_find_object_by_instance(cfg_handle instance, 
                                       cfg_handle *object);

/**
 * Find all objects or object instances matching to pattern.
 *
 * @param pattern   string object identifier possibly containing '*'
 *                  (see Configurator documentation for details)
 * @param p_num     OUT: number of found objects or object instances
 * @param p_set     OUT: array of object/object instances handles;
 *                    memory for the array is allocated using malloc()
 *
 * @return 0 or EINVAL if pattern format is incorrect some argument is NULL
 */
extern int cfg_find_pattern(const char *pattern,
                            unsigned int *p_num, cfg_handle **p_set);

/** The same function as cfg_find_pattern, but OID may be format string */
static inline int
cfg_find_pattern_fmt(unsigned int *p_num, cfg_handle **p_set,
                     const char *ptrn_fmt, ...)
{
    va_list ap;
    char    ptrn[CFG_OID_MAX];

    va_start(ap, ptrn_fmt);
    vsnprintf(ptrn, sizeof(ptrn), ptrn_fmt, ap);
    va_end(ap);

    return cfg_find_pattern(ptrn, p_num, p_set);
}

/**
 * Get handle of the oldest son of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param son       pointer to oldest son of the object or object instance.
 *
 * @returns sun's handle or CFG_HANDLE_INVALID
 */
extern int cfg_get_son(cfg_handle handle, cfg_handle *son);

/**
 * Get handle of the brother of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param brother   pointer to the brother's handle
 *
 * @returns brother's handle or CFG_HANDLE_INVALID
 */
extern int cfg_get_brother(cfg_handle handle, cfg_handle *brother);

/**
 * Get handle of the father of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param father    pointer to the father's handle 
 *
 * @returns father's handle or CFG_HANDLE_INVALID
 */
extern int cfg_get_father(cfg_handle handle, cfg_handle *father);

/**
 * Create an object instance.
 *
 * @param  oid      object identifier of the new instance
 * @param  handle   location for handle of the new instance
 * @param  type     value type (necessary for fast processing)
 * @param  ...      value to be assigned to the new instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_add_instance(const cfg_oid *oid, cfg_handle *handle,
                            cfg_val_type type, ...);

/**
 * Create an object instance.
 *
 * @param  oid      object identifier of the new instance
 *                  (string representation)
 * @param  p_handle location for handle of the new instance (OUT)
 * @param  type     value type (necessary for fast processing)
 * @param  ...      value to be assigned to the new instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_add_instance_str(const char *oid, cfg_handle *p_handle,
                                cfg_val_type type, ...);

/**
 * The same function as cfg_add_instance_str,
 * but OID may be format string.
 */
static inline int
cfg_add_instance_fmt(cfg_handle *p_handle, cfg_val_type type,
                     const void *val, const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_add_instance_str(oid, p_handle, type, val);
}

/**
 * Add instance with the first part of OID specified by handle and
 * the second part specified by format string and its parameters.
 *
 * @param p_handle      Locatin for handle of new instance
 * @param type          Type of value
 * @param val           Value
 * @param parent        Handle of the parent
 * @param suboid_fmt    Sub-identifier format string
 * @param ...           Format string arguments
 *
 * @return Status code.
 */
static inline int
cfg_add_instance_child_fmt(cfg_handle *p_handle, cfg_val_type type,
                           const void *val, cfg_handle parent,
                           const char *suboid_fmt, ...)
{
    va_list     ap;
    int         rc;
    char       *parent_oid;
    char       *oid_fmt;
    char       *oid;


    if (suboid_fmt == NULL)
        return TE_RC(TE_CONF_API, EINVAL);

    rc = cfg_get_oid_str(parent, &parent_oid);
    if (rc != 0)
        return rc;
    assert(parent_oid != NULL);

    oid_fmt = malloc(CFG_OID_MAX);
    if (oid_fmt == NULL)
    {
        free(parent_oid);
        return TE_RC(TE_CONF_API, ENOMEM);
    }
    oid = malloc(CFG_OID_MAX);
    if (oid == NULL)
    {
        free(oid_fmt);
        free(parent_oid);
        return TE_RC(TE_CONF_API, ENOMEM);
    }

    snprintf(oid_fmt, CFG_OID_MAX, "%s%s", parent_oid, suboid_fmt);
    free(parent_oid);

    va_start(ap, suboid_fmt);
    vsnprintf(oid, CFG_OID_MAX, oid_fmt, ap);
    va_end(ap);

    free(oid_fmt);

    rc = cfg_add_instance_str(oid, p_handle, type, val);

    free(oid);

    return rc;
}

/**
 * Delete an object instance.
 *
 * @param handle            object instance handle
 * @param with_children     delete the children subtree, if necessary
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_del_instance(cfg_handle handle, te_bool with_children);

/** Set instance by the OID. OID may be format string */
static inline int
cfg_del_instance_fmt(te_bool with_children, const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_del_instance(handle, with_children);
}

/**
 * Change object instance value.
 *
 * @param handle    object instance handle
 * @param  type     value type (necessary for fast processing)
 * @param  ...      new value to be assigned to the instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_set_instance(cfg_handle handle, cfg_val_type type, ...);

/** Set instance by the OID. OID may be format string */
static inline int
cfg_set_instance_fmt(cfg_val_type type, const void *val,
                     const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_set_instance(handle, type, val);
}

/**
 * Change object instance value locally. Commit should be called to
 * propagate changed as a bulk to the Test Agent later.
 *
 * @param handle    object instance handle
 * @param type      value type (necessary for fast processing)
 * @param ...       new value to be assigned to the instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_set_instance_local(cfg_handle handle,
                                  cfg_val_type type, ...);

/** Set instance by the OID. OID may be format string */
static inline int
cfg_set_instance_local_fmt(cfg_val_type type, const void *val,
                           const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_set_instance_local(handle, type, val);
}

/**
 * Commit Configurator database changes to the Test Agent.
 *
 * @param oid       subtree object identifier or NULL if whole
 *                  database should be synchronized
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_commit(const char *oid);

/** The same function as cfg_commit, but OID may be format string */
static inline int
cfg_commit_fmt(const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_commit(oid);
}


/**
 * Obtain value of the object instance. Memory for strings and
 * addresses is allocated by the routine using malloc().
 *
 * @param handle    object instance handle
 * @param type      location for value type, may be NULL
 * @param ...       OUT: location for the value
 *                    for integer values: int *
 *                    for strings: char **
 *                    for addreses: struct sockaddr **
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_get_instance(cfg_handle handle, cfg_val_type *type, ...);

/** Get instance by the OID. OID may be format string */
static inline int
cfg_get_instance_fmt(cfg_val_type *p_type, void *val,
                     const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance(handle, p_type, val);
}

/**
 * Obtain value of the object instance with synchronization with
 * the managed object. Memory for strings and
 * addresses is allocated by the routine using malloc().
 *
 * @param handle    object instance handle
 * @param type      location for the value type
 * @param  ...      OUT: location for the value
 *                    for integer values: int *
 *                    for strings: char **
 *                    for addreses: struct sockaddr **
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_get_instance_sync(cfg_handle handle,
                                 cfg_val_type *type, ...);

/** Get instance by the OID. OID may be format string */
static inline int
cfg_get_instance_sync_fmt(cfg_val_type *type, void *val,
                          const char *oid_fmt, ...)
{
    _CFG_HANDLE_BY_FMT;
    return cfg_get_instance_sync(handle, type, val);
}


/**
 * Synchronize Configurator database with managed objects.
 *
 * @param oid        identifier of the object instance or subtree or
 *                   NULL if whole database should be synchronized
 * @param subtree    1 if the subtree of the specified node should
 *                   be synchronized
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_synchronize(const char *oid, te_bool subtree);

/** The same function as cfg_synchronize, but OID may be format string */
static inline int
cfg_synchronize_fmt(te_bool subtree, const char *oid_fmt, ...)
{
    va_list ap;
    char    oid[CFG_OID_MAX];

    va_start(ap, oid_fmt);
    vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    return cfg_synchronize(oid, subtree);
}


/** Function handler called during instances enumeration */
typedef int (* cfg_inst_handler)(
    cfg_handle handle,    /**< Handle of the object instance */
    void      *user_data  /**< Data specified by the user */
);

/**
 * Enumerate all instances of the object (enumeration is stopped if
 * callback returns non-zero).
 *
 * @param handle    object handle
 * @param callback  the function to be called for each object instance
 * @param user_data opaque user data
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_enumerate(cfg_handle handle, cfg_inst_handler callback,
                         void *user_data);


/**
 * Reboot the Test Agent.
 *
 * @param ta_name      name of the Test Agent
 * @param restore   if TRUE, restore the current configuration
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_reboot_ta(const char *ta_name, te_bool restore);


/**
 * Create a backup.
 *
 * @param name      OUT: location for backup file name
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_create_backup(char **name);

/**
 * Verify the backup.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return status code (see te_errno.h)
 * @retval 0            current configuration is equal to backup
 * @retval ETEBACKUP    current configuration differs from backup
 */
extern int cfg_verify_backup(const char *name);

/**
 * Restore the backup.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_restore_backup(const char *name);

/**
 * Ask Configurator to forget about the backup, if known.
 *
 * @param name      Location of name returned by cfg_create_backup
 *                  (set to NULL on success)
 *
 * @return status code
 */
extern int cfg_release_backup(char **name);


/**
 * Create a configuration file.
 *
 * @param name      configuration file name
 * @param history   if TRUE "history" configuration file is created;
 *                    otherwise "backup" configuration file is created
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_create_config(const char *name, te_bool history);

/**
 * Clean up resources allocated by Configurator API.
 *
 * Usually user should not worry about calling of the function, since
 * it is called automatically using atexit() mechanism.
 */
extern void cfg_api_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_API_H__ */

