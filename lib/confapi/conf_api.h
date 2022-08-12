/** @file
 * @brief Configurator API
 *
 * Configurator API exports functionality that makes it possible to:
 * - @cmd{Register}/@cmd{Unregister} objects of configuration tree;
 * - traverse a configuration tree (get parent, neighbor and child nodes);
 * - @cmd{Add} new object instances;
 * - @cmd{Delete} existing object instances;
 * - change the value of an object instance;
 * - @cmd{Get} the value of an object instace.
 * .
 */

/**
 * @copyright
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
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
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_errno.h"
#include "conf_types.h"
#include "cs_common.h"
#include "conf_oid.h"
#include "te_kvpair.h"
#include "rcf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup confapi_base API: Configurator
 * @ingroup confapi
 * @{
 */

/** Cast to Configurator Instance Value Pointer */
#define CFG_IVP(x)     ((cfg_inst_val *)(x))

/**
 * Macro definition to be used in cfg_{add,set}_instance_fmp() calls.
 * Here it is assumed that 'long' type has the same size as 'void *'.
 */
#define CFG_VAL(_t, _v)     CVT_##_t, ((const void *)(long)(_v))


/** Constants for primary types */
typedef enum {
    CVT_INTEGER,     /**< Value of the type 'int' */
    CVT_UINT64,      /**< Value of the type 'uint64_t' */
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

/** @defgroup confapi_base_traverse Configuration tree traversal
 * @ingroup confapi_base
 * @{
 */

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_register_object_str(const char    *oid,
                                        cfg_obj_descr *descr,
                                        cfg_handle    *handle);

/**
 * Register new object using array object identifiers.
 *
 * @param oid       object identifier
 * @param descr     object properties description
 * @param handle    location for handle of the new object
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_register_object(const cfg_oid *oid,
                                    cfg_obj_descr *descr,
                                    cfg_handle    *handle);

/**
 * The same function as cfg_register_object_str,
 * but OID may be format string.
 */
extern te_errno cfg_register_object_fmt(cfg_obj_descr *descr,
                                        cfg_handle *handle,
                                        const char *oid_fmt, ...)
                                        __attribute__((format(printf, 3, 4)));


/**
 * Request Configurator to remove an object with a given id from
 * the data base of objects.
 *
 * @param id_fmt            a format string for the object id.
 *
 * @return                  Status code.
 */
extern te_errno cfg_unregister_object_str(const char *id_fmt, ...);


/**
 * Obtain parameters configured for the object.
 *
 * @param handle    object handle
 * @param descr     OUT: location for the object properties description
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_get_object_descr(cfg_handle     handle,
                                     cfg_obj_descr *descr);

/**
 * Obtain identifier of object or object instance by its handle.
 *
 * @param handle    handle of object or object instance
 * @param oid       OUT: location for the oid pointer (memory for the
 *                  string is allocated by the routine using malloc()
 *
 * @return 0 or TE_EINVAL if invalid handle is provided
 */
extern te_errno cfg_get_oid_str(cfg_handle handle, char **oid);

/**
 * Obtain identifier of object or object instance by its handle.
 *
 * @param handle    handle of object or object instance
 * @param oid       OUT: location for the oid pointer (memory for the
 *                    array is allocated by the routine using malloc()
 *
 * @return 0 or TE_EINVAL if invalid handle is provided
 */
extern te_errno cfg_get_oid(cfg_handle handle, cfg_oid **oid);

/**
 * Obtain sub-identifier of object by its handle.
 *
 * @param handle    handle of object
 * @param subid     OUT: location for the sub-identifier (should be
 *                  at least CFG_SUBID_MAX length)
 *
 * @return 0 or TE_EINVAL if invalid handle is provided
 */
extern te_errno cfg_get_subid(cfg_handle handle, char **subid);

/**
 * Obtain name of object instance by its handle.
 *
 * @param handle    handle of object instance
 * @param name      OUT: location for the name
 *
 * @return 0 or TE_EINVAL if invalid handle is provided
 */
extern te_errno cfg_get_inst_name(cfg_handle handle, char **name);

/**
 * Obtain integer name of object instance by its handle.
 *
 * @param handle    Handle of object instance
 * @param type      Type instance name
 * @param val       type == CVT_INTEGER -> (int *)
 *                  type == CVT_UINT64 -> (uint64_t *)
 *                  type == CVT_ADDRESS -> (struct sockaddr **)
 *
 * @return 0 or TE_EINVAL if invalid handle is provided
 */
extern te_errno cfg_get_inst_name_type(cfg_handle    handle,
                                       cfg_val_type  type,
                                       cfg_inst_val *val);

/**
 * Obtain name of object instance by its handle.
 *
 * @param str_oid   object instance identifier in string
 * @param i         index of the instance subid
 * @param name      OUT: location for the name
 *
 * @return 0 or TE_EINVAL if invalid handle is provided
 */
extern te_errno cfg_get_ith_inst_name(const char    *str_oid,
                                      unsigned int   i,
                                      char         **name);

/**
 * Find the object or object instance by its object identifier.
 * Root object identifier is "/"; root object instance identifier is "/:".
 *
 * @param oid       object identifier in string representation
 * @param p_handle  location for object or instance handle
 *
 * @return      Status code
 */
extern te_errno cfg_find_str(const char *oid, cfg_handle *p_handle);

/** The same function as cfg_find_str, but OID may be format string */
static inline te_errno
cfg_find_vfmt(cfg_handle *p_handle, const char *oid_fmt, va_list ap)
{
    char oid[CFG_OID_MAX];

    vsnprintf(oid, sizeof(oid), oid_fmt, ap);

    return cfg_find_str(oid, p_handle);
}

/** The same function as cfg_find_str, but OID may be format string */
extern te_errno cfg_find_fmt(cfg_handle *p_handle, const char *oid_fmt, ...)
                             __attribute__((format(printf, 2, 3)));


/**
 * Find the object or object instance by its object identifier.
 *
 * @param oid       object identifier
 * @param handle    location for object or instance handle
 *
 * @return Status code
 */
extern te_errno cfg_find(const cfg_oid *oid, cfg_handle *handle);

/**
 * Find object of the given instance.
 *
 * @param instance  instance handle
 * @param object    location for object handle
 *
 * @return Status code
 */
extern te_errno cfg_find_object_by_instance(cfg_handle  instance,
                                            cfg_handle *object);

/**
 * Find all objects or object instances matching a pattern.
 *
 * @param pattern       string object identifier possibly containing '*'
 *                      (see Configurator documentation for details)
 * @param p_nmatches    OUT: number of found objects or object instances
 * @param p_matches     OUT: array of object/(object instance) handles;
 *                      memory for the array is allocated using malloc()
 *
 * @return  0 or
 *          TE_EINVAL if a pattern format is incorrect or
 *                    some argument is NULL.
 */
extern te_errno cfg_find_pattern(const char    *pattern,
                                 unsigned int  *p_num,
                                 cfg_handle    **p_set);

/** The same function as cfg_find_pattern, but OID may be format string */
extern te_errno cfg_find_pattern_fmt(unsigned int *p_num, cfg_handle **p_set,
                                     const char *ptrn_fmt, ...)
                                     __attribute__((format(printf, 3, 4)));

/**
 * Type of callback function which can passed to cfg_find_pattern_iter_fmt().
 *
 * @param handle    Configurator object or instance handle
 * @param opaque    Opaque data
 *
 * @return Status code
 */
typedef te_errno (*cfg_handle_cb_func)(cfg_handle handle, void *opaque);

/**
 * Find all objects or object instances matching a pattern and call
 * @p cb_func for each object.
 *
 * @param cb_func   Pointer to a callback function.
 * @param opaque    Opaque argument to pass @p cb_func.
 * @param ptrn_fmt  Foramt string with the pattern.
 *
 * @return Status code
 */
extern te_errno cfg_find_pattern_iter_fmt(cfg_handle_cb_func cb_func,
                                          void *opaque,
                                          const char *ptrn_fmt, ...)
                                     __attribute__((format(printf, 3, 4)));

/**
 * Get handle of the oldest son of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param son       pointer to oldest son of the object or object instance.
 *
 * @returns Status code
 */
extern te_errno cfg_get_son(cfg_handle handle, cfg_handle *son);

/**
 * Get handle of the brother of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param brother   pointer to the brother's handle
 *
 * @returns Status code
 */
extern te_errno cfg_get_brother(cfg_handle handle, cfg_handle *brother);

/**
 * Get handle of the father of the object or object instance.
 *
 * @param handle    handle of the object or object instance
 * @param father    pointer to the father's handle
 *
 * @returns Status code
 */
extern te_errno cfg_get_father(cfg_handle handle, cfg_handle *father);
/**@}*/

/** @defgroup confapi_base_access Contriguration tree access operations
 * @ingroup confapi_base
 * @{
 */

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_add_instance(const cfg_oid *oid, cfg_handle *handle,
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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_add_instance_str(const char *oid, cfg_handle *p_handle,
                                     cfg_val_type type, ...);

/**
 * The same function as cfg_add_instance_str,
 * but OID may be format string.
 *
 * Use macro CFG_VAL() to make the second and the third arguments pair.
 * E.g. rc = cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 1), "/hello:tom");
 */
extern te_errno cfg_add_instance_fmt(cfg_handle *p_handle, cfg_val_type type,
                                     const void *val, const char *oid_fmt, ...)
                                     __attribute__((format(printf, 4, 5)));


/**
 * Create an object instance locally. Commit should be called to
 * propagate this creation as a bulk to the Test Agent later.
 *
 * @param  oid      object identifier of the new instance
 * @param  handle   location for handle of the new instance
 * @param  type     value type (necessary for fast processing)
 * @param  ...      value to be assigned to the new instance or NULL;
 *                    for integer values: int
 *                    for strings: char *
 *                    for addreses: struct sockaddr *
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_add_instance_local(const cfg_oid *oid,
                                       cfg_handle    *handle,
                                       cfg_val_type   type, ...);

/**
 * The same function as cfg_add_instance_local, but OID is in string format.
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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_add_instance_local_str(const char   *oid,
                                           cfg_handle   *p_handle,
                                           cfg_val_type  type, ...);

/**
 * The same function as cfg_add_instance_local_str,
 * but OID may be format string.
 *
 * Use macro CFG_VAL() to make the second and the third arguments pair.
 * E.g. rc = cfg_add_instance_local_fmt(NULL, CFG_VAL(INTEGER, 1),
 *                                      "/hello:tom");
 */
extern te_errno cfg_add_instance_local_fmt(cfg_handle *p_handle,
                                           cfg_val_type type,
                                           const void *val,
                                           const char *oid_fmt, ...)
                                        __attribute__((format(printf, 4, 5)));

/**
 * Add instance with the first part of OID specified by handle and
 * the second part specified by format string and its parameters.
 *
 * @param p_handle      Location for handle of new instance
 * @param type          Type of value
 * @param val           Value
 * @param parent        Handle of the parent
 * @param suboid_fmt    Sub-identifier format string
 * @param ...           Format string arguments
 *
 * @return Status code
 */
extern te_errno cfg_add_instance_child_fmt(cfg_handle *p_handle,
                                           cfg_val_type type,
                                           const void *val,
                                           cfg_handle parent,
                                           const char *suboid_fmt, ...)
                                        __attribute__((format(printf, 5, 6)));

/**
 * Delete an object instance.
 *
 * @param handle            object instance handle
 * @param with_children     delete the children subtree, if necessary
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_del_instance(cfg_handle handle, te_bool with_children);

/** Set instance by the OID. OID may be format string */
extern te_errno cfg_del_instance_fmt(te_bool with_children,
                                     const char *oid_fmt, ...)
                                     __attribute__((format(printf, 2, 3)));

/**
 * Delete an object instance locally. Commit should be called to propagate
 * this change together with other not committed local changes to TA.
 *
 * @param handle                Object instance handle.
 * @param with_children         Delete the children subtree, if necessary.
 *
 * @return Status code (see te_errno.h).
 */
extern te_errno cfg_del_instance_local(cfg_handle handle,
                                       te_bool with_children);

/**
 * Same as @b cfg_del_instance_local(), but accepts OID string.
 *
 * @param with_children         Delete the children subtree, if necessary.
 * @param oid_fmt               Format string for OID.
 * @param ...                   Format string arguments.
 *
 * @return Status code (see te_errno.h).
 */
extern te_errno cfg_del_instance_local_fmt(te_bool with_children,
                                           const char *oid_fmt, ...)
                                     __attribute__((format(printf, 2, 3)));

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_set_instance(cfg_handle handle, cfg_val_type type, ...);

/**
 * Set instance by the OID. OID may be format string.
 *
 * Use macro CFG_VAL() to make the first and the second arguments pair.
 * E.g. rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1), "/hello:");
 */
extern te_errno cfg_set_instance_fmt(cfg_val_type type, const void *val,
                                     const char *oid_fmt, ...)
                                     __attribute__((format(printf, 3, 4)));

/**
 * Set instance by the OID string.
 *
 * Use macro CFG_VAL() to make the first and the second arguments pair.
 * E.g. rc = cfg_set_instance_str(CFG_VAL(INTEGER, 1), "/hello:");
 *
 * @param type      value type (necessary for fast processing)
 * @param val       value to set
 * @param oid       OID string
 *
 * @return Status code.
 */
extern te_errno cfg_set_instance_str(cfg_val_type type, const void *val,
                                     const char *oid);

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_set_instance_local(cfg_handle handle,
                                       cfg_val_type type, ...);

/** Set instance by the OID. OID may be format string */
extern te_errno cfg_set_instance_local_fmt(cfg_val_type type, const void *val,
                                           const char *oid_fmt, ...)
                                        __attribute__((format(printf, 3, 4)));

/**
 * Commit Configurator database changes to the Test Agent.
 *
 * @param oid       subtree object identifier or NULL if whole
 *                  database should be synchronized
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_commit(const char *oid);

/** The same function as cfg_commit, but OID may be format string */
extern te_errno cfg_commit_fmt(const char *oid_fmt, ...)
                               __attribute__((format(printf, 1, 2)));

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_get_instance(cfg_handle handle,
                                 cfg_val_type *type, ...);

/** Get instance by the OID. OID may be format string */
extern te_errno cfg_get_instance_fmt(cfg_val_type *p_type, void *val,
                                     const char *oid_fmt, ...)
                                     __attribute__((format(printf, 3, 4)));

/** Type-safe version of cfg_get_instance_fmt() for integer values */
extern te_errno cfg_get_instance_int_fmt(int *val, const char *oid_fmt, ...)
                                         __attribute__((format(printf, 2, 3)));

/** Type-safe version of cfg_get_instance_fmt() for values of uint64_t type */
extern te_errno cfg_get_instance_uint64_fmt(uint64_t *val,
                                            const char *oid_fmt, ...)
                                        __attribute__((format(printf, 2, 3)));

/** Type-safe version of cfg_get_instance_fmt() for string values */
extern te_errno cfg_get_instance_string_fmt(char **val,
                                            const char *oid_fmt, ...)
                                          __attribute__((format(printf, 2, 3)));

/**
 * Type-safe version of cfg_get_instance_fmt() for values of
 * struct sockaddr type
 */
extern te_errno cfg_get_instance_addr_fmt(struct sockaddr **val,
                                          const char *oid_fmt, ...)
                                        __attribute__((format(printf, 2, 3)));

/**
 * Get instance by the OID string.
 *
 * @param p_type    location for value type, may be NULL
 * @param val       location for value
 * @param oid       OID string
 *
 * @return Status code.
 */
extern te_errno cfg_get_instance_str(cfg_val_type *p_type, void *val,
                                     const char *oid);

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_get_instance_sync(cfg_handle handle,
                                      cfg_val_type *type, ...);

/** Get instance by the OID. OID may be format string */
extern te_errno cfg_get_instance_sync_fmt(cfg_val_type *type, void *val,
                                          const char *oid_fmt, ...)
                                          __attribute__((format(printf, 3, 4)));

/** Type-safe version of cfg_get_instance_sync_fmt() for integer values */
extern te_errno cfg_get_instance_int_sync_fmt(int *val,
                                              const char *oid_fmt, ...)
                                        __attribute__((format(printf, 2, 3)));

/**
 * Type-safe version of cfg_get_instance_sync_fmt() for values of uint64_t type
 */
extern te_errno cfg_get_instance_uint64_sync_fmt(uint64_t *val,
                                                 const char *oid_fmt, ...)
                                        __attribute__((format(printf, 2, 3)));

/** Type-safe version of cfg_get_instance_sync_fmt() for string values */
extern te_errno cfg_get_instance_string_sync_fmt(char **val,
                                                 const char *oid_fmt, ...)
                                        __attribute__((format(printf, 2, 3)));

/**
 * Type-safe version of cfg_get_instance_sync_fmt() for values of
 * struct sockaddr type
 */
extern te_errno cfg_get_instance_addr_sync_fmt(struct sockaddr **val,
                                               const char *oid_fmt, ...)
                                        __attribute__((format(printf, 2, 3)));

/**@}*/

/** @defgroup confapi_base_sync Synchronization configuration tree with Test Agent
 * @ingroup confapi_base
 * @{
 */

/**
 * Synchronize Configurator database with managed objects.
 *
 * @param oid        identifier of the object instance or subtree
 * @param subtree    1 if the subtree of the specified node should
 *                   be synchronized
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_synchronize(const char *oid, te_bool subtree);

/** The same function as cfg_synchronize, but OID may be format string */
extern te_errno cfg_synchronize_fmt(te_bool subtree, const char *oid_fmt, ...)
                                    __attribute__((format(printf, 2, 3)));

/**@}*/

/** @addtogroup confapi_base_traverse
 * @{
 */

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
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_enumerate(cfg_handle handle, cfg_inst_handler callback,
                              void *user_data);
/**@}*/

/** @defgroup confapi_base_reboot Test Agent reboot
 * @ingroup confapi_base
 * @{
 */

/**
 * Reboot the Test Agent.
 *
 * @param ta_name      name of the Test Agent
 * @param restore      if TRUE, restore the current configuration
 * @param reboot_type  type of TA reboot
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_reboot_ta(const char *ta_name, te_bool restore,
                              rcf_reboot_type reboot_type);

/**@}*/

/** @defgroup confapi_base_backup Configuration backup manipulation
 * @ingroup confapi_base
 * @{
 */

/**
 * Create a backup.
 *
 * @param name      OUT: location for backup file name
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_create_backup(char **name);

/**
 * Verify the backup.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return Status code (see te_errno.h)
 * @retval 0            current configuration is equal to backup
 * @retval TE_EBACKUP   current configuration differs from backup
 */
extern te_errno cfg_verify_backup(const char *name);

/**
 * Restore the backup.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_restore_backup(const char *name);

/**
 * Restore the backup w/o processing history.
 *
 * @param name      name returned by cfg_create_backup
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_restore_backup_nohistory(const char *name);


/**
 * Ask Configurator to forget about the backup, if known.
 *
 * @param name      Location of name returned by cfg_create_backup
 *                  (set to NULL on success)
 *
 * @return Status code
 */
extern te_errno cfg_release_backup(char **name);


/**
 * Create a configuration file.
 *
 * @param name      configuration file name
 * @param history   if TRUE "history" configuration file is created;
 *                    otherwise "backup" configuration file is created
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_create_config(const char *name, te_bool history);

/**
 * Process a history configuration file.
 *
 * @param filename      history configuratin file name
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_process_history(const char *filename,
                                    const te_kvpair_h *expand_vars);

/**@}*/

/**
 * Macro to call cfg_wait_changes() from the test without check of
 * return value.
 */
#define CFG_WAIT_CHANGES    ((void)cfg_wait_changes())

/**
 * Wait for Configuration changes propagation.
 *
 * Required delays are configured using /conf_delay subtree.
 * Time to sleep is calculated as the maximum of required delays for
 * configuration changes done after the previous wait (regardless how
 * long time ago the changes are done).
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_wait_changes(void);

/**
 * Notify the Configurator that instances matching OID template are
 * touched by non-CS means (necesary for subsequent correct
 * cfg_wait_changes() processing).
 *
 * @param oid_tmpl      instance identifier format string (may contain
 *                      '*' symbols)
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno cfg_touch_instance(const char *oid_tmpl, ...);

/**
 * Starting from a given prefix, print a tree of objects or instances
 * into a file and(or) log.
 *
 * @param filename          output filename (NULL to skip)
 * @param log_lvl           TE log level (0 to skip)
 * @param id_fmt            a format string for the id of the root
 *                          from which we print.
 *
 * @return                  Status code.
 */
extern te_errno cfg_tree_print(const char *filename,
                               const unsigned int log_lvl,
                               const char *id_fmt, ...);

/**
 * Clean up resources allocated by Configurator API.
 *
 * Usually user should not worry about calling of the function, since
 * it is called automatically using atexit() mechanism.
 */
extern void cfg_api_cleanup(void);

/**
 * Copy a subtree pointed to by its OID format string to a
 * destination pointed to by OID of the tree to be created.
 *
 * @param dst_oid     Destination tree OID
 * @param src_oid_fmt Format string of the source tree OID
 *
 * @return            Status code.
 */
extern te_errno cfg_copy_subtree_fmt(const char *dst_oid,
                                     const char *src_oid_fmt,
                                     ...)
                __attribute__((format(printf, 2, 3)));

/**@}*/

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_API_H__ */

