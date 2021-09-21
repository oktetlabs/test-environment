/** @file
 * @brief Test Environment
 *
 * RCF definitions
 *
 *
 * Copyright (C) 2003-2021 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_H__
#define __TE_RCF_H__

#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "ipc_server.h"
#include "rcf_methods.h"
#include "rcf_api.h"
#include "rcf_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default select timeout in seconds */
#define RCF_SELECT_TIMEOUT      1
/** Default timeout (in seconds) for command processing on the TA */
#define RCF_CMD_TIMEOUT         100
/** Huge timeout for command processing on the TA */
#define RCF_CMD_TIMEOUT_HUGE    10000
/** TA reboot timeout in seconds */
#define RCF_REBOOT_TIMEOUT      60
/** TA shutdown timeout in seconds */
#define RCF_SHUTDOWN_TIMEOUT    5

/**
 * Timeout for CONFSET operation, in seconds.
 *
 * RCF_CMD_TIMEOUT is not big enough to allow performing time consuming
 * set operations (for example, to generate DH parameters and start
 * openvpn deamon) via Configurator.
 */
#define RCF_CONFSET_TIMEOUT (RCF_CMD_TIMEOUT * 3)

/** Special session identifiers */
enum {
    /** Session used for Log gathering */
    RCF_SID_GET_LOG = 1,
    /** Session used for TA check */
    RCF_SID_TACHECK,

    /** Unused SID, must be the last in the enum */
    RCF_SID_UNUSED,
};

/** One request from the user */
typedef struct usrreq {
    struct usrreq            *next;
    struct usrreq            *prev;
    rcf_msg                  *message;
    struct ipc_server_client *user;
    uint32_t                  timeout;  /**< Timeout in seconds */
    time_t                    sent;
} usrreq;

/** A description for a task/thread to be executed at TA startup */
typedef struct ta_initial_task {
    rcf_execute_mode        mode;          /**< Task execution mode */
    char                   *entry;         /**< Procedure entry point */
    int                     argc;          /**< Number of arguments */
    char                  **argv;          /**< Arguments as strings */
    struct ta_initial_task *next;          /**< Link to the next task */
} ta_initial_task;

/** Structure for one Test Agent */
typedef struct ta {
    struct ta          *next;               /**< Link to the next TA */
    rcf_talib_handle    handle;             /**< Test Agent handle returted
                                                 by start() method */
    char               *name;               /**< Test Agent name */
    char               *type;               /**< Test Agent type */
    te_bool             enable_synch_time;  /**< Enable synchronize time */
    te_kvpair_h         conf;               /**< Configurations list of kv_pairs */
    usrreq              sent;               /**< User requests sent
                                                 to the TA */
    usrreq              waiting;            /**< User requests waiting
                                                 for unblocking of
                                                 TA connection */
    usrreq              pending;            /**< User requests pending
                                                 until answer on previous
                                                 request with the same SID
                                                 is received */
    unsigned int        flags;              /**< Test Agent flags */
    time_t              reboot_timestamp;   /**< Time of reboot command
                                                 sending (in seconds) */
    int                 sid;                /**< Free session identifier
                                                 (starts from 2) */
    te_bool             conn_locked;        /**< Connection is locked until
                                                 the response from TA
                                                 is received */
    int                 lock_sid;           /**< SID of the command
                                                 locked the connection */
    void               *dlhandle;           /**< Dynamic library handle */
    ta_initial_task    *initial_tasks;      /**< Startup tasks */
    char               *cold_reboot_ta;     /**< Cold reboot TA name */
    const char         *cold_reboot_param;  /**< Cold reboot params */
    int                 cold_reboot_timeout; /**< Waiting time for a cold
                                                  reboot */

    te_bool            dynamic;             /**< Dynamic creation flag */

    struct rcf_talib_methods m; /**< TA-specific Methods */
} ta;

/**
 * TA check initiator data.
 */
typedef struct ta_check {
    usrreq         *req;    /**< User request */
    unsigned int    active; /**< Number of active checks */
} ta_check;

extern ta_check ta_checker;
extern fd_set set0;

/**
 * Obtain TA structure address by Test Agent name.
 *
 * @param name          Test Agent name
 *
 * @return TA structure pointer or NULL
 */
extern ta *rcf_find_ta_by_name(char *name);

/**
 * Check if a message with the same SID is already sent.
 *
 * @param req           request list anchor (ta->sent or ta->failed)
 * @param sid           session identifier of the received user request
 */
extern usrreq *rcf_find_user_request(usrreq *req, int sid);

/**
 * Respond to user request and remove the request from the list.
 *
 * @param req           request with already filled out parameters
 */
extern void rcf_answer_user_request(usrreq *req);

/**
 * Respond to all user requests in the specified list with specified error.
 *
 * @param req           anchor of request list (&ta->sent or &ta->pending)
 * @param error         error to be filled in
 */
extern void rcf_answer_all_requests(usrreq *req, int error);

/**
 * Mark test agent as recoverable dead.
 *
 * @param agent     Test Agent
 */
extern void rcf_set_ta_dead(ta *agent);

/**
 * Mark test agent as unrecoverable dead.
 *
 * @param agent     Test Agent
 */
extern void rcf_set_ta_unrecoverable(ta *agent);

/**
 * Initialize Test Agent or recovery it after reboot.
 * Test Agent is marked as "unrecoverable dead" in the case of failure.
 *
 * @param agent         Test Agent structure
 *
 * @return Status code
 */
extern int rcf_init_agent(ta *agent);

/**
 * Send command to the Test Agent according to user request.
 *
 * @param agent         Test Agent structure
 * @param req           user request
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rcf_send_cmd(ta *agent, usrreq *req);

/**
 * Allocate memory for user request.
 */
extern usrreq *rcf_alloc_usrreq(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RCF_H__ */
