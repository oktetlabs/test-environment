/** @file
 * @brief iSCSI Initiator-related defintions
 *
 * iSCSI Initiator configuration
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#ifndef __TE_ISCSI_INITIATOR_H
#define __TE_ISCSI_INITIATOR_H 1

#include <semaphore.h>

#include "te_iscsi.h"

/* Debug logs */
#define ISCSI_DEBUG_LOG_
#ifdef ISCSI_DEBUG_LOG
#define IVERB(args...) fprintf(stderr, args); fprintf(stderr, "\n")
#else
#define IVERB VERB
#endif

/** Maximum length of the (initiator|target)(name|alias) */
#define MAX_NAME_LENGTH 256

/** Maximum address length */
#define MAX_ADDR_LENGTH 20

/** Yes or No */
#define YES_NO_LENGTH 4

/** CHAP or None or CHAP,None */
#define AUTH_METHOD_LENGTH 11

/** Normal or Discovery */
#define SESSION_TYPE_LENGTH 10

/** HeaderDigest length CRC32R,None */
#define DIGEST_LENGTH 15

/** Maximum length of the list of cids of the initiator */
#define MAX_CID_LIST_LENGTH 100

/** Maximum length of the CLI command */
#define MAX_CMD_SIZE 1024

/** Length of the peer_secret, peer_name, local_secret, local_name */
#define SECURITY_COMMON_LENGTH 256

/** Length of Open iSCSI record ID or Microsoft session ID */
#define SESSION_ID_LENGTH 64

/** Maximum number of targets the Initiator can connect to */
#define MAX_TARGETS_NUMBER 3

/** Maximum number of the connections with one target */
#define MAX_CONNECTIONS_NUMBER 10

/** Default targets port */
#define ISCSI_TARGET_DEFAULT_PORT 3260

/** 
 * Host bus adapter default value. If
 * the Initiator is the only scsi device on the
 * system and it is loaded the first time since
 * the last reboot than it is zero.
 */
#define DEFAULT_HOST_BUS_ADAPTER            0


/** Maximum length of device name */
#define MAX_DEVICE_NAME_LEN 32

/**
 * Lun of the target to connect to.
 */
#define DEFAULT_LUN_NUMBER 0

/**
 * Types of the Initiator to configure.
 * The default type of the Initiator is UNH.
 * The type of the Initiator can be changed via
 * Configurator.
 */
typedef enum {
    UNH,       /**< UNH Initiator (GPL 2) */
    L5,        /**< Level 5 Networks */
    OPENISCSI, /**< Open iSCSI */
    MICROSOFT, /**< Microsoft iSCSI */
} iscsi_initiator_type;

/** Encoding of challenge and response */
typedef enum {
    BASE_16,   /**< Hexadecimal encoding */
    BASE_64,   /**< Base 64 encoding */
} enc_fmt_e;

/**
 * Security related data.
 * This structure is per target structure.
 * The current supported security protocol
 * is CHAP.
 */
typedef struct iscsi_tgt_chap_data {
    char        chap[AUTH_METHOD_LENGTH];  /**< AuthMethod: 
                                                (None|CHAP|CHAP,None) */
    enc_fmt_e   enc_fmt;                   /**< Encoding of challenge
                                                and response */
    int         challenge_length;          /**< Length of the challenge */
    
    char        peer_name[SECURITY_COMMON_LENGTH]; /**< Peer Name (pn in UNH
                                                        notation) */
    char        peer_secret[SECURITY_COMMON_LENGTH]; /**< Peer Secret (px in
                                                          UNH notation) */
    
    char        local_name[SECURITY_COMMON_LENGTH]; /**< Local Name (ln in UNH
                                                         notation) */
    char        local_secret[SECURITY_COMMON_LENGTH]; /**< Local Secret (lx in
                                                           UNH Notation */
    int         target_auth; /**< If TRUE, then Target authentication is
                                  required during the Security Phase */
} iscsi_tgt_chap_data_t;

