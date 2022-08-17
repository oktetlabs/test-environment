/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Messages definition
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_MESSAGES_H__
#define __TE_CONF_MESSAGES_H__

#include "conf_api.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/** Discover name of the Configurator IPC server */
static inline const char *
cs_server_name(void)
{
    static const char *cs_name = NULL;

    if ((cs_name == NULL) && (cs_name = getenv("TE_CS")) == NULL)
        cs_name = "TE_CS";

    return cs_name;
}

/** Configurator's server name */
#define CONFIGURATOR_SERVER     cs_server_name()

/** Type of IPC used by Configurator */
#define CONFIGURATOR_IPC        (TRUE) /* Connection-oriented IPC */

/** Message types */
enum {
    CFG_REGISTER,  /**< Register object: IN: OID, description;
                        OUT: handle */
    CFG_UNREGISTER,  /**< Unregister object: IN: OID. */
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
    CFG_COPY,      /**< Copy subtree: IN: source handle, destination handle */
    CFG_SYNC,      /**< Synchronize: IN: OID, subtree flag */
    CFG_REBOOT,    /**< Reboot TA: IN: TA name, restore flag */
    CFG_BACKUP,    /**< Create/verify/restore backup:
                        IN: op, file name (for verify restore);
                        OUT: file name (for create) */
    CFG_CONFIG,    /**< Create configuration file:
                        IN: file name, history flag */
    CFG_CONF_TOUCH,/**< Update conf_delay after touching the instance by
                        non-CS means */
    CFG_CONF_DELAY,/**< Sleep conf_delay */
    CFG_SHUTDOWN,  /**< Shutdown the Configurator */
    CFG_ADD_DEPENDENCY, /**< Add a dependency */
    CFG_TREE_PRINT,/**< Print a tree of obj|ins from a prefix */
    CFG_PROCESS_HISTORY,/**< Process history configuration file
                             IN: file name, key-value pairs to substitute */
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

/** CFG_REGISTER message content */
typedef struct cfg_register_msg {
    CFG_MSG_FIELDS
    cfg_val_type  val_type; /**< Type of the object instance value */
    te_bool       vol;      /**< Object is volatile */
    te_bool       no_parent_dep; /**< Object should not depend on parent */
    te_bool       substitution;  /**< The object uses substitution */
    uint8_t       access;   /**< Access rights */
    uint16_t      def_val;  /**< Default value offset from start of OID
                                 or 0 if no default value is provided */
    cfg_handle    handle;   /**< OUT: handle of created object
                                 object identifier */
    char          oid[0];   /**< IN: start of the object identifier */
} cfg_register_msg;

/** CFG_UNREGISTER message content */
typedef struct cfg_unregister_msg {
    CFG_MSG_FIELDS
    char          id[1];   /**< IN: start of the object identifier */
} cfg_unregister_msg;

/** CFG_FIND message content */
typedef struct cfg_find_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< OUT: handle of found object */
    char          oid[0];   /**< In: start of the object identifier */
} cfg_find_msg;

/** CFG_GET_DESCR message content */
typedef struct cfg_get_descr_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    cfg_obj_descr descr;    /**< OUT: object description */
} cfg_get_descr_msg;

/** CFG_GET_OID message content */
typedef struct cfg_get_oid_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    char          oid[0];   /**< OUT: start of the object identifier */
} cfg_get_oid_msg;

/** CFG_GET_ID message content */
typedef struct cfg_get_id_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    char          id[0];    /**< OUT: start of the sub-identifier or name */
} cfg_get_id_msg;

/** CFG_PATTERN message content  */
typedef struct cfg_pattern_msg {
    CFG_MSG_FIELDS
    char       pattern[0];  /**< IN: start of the pattern */
    cfg_handle handles[0];  /**< OUT: start of handles array */
} cfg_pattern_msg;

/** CFG_FAMILY message content  */
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

/** CFG_ADD message content  */
typedef struct cfg_add_msg {
    CFG_MSG_FIELDS
    cfg_handle      handle;         /**< OUT: object instance handle */
    te_bool         local;          /**< Local add */
    cfg_val_type    val_type;       /**< Object value type */
    uint8_t         oid_offset;     /**< Offset to OID from the
                                         message start */
    union {
        struct sockaddr val_addr[0];    /**< start of sockaddr value */
        char            val_str[0];     /**< start of string value*/
        int             val_int;        /**< int value */
        uint64_t        val_uint64;     /**< uint64_t value */
    } val;
} cfg_add_msg;

