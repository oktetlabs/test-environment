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

#ifndef __LOG_MSG_H__
#define __LOG_MSG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <obstack.h>

#include "rgt_common.h"
#include "io.h"

/* Check if we have definitions from Test Envirounment */
#ifdef HAVE_LOGGER_DEFS_H
#include "logger_defs.h"
#endif


#ifdef HAVE_LOGGER_DEFS_H

/** @name A set of macros used in string representation of log level */
#define LGLVL_ERROR_STR       "ERROR"
#define LGLVL_WARNING_STR     "WARN"
#define LGLVL_RING_STR        "RING"
#define LGLVL_INFORMATION_STR "INFO"
#define LGLVL_VERBOSE_STR     "VERB"
#define LGLVL_ENTRY_EXIT_STR  "ENTRY/EXIT"
#define LGLVL_UNKNOWN_STR     "UNKNOWN"
/*@}*/
#else
/** Macro for dummy log level represenation */
#define LGLVL_EMPTY_STR ""
#endif

/* Forward declaration */
struct log_msg;

/** 
 * Structure that represents argument in its raw representation
 * There must be some more information given to determine which type of data 
 * it consists of. (This information can be obtained form format string)
 */
typedef struct msg_arg {
    struct msg_arg *next; /**< Pointer to the next argument */
    uint8_t        *val;  /**< Pointer to raw argument content 
                               (numbers are keeped in network byte order) */
    int             len;  /**< Number of bytes allocated for the argument */
} msg_arg;


/** Structure that keeps log message in an universal format */
typedef struct log_msg {
    struct obstack *obstk;    /**< Internal field: Obstack for the message */

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
#define CMSG_ENTITY_NAME "engine"
#define CMSG_USER_NAME   "test"

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
enum result_status {
    RES_STATUS_PASS, /**< Success */
    RES_STATUS_FAIL, /**< Failure */
};

/** Structure for keeping session/package/test result information */
typedef struct result_info {
    enum result_status  status; /**< Result status */
    char               *err;    /**< An error message in the case of status 
                                     field different from RES_STATUS_PASS */
} result_info;

/**
 * Structure that represents information about a particular test.
 * It is used for passing information about start test events and also for
 * test termination events.
 */
typedef struct test_info {
    char        *name;        /**< Test name */
    char        *objective;   /**< Objectives of a test */
    char        *author;      /**< Test author */
} test_info;

/**
 * Structure that represents information about a particular package.
 * It is used for passing information about start package events and also for
 * package termination events. 
 */
typedef struct pkg_info {
    char        *name;        /**< Package name */
    char        *title;       /**< Package title */
    char        *author;      /**< Package author */
} pkg_info;

/** Structure that represents information about a particular session. */
typedef struct session_info {
    char        *objective;   /**< Objectives of a session */
    int          n_branches;  /**< Number of branches in a session */
} session_info;

typedef struct node_info {
    union {
        test_info    test;
        pkg_info     pkg;
        session_info sess;
    } node_specific;
    enum node_type  ntype;
    param          *params;      /**< List of parameters */
    uint32_t        start_ts[2]; /**< Timestamp of a "node start" event */
    uint32_t        end_ts[2];   /**< Timestamp of a "node end" event */
    result_info     result;      /**< Node result info */
} node_info;

typedef int (* f_process_ctrl_log_msg)(node_info *);
typedef int (* f_process_reg_log_msg)(log_msg *);

/** The set of generic control event types */
enum ctrl_event_type {
    CTRL_EVT_START,  /**< Strart control message */
    CTRL_EVT_END,    /**< End control message */
    CTRL_EVT_LAST    /**< Last marker - the biggest value of the all evements */
};

/** External declarations of a set of message processing functions */
extern f_process_ctrl_log_msg ctrl_msg_proc[CTRL_EVT_LAST][NT_LAST];
extern f_process_reg_log_msg  reg_msg_proc;

/** 
 * The list of events that can be generated from the flow tree 
 * for a particular node
 */
enum event_type {
    MORE_BRANCHES /**< An additional branch is added on a session */
};

/**
 * Process control message: Insert a new node into the flow tree if it's 
 * a start event; Close node if it's an end event.
 *
 * @param  msg   Pointer to the log message to be processed.
 *
 * @return  Status of operation.
 */
int rgt_process_control_message(log_msg *msg);

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
void rgt_process_regular_message(log_msg *msg);

/**
 * Processes event occured on a node of the flow tree.
 * Currently the only event that is actually processed is MORE_BRANCHES.
 *
 * @params ntype  Type of a node on which an event has occured.
 * @param  evt    Type of an event.
 * @param  node   User-specific data that is passed on  creation of the node.
 *
 * @return  Nothing.
 */
void rgt_process_event(enum node_type ntype, enum event_type evt, node_info *node);

void log_msg_init_arg(log_msg *msg);

/**
 * Return pointer to the log message argument. The first call of the function
 * returns pointer to the first argument. The second call returns pointer to 
 * the second argument and so on.
 *
 * @param  msg  Message which argument we are going to obtain
 *
 * @return Pointer to an argument of a message
 */
msg_arg *get_next_arg(log_msg *msg);

/**
 * Allocates a new log_msg structure from global memory pool for log messages.
 *
 * @return An address of log_msg structure.
 */
log_msg *alloc_log_msg();

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
void free_log_msg(log_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_MSG_H__ */