typedef struct iscsi_connection_data {
    iscsi_connection_status status; /**< Connection status (UP, DOWN etc) */

    int               conf_params; /**< OR of OFFER_XXX flags */

    char              initiator_name[MAX_NAME_LENGTH]; /**< InitiatorName */ 
    char              initiator_alias[MAX_NAME_LENGTH]; /**< InitiatorAlias */
    int               max_connections;               /**< MaxConnections */
    char              initial_r2t[YES_NO_LENGTH];    /**< InitialR2T */
    char              header_digest[DIGEST_LENGTH];  /**< HeaderDigest */
    char              data_digest[DIGEST_LENGTH];    /**< DataDigest */
    char              immediate_data[YES_NO_LENGTH]; /**< ImmediateData */
    /** MaxRecvDataSegmentLength */
    int               max_recv_data_segment_length;
    int               first_burst_length; /**< FirstBurstLength */
    int               max_burst_length;   /**< MaxBurstLength 
                                               (>= FirstBurstLength) */
    int               default_time2wait;  /**< DefaultTime2Wait */
    int               default_time2retain; /**< DefaultTime2Retain */
    int               max_outstanding_r2t; /**< MaxOutstandingR2T */
    char              data_pdu_in_order[YES_NO_LENGTH]; /**< DataPDUInOrder */
    /** DataSequenceInOrder */
    char              data_sequence_in_order[YES_NO_LENGTH];
    int               error_recovery_level; /**< ErrorRecoveryLevel */
    char              session_type[SESSION_TYPE_LENGTH]; /**< SessionType */

    iscsi_tgt_chap_data_t chap; /**< Serurity related data */
    char              device_name[MAX_DEVICE_NAME_LEN];
    char              connection_id[SESSION_ID_LENGTH]; 
                      /**< Windows iSCSI connection ID */
    int               prepare_device_attempts; 
                      /**< How many checks were made for device readiness */
    pthread_mutex_t   status_mutex; /**< Mutex to guard status field */
} iscsi_connection_data_t;


/**
 * Per target data of the Initiator.
 * Most of the fields correspond to operational parameters
 * with the same name. See RFC3260 for allowed values.
 */
typedef struct iscsi_target_data {
    int               target_id; /**< Id of the Target */
    te_bool           is_active;

    int               number_of_open_connections;
                                 /**< Number of initiated connections 
                                   *  to the target */   
    
    char              target_name[MAX_NAME_LENGTH];  /**< TargetName */
    char              target_addr[MAX_ADDR_LENGTH];  /**< TargetAddr */
    int               target_port;                   /**< TargetPort */

    iscsi_connection_data_t conns[MAX_CONNECTIONS_NUMBER];

    char              session_id[SESSION_ID_LENGTH];
                     /**< Open iSCSI db record id or Microsoft session ID */
} iscsi_target_data_t;

/**
 * Asynchronous connection status change request.
 */
typedef struct iscsi_connection_req {
    int     target_id; /**< Target ID */
    int     cid;       /**< Connection ID */
    int     status;    /**< Desired connection status */

    struct iscsi_connection_req *next; /** Queue chain link */
} iscsi_connection_req;

/**
 * Initiator data structure.
 * Contains general information about the Initiator and
 * per target data.
 */
typedef struct iscsi_initiator_data {
    iscsi_initiator_type  init_type; /**< Type of the Initiator */

    int                   host_bus_adapter;  /**< Number of the host bus 
                                                  adapter. Usually 0 */
    int                   verbosity;  /**< Initiator verbosity level */

    char                  script_path[MAX_CMD_SIZE]; 
                                     /**< Path to iSCSI config scripts */

    int                   n_targets; /**< Number of configured targets */
    int                   n_connections; 
                          /**< Total number of brought up connections */
    iscsi_target_data_t   targets[MAX_TARGETS_NUMBER]; /**< Per target data */
    pthread_mutex_t       initiator_mutex;    /**< Request mutex */
    sem_t                 request_sem;
                          /**< Pending request semaphore */
    te_bool               request_thread_started;
    pthread_t             request_thread;
    pthread_t             timer_thread;
    iscsi_connection_req *request_queue_head; /**< Request queue head */
    iscsi_connection_req *request_queue_tail; /**< Request queue tail */
} iscsi_initiator_data_t;

/**
 * Varieties of configuration parameters
 */
enum iscsi_target_param_kind_t { 
    ISCSI_FIXED_PARAM, /**< Constant value */
    ISCSI_GLOBAL_PARAM, /**< Per-target values */
    ISCSI_OPER_PARAM,  /**< iSCSI operational parameters */
    ISCSI_SECURITY_PARAM /**< iSCSI security parameters */
};

/**
 * Generic parameter description for iscsi_write_param()
 */
typedef struct iscsi_target_param_descr_t
{
    uint32_t offer; /**< OFFER_XXX mask */
    char    *name;  /**< Parameter name */
    te_bool  is_string; /**< TRUE if the corresponding field
                         *  is `char *', 
                         *  FALSE if it is `int'
                         */
    enum iscsi_target_param_kind_t kind; 
                         /**< Parameter type.
                          *  It is used to choose one of data
                          *  structures to take a field from
                          */
    int      offset; /**< Field offset */
    char  *(*formatter)(void *); /**< Converter function from the raw
                                  *   field value to text representation
                                  *   (may be NULL)
                                  */
    te_bool (*predicate)(iscsi_target_data_t *,
                         iscsi_connection_data_t *,
                         iscsi_tgt_chap_data_t *);
                                 /**< Predicate function to determine if
                                  *   a given parameter really needs to be
                                  *   configured depending on other
                                  *   parameters.
                                  *   NULL == always TRUE
                                  */
} iscsi_target_param_descr_t;

