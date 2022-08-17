/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Configurator database definitions
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_DB_H__
#define __TE_CONF_DB_H__

#include <stdint.h>

#include "te_defs.h"
#include "logger_ten.h"
#include "rcf_common.h"
#include "conf_api.h"
#include "conf_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_INST_HANDLE_TO_INDEX(_handle)    (_handle & 0xFFFF)

/** Configurator dependency item */
typedef struct cfg_dependency {
    struct cfg_object     *depends;
    te_bool                object_wide;
    struct cfg_dependency *next;
} cfg_dependency;

/* Configurator object */
typedef struct cfg_object {
    cfg_handle         handle;  /**< Handle of the object */
    char              *oid;     /**< OID of the object */
    char               subid[CFG_SUBID_MAX];
                                /**< Own sub-identifier of the object */
    cfg_val_type       type;    /**< Type of the object instance value */
    uint8_t            access;  /**< Access rights */
    char              *def_val; /**< Default value */
    te_bool            vol;     /**< The object is volatile */

    /** @name Family */
    struct cfg_object *father;  /**< Link to father */
    struct cfg_object *son;     /**< Link to the first son */
    struct cfg_object *brother; /**< Link to the next brother */
    /*@}*/

    /** @name Dependency tracking */
    unsigned               ordinal_number;
    /**< Ordinal number of this object in the topologically sorted list */
    struct cfg_dependency *depends_on;
                               /**< Objects this object depends on */
    struct cfg_dependency *dependants;
                               /**< Objects depending on this object */
    struct cfg_object     *dep_next; /**< The next object in topological
                                          order */
    struct cfg_object     *dep_prev; /**< The previous object in toplogical
                                          order */

    te_bool substitution;   /**< The object uses substitution */
} cfg_object;

#define CFG_DEP_INITIALIZER  0, NULL, NULL, NULL, NULL

extern cfg_object cfg_obj_root;
extern cfg_object **cfg_all_obj;
extern int cfg_all_obj_size;

/** Check if object is /agent */
static inline te_bool
cfg_object_agent(cfg_object *obj)
{
    return (obj->father == &cfg_obj_root) &&
           (strcmp(obj->subid, "agent") == 0);
}

#define CFG_OBJ_HANDLE_VALID(_handle) \
    (_handle < (uint32_t)cfg_all_obj_size && cfg_all_obj[_handle] != NULL)

#define CFG_GET_OBJ(_handle) \
    (CFG_OBJ_HANDLE_VALID(_handle) ? cfg_all_obj[_handle] : NULL)

/** Configurator object instance */
typedef struct cfg_instance {
    cfg_handle  handle;             /**< Handle of the instance */
    char       *oid;                /**< OID of the instance */
    char        name[CFG_INST_NAME_MAX];    /**< Own name of the instance */
    cfg_object *obj;                /**< Object of the instance */
    te_bool     added;              /**< Wheter this instance was added
                                         to the Test Agent or not
                                         (has sense only for read-create
                                         instances) */
    te_bool     remove;             /**< Whether this instance should be
                                         removed from TA when committing
                                         changes */

    /** @name Family */
    struct cfg_instance *father;    /**< Link to father */
    struct cfg_instance *son;       /**< Link to the first son */
    struct cfg_instance *brother;   /**< Link to the next brother */
    /*@}*/

    union  cfg_inst_val  val;
} cfg_instance;

extern cfg_instance cfg_inst_root;
extern cfg_instance **cfg_all_inst;
extern int cfg_all_inst_size;
extern uint16_t cfg_inst_seq_num;

#define CFG_TA_PREFIX   "/agent:"
#define CFG_RCF_PREFIX  "/rcf:"

/**
 * Check if instance is /agent:*
 *
 * @param inst  instance
 *
 * @return TRUE if instance is /agent:*
 */
static inline te_bool
cfg_inst_agent(cfg_instance *inst)
{
    return (inst->father == &cfg_inst_root) &&
           (strcmp(inst->obj->subid, "agent") == 0);
}

