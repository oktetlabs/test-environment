/** @file
 * @brief Configurator
 *
 * Messages definition
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CONF_MESSAGES_H__
#define __TE_CONF_MESSAGES_H__

#include "conf_api.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/** Configurator's server name */
#define CONFIGURATOR_SERVER     "TE_CS"

/** Message types */
enum {
    CFG_REGISTER,  /**< Register object: IN: OID, description;
                        OUT: handle */
    CFG_FIND,      /**< Find handle by OID: IN: OID; OUT: handle */
    CFG_GET_DESCR, /**< Get description by handle:
                        IN: handle; OUT: description */
    CFG_GET_OID,   /**< Get OID: IN: handle OUT: OID */
    CFG_GET_ID,    /**< Get sub-identifier or object instance name */
    CFG_PATTERN,   /**< Find by pattern: IN: pattern;
                        OUT: array of handles */
    CFG_FAMILY,    /**< Get son, father or brother:
                        IN: handle, member name;  OUT: handle */
    CFG_ADD,       /**< Add instance: IN: OID, value; OUT: handle */
    CFG_DEL,       /**< Delete instance: IN: handle, children flag */
    CFG_SET,       /**< Set instance: IN: handle, value */
    CFG_COMMIT,    /**< Commit Configurator database changes to Test
                        Agent(s): IN: subtree OID */
    CFG_GET,       /**< Get value: IN: handle, sync flag; OUT: value */
    CFG_SYNC,      /**< Synchronize: IN: OID, subtree flag */
    CFG_REBOOT,    /**< Reboot TA: IN: TA name, restore flag */
    CFG_BACKUP,    /**< Create/verify/restore backup:
                        IN: op, file name (for verify restore);
                        OUT: file name (for create) */
    CFG_CONFIG,    /**< Create configuration file:
                        IN: file name, history flag */
    CFG_SHUTDOWN   /**< Shutdown the Configurator */
};

/* Set of generic fields of the Configurator message */
#define CFG_MSG_FIELDS \
    uint8_t     type;    /**< Message type */                       \
    uint32_t    len;     /**< Length of the whole message */        \
    int         rc;      /**< OUT: errno defined in te_errno.h */   \

/** Generic Configurator message structure */
typedef struct cfg_msg {
    CFG_MSG_FIELDS
} cfg_msg;

/** Configurator message structure */
typedef struct cfg_register_msg {
    CFG_MSG_FIELDS
    cfg_obj_descr descr;        /**< Object description */
    cfg_handle    handle;   /**< OUT: handle of created object */
    char          oid[0];   /**< IN: start of the object identifier */
} cfg_register_msg;

/** Configurator message structure */
typedef struct cfg_find_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< OUT: handle of found object */
    char          oid[0];   /**< In: start of the object identifier */
} cfg_find_msg;

/** Configurator message structure */
typedef struct cfg_get_descr_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    cfg_obj_descr descr;    /**< OUT: object description */
} cfg_get_descr_msg;

/** Configurator message structure */
typedef struct cfg_get_oid_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    char          oid[0];   /**< OUT: start of the object identifier */
} cfg_get_oid_msg;

/** Configurator message structure */
typedef struct cfg_get_id_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    char          id[0];    /**< OUT: start of the sub-identifier or name */
} cfg_get_id_msg;

/** Configurator message structure */
typedef struct cfg_pattern_msg {
    CFG_MSG_FIELDS
    char       pattern[0];  /**< IN: start of the pattern */
    cfg_handle handles[0];  /**< OUT: start of handles array */
} cfg_pattern_msg;

/** Configurator message structure */
typedef struct cfg_family_msg {
    CFG_MSG_FIELDS
    uint8_t who;            /**< IN: Family member to get */
/** Object father */
#define CFG_FATHER      1
/** Object brother */
#define CFG_BROTHER     2
/** Object son */
#define CFG_SON         3
    cfg_handle    handle;   /**< Object handle (IN and OUT) */
} cfg_family_msg;

/** Configurator message structure */
typedef struct cfg_add_msg {
    CFG_MSG_FIELDS
    cfg_handle      handle;         /**< OUT: object instance handle */
    cfg_val_type    val_type;       /**< Object value type */
    uint8_t         oid_offset;     /**< Offset to OID from the 
                                         message start */
    struct sockaddr val_addr[0];    /**< start of sockaddr value */
    char            val_str[0];     /**< start of string value*/
    int             val_int;        /**< int value */
} cfg_add_msg;

/** Configurator message structure */
typedef struct cfg_del_msg {
    CFG_MSG_FIELDS
    cfg_handle handle;  /**< IN: object to be deleted */
} cfg_del_msg;

/** Configurator message structure */
typedef struct cfg_set_msg {
    CFG_MSG_FIELDS
    /* IN fields - should not be in answer */
    cfg_handle      handle;      /**< Object instance handle */
    te_bool         local;       /**< Local set */
    cfg_val_type    val_type;    /**< Object value type */
    struct sockaddr val_addr[0]; /**< start of sockaddr value */
    char            val_str[0];  /**< start of string value */
    int             val_int;     /**< int value */
} cfg_set_msg;

/** Configurator message structure */
typedef struct cfg_commit_msg {
    CFG_MSG_FIELDS
    char    oid[0];  /**< start of object identifier */
} cfg_commit_msg;

/** Configurator message structure */
typedef struct cfg_get_msg {
    CFG_MSG_FIELDS
    te_bool         sync;        /**< Synchronization get */
    cfg_handle      handle;      /**< IN */
    cfg_val_type    val_type;    /**< Object value type */
    struct sockaddr val_addr[0]; /**< start of sockaddr value */
    char            val_str[0];  /**< start of string value */
    int             val_int;     /**< int value */
} cfg_get_msg;

/** Configurator message structure */
typedef struct cfg_sync_msg {
    CFG_MSG_FIELDS
    te_bool subtree;  /**< subtree synchronization*/
    char    oid[0];   /**< start of object identifier */
} cfg_sync_msg;

/** Configurator message structure */
typedef struct cfg_reboot_msg {
    CFG_MSG_FIELDS
    /* IN fields - should not be in the answer */
    te_bool restore;    /**< Restore current configuration */
    char    ta_name[0]; /**< start of Test Agent name*/
} cfg_reboot_msg;

/** Configurator message structure */
typedef struct cfg_backup_msg {
    CFG_MSG_FIELDS
    uint8_t op;            /**< Operation, always present */
/** Create configuration backup */
#define CFG_BACKUP_CREATE       1   

/** Verify configuration backup */
#define CFG_BACKUP_VERIFY       2   

/** Restore configuration backup */
#define CFG_BACKUP_RESTORE      3   

/** Release configuration backup */
#define CFG_BACKUP_RELEASE      4   
    char    filename[0];  /**< IN or OUT depending on operation */
} cfg_backup_msg;

/** Configurator message structure */
typedef struct cfg_config_msg {
    CFG_MSG_FIELDS
    te_bool history;     /**< add to command history (if true) */
    char    filename[0]; /**< IN */
} cfg_config_msg;

/** Configurator message structure */
typedef struct cfg_shutdown_msg {
    CFG_MSG_FIELDS
} cfg_shutdown_msg;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process message with user request.
 *
 * @param msg           location of message pointer (message may be updated
 *                      or re-allocated by the function)
 * @param update_dh     if true, add the command to dynamic history
 */
extern void cfg_process_msg(cfg_msg **msg, te_bool update_dh);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_CONF_MESSAGES_H__ */