/** Trailing element in iscsi_target_param_descr_t array */
#define ISCSI_END_PARAM_TABLE {0, NULL, FALSE, 0, -1, NULL, NULL}

/** Table of common constants for iSCSI used by iscsi_write_param() */
typedef struct iscsi_constant_t
{
    int  zero;                  /**< 0 */
    char true[2];               /**< "T" */
    char wildcard[2];           /**< "*" */
    char l5_tgt_auth[19];       /**< "CHAPWithTgtAuth */
} iscsi_constant_t;

/** Returns a pointer to master iSCSI initiator parameter table */
extern iscsi_initiator_data_t *iscsi_configuration(void);

#ifdef __CYGWIN__
/**
 *  Sends a printf-formatted string to MS iscsicli process.
 *  The process is (re)started if necessary
 *
 *  @return       Status code
 *  @param  fmt   Format string
 */
extern te_errno iscsi_send_to_win32_iscsicli(const char *fmt, ...);

/**
 *  Wait for a response from MS iscsicli process. A response should match
 *  one of @p pattern, @p abort_pattern or @p terminal_pattern.
 *  If @p pattern is matched, its sub-patterns starting from @p first_part
 *  till @p first_part + @p nparts are stored into @p buffers.
 *
 *  @return             Status code
 *  @retval 0           `pattern' has been matched
 *  @retval TE_EFAIL    `abort_pattern' has been matched
 *  @retval TE_ENODATA  `terminal_pattern' has been matched
 *
 *  @param pattern              Data pattern
 *  @param abort_pattern        Error message pattern or NULL
 *  @param terminal_pattern     Terminating line pattern or NULL
 *  @param first_part           First sub-pattern to store into @p buffers
 *  @param nparts               No of sub-patterns to store into @p buffers
 *  @param maxsize              Maximal length that fit into @p buffers
 *  @param buffers              A array of pointers to pre-allocated buffers
 *                              each holding at least @p maxsize bytes.
 *                              May be NULL if @p nparts == 0
 */
extern te_errno iscsi_win32_wait_for(regex_t *pattern, 
                                     regex_t *abort_pattern,
                                     regex_t *terminal_pattern,
                                     int first_part, 
                                     int nparts, 
                                     int maxsize, char **buffers);

/**
 * Attempt to disable all kinds of disk cacheing for iSCSI device
 *
 * @return         Status code
 * @param  devname Device name
 */
extern te_errno iscsi_win32_disable_readahead(const char *devname);

/**
 * Attempt to kill MS iscsicli process if it is running.
 * This function is implicitly called by iscsi_send_to_win32_iscsicli()
 *
 * @return         Status code
 */
extern te_errno iscsi_win32_finish_cli(void);

/**
 * Logs a Win32 error.
 * 
 * @param function      Function name
 * @param line          Line of code
 * @param errcode       Win32 error code. 
 *                      If 0, then actual error code is obtained 
 *                      from GetLastError()
 */
extern void iscsi_win32_report_error(const char *function, int line,
                                     unsigned long errcode);

/**
 * Logs the latest Win32 system error in the current location.
 * 
 * @sa iscsi_win32_report_error()
 */
#define ISCSI_WIN32_REPORT_ERROR() iscsi_win32_report_error(__FUNCTION__, __LINE__, 0)

/**
 * Logs a specific Win32 return code
 *
 * @sa iscsi_win32_report_error()
 */
#define ISCSI_WIN32_REPORT_RESULT(_rc) iscsi_win32_report_error(__FUNCTION__, __LINE__, (_rc))

#define ISCSI_AGENT_TYPE TE_TA_WIN32

/**
 * Connection request handler for MS iSCSI 
 *
 *  @return    Status code
 *  @param req Connection request
 */
extern te_errno iscsi_initiator_win32_set(iscsi_connection_req *req);

/**
 * Wait for Win32 SCSI device associated with the Initiator is ready.
 * 
 * @return      Status code
 * @param conn  Connection data
 */
extern te_errno iscsi_win32_prepare_device(iscsi_connection_data_t *conn);
extern te_bool iscsi_win32_init_regexps(void);

#else

extern int ta_system(const char *cmd);
extern int ta_system_ex(const char *cmd, ...);

/**
 * Connection request handler for UNH initiator
 *
 *  @return    Status code
 *  @param req Connection request
 */
extern te_errno iscsi_initiator_unh_set(iscsi_connection_req *req);