/**
 * Get name of the TA from /agent:xxx... object identifier.
 *
 * @param oid   instance identifier
 * @param ta    TA name location (RCF_MAX_NAME length)
 *
 * @return TRUE if name is extracted or FALSE if OID content is unexpected
 */
static inline te_bool
cfg_get_ta_name(const char *oid, char *ta)
{
    char *s = (char *)oid + strlen(CFG_TA_PREFIX);
    char *tmp;
    int   n;

    if (strcmp_start(CFG_TA_PREFIX, oid) != 0)
        return FALSE;

    n = ((tmp = strchr(s, '/')) == NULL) ? (int)strlen(s) : tmp - s;

    if (n >= RCF_MAX_NAME)
        return FALSE;

    memcpy(ta, s, n);
    ta[n] = 0;

    return TRUE;
}

#define CFG_INST_HANDLE_VALID(_handle) (                                \
    (_handle) != CFG_HANDLE_INVALID &&                                  \
    CFG_INST_HANDLE_TO_INDEX(_handle) < (uint32_t)cfg_all_inst_size &&  \
    cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(_handle)] != NULL &&          \
    cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(_handle)]->handle == _handle)

#define CFG_GET_INST(_handle) \
    (CFG_INST_HANDLE_VALID(_handle) ? \
     cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(_handle)] : NULL)

/*----------------- User request processing ----------------------------*/

/* SIze of the buffer requered for messages and responses except pattern */
#define CFG_BUF_LEN     (CFG_OID_MAX + sizeof(cfg_get_oid_msg))

/**
 * Process message with user request.
 *
 * @param msg   message pointer (it is assumed that message buffer
 *              length is long enough, for example for
 *              cfg_process_msg_get_oid it should be >= CFG_RECV_BUF_LEN)
 */
extern void cfg_process_msg_register(cfg_register_msg *msg);
extern void cfg_process_msg_find(cfg_find_msg *msg);
extern void cfg_process_msg_get_descr(cfg_get_descr_msg *msg);
extern void cfg_process_msg_get_oid(cfg_get_oid_msg *msg);
extern void cfg_process_msg_get_id(cfg_get_id_msg *msg);
extern void cfg_process_msg_family(cfg_family_msg *msg);
extern void cfg_process_msg_add_dependency(cfg_add_dependency_msg *msg);
extern void cfg_process_msg_tree_print(cfg_tree_print_msg *msg);
extern void cfg_process_msg_unregister(cfg_unregister_msg *msg);

/**
 * Process a user request to find all objects or object instances
 * matching a pattern.
 *
 * @param msg   message pointer
 *
 * @return message pointer of pointer to newly allocated message if the
 *         resulting message length > CFG_BUF_LEN
 */
extern cfg_pattern_msg *cfg_process_msg_pattern(cfg_pattern_msg *msg);

/*------------------------ DB operations --------------------------------*/

/**
 * Find object for specified instance object identifier.
 *
 * @param oid_s   object instance identifier in string representation
 *
 * @return object structure pointer or NULL
 */
extern cfg_object *cfg_get_object(const char *oid_s);

/**
 * Find object for specified object identifier.
 *
 * @param obj_id_str   object identifier in string representation
 *
 * @return pointer to object structure or NULL
 */
extern cfg_object *cfg_get_obj_by_obj_id_str(const char *obj_id_str);

/**
 * Find instance for specified instance identifier.
 *
 * @param ins_id_str   instance identifier in string representation
 *
 * @return pointer to instance structure or NULL
 */
extern cfg_instance *cfg_get_ins_by_ins_id_str(const char *ins_id_str);

/*
 * Add instance with given object for all existing parent instances
 *
 * @param obj Object
 *
 * @return Status code
 */
extern te_errno cfg_add_all_inst_by_obj(cfg_object *obj);

