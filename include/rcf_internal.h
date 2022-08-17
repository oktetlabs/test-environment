/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * RCF subsystem internal definitions (used by RCF API library,
 * RCF process and RCF PCH library).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RCF_INTERNAL_H__
#define __TE_RCF_INTERNAL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of RCF message or protocol command/answer */
#if (RCF_MAX_VAL + RCF_MAX_PATH * 2 + TE_OVERHEAD + RCF_MAX_ID) > 4 * 1024
#define RCF_MAX_LEN  (RCF_MAX_VAL + RCF_MAX_PATH * 2 + \
                      RCF_MAX_ID + TE_OVERHEAD)
#else
#define RCF_MAX_LEN  (4 * 1024)
#endif

/** Special SID for TA get log operation */
#define RCF_TA_GET_LOG_SID      1

/** @name Message flags */
#define BINARY_ATTACHMENT       1   /**< Binary attachment is provided;
                                         file is saved in lfile */
#define INTERMEDIATE_ANSWER     2   /**< Packet is received on the TA, but
                                         traffic receiving continues */
#define PARAMETERS_ARGV         4   /**< Routine parameters are passed in
                                         argc/argv mode */
#define AGENT_REBOOT            8   /**< Reboot Test Agent process */
#define HOST_REBOOT            16   /**< Reboot the host with Test Agent
                                         process */
#define COLD_REBOOT            32   /**< Cold reboot host */
/*@}*/

/** @name Traffic flags */
#define TR_POSTPONED            1
#define TR_RESULTS              2
#define TR_NO_PAYLOAD           4
#define TR_SEQ_MATCH            8
#define TR_MISMATCH             0x10
/*@}*/


/** RCF operation codes */
typedef enum {
    RCFOP_SHUTDOWN = 1,     /**< Shutdown RCF */
    RCFOP_TALIST,           /**< Get list of Test Agents */
    RCFOP_TACHECK,          /**< Check running TA */
    RCFOP_TATYPE,           /**< Get TA type */
    RCFOP_SESSION,          /**< Create a session */
    RCFOP_REBOOT,           /**< Reboot the Test Agent */
    RCFOP_CONFGET,          /**< Configuration command "get" */
    RCFOP_CONFSET,          /**< Configuration command "set" */
    RCFOP_CONFADD,          /**< Configuration command "add" */
    RCFOP_CONFDEL,          /**< Configuration command "del" */
    RCFOP_GET_LOG,          /**< Obtain log */
    RCFOP_VREAD,            /**< Get variable value */
    RCFOP_VWRITE,           /**< Change variable value */
    RCFOP_FPUT,             /**< Load file to TA */
    RCFOP_FGET,             /**< Load file from TA */
    RCFOP_FDEL,             /**< Delete file from TA */
    RCFOP_CSAP_CREATE,      /**< Create CSAP */
    RCFOP_CSAP_DESTROY,     /**< Destroy CSAP */
    RCFOP_CSAP_PARAM,       /**< Obtain CSAP parameter value */
    RCFOP_TRSEND_START,     /**< Start traffic generation */
    RCFOP_TRSEND_STOP,      /**< Stop traffic generation */
    RCFOP_TRRECV_START,     /**< Start traffic receiving */
    RCFOP_TRRECV_STOP,      /**< Stop traffic receiving */
    RCFOP_TRRECV_GET,       /**< Get received packets */
    RCFOP_TRRECV_WAIT,      /**< Wait for finish receiving packets */
    RCFOP_TRSEND_RECV,      /**< Send one packet and receive an answer */
    RCFOP_TRPOLL,           /**< Wait for send or receive completion */
    RCFOP_TRPOLL_CANCEL,    /**< Cancel poll request with specified ID */
    RCFOP_EXECUTE,          /**< Execute a routine in various contexts */
    RCFOP_RPC,              /**< Execute an RPC */
    RCFOP_KILL,             /**< Kill the process */
    RCFOP_CONFGRP_START,    /**< Start of configuration group */
    RCFOP_CONFGRP_END,      /**< End of configuration group */
    RCFOP_ADD_TA,           /**< Add "live start/stop" TA */
    RCFOP_DEL_TA,           /**< Delete "live start/stop" TA */
    RCFOP_TADEAD,           /**< Inform RCF that TA is dead */
    RCFOP_GET_SNIFFERS,     /**< Obtain the list of sniffers */
    RCFOP_GET_SNIF_DUMP,    /**< Pull out capture logs of the sniffer */
} rcf_op_t;