/**
 * Connection request handler for Open-iSCSI initiator
 *
 *  @return    Status code
 *  @param req Connection request
 */
extern te_errno iscsi_initiator_openiscsi_set(iscsi_connection_req *req);

/**
 * Connection request handler for L5 initiator under Linux
 *
 *  @return    Status code
 *  @param req Connection request
 */
extern te_errno iscsi_initiator_l5_set(iscsi_connection_req *req);

/**
 * Stop Open-iSCSI managing daemon
 *
 * @return status code
 */
extern te_errno iscsi_openiscsi_stop_daemon(void);

#define ISCSI_WIN32_REPORT_ERROR() ((void)0)

#define ISCSI_AGENT_TYPE TE_TA_UNIX

#endif

/**
 * Check whether a given parameter @p param needs to be configured 
 * in a certain situation.
 *
 * @return TRUE if the parameter should be configured
 *
 * @param param         Parameter description
 * @param tgt_data      Target-wide parameters
 * @param conn_data     iSCSI operational parameters
 * @param auth_data     iSCSI security parameters
 */
extern te_bool iscsi_is_param_needed(iscsi_target_param_descr_t *param,
                                     iscsi_target_data_t *tgt_data,
                                     iscsi_connection_data_t *conn_data,
                                     iscsi_tgt_chap_data_t *auth_data);

/**
 * Write an iSCSI parameter to a certain destination.
 *
 * @param outfunc       Output callback. It takes two arguments:
 *                      @p destination and a buffer with the parameter value
 * @param destination   Opaque data passed to @p outfunc
 * @param param         Parameter description
 * @param tgt_data      Target-wide parameters
 * @param conn_data     iSCSI operational parameters
 * @param auth_data     iSCSI security parameters
 */
extern void iscsi_write_param(void (*outfunc)(void *, char *),
                              void *destination,
                              iscsi_target_param_descr_t *param,
                              iscsi_target_data_t *tgt_data,
                              iscsi_connection_data_t *conn_data,
                              iscsi_tgt_chap_data_t *auth_data);

/**
 * Callback function for iscsi_write_param().
 * It writes the value to a FILE stream
 *
 * @param destination   FILE pointer
 * @param what          Parameter value
 */
extern void iscsi_write_to_file(void *destination, char *what);

/**
 * Callback function for iscsi_write_param().
 * It just copies @p what to @p destination.
 *
 * @param destination   Buffer pointer
 * @param what          Parameter value
 */
extern void iscsi_put_to_buf(void *destination, char *what);

/**
 * Callback function for iscsi_write_param().
 * It appends @p what to @p destination as per strcat().
 *
 * @param destination   Buffer pointer
 * @param what          Parameter value
 */
extern void iscsi_append_to_buf(void *destination, char *what);

/**
 * Formatting function for iscsi_write_param().
 *
 * @return "0" if @p val is "None", "1" otherwise
 * @param val   String value
 */
extern char *iscsi_not_none(void *val);

/**
 * Formatting function for iscsi_write_param().
 *
 * @return "1" if @p val is "Yes", "0" otherwise
 * @param val   String value
 */
extern char *iscsi_bool2int(void *val);

/**
 * Predicate function for iscsi_write_param().
 * 
 * @return TRUE if target authentication is requested
 *
 * @param param         Parameter description
 * @param tgt_data      Target-wide parameters
 * @param conn_data     iSCSI operational parameters
 * @param auth_data     iSCSI security parameters
 */
extern te_bool iscsi_when_tgt_auth(iscsi_target_data_t *target_data,
                                   iscsi_connection_data_t *conn_data,
                                   iscsi_tgt_chap_data_t *auth_data);

/**
 * Predicate function for iscsi_write_param().
 * 
 * @return TRUE if target authentication is not requested
 *
 * @param param         Parameter description
 * @param tgt_data      Target-wide parameters
 * @param conn_data     iSCSI operational parameters
 * @param auth_data     iSCSI security parameters
 */
extern te_bool iscsi_when_not_tgt_auth(iscsi_target_data_t *target_data,
                                       iscsi_connection_data_t *conn_data,
                                       iscsi_tgt_chap_data_t *auth_data);

/**
 * Predicate function for iscsi_write_param().
 * 
 * @return TRUE if any authentication is requested
 *
 * @param param         Parameter description
 * @param tgt_data      Target-wide parameters
 * @param conn_data     iSCSI operational parameters
 * @param auth_data     iSCSI security parameters
 */
extern te_bool iscsi_when_chap(iscsi_target_data_t *target_data,
                               iscsi_connection_data_t *conn_data,
                               iscsi_tgt_chap_data_t *auth_data);


#endif
