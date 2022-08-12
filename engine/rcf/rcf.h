/** @file
 * @brief Test Environment
 *
 * RCF definitions
 *
 *
 * Copyright (C) 2004-2021 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 *
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
/** TA host reboot timeout in seconds */
#define RCF_HOST_REBOOT_TIMEOUT 600
/** TA shutdown timeout in seconds */
#define RCF_SHUTDOWN_TIMEOUT    5

/** Timeout to wait for logs in seconds */
#define RCF_LOG_FLUSHED_TIMEOUT 10
/**
 * The interval between sending a reboot request to the agent
 * and receiving a response in seconds
 */
#define RCF_ACK_HOST_REBOOT_TIMEOUT 10
 /** Timeout for cold reboot in seconds */
#define RCF_COLD_REBOOT_TIMEOUT 600

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

typedef struct ta ta;
typedef struct usrreq usrreq;

/**
 * The prototype of the function that is called when receiving
 * a response from the agent.
 *
 * @param ta  Test Agent
 * @param req Request
 *
 * @return Status code
 */
typedef te_errno (* userreq_callback)(ta *agent, usrreq *req);

/** One request from the user */
struct usrreq {
    struct usrreq            *next;
    struct usrreq            *prev;
    rcf_msg                  *message;
    struct ipc_server_client *user;
    uint32_t                  timeout;  /**< Timeout in seconds */
    time_t                    sent;
    userreq_callback          cb;
};

/** A description for a task/thread to be executed at TA startup */
typedef struct ta_initial_task {
    rcf_execute_mode        mode;          /**< Task execution mode */
    char                   *entry;         /**< Procedure entry point */
    int                     argc;          /**< Number of arguments */
    char                  **argv;          /**< Arguments as strings */
    struct ta_initial_task *next;          /**< Link to the next task */
} ta_initial_task;

/**
 * State of the reboot
 *
 * +-------------+     +-------------+     +-------------+
 * |             | (1) |             | (2) |             |
 * |    IDLE     |---->|  LOG_FLUSH  |---->|   WAITING   |----+
 * |             |     |             |     |             |    |
 * +-------------+     +-------------+     +-------------+    |
 *                                                            |
 * +----------------------------------------------------------+
 * |
 * |     +-------------+     +-------------+
 * | (3) |             | (4) |             | (5)
 * +---->| WAITING_ACK |---->|  REBOOTING  |----> goto IDLE
 *       |             |     |             |
 *       +-------------+     +-------------+
 *
 * (1),(2),(3),(4) - Event for switching to the next state
 *
 * - @c IDLE - The normal state of the agent
 * - (1)  - An user requested to reboot TA
 * - @c LOG_FLUSH - RCF is waiting for a response (10 second)
 *   to the @c GET_LOG last command
 * - (2) - RCF received an answer from the agent to the GET_LOG last command
 * - @c WAITING - RCF is forming a request to reboot the TA and sending it
 * - (3) - RCF sent a reboot request to TA
 * - @c WAITING_ACK - RCF is waiting for confirmation (10 second) of receiving
 *                    a reboot request from the TA
 * - (4) - RCF received confirmation from the TA
 * - @c REBOOTING - RCF is waiting for the reboot to finish using the specified
 *                  timeout. If the waiting time has expired, the RCF marks the
 *                  agent unrecoverable dead. Either switсh to the next reboot
 *                  type if it allowed
 * - (5) - RCF is initializing the TA process
 *
 * In case of TA process reboot, RCF goes from state LOG_FLUSH to (5)
 * immediately.
 */
typedef enum ta_reboot_state {
    /** The normal state of the agent */
    TA_REBOOT_STATE_IDLE,
    /** Waiting for the log flush command */
    TA_REBOOT_STATE_LOG_FLUSH,
    /** Send a reboot request to agent and wait for confirmation of sending */
    TA_REBOOT_STATE_WAITING,
    /** Wait for a response from the agent to the reboot command */
    TA_REBOOT_STATE_WAITING_ACK,
    /** Waiting for the end of the reboot */
    TA_REBOOT_STATE_REBOOTING,
} ta_reboot_state;

/** Type of the reboot */
typedef enum ta_reboot_type {
    /** Restart TA process */
    TA_REBOOT_TYPE_AGENT = 0x1,
    /** Reboot TA host */
    TA_REBOOT_TYPE_HOST = 0x2,
    /** Cold reboot the host using assigned power control agent */
    TA_REBOOT_TYPE_COLD = 0x4,
} ta_reboot_type;

/** Contextual information for rebooting the agent */
typedef struct ta_reboot_context {
    /** Current reboot state */
    ta_reboot_state state;
    /** Bitmask of the requested reboot types by the user */
    int requested_types;
    /** Bitmask of the allowed reboot types by the user */
    int allowed_types;
    /**
     * Current reboot type.
     * The current reboot type will increase to the requested type according
     * to the Reboot types are enumerated from the lowest one up to
     * @a requested_type until until one of them completes successfully
     */
    ta_reboot_type current_type;
    /** Timestamp of one reboot state */
    time_t reboot_timestamp;
    /** User request with reboot message */
    usrreq *req;
    /**
     * The flag to check that the response from the agent is received.
     * Use for the reboot context only.
     */
    te_bool is_answer_recv;
    /** The error that occurred during the reboot */
    te_errno error;
    /**
     * This field is used to avoid a lot of message
     * "Agent in the reboot state" in the logs
     */
    te_bool is_agent_reboot_msg_sent;
    /** Number of agent restart attempts */
    unsigned int restart_attempt;
    /** The flag to check that timeout for cold reboot is expired */
    te_bool is_cold_reboot_time_expired;
} ta_reboot_context;

/** Structure for one Test Agent */
struct ta {
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

    ta_reboot_context reboot_ctx; /**< Reboot context */
};

/**
 * TA check initiator data.
 */
typedef struct ta_check {
    usrreq         *req;    /**< User request */
    unsigned int    active; /**< Number of active checks */
} ta_check;

extern ta_check ta_checker;
extern fd_set set0;
extern struct timeval tv0;

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

/**
 * Entry point of reboot state machine.
 *
 * @param agent Test Agent structure
 */
extern void rcf_ta_reboot_state_handler(ta *agent);

/**
 * Set the specified agent reboot state, log the message about it
 * and and remember the timestamp of switching to a new state.
 *
 * @param agent Test Agent structure
 * @param state Reboot state
 */
extern void rcf_set_ta_reboot_state(ta *agent, ta_reboot_state state);

/**
 * Check that from the point of view of the reboot context
 * command can be sent to the agent
 *
 * @param agent Test Agent structure
 * @param req   User request
 *
 * @return @c TRUE if command should be sent
 */
extern te_bool rcf_ta_reboot_before_req(ta *agent, usrreq *req);

/**
 * Check that in terms of the reboot context the waiting
 * requests should be processed
 *
 * @param agent  Test Agent structure
 * @param opcode Request operation code
 * @return @c FLASE if the waiting requests should be processed
 */
extern te_bool rcf_ta_reboot_on_req_reply(ta *agent, rcf_op_t opcode);

/**
 * Initialize reboot context for the TA
 *
 * @param agent Test Agent structure
 */
extern void rcf_ta_reboot_init_ctx(ta *agent);

/**
 * Get the next available reboot type
 *
 * @param agent Test Agent structure
 */
extern void rcf_ta_reboot_get_next_reboot_type(ta *agent);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RCF_H__ */