/** Definition of the RCF internal protocol message format */
typedef struct rcf_msg {
    rcf_op_t opcode;             /**< Operation code - see above */
    uint32_t seqno;              /**< Sequence number */
    int      flags;              /**< Auxiliary flag */
    int      sid;                /**< Session identifier */
    te_errno error;              /**< Error code (in the answer) */
    char     ta[RCF_MAX_NAME];   /**< Test Agent name */
    int      handle;             /**< CSAP handle or PID */
    int      num;                /**< Number of sent/received packets
                                      or process priority*/
    uint32_t timeout;            /**< Timeout value (RCFOP_TRSEND_RECV,
                                      RCFOP_TRRECV_START, RCFOP_TRPOLL,
                                      RCFOP_RPC) */
    int      intparm;            /**< Integer parameter:
                                       variable type;
                                       routine arguments passing mode;
                                       execute mode;
                                       postponed and results flags
                                       (tr_* commands);
                                       routine return code (RCFOP_EXECUTE);
                                       encode data length (RCFOP_RPC);
                                       answer error (RCFOP_TRSEND_RECV);
                                       poll request ID (RCFOP_TRPOLL,
                                       RCFOP_TRPOLL_CANCEL) */
    size_t   data_len;          /**< Length of additional data */
    char     id[RCF_MAX_ID];    /**< TA type;
                                     variable name;
                                     routine name;
                                     object identifier;
                                     stack identifier;
                                     RPC server name */

    char  file[RCF_MAX_PATH];   /**< Local full file name */
    char  value[RCF_MAX_VAL];   /**< Value of the variable or object
                                     instance */

    char  data[0];              /**< Start of additional for commands:
                                     RCFOP_TALIST (list of names);
                                     RCFOP_TAREBOOT (parameters);
                                     RCFOP_CSAP_CREATE (parameters);
                                     RCFOP_EXECUTE (parameters);
                                     RCFOP_RPC (encoded data);
                                     RCFOP_GET/PUT (remote file) */
} rcf_msg;

/** Parameters generated by rcf_make_params function */
typedef struct rcf_params {
    int data_len;     /**< Length of the rest of data */
    int argv;         /**< If true, parameters are passed in argv list */
    int argc;         /**< Number of parameters */
    /* List of strings or pairs: <type, value> */
} rcf_params;

/* Auxiliary macros */

/** Insert new element in list */
#define QEL_INSERT(list, new) \
    (list)->next = ((new)->next = ((new)->prev = list)->next)->prev = (new)

/**
 * Delete element from list; note, that you need not know reference to
 * list to delete element from it
 */
#define  QEL_DELETE(x)                                 \
    do {                                               \
        ((x)->prev->next =(x)->next)->prev =(x)->prev; \
        (x)->next = (x)->prev = x;                     \
    } while(0)


/**
 * Convert RCF operation number to text.
 *
 * @param op      operation number
 *
 * @return Pointer to null-terminated string.
 */
static inline const char *
rcf_op_to_string(rcf_op_t op)
{
    switch (op)
    {
        case RCFOP_SHUTDOWN:        return "shutdown";
        case RCFOP_TALIST:          return "TA list";
        case RCFOP_TACHECK:         return "TA check";
        case RCFOP_TATYPE:          return "TA type";
        case RCFOP_SESSION:         return "session";
        case RCFOP_REBOOT:          return "reboot";
        case RCFOP_CONFGET:         return "configure get";
        case RCFOP_CONFSET:         return "configure set";
        case RCFOP_CONFADD:         return "configure add";
        case RCFOP_CONFDEL:         return "configure delete";
        case RCFOP_CONFGRP_START:   return "configure group start";
        case RCFOP_CONFGRP_END:     return "configure group end";
        case RCFOP_GET_LOG:         return "get log";
        case RCFOP_VREAD:           return "vread";
        case RCFOP_VWRITE:          return "vwrite";
        case RCFOP_FPUT:            return "fput";
        case RCFOP_FGET:            return "fget";
        case RCFOP_FDEL:            return "fdel";
        case RCFOP_CSAP_CREATE:     return "csap create";
        case RCFOP_CSAP_DESTROY:    return "csap destroy";
        case RCFOP_CSAP_PARAM:      return "csap param";
        case RCFOP_TRSEND_START:    return "trsend start";
        case RCFOP_TRSEND_STOP:     return "trsend stop";
        case RCFOP_TRRECV_START:    return "trrecv start";
        case RCFOP_TRRECV_STOP:     return "trrecv stop";
        case RCFOP_TRRECV_GET:      return "trrecv get";
        case RCFOP_TRRECV_WAIT:     return "trrecv wait";
        case RCFOP_TRSEND_RECV:     return "trsendrecv";
        case RCFOP_TRPOLL:          return "trpoll";
        case RCFOP_TRPOLL_CANCEL:   return "trpoll_cancel";
        case RCFOP_EXECUTE:         return "execute";
        case RCFOP_RPC:             return "rpc";
        case RCFOP_KILL:            return "kill";
        case RCFOP_GET_SNIFFERS:    return "get sniffers";
        case RCFOP_GET_SNIF_DUMP:   return "get snif dump";
        default:                    return "(unknown)";
    }
}

/** Type of IPC used by RCF on Test Engine */
#define RCF_IPC     (TRUE)  /* Connection-oriented IPC */

#ifdef __cplusplus
}
#endif
#endif /* __TE_RCF_INTERNAL_H__ */