/** CFG_DEL message content  */
typedef struct cfg_del_msg {
    CFG_MSG_FIELDS
    cfg_handle handle;  /**< IN: object to be deleted */
    te_bool    local;   /**< IN: local delete */
} cfg_del_msg;

/** CFG_SET message content  */
typedef struct cfg_set_msg {
    CFG_MSG_FIELDS
    /* IN fields - should not be in answer */
    cfg_handle   handle;        /**< Object instance handle */
    te_bool      local;         /**< Local set */
    cfg_val_type val_type;      /**< Object value type */
    union {
        struct sockaddr val_addr[0]; /**< start of sockaddr value */
        char            val_str[0];  /**< start of string value */
        int             val_int;     /**< int value */
        uint64_t        val_uint64;  /**< uint64_t value */
    } val;
} cfg_set_msg;

/** CFG_COMMIT message content  */
typedef struct cfg_commit_msg {
    CFG_MSG_FIELDS
    char    oid[0];  /**< start of object identifier */
} cfg_commit_msg;

/** CFG_GET message content  */
typedef struct cfg_get_msg {
    CFG_MSG_FIELDS
    te_bool         sync;        /**< Synchronization get */
    cfg_handle      handle;      /**< IN */
    cfg_val_type    val_type;    /**< Object value type */
    union {
        struct sockaddr val_addr[0]; /**< start of sockaddr value */
        char            val_str[0];  /**< start of string value */
        int             val_int;     /**< int value */
        uint64_t        val_uint64;  /**< uint64_t value */
    } val;
} cfg_get_msg;

/** CFG_COPY message content  */
typedef struct cfg_copy_msg {
    CFG_MSG_FIELDS
    cfg_handle src_handle;   /**< Source handle */
    te_bool    is_obj;       /**< Destination is an object */
    char       dst_oid[0];   /**< Destination OID */
} cfg_copy_msg;

/** CFG_SYNC message content  */
typedef struct cfg_sync_msg {
    CFG_MSG_FIELDS
    te_bool subtree;  /**< subtree synchronization*/
    char    oid[0];   /**< start of object identifier */
} cfg_sync_msg;

/** CFG_REBOOT message content  */
typedef struct cfg_reboot_msg {
    CFG_MSG_FIELDS
    /* IN fields - should not be in the answer */
    rcf_reboot_type reboot_type; /** Reboot type */
    te_bool restore;    /**< Restore current configuration */
    char    ta_name[0]; /**< start of Test Agent name*/
} cfg_reboot_msg;

/** CFG_BACKUP message content  */
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

/** Restore configuration backup w/o trying to process history */
#define CFG_BACKUP_RESTORE_NOHISTORY  5
    unsigned int subtrees_num; /**< Number of subtrees */
    size_t subtrees_offset;    /**< Offset to subtrees string from message
                                    start. The string must contain subtrees
                                    separated by \0*/
    size_t filename_offset;    /**< Offset of the filename (terminated by \0)
                                    from message start */
} cfg_backup_msg;

/** CFG_CONFIG message content  */
typedef struct cfg_config_msg {
    CFG_MSG_FIELDS
    te_bool history;     /**< add to command history (if true) */
    char    filename[0]; /**< IN */
} cfg_config_msg;

/** CFG_CONF_TOUCH message content */
typedef struct cfg_conf_touch_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    char          oid[0];   /**< IN: start of the instance identifier */
} cfg_conf_touch_msg;

/** CFG_ADD_DEPENDENCY message content */
typedef struct cfg_add_dependency_msg {
    CFG_MSG_FIELDS
    cfg_handle    handle;   /**< IN: object handle */
    te_bool       object_wide; /**< IN: whether dependency is object wide */
    char          oid[0];   /**< IN: start of the master
                                 instance identifier */
} cfg_add_dependency_msg;

/** CFG_SHUTDOWN message content */
typedef struct cfg_shutdown_msg {
    CFG_MSG_FIELDS
} cfg_shutdown_msg;

/** CFG_TREE_PRINT message content */
typedef struct cfg_tree_print_msg {
    CFG_MSG_FIELDS
    unsigned int  log_lvl;  /**< IN:  log level */
    size_t        id_len;   /**< IN:  obj|ins id string length */
    size_t        flname_len;/**< IN: output filename length */
    char          buf[1];   /**< IN:  id + filename */
} cfg_tree_print_msg;

/** CFG_PROCESS_HISTORY message content */
typedef struct cfg_process_history_msg {
    CFG_MSG_FIELDS
    char    filename[0]; /**< IN: file name */
} cfg_process_history_msg;

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
