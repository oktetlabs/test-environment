/** @file
 * @brief Test Environment
 *
 * RCF subsystem internal definitions (used by RCF API library, 
 * RCF process and RCF PCH library).
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_RCF_INTERNAL_H__
#define __TE_RCF_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of RCF message or protocol command/answer */
#if (RCF_MAX_VAL + RCF_MAX_PATH * 2 + TE_OVERHEAD + RCF_MAX_ID) > 1024
#define RCF_MAX_LEN  (RCF_MAX_VAL + RCF_MAX_PATH * 2 + \
                      RCF_MAX_ID + TE_OVERHEAD)
#else
#define RCF_MAX_LEN  (4 * 1024)
#endif 

/** IPC RCF Server name */
#define RCF_SERVER      "TE_RCF"

/** Special SID for TA get log operation */
#define RCF_TA_GET_LOG_SID      1

/** @name Message flags */
#define BINARY_ATTACHMENT       1   /**< Binary attachment is provided;
                                         file is saved in lfile */
#define INTERMEDIATE_ANSWER     2   /**< Packet is received on the TA, but
                                         traffic receiving continues */
#define PARAMETERS_ARGV         4   /**< Routine parameters are passed in 
                                         arc/argv mode */
/*@}*/

/** @name Traffic flags */
#define TR_POSTPONED            1
#define TR_RESULTS              2
/*@}*/


/** RCF operation codes */
typedef enum {
    RCFOP_SHUTDOWN,         /**< Shutdown RCF */
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
    RCFOP_EXECUTE,          /**< Execute a routine in various contexts */
    RCFOP_KILL,             /**< Kill the process */
    RCFOP_CONFGRP_START,    /**< Start of configuration group */
    RCFOP_CONFGRP_END,      /**< End of configuration group */
    RCFOP_TADEAD            /**< Inform RCF that TA is dead */
} rcf_op_t;


/** Definition of the RCF internal protocol message format */
typedef struct rcf_msg {
    rcf_op_t opcode;          /**< Operation code - see above */
    int   flags;              /**< Auxiliary flag */
    int   sid;                /**< Session identifier */
    int   error;              /**< Error code (in the answer) */
    char  ta[RCF_MAX_NAME];   /**< Test Agent name */
    int   handle;             /**< CSAP handle or PID */
    int   num;                /**< Number of sent/received packets or 
                                   function arguments */
    unsigned int timeout;     /**< Timeout value (RCFOP_TRSEND_RECV,
                                   RCFOP_TRRECV_START) */
    int   intparm;            /**< Integer parameter: 
                                   variable type;
                                   routine arguments passing mode (argv);
                                   process priority (valid is >= 0);
                                   postponed and results flags
                                   (tr_* commands);
                                   routine return code (RCFOP_EXECUTE);
                                   answer error (RCFOP_TRSEND_RECV) */
    size_t data_len;          /**< Length of additional data */
    
    char  id[RCF_MAX_ID];     /**< TA type;
                                   variable name;
                                   routine name;
                                   object identifier;
                                   stack identifier */
                                 
    char  file[RCF_MAX_PATH]; /**< Local full file name */
    char  value[RCF_MAX_VAL]; /**< Value of the variable or object
                                   instance */

    char  data[0];            /**< Start of additional for commands:
                                   RCFOP_TALIST (list of names);
                                   RCFOP_TAREBOOT (parameters);
                                   RCFOP_CSAP_CREATE (parameters);
                                   RCFOP_EXECUTE (parameters);
                                   RCFOP_GET and RCFOP_PUT (remote file) */
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
 * @param op    - operation number
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
        case RCFOP_EXECUTE:         return "execute";
        case RCFOP_KILL:            return "kill";
        default:                    return "(unknown)";
    }
}

/** 
 * The ways a function may be called on TA
 */
enum rcf_start_modes { 
    RCF_START_FUNC,    /**< Execute a function in the same context */
    RCF_START_THREAD,  /**< Execute a function in another thread */
    RCF_START_FORK     /**< Execute a function in a forked process */
};

#ifdef __cplusplus
}
#endif
#endif /* __TE_RCF_INTERNAL_H__ */
