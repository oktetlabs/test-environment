/** @file
 * @brief Configurator
 *
 * Configurator database definitions
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CONF_DB_H__
#define __TE_CONF_DB_H__
#ifdef __cplusplus
extern "C" {
#endif

#define CFG_INST_HANDLE_TO_INDEX(_handle)    (_handle & 0xFFFF)

/* Configurator object */
typedef struct cfg_object {
    cfg_handle         handle;  /**< Handle of the object */
    char              *oid;     /**< OID of the object */
    char               subid[CFG_SUBID_MAX]; 
                                /**< Own sub-identifier of the object */
    cfg_val_type       type;    /**< Type of the object instance value */
    uint8_t            access;  /**< Access rights */
    
    /** @name Family */
    struct cfg_object *father;  /**< Link to father */
    struct cfg_object *son;     /**< Link to the first son */
    struct cfg_object *brother; /**< Link to the next brother */
    /*@}*/
} cfg_object;

extern cfg_object cfg_obj_root;
extern cfg_object **cfg_all_obj;
extern int cfg_all_obj_size;

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
extern int cfg_inst_seq_num;

#define CFG_INST_HANDLE_VALID(_handle) \
    (CFG_INST_HANDLE_TO_INDEX(_handle) < (uint32_t)cfg_all_inst_size && \
     cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(_handle)] != NULL &&         \
     cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(_handle)]->handle == _handle)
    
#define CFG_GET_INST(_handle) \
    (CFG_INST_HANDLE_VALID(_handle) ? \
     cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(_handle)] : NULL)

/*----------------- User request processing ----------------------------*/

/* SIze of the buffer requered for messages and responses except pattern */
#define CFG_BUF_LEN \
    (CFG_OID_LEN_MAX * (CFG_SUBID_MAX + CFG_INST_NAME_MAX + 2) + \
     sizeof(cfg_get_oid_msg))

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

/**
 * Process pattern user request.
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
 * Add instance to the database.
 *
 * @param oid_s         object instance identifier
 * @param handle        location for handle of the new instance
 * @param type          instance value type
 * @param val           value to be assigned to the instance
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_db_add(char *oid_s, cfg_handle *handle, 
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
 * Initialize the database during startup or re-initialization.
 *
 * @return 0 (success) or ENOMEM
 */
extern int cfg_db_init(void);

/**
 * Destroy the database before shutdown.
 */
extern void cfg_db_destroy(void);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_DB_H__ */