/**
 * Add instance to the database.
 *
 * @param oid_s         object instance identifier
 * @param handle        location for handle of the new instance
 * @param type          instance value type
 * @param val           value to be assigned to the instance
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_db_add(const char *oid_s, cfg_handle *handle,
                      cfg_val_type type, cfg_inst_val val);

/**
 * Delete instance from the database.
 *
 * @param handle        object instance handle
 */
extern void cfg_db_del(cfg_handle handle);

/**
 * Check that it's possible to delete instance from the database.
 *
 * @param handle        object instance handle
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_db_del_check(cfg_handle handle);

/**
 * Change instance value.
 *
 * @param handle        object instance handle
 * @param val           value to be assigned to the instance
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_db_set(cfg_handle handle, cfg_inst_val val);

/**
 * Get instance value.
 *
 * @param handle        object instance handle
 * @param val           location for the value
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_db_get(cfg_handle handle, cfg_inst_val *val);

/**
 * Find instance in the database.
 *
 * @param oid_s         object instance identifier
 * @param handle        location for found object or object instance
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_db_find(const char *oid_s, cfg_handle *handle);

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
extern te_errno cfg_db_find_pattern(const char *pattern,
                                    unsigned int *p_nmatches,
                                    cfg_handle **p_matches);
/**
 * Initialize the database during startup or re-initialization.
 *
 * @return 0 (success) or TE_ENOMEM
 */
extern int cfg_db_init(void);

/**
 * Destroy the database before shutdown.
 */
extern void cfg_db_destroy(void);

/**
 * Check if the object identifier (possibly wildcard) matches some
 * volatile object on the Test Agent.
 *
 * @param [in] oid_in       object identifier in string representation
 * @param [out]oid_out      the found object identifier in string
 *
 * @return TRUE (match) or FALSE (does not match)
 */
extern te_bool cfg_oid_match_volatile(const char *oid_s, char **oid_out);

/** Delay for configuration changes accomodation */
extern uint32_t cfg_conf_delay;

/**
 * Update the current configuration delay after adding/deleting/changing
 * an instance.
 *
 * @param oid   instance OID
 */
extern void cfg_conf_delay_update(const char *oid);

/** Sleep the delay and reset it */
static inline void
cfg_conf_delay_reset(void)
{
    if (cfg_conf_delay > 0)
    {
        RING("Sleep %u milliseconds to propagate configuration changes",
             cfg_conf_delay);
        usleep(cfg_conf_delay * 1000);
        cfg_conf_delay = 0;
    }
}

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
extern te_errno  cfg_db_tree_print(const char *filename,
                                   const unsigned int log_lvl,
                                   const char *id_fmt, ...);

/**
 * log_msg() helper to print a log upon arrival of this type of msg.
 *
 * @param msg               Message (usually received by Configurator).
 * @param cfg_log_lvl       Level at which to log the message itself
 *                          (maybe different from  msg->log_lvl)
 *
 * @return
 */
extern void cfg_db_tree_print_msg_log(cfg_tree_print_msg *msg,
                                      const unsigned int cfg_log_lvl);

/**
 * Print all dependancies of an object into a file and(or) log.
 *
 * @param filename          output filename (NULL to skip)
 * @param log_lvl           TE log level (0 to skip)
 * @param id_fmt            a format string for the id of an obj.
 *
 * @return                  Status code.
 */
extern te_errno cfg_db_obj_print_deps(const char *filename,
                                      const unsigned int log_lvl,
                                      const char *id_fmt, ...);

/**
 * Remove an object from the data base.
 *
 * @param id        id string of an object to be removed.
 * @param log_lvl   Log level for messages about forced actions
 *                  (instance delete, dependancy cut, etc. )
 *
 * @return  0 or TE_EINVAL, if error occurred.
 *
 */
extern te_errno cfg_db_unregister_obj_by_id_str(char *id,
                                                const unsigned int log_lvl);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_DB_H__ */

