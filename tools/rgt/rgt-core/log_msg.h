/*
 * Test Environment: RGT Core
 * Common data structures and declarations.
 * Different structures that represent log message are declared.
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
 * Author: Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RGT_LOG_MSG_H__
#define __TE_RGT_LOG_MSG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <obstack.h>

#include "rgt_common.h"
#include "io.h"

/* Check if we have definitions from Test Envirounment */
#ifdef HAVE_TE_RAW_LOG_H
#include "te_raw_log.h"
#else
#error RGT cannot be built without te_raw_log.h file exported by TE sources
#endif
#ifdef HAVE_LOGGER_DEFS_H
#include "logger_defs.h"
#else
#error RGT cannot be built without logger_defs.h file exported by TE sources
#endif


/** @name A set of macros used in string representation of log level */
#define RGT_LL_ERROR_STR       "ERROR"
#define RGT_LL_WARN_STR        "WARN"
#define RGT_LL_RING_STR        "RING"
#define RGT_LL_INFO_STR        "INFO"
#define RGT_LL_VERB_STR        "VERB"
#define RGT_LL_ENTRY_EXIT_STR  "ENTRY/EXIT"
#define RGT_LL_UNKNOWN_STR     "UNKNOWN"
/*@}*/

/* Forward declaration */
struct log_msg;

/** 
 * Structure that represents argument in its raw representation
 * There must be some more information given to determine which type
 * of data it consists of. (This information can be obtained form
 * format string)
 */
typedef struct msg_arg {
    struct msg_arg *next; /**< Pointer to the next argument */
    uint8_t        *val;  /**< Pointer to raw argument content 
                               (numbers are keeped in network byte order) */
    int             len;  /**< Number of bytes allocated for the argument */
} msg_arg;


/** Structure that keeps log message in an universal format */
typedef struct log_msg {
    struct obstack *obstk;    /**< Internal field: 
                                   Obstack for the message */

    char       *entity;       /**< Entity name of the message */
    char       *user;         /**< User name of the message */
    uint32_t    timestamp[2]; /**< Timestamp value */
    const char *level;        /**< Log level */
    char       *fmt_str;      /**< Raw format string */
    msg_arg    *args;         /**< List of arguments for format string */
    msg_arg    *cur_arg;      /**< Internal field: 
                                   used by get_next_arg function */
    int         args_count;   /**< Total number of the arguments */

    char       *txt_msg;
} log_msg;


/* 
 * The following declarations are about Control Log Messages that 
 * outline test execution flow.
 */

/* 
 * Entity and user names of control messages.
 * These constants were got from OKT-HLD-0000095-TE_TS document.
 */
#define CMSG_ENTITY_NAME "Tester"
#define CMSG_USER_NAME   "Control"

#define CNTR_MSG_TEST    "TEST"
#define CNTR_MSG_PACKAGE "PACKAGE"
#define CNTR_MSG_SESSION "SESSION"

#define CNTR_BIN2STR(val_) \
    (val_ == NT_TEST ? CNTR_MSG_TEST :            \
     val_ == NT_PACKAGE ? CNTR_MSG_PACKAGE :      \
     val_ == NT_SESSION ? CNTR_MSG_SESSION : (assert(0), ""))

/* 
 * Structures that are used for representation of control log messages.
 * They are high level structures obtained from "struct log_msg" objects.
 */

/** Structure that represents session/test/package "parameter" entity */
typedef struct param {
    struct param *next; /**< Pointer to the next parameter */
    
    char *name; /**< Parameter name */
    char *val;  /**< Parameter value in string representation */
} param;

/** Possible results of test, package or session */
typedef enum result_status {
    RES_STATUS_PASSED, 
    RES_STATUS_KILLED, 
    RES_STATUS_DUMPED, 
    RES_STATUS_SKIPPED, 
    RES_STATUS_FAKED, 
    RES_STATUS_FAILED, 
    RES_STATUS_EMPTY,
} result_status_t;

/** Structure for keeping session/package/test result information */
typedef struct result_info {
    enum result_status  status; /**< Result status */
    char               *err;    /**< An error message in the case of
                                     status field different from
                                     RES_STATUS_PASS */
} result_info_t;

/**
 * Structure that represents information about a particular entry.
 * It is used for passing information about start/end events. 
 */
typedef struct node_descr {
    char *name;       /**< Entry name */
    char *objective;  /**< Objectives of the entry */
    char *author;     /**< Entry author */
    int   n_branches; /**< Number of branches in the entry */
} node_descr_t;

typedef struct node_info {
    node_type_t     type;        /**< Node type */
    node_descr_t    descr;       /**< Description of the node */
    param          *params;      /**< List of parameters */
    uint32_t        start_ts[2]; /**< Timestamp of a "node start" event */
    uint32_t        end_ts[2];   /**< Timestamp of a "node end" event */
    result_info_t   result;      /**< Node result info */
} node_info_t;

typedef int (* f_process_ctrl_log_msg)(node_info_t *);
typedef int (* f_process_reg_log_msg)(log_msg *);

/** The set of generic control event types */
enum ctrl_event_type {
    CTRL_EVT_START,  /**< Strart control message */
    CTRL_EVT_END,    /**< End control message */
    CTRL_EVT_LAST    /**< Last marker - the biggest value of the all
                          evements */
};

/** External declarations of a set of message processing functions */
extern f_process_ctrl_log_msg ctrl_msg_proc[CTRL_EVT_LAST][NT_LAST];
extern f_process_reg_log_msg  reg_msg_proc;

/** 
 * The list of events that can be generated from the flow tree 
 * for a particular node
 */
enum event_type {
    MORE_BRANCHES /**< An additional branch is added on the entry */
};

/**
 * Process control message: Insert a new node into the flow tree if it's 
 * a start event; Close node if it's an end event.
 *
 * @param  msg   Pointer to the log message to be processed.
 *
 * @return  Status of operation.
 */
extern int rgt_process_control_message(log_msg *msg);

/**
 * Process regular log message:
 *   Checks if a message passes through user-defined filters,
 *   Attaches a message to the flow tree, or calls reg_msg_proc function
 *   depending on operation mode of the rgt.
 *
 * @param  msg   Pointer to the log message to be processed.
 *
 * @return  Nothing.
 */
extern void rgt_process_regular_message(log_msg *msg);

/**
 * Processes event occured on a node of the flow tree.
 * Currently the only event that is actually processed is MORE_BRANCHES.
 *
 * @param type   Type of a node on which an event has occured.
 * @param evt    Type of an event.
 * @param node   User-specific data that is passed on creation of the node.
 *
 * @return  Nothing.
 */
extern void rgt_process_event(node_type_t type, enum event_type evt,
                              node_info_t *node);

extern void log_msg_init_arg(log_msg *msg);

/**
 * Return pointer to the log message argument. The first call of the
 * function returns pointer to the first argument. The second call
 * returns pointer to the second argument and so on.
 *
 * @param  msg  Message which argument we are going to obtain
 *
 * @return Pointer to an argument of a message
 */
extern msg_arg *get_next_arg(log_msg *msg);

/**
 * Allocates a new log_msg structure from global memory pool for log
 * messages.
 *
 * @return An address of log_msg structure.
 */
extern log_msg *alloc_log_msg();

/**
 * Frees log message.
 *
 * @param  msg   Log message to be freed.
 *
 * @return  Nothing.
 *
 * @se 
 *     The freeing of a log message leads to freeing all messages allocated 
 *     after the message.
 */
extern void free_log_msg(log_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_LOG_MSG_H__ */
