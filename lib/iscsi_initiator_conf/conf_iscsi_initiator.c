/** @file
 * @brief iSCSI Initiator configuration
 *
 * Unix TA configuring support
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
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
 */

/**
 * TODO: in each set function the check for the correctness of the
 * input parameter should be added
 */
#include "te_config.h"
#include "package.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <glob.h>
#include <pthread.h>
#include <semaphore.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_shell_cmd.h"
#include "ndn_iscsi.h"
#include "te_iscsi.h"
#include "te_tools.h"

extern int ta_system(const char *cmd);

/* Type of the agent (usually TE_TA_UNIX or TE_TA_WIN32),
 *  used primarily for error codes
 */
static int iscsi_agent_type;

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

/** Length of Open iSCSI record ID */
#define RECORD_ID_LENGTH 32

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
    BASE_16,   /**< Base 16 encoding */
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
    int         target_auth; /**< If TRUE than Target authentication is
                                  required during the Security Phase */
} iscsi_tgt_chap_data_t;

typedef struct iscsi_connection_data {
    iscsi_connection_status status;

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
    
    
    char              target_name[MAX_NAME_LENGTH];  /**< TargetName */
    char              target_addr[MAX_ADDR_LENGTH];  /**< TargetAddr */
    int               target_port;                   /**< TargetPort */

    iscsi_connection_data_t conns[MAX_CONNECTIONS_NUMBER];

    char              record_id[RECORD_ID_LENGTH]; 
                     /**< Open iSCSI db record id */
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
    pthread_t             request_thread;
    iscsi_connection_req *request_queue_head; /**< Request queue head */
    iscsi_connection_req *request_queue_tail; /**< Request queue tail */
} iscsi_initiator_data_t;


/** Initiator data */
static iscsi_initiator_data_t *init_data = NULL;

/**
 * Function returns target ID from the name of the
 * instance:
 * /agent:Agt_A/iscsi_initiator:/target_data:target_x/...
 * the target id is 'x'
 *
 * @param oid    The full name of the instance
 *
 * @return ID of the target
 */
static int
iscsi_get_target_id(const char *oid)
{
    int   tgt_id;
    
    sscanf(oid, "/agent:%*[^/]/iscsi_initiator:/target_data:target_%d", 
           &tgt_id);

    return tgt_id;
}

/**
 * Function returns CID from the name of the instance.
 *
 * @param oid The full name of the instance
 *
 * @return ID of the connection
 */
static int
iscsi_get_cid(const char *oid)
{
    int cid;

    sscanf(oid, 
           "/agent:%*[^/]/iscsi_initiator:/target_data:target_%*d/conn:%d", 
           &cid);

    return cid;
}

/**
 * Function initalize default parameters:
 * operational parameters and security parameters.
 *
 * @param tgt_data    Structure of the target data to
 *                    initalize.
 */
void
iscsi_init_default_connection_parameters(iscsi_connection_data_t *conn_data)
{
    memset(conn_data, 0, sizeof(conn_data));

    conn_data->status = ISCSI_CONNECTION_REMOVED;
    strcpy(conn_data->initiator_name, DEFAULT_INITIATOR_NAME);
    strcpy(conn_data->initiator_alias, DEFAULT_INITIATOR_ALIAS);
    
    conn_data->max_connections = DEFAULT_MAX_CONNECTIONS;
    strcpy(conn_data->initial_r2t, DEFAULT_INITIAL_R2T);
    strcpy(conn_data->header_digest, DEFAULT_HEADER_DIGEST);
    strcpy(conn_data->data_digest, DEFAULT_DATA_DIGEST);
    strcpy(conn_data->immediate_data, DEFAULT_IMMEDIATE_DATA);
    conn_data->max_recv_data_segment_length = 
        DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH;
    conn_data->first_burst_length = DEFAULT_FIRST_BURST_LENGTH;
    conn_data->max_burst_length = DEFAULT_MAX_BURST_LENGTH;
    conn_data->default_time2wait = DEFAULT_DEFAULT_TIME2WAIT;
    conn_data->default_time2retain = DEFAULT_DEFAULT_TIME2RETAIN;
    conn_data->max_outstanding_r2t = DEFAULT_MAX_OUTSTANDING_R2T;
    strcpy(conn_data->data_pdu_in_order, DEFAULT_DATA_PDU_IN_ORDER);
    strcpy(conn_data->data_sequence_in_order, 
           DEFAULT_DATA_SEQUENCE_IN_ORDER);
    conn_data->error_recovery_level = DEFAULT_ERROR_RECOVERY_LEVEL;
    strcpy(conn_data->session_type, DEFAULT_SESSION_TYPE);

    /* target's chap */
    conn_data->chap.target_auth = FALSE;
    *(conn_data->chap.peer_secret) = '\0';
    *(conn_data->chap.local_name) = '\0';
    strcpy(conn_data->chap.chap, "None");
    conn_data->chap.enc_fmt = BASE_16;
    conn_data->chap.challenge_length = DEFAULT_CHALLENGE_LENGTH;
    *(conn_data->chap.peer_name) = '\0';
    *(conn_data->chap.local_secret) = '\0';
}
/**
 * Function initalize default parameters:
 * operational parameters and security parameters.
 *
 * @param tgt_data    Structure of the target data to
 *                    initalize.
 */
void
iscsi_init_default_tgt_parameters(iscsi_target_data_t *tgt_data)
{
    int i;
    memset(tgt_data, 0, sizeof(tgt_data));
    
    strcpy(tgt_data->target_name, DEFAULT_TARGET_NAME);
    
    for (i = 0; i < MAX_CONNECTIONS_NUMBER; i++)
    {
        iscsi_init_default_connection_parameters(&tgt_data->conns[i]);
    }
}
/** Configure all targets (id: 0..MAX_TARGETS_NUMBER */
#define ISCSI_CONF_ALL_TARGETS -1

/** Configure only general Initiator data */
#define ISCSI_CONF_NO_TARGETS -2

static void *iscsi_initator_conn_request_thread(void *arg);

/**
 * Function configures the Initiator.
 *
 * @param how     ISCSI_CONF_ALL_TARGETS or ISCSI_CONF_NO_TARGETS
 *                or ID of the target to configure with the Initiator.
 *                The target is Initalized with default parameters.
 */
static void
iscsi_init_default_ini_parameters(int how)
{
    init_data = malloc(sizeof(iscsi_initiator_data_t));
    memset(init_data, 0, sizeof(init_data));

    pthread_mutex_init(&init_data->initiator_mutex, NULL);
    sem_init(&init_data->request_sem, 0, 0);

    /* init_id */
    /* init_type */
    /* initiator name */
    init_data->init_type = UNH;
    init_data->host_bus_adapter = DEFAULT_HOST_BUS_ADAPTER;
    
    if (how == ISCSI_CONF_NO_TARGETS)
    {
        int j;

        for (j = 0;j < MAX_TARGETS_NUMBER;j++)
        {
            init_data->targets[j].target_id = -1;
        }

        VERB("No targets were configured");
    }
    else if (how == ISCSI_CONF_ALL_TARGETS)
    {
        int j;

        for (j = 0;j < MAX_TARGETS_NUMBER;j++)
        {
            iscsi_init_default_tgt_parameters(&init_data->targets[j]);
            init_data->targets[j].target_id = j;
        }
    }
    else
    {
         iscsi_init_default_tgt_parameters(&init_data->targets[how]);
         init_data->targets[how].target_id = how;
    }
}

/**
 * Returns the last command received by the Initiator.
 */
static te_errno
iscsi_initiator_get(const char *gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    *value = '\0';
    return 0;
}

#if 0
/**
 * Executes te_shell_cmd() function.
 *
 * @param cmd    format of the command followed by
 *               parameters
 *
 * @return errno
 */
static te_errno
te_shell_cmd_ex(const char *cmd, ...)
{
    char    cmdline[MAX_CMD_SIZE];
    va_list ap;

    va_start(ap, cmd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);

    IVERB("iSCSI Initiator: %s\n", cmdline);
    return te_shell_cmd(cmdline, -1, NULL, NULL) > 0 ? 0 : TE_ESHCMD;
}
#endif

/**
 * Executes ta_system.
 *
 * @param cmd    format of the command followed by parameters
 *
 * @return -1 or result of the ta_system
 */
static int
ta_system_ex(const char *cmd, ...)
{
    char   *cmdline;
    int     status = 0;
    va_list ap;

    if ((cmdline = (char *)malloc(MAX_CMD_SIZE)) == NULL)
    {
        ERROR("Not enough memory\n");
        return TE_ENOMEM;
    }

    IVERB("fmt=\"%s\"\n", cmd);

    va_start(ap, cmd);
    vsnprintf(cmdline, MAX_CMD_SIZE, cmd, ap);
    va_end(ap);

    IVERB("iSCSI Initiator: %s\n", cmdline);

    status = ta_system(cmdline);
    IVERB("%s(): ta_system() call returns 0x%x\n", __FUNCTION__, status);
    fflush(stderr);

    free(cmdline);

    return WIFEXITED(status) ? 0 : TE_ESHCMD;
}

/**
 * Checks return code of the configuration
 * of the Initiator. If it is not zero calls return;
 *
 * @param rc_         return code
 * @param parameter_  name of the parameter which was configured
 */
#define CHECK_SHELL_CONFIG_RC(rc_, parameter_) \
    do {                                                                    \
        if (rc_ != 0)                                                       \
        {                                                                   \
            ERROR("Setting %s parameter failed, rc=%d",                     \
                  (parameter_), (rc_));                                     \
            return (rc_);                                                   \
        }                                                                   \
    } while (0)

enum iscsi_target_param_kind_t { 
    ISCSI_FIXED_PARAM,
    ISCSI_GLOBAL_PARAM,
    ISCSI_OPER_PARAM,
    ISCSI_SECURITY_PARAM
};

typedef struct iscsi_target_param_descr_t
{
    uint32_t offer;
    char    *name;
    te_bool  is_string;
    enum iscsi_target_param_kind_t kind;
    int      offset;
    char  *(*formatter)(void *);
    te_bool (*predicate)(iscsi_target_data_t *,
                         iscsi_connection_data_t *,
                         iscsi_tgt_chap_data_t *);
} iscsi_target_param_descr_t;

#define ISCSI_END_PARAM_TABLE {0, NULL, FALSE, 0, -1, NULL, NULL}

typedef struct iscsi_constant_t
{
    int zero;
    char true[2];
    char wildcard[2];
    char l5_tgt_auth[19];
} iscsi_constant_t;

static te_bool
iscsi_is_param_needed(iscsi_target_param_descr_t *param,
                      iscsi_target_data_t *tgt_data,
                      iscsi_connection_data_t *conn_data,
                      iscsi_tgt_chap_data_t *auth_data)
{
    return (param->predicate != NULL ?
            param->predicate(tgt_data, conn_data, auth_data) : 
            TRUE);
}

static void
iscsi_write_param(void (*outfunc)(void *, char *),
                  void *destination,
                  iscsi_target_param_descr_t *param,
                  iscsi_target_data_t *tgt_data,
                  iscsi_connection_data_t *conn_data,
                  iscsi_tgt_chap_data_t *auth_data)
{
    static iscsi_constant_t constants = {0, "T", "*", 
                                         "CHAPWithTargetAuth"
    };
    void *dataptr;

    switch(param->kind)
    {
        case ISCSI_FIXED_PARAM:
            dataptr = &constants;
            break;
        case ISCSI_GLOBAL_PARAM:
            dataptr = tgt_data;
            break;
        case ISCSI_OPER_PARAM:
            dataptr = conn_data;
            break;
        case ISCSI_SECURITY_PARAM:
            dataptr = auth_data;
            break;
        default:
            ERROR("Internal error at %s:%d %s(): bad parameter kind",
                  __FILE__, __LINE__, __FUNCTION__);
            return;
    }
    if (param->formatter != NULL)
    {
        char *result;
        
        result = param->formatter((char *)dataptr + param->offset);
        outfunc(destination, result);
    }
    else
    {
        if (param->is_string)
            outfunc(destination, (char *)dataptr + param->offset);
        else
        {
            char number[32];
            snprintf(number, sizeof(number), 
                     "%d", *(int *)((char *)dataptr + 
                                    param->offset));
            outfunc(destination, number);
        }
    }
}

static void
iscsi_write_to_file(void *destination, char *what)
{
    fputs(what, destination);
}

static void
iscsi_put_to_buf(void *destination, char *what)
{
    strcpy(destination, what);
}

static char *
iscsi_not_none (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "None") == 0 ? '0' : '1');
    return buf;
}

static char *
iscsi_bool2int (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "Yes") == 0 ? '1' : '0');
    return buf;
}

static te_bool
iscsi_when_tgt_auth(iscsi_target_data_t *target_data,
                    iscsi_connection_data_t *conn_data,
                    iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return auth_data->target_auth;
}

static te_bool
iscsi_when_not_tgt_auth(iscsi_target_data_t *target_data,
                        iscsi_connection_data_t *conn_data,
                        iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return !auth_data->target_auth;
}

static te_bool
iscsi_when_chap(iscsi_target_data_t *target_data,
                iscsi_connection_data_t *conn_data,
                iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return strstr(auth_data->chap, "CHAP") != NULL;
}

/****** L5 Initiator specific stuff  ****/

static void
iscsi_l5_write_param(FILE *destination, 
                     iscsi_target_param_descr_t *param,
                     iscsi_target_data_t *tgt_data,
                     iscsi_connection_data_t *conn_data,
                     iscsi_tgt_chap_data_t *auth_data)
{
    const char *n;

    if (!iscsi_is_param_needed(param, tgt_data, conn_data, auth_data))
        return;

    for (n = param->name; *n != '\0'; n++)
    {
        if (*n != '_')
            fputc(*n, destination);
    }
    fputs(": ", destination);
    iscsi_write_param(iscsi_write_to_file, destination, param, 
                      tgt_data, conn_data, auth_data);
    fputc('\n', destination);
}

static int
iscsi_l5_write_target_params(FILE *destination, 
                             iscsi_target_data_t *target)
{
    iscsi_target_param_descr_t *p;
    iscsi_connection_data_t    *connection = target->conns;

#define PARAMETER(field, offer, type) \
    {OFFER_##offer, #field, type, ISCSI_OPER_PARAM, \
     offsetof(iscsi_connection_data_t, field), NULL, NULL}
#define GPARAMETER(name, field, type) \
    {0, name, type, ISCSI_GLOBAL_PARAM, \
     offsetof(iscsi_target_data_t, field), NULL, NULL}
#define AUTH_PARAM(field, name, type, predicate) \
     {0, name, type, ISCSI_SECURITY_PARAM, \
      offsetof(iscsi_tgt_chap_data_t, field), NULL, predicate}
#define CONSTANT(name, field, type, predicate) \
    {0, name, type, ISCSI_FIXED_PARAM, \
    offsetof(iscsi_constant_t, field), NULL, predicate}


    static iscsi_target_param_descr_t session_params[] =
        {
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(initial_r2t, INITIAL_R2T,  TRUE),
            PARAMETER(immediate_data, IMMEDIATE_DATA,  TRUE),
            PARAMETER(first_burst_length, FIRST_BURST_LENGTH,  FALSE),
            PARAMETER(max_burst_length, MAX_BURST_LENGTH,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            PARAMETER(max_outstanding_r2t, MAX_OUTSTANDING_R2T,  FALSE),
            PARAMETER(data_pdu_in_order, DATA_PDU_IN_ORDER,  TRUE),
            PARAMETER(data_sequence_in_order, DATA_SEQUENCE_IN_ORDER,  TRUE),
            PARAMETER(error_recovery_level, ERROR_RECOVERY_LEVEL,  FALSE),
            AUTH_PARAM(local_name, "TargetCHAPName", TRUE, iscsi_when_tgt_auth),
            AUTH_PARAM(local_secret, "TargetCHAPSecret", TRUE, iscsi_when_tgt_auth),
            ISCSI_END_PARAM_TABLE
        };
    static iscsi_target_param_descr_t connection_params[] =
        {
            GPARAMETER("Host", target_addr, TRUE),
            GPARAMETER("Port", target_port, FALSE),
            PARAMETER(header_digest, HEADER_DIGEST,  TRUE),
            PARAMETER(data_digest, DATA_DIGEST,  TRUE),
            PARAMETER(max_recv_data_segment_length, MAX_RECV_DATA_SEGMENT_LENGTH,  
                      FALSE),
            AUTH_PARAM(chap, "AuthMethod", TRUE, iscsi_when_not_tgt_auth),
            CONSTANT("AuthMethod", l5_tgt_auth, TRUE, iscsi_when_tgt_auth),
            AUTH_PARAM(peer_name, "CHAPName", TRUE, iscsi_when_chap),
            AUTH_PARAM(peer_secret, "CHAPSecret", TRUE, iscsi_when_chap),
            ISCSI_END_PARAM_TABLE
        };

    for (p = session_params; p->name != NULL; p++)
    {
#if 0
        if ((connection->conf_params & p->offer) == p->offer)
        {
#endif
            iscsi_l5_write_param(destination, p, 
                                 target, connection, &connection->chap);
#if 0
        }
#endif
    }
    /** Other authentication parameters are not supported by
     *  L5 initiator, both on the level of script and on 
     *  the level of ioctls
     */

    for (; connection < target->conns + MAX_CONNECTIONS_NUMBER;
         connection++)
    {
        if (connection->status != ISCSI_CONNECTION_REMOVED)
        {
            fprintf(destination, "\n\n[target%d_conn%d]\n",
                    target->target_id,
                    (int)(connection - target->conns));

            for (p = connection_params; p->name != NULL; p++)
            {
#if 0
                if ((connection->conf_params & p->offer) == p->offer)
                {
#endif
                    iscsi_l5_write_param(destination, p, 
                                         target, connection, &connection->chap);
#if 0
                }
#endif
            }
        }
    }
    return 0;
#undef CONSTANT
#undef PARAMETER
#undef GPARAMETER
#undef AUTH_PARAM
}

static int
iscsi_l5_write_config(iscsi_initiator_data_t *iscsi_data)
{
    static char filename[MAX_CMD_SIZE];
    FILE *destination;
    iscsi_target_data_t *target;
    int conn_no;
    te_bool is_first;
    
    snprintf(filename, sizeof(filename),
             "%s/configs/te",
             *iscsi_data->script_path != '\0' ? 
             iscsi_data->script_path : ".");
    destination = fopen(filename, "w");
    if (destination == NULL)
    {
        ERROR("Cannot open '%s' for writing: %s",
              filename, strerror(errno));
        return TE_OS_RC(iscsi_agent_type, errno);
    }
             
    target = iscsi_data->targets;
    if (target->target_id < 0)
    {
        ERROR("No targets configured");
        return TE_RC(iscsi_agent_type, TE_ENOENT);
    }
    /** NOTE: L5 Initator seems to be unable 
     *  to have different Initator names for
     *  different Targets/Connections.
     *  So we just using the first one
     */
    fprintf(destination, "[INITIATOR]\n"
                         "Name: %s\n"
                         "Targets:",
            target->conns[0].initiator_name);
    is_first = TRUE;
    for (; target < iscsi_data->targets + MAX_TARGETS_NUMBER; target++)
    {
        if (target->target_id >= 0)
        {
            target->is_active = FALSE;
            for (conn_no = 0; conn_no < MAX_CONNECTIONS_NUMBER; conn_no++)
            {
                if (target->conns[conn_no].status != ISCSI_CONNECTION_REMOVED)
                {
                    target->is_active = TRUE;
                    break;
                }
            }
            if (target->is_active)
            {
                fprintf(destination, "%s target%d", 
                        is_first ? "" : ",",
                        target->target_id);
                is_first = FALSE;
            }
        }
    }

    target = iscsi_data->targets;
    for (; target < iscsi_data->targets + MAX_TARGETS_NUMBER; target++)
    {
        if (target->target_id >= 0 && target->is_active)
        {
            fprintf(destination, 
                    "\n\n[target%d]\n"
                    "TargetName: %s\n"
                    "Connections: ",
                    target->target_id,
                    target->target_name);
            is_first = TRUE;
            for (conn_no = 0; conn_no < MAX_CONNECTIONS_NUMBER; conn_no++)
            {
                if (target->conns[conn_no].status != ISCSI_CONNECTION_REMOVED)
                {
                    fprintf(destination, "%s target%d_conn%d",
                            is_first ? "" : ",",
                            target->target_id,
                            conn_no);
                    is_first = FALSE;
                }
            }
            fputs("\n\n", destination);
            iscsi_l5_write_target_params(destination, target);
        }
    }
    fclose(destination);
    ta_system_ex("cat %s", filename);
    return ta_system_ex("cd %s; ./iscsi_setconfig te", 
                           init_data->script_path);
}

/*** END of L5 specific stuff ***/


static int
iscsi_openiscsi_set_param(const char *recid,
                          iscsi_target_param_descr_t *param,
                          iscsi_target_data_t *target,
                          iscsi_connection_data_t *connection,
                          iscsi_tgt_chap_data_t *auth_data)
{
    int rc = 0;
    static char buffer[1024];

    if (!iscsi_is_param_needed(param, target, connection, auth_data))
        return 0;

    iscsi_write_param(iscsi_put_to_buf, buffer,
                      param, target, connection, auth_data);

    RING("Setting %s to %s", param->name, buffer);
    rc = ta_system_ex("iscsiadm -m node --record=%s --op=update "
                      "--name=%s --value=%s", recid,
                      param->name, buffer);
    return TE_RC(iscsi_agent_type, rc);
}


#define ISCSID_PID_FILE "/var/run/iscsid.pid"
#define ISCSID_RECORD_FILE "/var/db/iscsi/node.db"

static int iscsi_openiscsi_stop_daemon(void);

static int
iscsi_openiscsi_start_daemon(iscsi_target_data_t *target, te_bool force_start)
{
    int   rc = 0;
    FILE *name_file;
    FILE *pid_file;
    int   i;   

    RING("Starting iscsid daemon");
    fputs("> starting iscsid daemon\n", stderr);

    pid_file = fopen(ISCSID_PID_FILE, "r");

    if (pid_file != NULL)
    {
        int iscsid_pid = 0;
        if (fscanf(pid_file, "%d", &iscsid_pid) == 1)
        {
            if (force_start)
            {
                WARN("Stale iscsid (pid = %d) found, killing, iscsid_pid");
                ta_system("iscsiadm --stop");
                te_usleep(1000);
                kill(iscsid_pid, SIGKILL);
            }
            else
            {
                if (kill(iscsid_pid, 0) == 0)
                {
                    fclose(pid_file);
                    return 0;
                }
            }
        }
        fclose(pid_file);
    }
    remove(ISCSID_PID_FILE);
    remove(ISCSID_RECORD_FILE);

    name_file = fopen("/tmp/initiatorname.iscsi", "w");
    if (name_file == NULL)
    {
        rc = errno;
        ERROR("Cannot open /tmp/initiatorname.iscsi: %s",
              strerror(rc));
        return TE_OS_RC(iscsi_agent_type, rc);
    }
    fprintf(name_file, "InitiatorName=%s\n", target->conns[0].initiator_name);
    if (*target->conns[0].initiator_alias != '\0')
        fprintf(name_file, "InitiatorAlias=%s\n", target->conns[0].initiator_alias);
    fclose(name_file);

    ta_system_ex("iscsid %s -c /dev/null -i /tmp/initiatorname.iscsi", 
             init_data->verbosity > 0 ? "-d255" : "");

    for (i = 10; i != 0; i--)
    {
        if (access(ISCSID_PID_FILE, R_OK) == 0)
        {
            return 0;
        }
        te_usleep(1000);
    }
    ERROR("Cannot check that iscsid actually started");
    iscsi_openiscsi_stop_daemon();
    
    return TE_RC(iscsi_agent_type, TE_EFAIL);
}

static int
iscsi_openiscsi_stop_daemon(void)
{
    FILE *pid_file;

    RING("Stopping iscsid daemon");

    pid_file = fopen(ISCSID_PID_FILE, "r");

    if (pid_file != NULL)
    {
        int iscsid_pid = 0;
        if (fscanf(pid_file, "%d", &iscsid_pid) == 1)
        {
            fprintf(stderr, "< stopping iscsid daemon pid = %d\n", iscsid_pid);
            if (kill(iscsid_pid, 0) == 0)
            {
                ta_system("iscsiadm --stop");
                te_usleep(1000);
                kill(iscsid_pid, SIGKILL);
            }
        }
        fclose(pid_file);
    }
    remove(ISCSID_PID_FILE);

    return 0;
}

static int
iscsi_openiscsi_set_target_params(iscsi_target_data_t *target)
{
    iscsi_target_param_descr_t *p;
#define PARAMETER(field, name, offer, type) \
    {OFFER_##offer, name, type, ISCSI_OPER_PARAM, offsetof(iscsi_connection_data_t, field), NULL, NULL}
#define TGT_PARAMETER(field, name, type) \
    {0, name, type, ISCSI_GLOBAL_PARAM, offsetof(iscsi_target_data_t, field), NULL, NULL}
#define AUTH_PARAM(field, name, predicate) \
        {0, "node.session.auth." name, TRUE, ISCSI_SECURITY_PARAM, \
         offsetof(iscsi_tgt_chap_data_t, field), NULL, predicate}

    static iscsi_target_param_descr_t params[] =
        {
            TGT_PARAMETER(target_name, "node.name", TRUE),
            TGT_PARAMETER(target_addr, "node.conn[0].address", TRUE),
            TGT_PARAMETER(target_port, "node.conn[0].port", FALSE),
            PARAMETER(max_connections, 
                      "node.session.iscsi.MaxConnections", MAX_CONNECTIONS, FALSE),
            PARAMETER(initial_r2t, 
                      "node.session.iscsi.InitialR2T", INITIAL_R2T,  TRUE),
            PARAMETER(header_digest, 
                      "node.conn[0].iscsi.HeaderDigest", HEADER_DIGEST,  TRUE),
            PARAMETER(data_digest, 
                      "node.conn[0].iscsi.DataDigest", DATA_DIGEST,  TRUE),
            PARAMETER(immediate_data, 
                      "node.session.iscsi.ImmediateData", IMMEDIATE_DATA,  TRUE),
            PARAMETER(max_recv_data_segment_length, 
                      "node.conn[0].iscsi.MaxRecvDataSegmentLength", 
                      MAX_RECV_DATA_SEGMENT_LENGTH,  
                      FALSE),
            PARAMETER(first_burst_length, 
                      "node.session.iscsi.FirstBurstLength", 
                      FIRST_BURST_LENGTH,  FALSE),
            PARAMETER(max_burst_length, 
                      "node.session.iscsi.MaxBurstLength", 
                      MAX_BURST_LENGTH,  FALSE),
            PARAMETER(default_time2wait, 
                      "node.session.iscsi.DefaultTime2Wait", 
                      DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, 
                      "node.session.iscsi.DefaultTime2Retain", 
                      DEFAULT_TIME2RETAIN,  FALSE),
            PARAMETER(max_outstanding_r2t, 
                      "node.session.iscsi.MaxOutstandingr2t", 
                      MAX_OUTSTANDING_R2T,  FALSE),
#if 0 /* not implemented in open iscsi */
            PARAMETER(data_pdu_in_order, 
                      "node.session.iscsi.DataPDUInOrder", 
                      DATA_PDU_IN_ORDER,  TRUE),
            PARAMETER(data_sequence_in_order, 
                      "node.session.iscsi.DataSequenceInOrder", 
                      DATA_SEQUENCE_IN_ORDER,  TRUE),
#endif
#if 0 /* not implemented in the initiator config tree */
            PARAMETER(if_marker, 
                      "node.session.iscsi.IFMarker", 
                      IF_MARKER,  TRUE),
            PARAMETER(of_marker, 
                      "node.session.iscsi.OFMarker", 
                      OF_MARKER,  TRUE),

#endif
#if 0 /* not implemented both in the initiator and the config tree */
            PARAMETER(if_mark_int, 
                      "node.session.iscsi.IFMarkInt", 
                      IF_MARKER,  TRUE),
            PARAMETER(of_mark_int, 
                      "node.session.iscsi.OFMarkInt", 
                      OF_MARKER,  TRUE),

#endif

            PARAMETER(error_recovery_level, 
                      "node.session.iscsi.ERL", 
                      ERROR_RECOVERY_LEVEL,  FALSE),
            AUTH_PARAM(chap, "authmethod", NULL),
            AUTH_PARAM(peer_name, "username", NULL),
            AUTH_PARAM(peer_secret, "password", NULL),
            AUTH_PARAM(local_name, "username_in", iscsi_when_tgt_auth),
            AUTH_PARAM(local_secret, "password_in", iscsi_when_tgt_auth),
            ISCSI_END_PARAM_TABLE
        };

    int   rc;

    for (p = params; p->name != NULL; p++)
    {
        if (p->offer == 0 || 
            ((target->conns[0].conf_params & p->offer) == p->offer))
        {
            if ((rc = iscsi_openiscsi_set_param(target->record_id, 
                                                p, target, target->conns,
                                                &target->conns[0].chap)) != 0)
            {
                ERROR("Unable to set param %s: %r", p->name, rc);
                return rc;
            }
        }
    }
    return 0;
#undef TGT_PARAMETER
#undef PARAMETER
#undef AUTH_PARAM
}

static const char *
iscsi_openiscsi_alloc_node(iscsi_initiator_data_t *data,
                           const char *target, 
                           int target_port)
{
    char      buffer[80];
    FILE     *nodelist;
    int       status;

    static char recid[RECORD_ID_LENGTH];

    
    snprintf(buffer, sizeof(buffer), 
             "iscsiadm %s -m node --op=new --portal=%s:%d",
             data->verbosity != 0 ? "-d255" : "",
             target, target_port);
    RING("Attempting to create a record for %s:%d", target, target_port);
    nodelist = popen(buffer, "r");
    if (nodelist == NULL)
    {
        ERROR("Unable to get the list of records: %s",
              strerror(errno));
        return NULL;
    }

    if (fgets(buffer, sizeof(buffer), nodelist) == NULL)
    {
        ERROR("EOF from iscsiadm, something's wrong");
        return NULL;
    }
    
    RING("Got '%s' from iscsiadm", buffer);
    status = pclose(nodelist);
    if (status != 0)
    {
        if (status < 0)
            WARN("Error while waiting for iscsiadm: %s", strerror(errno));
        else
            WARN("iscsiadm terminated abnormally wuth code %x", 
                 (unsigned)status);
    }
    
    *recid = '\0';
    if (sscanf(buffer, "new iSCSI node record added: [%[^]]]", recid) != 1)
    {
        ERROR("Unparsable output from iscsiadm: '%s'", buffer);
        return NULL;
    }

    return recid;
}

/** Format of the set command for UNH Initiator */
static const char *conf_iscsi_unh_set_fmt =
"iscsi_manage init set%s %s=%s target=%d host=%d";

/** Format of the set int value command for UNH Initiator */
static const char *conf_iscsi_unh_set_int_fmt =
"iscsi_manage init set%s %s=%d target=%d host=%d";

/** Format of the force command for UNH Initiator */
static const char *conf_iscsi_unh_force_fmt =
"iscsi_manage init force %s=%s target=%d host=%d";

/** 
 * Format of the force string command for UNH Initiator.
 * The string value is written in notation : "...".
 */
static const char *conf_iscsi_unh_force_string_fmt =
"iscsi_manage init force %s=\"%s\" target=%d host=%d";

/** Format of the set int value command for UNH Initiator */
static const char *conf_iscsi_unh_force_int_fmt =
"iscsi_manage init force %s=%d target=%d host=%d";

/** Format of the force for flag command for UNH Initiator */
static const char *conf_iscsi_unh_force_flag_fmt =
"iscsi_manage init force %s target=%d host=%d";

#define SHOULD_OFFER(offer_, param_) \
    (((offer_) & (param_)) == (param_))

/**
 * Sets parameter of the UNH Initiator.
 *
 * @param param_            Name of the parameter
 * @param value_            New value of the parameter
 * @param target_id_        ID of the target for which the parameter
 *                          is set
 */
#define ISCSI_UNH_SET_UNNEGOTIATED(param_, value_, target_id_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_SET_UNNEGOTIATED(%s,%p,%d)\n",                    \
                param_, value_, target_id_);                      \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_fmt,                            \
                         "",                                                \
                         (param_),                                          \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (param_));                                                      \
    } while (0)

/**
 * Sets parameter of the UNH Initiator.
 *
 * @param param_            Name of the parameter
 * @param value_            New value of the parameter
 * @param target_id_        ID of the target for which the parameter
 *                          is set
 * @param mask_             Bit mask for the parameter
 * @param offered_params_   Set of params that should be offered by the
 *                          Initiator during the login phase.
 */
#define ISCSI_UNH_SET(param_, value_, target_id_, mask_, offered_params_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_SET(%s,%p,%d)\n",                                \
                param_, value_, target_id_);                      \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_fmt,                            \
                         SHOULD_OFFER((offered_params_), (mask_)) ? "":"p",\
                         (param_),                                          \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (param_));                                                      \
    } while (0)

/**
 * Sets int parameter of the UNH Initiator.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 * @param mask_             Bit mask for the parameter
 * @param offered_params_   Set of params that should be offered by the
 *                          Initiator during the login phase.
 */
#define ISCSI_UNH_SET_INT(param_, value_, target_id_, mask_, offered_params_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_SET_INT(%s,%d,%d)\n",                            \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_int_fmt,                        \
                         SHOULD_OFFER((offered_params_), (mask_)) ? "":"p",\
                         (param_),                                          \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (param_));                                                      \
    } while (0)
/**
 * "Forces" parameter of the UNH Initiator.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE(param_, value_, target_id_, info_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_FORCE(%s,%p,%d)\n",                              \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_force_fmt, (param_),                \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (info_));                                                       \
    } while (0)

/**
 * "Forces" string parameter of the UNH Initiator.
 * Is called when the parameter should be passed in '"'.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE_STRING(param_, value_, target_id_, info_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_FORCE(%s,%p,%d)\n",                              \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_force_string_fmt, (param_),         \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (info_));                                                       \
    } while (0)
/**
 * "Forces" int parameter of the UNH Initiator.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE_INT(param_, value_, target_id_, info_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_FORCE_INT(%s,%d,%d)\n",                          \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_force_int_fmt, (param_),            \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (info_));                                                       \
    } while (0)

/**
 * "Forces" flag parameter of the UNH Initiator.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE_FLAG(flag_, target_id_, info_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_FORCE_FLAG(%s,%d)\n",                              \
                flag_, target_id_);                                         \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_force_flag_fmt, (flag_),            \
                         (target_id_),                                      \
                         init_data->host_bus_adapter),                      \
            (info_));                                                       \
    } while (0)

#if 0
/**
 * Parses the command received by the Initiator and retreives
 * connection ID and target ID from it.
 *
 * @param cmdline     Comman to retreive from
 * @param status      (OUT) UP or DOWN
 * @param cid         (OUT) Connection ID
 * @param target      (OUT) Target ID
 * 
 * @return 0 or errno
 */
static int
iscsi_get_cid_and_target(const char *cmdline, te_bool *status,
                         int *cid, int *target)
{
    char buffer[32] = "";
    if (sscanf(cmdline, "%32s %d %d", buffer, cid, target) != 3)
    {
        ERROR("Error parsing '%s'", cmdline);
        return TE_EINVAL;
    }
    if (strcmp(buffer, "up") == 0)
        *status = TRUE;
    else if (strcmp(buffer, "down") == 0)
        *status = FALSE;
    else
    {
        ERROR("Invalid status value '%s'", buffer);
        return TE_EINVAL;
    }
    return 0;
}
#endif

/**
 * 
 * @param target_id The target with connection to which the operation should
 *                  be performed.
 * @param cid       CID of the new connection or ISCSI_CONNECTION_REMOVED
 */
static int
iscsi_initiator_unh_set(iscsi_connection_req *req)

{
    int                      rc = -1;
    iscsi_target_data_t     *target;
    iscsi_connection_data_t *conn;

    int                     offer;

    target = &init_data->targets[req->target_id];
    conn = &target->conns[req->cid];
    
    offer = conn->conf_params;

    IVERB("Current number of open connections: %d", 
          target->number_of_open_connections);
    
    if (req->status == ISCSI_CONNECTION_DOWN || 
        req->status == ISCSI_CONNECTION_REMOVED)
    {
        int former_status;
        
        pthread_mutex_lock(&init_data->initiator_mutex);
        former_status = target->conns[req->cid].status;
        target->conns[req->cid].status = ISCSI_CONNECTION_CLOSING;
        pthread_mutex_unlock(&init_data->initiator_mutex);

        rc = ta_system_ex("iscsi_config down cid=%d target=%d host=%d",
                          req->cid, req->target_id, 
                          init_data->host_bus_adapter);

        pthread_mutex_lock(&init_data->initiator_mutex);
        target->conns[req->cid].status = req->status;
        pthread_mutex_unlock(&init_data->initiator_mutex);

        if (former_status == ISCSI_CONNECTION_UP && 
            target->number_of_open_connections > 0)
        {
            target->number_of_open_connections--;
            init_data->n_connections--;
        }

        if (rc != 0)
        {
            ERROR("Failed to close the connection "
                  "with CID = %d", req->cid);
            return TE_RC(iscsi_agent_type, EINVAL);
        }
        INFO("Connections with ID %d is closed", req->cid);

    }
    else
    {
        /* We should open new connection */
        /* 1: configurating the Initiator */
        
        CHECK_SHELL_CONFIG_RC(
            ta_system_ex("iscsi_manage init restore target=%d host=%d",
                         req->target_id, init_data->host_bus_adapter),
            "Restoring");
        
        if (strcmp(conn->session_type, "Normal") == 0)
            ISCSI_UNH_SET_UNNEGOTIATED("TargetName", target->target_name, req->target_id);
        
        if (target->number_of_open_connections == 0)
        {
            if (strcmp(conn->session_type, "Normal") == 0)
            {
                ISCSI_UNH_SET_INT("MaxConnections", conn->max_connections,
                                  req->target_id, OFFER_MAX_CONNECTIONS, offer);
                ISCSI_UNH_SET("InitialR2T", conn->initial_r2t, req->target_id, 
                              OFFER_INITIAL_R2T, offer);
                ISCSI_UNH_SET("ImmediateData", conn->immediate_data, req->target_id,
                              OFFER_IMMEDIATE_DATA, offer);
                ISCSI_UNH_SET_INT("MaxBurstLength", 
                                  conn->max_burst_length, req->target_id,
                                  OFFER_MAX_BURST_LENGTH, offer);
                ISCSI_UNH_SET_INT("FirstBurstLength", 
                                  conn->first_burst_length, req->target_id,
                                  OFFER_FIRST_BURST_LENGTH, offer);
                ISCSI_UNH_SET_INT("MaxOutstandingR2T", 
                                  conn->max_outstanding_r2t, req->target_id,
                                  OFFER_MAX_OUTSTANDING_R2T, offer);
                ISCSI_UNH_SET("DataPDUInOrder", 
                              conn->data_pdu_in_order, req->target_id,
                              OFFER_DATA_PDU_IN_ORDER, offer);
                ISCSI_UNH_SET("DataSequenceInOrder", 
                              conn->data_sequence_in_order, req->target_id,
                              OFFER_DATA_SEQUENCE_IN_ORDER, offer);
            }
            
            ISCSI_UNH_SET("HeaderDigest", conn->header_digest, req->target_id,
                          OFFER_HEADER_DIGEST, offer);
            
            ISCSI_UNH_SET("DataDigest", conn->data_digest, req->target_id,
                          OFFER_DATA_DIGEST, offer);
            
            ISCSI_UNH_SET_INT("MaxRecvDataSegmentLength", 
                              conn->max_recv_data_segment_length,
                              req->target_id, OFFER_MAX_RECV_DATA_SEGMENT_LENGTH,
                              offer);
            
            ISCSI_UNH_SET_INT("DefaultTime2Wait", 
                              conn->default_time2wait, req->target_id,
                              OFFER_DEFAULT_TIME2WAIT, offer);
            
            ISCSI_UNH_SET_INT("DefaultTime2Retain", 
                              conn->default_time2retain, req->target_id,
                              OFFER_DEFAULT_TIME2RETAIN, offer);
            
            ISCSI_UNH_SET_INT("ErrorRecoveryLevel", 
                              conn->error_recovery_level, req->target_id,
                              OFFER_ERROR_RECOVERY_LEVEL, offer);
            
            ISCSI_UNH_SET_UNNEGOTIATED("SessionType", conn->session_type, req->target_id);
            
            
            
        }
        ISCSI_UNH_SET_UNNEGOTIATED("AuthMethod", conn->chap.chap, req->target_id);
        
        /* Target' CHAP */
        if (conn->chap.target_auth)
        {
            ISCSI_UNH_FORCE_FLAG("t", req->target_id,
                                 "Target Authentication");
        }
        
        ISCSI_UNH_FORCE_STRING("px", conn->chap.peer_secret, req->target_id,
                               "Peer Secret");
        
        ISCSI_UNH_FORCE("ln", conn->chap.local_name, req->target_id,
                        "Local Name");
        
        if (conn->chap.enc_fmt == BASE_64)
            ISCSI_UNH_FORCE_FLAG("b", req->target_id,
                                 "Encoding Format");
        
        ISCSI_UNH_FORCE_INT("cl", conn->chap.challenge_length, req->target_id,
                            "Challenge Length");
        
        ISCSI_UNH_FORCE("pn", conn->chap.peer_name, req->target_id,
                        "Peer Name");
        
        ISCSI_UNH_FORCE_STRING("lx", conn->chap.local_secret, req->target_id,
                               "Local Secret");
        
        ISCSI_UNH_FORCE_INT("sch", 1, req->target_id,
                            "Load-balancing"); /* Turning on round-robin */
        
        /* Initiator itself */
        ISCSI_UNH_SET_UNNEGOTIATED("InitiatorName", 
                                   conn->initiator_name,
                                   req->target_id);
#if 0
        ISCSI_UNH_SET_UNNEGOTIATED("InitiatorAlias", 
                                   conn->initiator_alias, 
                                   target_id);
#endif
        /* Now the connection should be opened */
        rc = ta_system_ex("iscsi_config up ip=%s port=%d "
                          "cid=%d target=%d host=%d lun=%d",
                          target->target_addr,
                          target->target_port,
                          req->cid, req->target_id, 
                          init_data->host_bus_adapter,
                          DEFAULT_LUN_NUMBER);
        pthread_mutex_lock(&init_data->initiator_mutex);
        conn->status = (rc == 0 ? ISCSI_CONNECTION_WAITING_DEVICE : 
                        ISCSI_CONNECTION_ABNORMAL);
        pthread_mutex_unlock(&init_data->initiator_mutex);

        if (rc != 0)
        {
            ERROR("Failed to establish connection with cid=%d", req->cid);
            return TE_RC(iscsi_agent_type, rc);
        }
        target->number_of_open_connections++;
        init_data->n_connections++;
    }
    return 0;
}

static int
iscsi_initiator_l5_set(iscsi_connection_req *req)
{
    int                  rc = -1;
    int                  former_status;
    iscsi_target_data_t *target = init_data->targets + req->target_id;
    
    switch (req->status)
    {
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            RING("Stopping connection %d, %d", req->target_id, req->cid);
            fprintf(stderr, "emergency: Stopping connection %d, %d\n", req->target_id, req->cid);
            
            if (strcmp(target->conns[req->cid].session_type, "Discovery") != 0)
            {
                pthread_mutex_lock(&init_data->initiator_mutex);
                former_status = target->conns[req->cid].status;
                target->conns[req->cid].status = ISCSI_CONNECTION_CLOSING;
                pthread_mutex_unlock(&init_data->initiator_mutex);
                rc = ta_system_ex("cd %s; ./iscsi_stopconns %s target%d_conn%d", 
                                  init_data->script_path,
                                  init_data->verbosity != 0 ? "-v" : "",
                                  req->target_id, req->cid);
                pthread_mutex_lock(&init_data->initiator_mutex);
                target->conns[req->cid].status = req->status;
                pthread_mutex_unlock(&init_data->initiator_mutex);

                if (rc != 0)
                {
                    ERROR("Unable to stop initiator connection %d, %d, "
                          "status = %d", 
                          req->target_id, req->cid, rc);
                    return TE_RC(iscsi_agent_type, TE_ESHCMD);
                }
                if (former_status == ISCSI_CONNECTION_UP && 
                    target->number_of_open_connections > 0)
                {
                    target->number_of_open_connections--;
                    init_data->n_connections--;
                }
                fprintf(stderr, "emergency: stopped connection %d, %d\n", req->target_id, req->cid);
            }
            break;
        case ISCSI_CONNECTION_UP:
            if (init_data->n_connections == 0)
            {
                rc = iscsi_l5_write_config(init_data);
                if (rc != 0)
                    return rc;
            }

            pthread_mutex_lock(&init_data->initiator_mutex);
            target->conns[req->cid].status = ISCSI_CONNECTION_ESTABLISHING;
            pthread_mutex_unlock(&init_data->initiator_mutex);

            if (strcmp(target->conns[req->cid].session_type, "Discovery") != 0)
            {
                rc = ta_system_ex("cd %s; ./iscsi_startconns %s target%d_conn%d", 
                                  init_data->script_path,
                                  init_data->verbosity != 0 ? "-v" : "",
                                  req->target_id, req->cid);
            }
            else
            {
                if (target->number_of_open_connections == 0)
                {
                    rc = ta_system_ex("cd %s; ./iscsi_discover te", 
                                         init_data->script_path);
                }
                else
                {
                    WARN("Discovery session already in progress");
                    rc = 0;
                }
            }
            pthread_mutex_lock(&init_data->initiator_mutex);
            target->conns[req->cid].status = (rc == 0 ? 
                                              ISCSI_CONNECTION_WAITING_DEVICE :
                                              ISCSI_CONNECTION_ABNORMAL);
            pthread_mutex_unlock(&init_data->initiator_mutex);
            if (rc != 0)
            {
                ERROR("Unable to start initiator connection %d, %d",
                      req->target_id, req->cid);
                return TE_RC(iscsi_agent_type, rc);
            }
            target->number_of_open_connections++;
            init_data->n_connections++;
            break;
        default:
            ERROR("Invalid operational code %d", req->status);
            return TE_RC(iscsi_agent_type, TE_EINVAL);
    }
    return 0;
}

static int
iscsi_initiator_openiscsi_set(iscsi_connection_req *req)
{
    int                     rc = -1;
    int                     rc2 = -1;
    iscsi_target_data_t    *target = init_data->targets + req->target_id;
    int                     former_status;

    if (req->status == ISCSI_CONNECTION_DOWN || 
        req->status == ISCSI_CONNECTION_REMOVED) 
    {
        if (*target->record_id == '\0')
        {
            ERROR("Target %d has no associated record id", req->target_id);
            return TE_RC(iscsi_agent_type, TE_EINVAL);
        }
        pthread_mutex_lock(&init_data->initiator_mutex);
        former_status = target->conns[req->cid].status;
        target->conns[req->cid].status = ISCSI_CONNECTION_CLOSING;
        pthread_mutex_unlock(&init_data->initiator_mutex);
        rc = ta_system_ex("iscsiadm -m node --record=%s --logout",
                          target->record_id);
        rc2 = ta_system_ex("iscsiadm -m node --record=%s --op=delete",
                           target->record_id);
        *target->record_id = '\0';
        if (former_status == ISCSI_CONNECTION_UP && 
            target->number_of_open_connections > 0)
        {
            target->number_of_open_connections--;
            init_data->n_connections--;
        }
        if (init_data->n_connections == 0)
        {
            iscsi_openiscsi_stop_daemon();
        }
        
        pthread_mutex_lock(&init_data->initiator_mutex);
        target->conns[req->cid].status = req->status;
        pthread_mutex_unlock(&init_data->initiator_mutex);
        return rc != 0? TE_RC(iscsi_agent_type, rc) : 
            (rc2 != 0 ? TE_RC(iscsi_agent_type, rc2) : 0);
    }
    else
    {
        pthread_mutex_lock(&init_data->initiator_mutex);
        target->conns[req->cid].status = ISCSI_CONNECTION_ESTABLISHING;
        pthread_mutex_unlock(&init_data->initiator_mutex);

        rc = iscsi_openiscsi_start_daemon(target, 
                                          init_data->n_connections == 0);
        if (rc != 0)
            return rc;

        if (*target->record_id == '\0')
        {
            const char *id = iscsi_openiscsi_alloc_node(init_data,
                                                        target->target_addr,
                                                        target->target_port);
            if (id == NULL)
            {
                return TE_RC(iscsi_agent_type, TE_ETOOMANY);
            }
            strcpy(target->record_id, id);
        }
        rc = iscsi_openiscsi_set_target_params(target);
        if (rc != 0)
        {
            return rc;
        }
        
        rc = ta_system_ex("iscsiadm -m node --record=%s --login",
                             target->record_id);
        if (rc == 0)
        {
            target->number_of_open_connections++;
            init_data->n_connections++;
            pthread_mutex_lock(&init_data->initiator_mutex);
            target->conns[req->cid].status = ISCSI_CONNECTION_WAITING_DEVICE;
            pthread_mutex_unlock(&init_data->initiator_mutex);
        }
        else
        {
            pthread_mutex_lock(&init_data->initiator_mutex);
            target->conns[req->cid].status = ISCSI_CONNECTION_ABNORMAL;
            pthread_mutex_unlock(&init_data->initiator_mutex);
        }
        
        return TE_RC(iscsi_agent_type, rc);
    }
    
    return 0;
}

static te_errno
iscsi_post_connection_request(int target_id, int cid, int status)
{
    iscsi_connection_req *req;

    RING("Posting connection status change request: %d,%d -> %d", target_id, cid, status);

    switch (status)
    {
        case ISCSI_CONNECTION_UP:
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            /* status is ok */
            break;
        default:
            return TE_RC(iscsi_agent_type, TE_EINVAL);
    }

    req = malloc(sizeof(*req));
    if (req == NULL)
        return TE_OS_RC(iscsi_agent_type, errno);
    req->target_id = target_id;
    req->cid       = cid;
    req->status    = status;
    req->next      = NULL;
    pthread_mutex_lock(&init_data->initiator_mutex);
    if (init_data->request_queue_tail != NULL)
        init_data->request_queue_tail->next = req;
    else
        init_data->request_queue_head = req;
    init_data->request_queue_tail = req;
    pthread_mutex_unlock(&init_data->initiator_mutex);
    sem_post(&init_data->request_sem);
    return 0;
}

static te_errno
iscsi_prepare_device (iscsi_connection_data_t *conn, int target_id)
{
    char        dev_pattern[128];
    glob_t      devices;
    int         rc = 0;
    char       *nameptr;
    FILE       *hba = NULL;

    switch (init_data->init_type)
    {
        case L5:
            hba = popen("T=`grep -l efabiscsi "
                        "/sys/class/scsi_host/host*/proc_name` && "
                        "B=${T%/proc_name} && "
                        "echo ${B##*/host}", "r");
            if (hba == NULL)
            {
                return TE_RC(iscsi_agent_type, TE_EAGAIN);
            }
            
            rc = fscanf(hba, "%d", &init_data->host_bus_adapter);
            if (rc <= 0)
            {
                return TE_RC(iscsi_agent_type, TE_EAGAIN);
            }
            pclose(hba);
            break;
        default:
        {
            if ((rc = glob("/sys/bus/scsi/devices/*/vendor", GLOB_ERR, NULL, &devices)) != 0)
            {
                switch(rc)
                {
                    case GLOB_NOSPACE:
                        ERROR("Cannot read a list of host bus adpaters: no memory");
                        return TE_RC(iscsi_agent_type, TE_ENOMEM);
                    case GLOB_ABORTED:
                        ERROR("Cannot read a list of host bus adapters: read error");
                        return TE_RC(iscsi_agent_type, TE_EIO);
                    case GLOB_NOMATCH:
                        pthread_mutex_lock(&init_data->initiator_mutex);
                        *conn->device_name = '\0';
                        pthread_mutex_unlock(&init_data->initiator_mutex);
                        return TE_RC(iscsi_agent_type, TE_EAGAIN);
                    default:
                        ERROR("unexpected error on glob()");
                        return TE_RC(iscsi_agent_type, TE_EFAIL);
                }
            }
            else
            {
                unsigned i;
                
                for (i = 0; i < devices.gl_pathc; i++)
                {
                    RING("Trying %s", devices.gl_pathv[i]);
                    hba = fopen(devices.gl_pathv[i], "r");
                    if (hba == NULL)
                    {
                        WARN("Cannot open %s: %s", devices.gl_pathv[i], 
                             strerror(errno));
                    }
                    else
                    {
                        *dev_pattern = '\0';
                        fgets(dev_pattern, sizeof(dev_pattern) - 1, hba);
                        RING("Vendor reported is %s", dev_pattern);
                        fclose(hba);
                        if (strstr(dev_pattern, "UNH") != NULL)
                        {
                            rc = sscanf(devices.gl_pathv[i], "/sys/bus/scsi/devices/%d:", 
                                        &init_data->host_bus_adapter);
                            if (rc != 1)
                            {
                                ERROR("Something strange with /sys/bus/scsi/devices");
                                globfree(&devices);
                                return TE_RC(iscsi_agent_type, TE_EFAIL);
                            }
                            break;
                        }
                    }
                }
                if (i == devices.gl_pathc)
                {
                    globfree(&devices);
                    return TE_RC(iscsi_agent_type, TE_EAGAIN);
                }
                globfree(&devices);
            }
            RING("Host bus adapter detected as %d", init_data->host_bus_adapter);
            break;
        }
    }
       
    sprintf(dev_pattern, "/sys/bus/scsi/devices/%d:*:%d/block*", 
            init_data->host_bus_adapter, target_id);
    if ((rc = glob(dev_pattern, GLOB_ERR, NULL, &devices)) != 0)
    {
        switch(rc)
        {
            case GLOB_NOSPACE:
                ERROR("Cannot read a list of devices: no memory");
                return TE_RC(iscsi_agent_type, TE_ENOMEM);
            case GLOB_ABORTED:
                ERROR("Cannot read a list of devices: read error");
                return TE_RC(iscsi_agent_type, TE_EIO);
            case GLOB_NOMATCH:
                RING("No items for %s", dev_pattern);
                pthread_mutex_lock(&init_data->initiator_mutex);
                *conn->device_name = '\0';
                pthread_mutex_unlock(&init_data->initiator_mutex);
                return TE_RC(iscsi_agent_type, TE_EAGAIN);
            default:
                ERROR("unexpected error on glob()");
                return TE_RC(iscsi_agent_type, TE_EFAIL);
        }
    }

    if (devices.gl_pathc > 1)
    {
        WARN("Stale devices detected; hoping we choose the right one");
    }

    if (realpath(devices.gl_pathv[devices.gl_pathc - 1], dev_pattern) == NULL)
    {
        rc = TE_OS_RC(iscsi_agent_type, errno);
        WARN("Cannot resolve %s: %r", 
             devices.gl_pathv[devices.gl_pathc - 1],
             rc);
    }
    else
    {
        int  fd;

        nameptr = strrchr(dev_pattern, '/');
        if (nameptr == NULL)
            WARN("Strange sysfs name: %s", dev_pattern);
        else
        {
            pthread_mutex_lock(&init_data->initiator_mutex);
            strcpy(conn->device_name, "/dev/");
            strcat(conn->device_name, nameptr + 1);
            pthread_mutex_unlock(&init_data->initiator_mutex);
        }
        /** Now checking that the device is active */
        strcpy(dev_pattern, devices.gl_pathv[0]);
        nameptr = strrchr(dev_pattern, '/');
        strcpy(nameptr + 1, "state");
        fd = open(dev_pattern, O_RDONLY);
        if (fd < 0)
        {
            rc = TE_OS_RC(iscsi_agent_type, errno);
            ERROR("Cannot get device state for %s: %r", 
                  dev_pattern, rc);
        }
        else
        {
            char state[16] = "";
            read(fd, state, sizeof(state) - 1);
            if (strcmp(state, "running\n") != 0)
            {
                WARN("Device is present but not ready: %s", state);
                pthread_mutex_lock(&init_data->initiator_mutex);
                *conn->device_name = '\0';
                pthread_mutex_unlock(&init_data->initiator_mutex);
                rc = TE_RC(iscsi_agent_type, TE_EAGAIN);
            }
            close(fd);
        }
    }
    globfree(&devices);
    if (rc == 0)
    {
        int fd = open(conn->device_name, O_WRONLY | O_SYNC);
        
        if (fd < 0)
        {
            rc = TE_OS_RC(iscsi_agent_type, errno);
            if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            {
                RING("Device %s is not ready :(", conn->device_name);
                pthread_mutex_lock(&init_data->initiator_mutex);
                *conn->device_name = '\0';
                pthread_mutex_unlock(&init_data->initiator_mutex);
                rc = TE_RC(iscsi_agent_type, TE_EAGAIN);
            }
            else
            {
                ERROR("Cannot open a device %s: %r",
                      conn->device_name, rc);
            }
        }
        else
        {
            const char buf[16] = "testing";
            errno = 0;
            if (write(fd, buf, sizeof(buf)) != sizeof(buf))
            {
                rc = (errno == 0 ? TE_RC(iscsi_agent_type, TE_ENOSPC) :
                      TE_OS_RC(iscsi_agent_type, errno));
                ERROR("Cannot write to device %s: %r",
                      conn->device_name, rc);
                close(fd);
            }
            else
            {
                if (close(fd) != 0)
                {
                    rc = TE_OS_RC(iscsi_agent_type, errno);
                    ERROR("Error syncing data to %s: %r",
                          conn->device_name, rc);
                }
            }
        }
    }
    return rc;
}

static void *
iscsi_initator_conn_request_thread(void *arg)
{
    iscsi_connection_req *current_req = NULL;
    
    int old_status;

    UNUSED(arg);
    
    for (;;)
    {
        sem_wait(&init_data->request_sem);
        RING("Got connection status change request");
        pthread_mutex_lock(&init_data->initiator_mutex);
        current_req = init_data->request_queue_head;
        if (current_req != NULL)
        {
            init_data->request_queue_head = current_req->next;
            if (init_data->request_queue_head == NULL)
                init_data->request_queue_tail = NULL;
        }
        pthread_mutex_unlock(&init_data->initiator_mutex);
        if (current_req == NULL)
            continue;

        if (current_req->cid == ISCSI_ALL_CONNECTIONS)
        {
            if (init_data->init_type == OPENISCSI)
            {
                iscsi_openiscsi_stop_daemon();
            }
            return NULL;
        }

        old_status = init_data->targets[current_req->target_id].
            conns[current_req->cid].status;

        switch (old_status)
        {
            case ISCSI_CONNECTION_DOWN:
            case ISCSI_CONNECTION_REMOVED:
                if (current_req->status != ISCSI_CONNECTION_UP)
                {
                    RING("Connection %d,%d is already down, nothing to do",
                         current_req->target_id, current_req->cid);
                    pthread_mutex_lock(&init_data->initiator_mutex);
                    init_data->targets[current_req->target_id].
                        conns[current_req->cid].status = current_req->status;
                    pthread_mutex_unlock(&init_data->initiator_mutex);
                    free(current_req);
                    continue;
                }
                break;
            case ISCSI_CONNECTION_UP:
            {
                if (current_req->status == ISCSI_CONNECTION_UP)
                {
                    iscsi_connection_req *down_req = NULL;
                    WARN("Connection %d:%d is already up, "
                         "trying to bring it down first",
                         current_req->target_id, current_req->cid);
                    pthread_mutex_lock(&init_data->initiator_mutex);
                    current_req->next = init_data->request_queue_head;
                    if (init_data->request_queue_tail == NULL)
                        init_data->request_queue_tail = current_req;
                    down_req  = malloc(sizeof(*down_req));
                    *down_req = *current_req;
                    down_req->status = ISCSI_CONNECTION_DOWN;
                    down_req->next = current_req;
                    init_data->request_queue_head = down_req;
                    sem_post(&init_data->request_sem); /* for down request */
                    sem_post(&init_data->request_sem); /* for up request */
                    pthread_mutex_unlock(&init_data->initiator_mutex);
                    continue;
                }
                break;
            }
            default:
            {
                if (current_req->status == ISCSI_CONNECTION_UP)
                {
                    ERROR("Connection %d:%d is in inconsistent state, "
                          "refusing to bring it up",
                          current_req->target_id, current_req->cid);
                    free(current_req);
                    continue;
                }
            }
        }
            
        switch (init_data->init_type)
        {
            case UNH:
                iscsi_initiator_unh_set(current_req);
                break;
            case L5:
                iscsi_initiator_l5_set(current_req);
                break;
            case OPENISCSI:
                iscsi_initiator_openiscsi_set(current_req);
                break;
            default:
                ERROR("Corrupted init_data->init_type: %d",
                      init_data->init_type);
        }
        if (init_data->targets[current_req->target_id].
            conns[current_req->cid].status == 
            ISCSI_CONNECTION_WAITING_DEVICE)
        {
            if (init_data->targets[current_req->target_id].
                number_of_open_connections > 1)
            {
                    pthread_mutex_lock(&init_data->initiator_mutex);
                    init_data->targets[current_req->target_id].
                        conns[current_req->cid].status = ISCSI_CONNECTION_UP;
                    pthread_mutex_unlock(&init_data->initiator_mutex);
            }
            else
            {
                int i, rc;
                
                for (i = 30; i != 0; i--)
                {
                    rc = iscsi_prepare_device(&init_data->targets[current_req->target_id].
                                              conns[current_req->cid],
                                              current_req->target_id);
                    if (rc == 0 || TE_RC_GET_ERROR(rc) != TE_EAGAIN)
                        break;
                    te_usleep(500000);
                }
                if (rc != 0)
                {
                    ERROR("Cannot prepare SCSI device for connection %d,%d: %r",
                          current_req->target_id, current_req->cid, rc);
                }
                else
                {
                    pthread_mutex_lock(&init_data->initiator_mutex);
                    init_data->targets[current_req->target_id].
                        conns[current_req->cid].status = ISCSI_CONNECTION_UP;
                pthread_mutex_unlock(&init_data->initiator_mutex);
                }
            }
        }
        free(current_req);    
    }
}

static te_errno
iscsi_initiator_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    UNUSED(value);
    return 0;
}

/* AuthMethod */
static te_errno
iscsi_initiator_chap_set(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].chap.chap, 
           value);

    return 0;
}

static te_errno
iscsi_initiator_chap_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.chap);

    return 0;
}

/* Peer Name */
static te_errno
iscsi_initiator_peer_name_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].chap.peer_name, 
           value);

    return 0;
}

static te_errno
iscsi_initiator_peer_name_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.peer_name);

    return 0;
}

/* Challenge Length */
static te_errno
iscsi_initiator_challenge_length_set(unsigned int gid, const char *oid,
                                     char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].chap.challenge_length = 
        atoi(value);

    return 0;
}

static te_errno
iscsi_initiator_challenge_length_get(unsigned int gid, const char *oid,
                                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.challenge_length);

    return 0;
}

/* Encoding Format */
static te_errno
iscsi_initiator_enc_fmt_set(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].chap.enc_fmt = atoi(value);

    return 0;
}

static te_errno
iscsi_initiator_enc_fmt_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.enc_fmt);

    return 0;
}

/* Connection structure */
static te_errno
iscsi_conn_add(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
    int cid = iscsi_get_cid(oid);
    int tgt_id = iscsi_get_target_id(oid);

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(value);

    IVERB("Adding connection with id=%d to target with id %d", cid, tgt_id);
    iscsi_init_default_connection_parameters(&init_data->targets[tgt_id].
                                             conns[cid]);

    init_data->targets[tgt_id].conns[cid].status = ISCSI_CONNECTION_DOWN;

    return 0;
}

static te_errno
iscsi_conn_del(unsigned int gid, const char *oid,
               const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    return iscsi_post_connection_request(iscsi_get_target_id(oid),
                                         iscsi_get_cid(oid), 
                                         ISCSI_CONNECTION_REMOVED);
}

static te_errno
iscsi_conn_list(unsigned int gid, const char *oid,
                char **list, const char *instance, ...)
{
    int   cid;
    int   tgt_id = iscsi_get_target_id(oid);
    char  conns_list[MAX_CONNECTIONS_NUMBER * 15];
    char  conn[15];
    
    UNUSED(gid);
    UNUSED(instance);
    
    conns_list[0] = '\0';
    
    for (cid = 0; cid < MAX_CONNECTIONS_NUMBER; cid++)
    {
        if (init_data->targets[tgt_id].conns[cid].status != 
            ISCSI_CONNECTION_REMOVED)
        {
            sprintf(conn,
                    "%d ", cid);
            strcat(conns_list, conn);
        }
    }

    *list = strdup(conns_list);
    if (*list == NULL)
    {
        ERROR("Failed to allocate memory for the list of connections.");
        return TE_RC(iscsi_agent_type, TE_ENOMEM);
    }

    return 0;
}

/* Target Data */
static te_errno
iscsi_target_data_add(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int tgt_id = iscsi_get_target_id(oid);
    
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(value);

    if (init_data->n_targets == 0)
    {
        int rc;
        rc = pthread_create(&init_data->request_thread, NULL, 
                            iscsi_initator_conn_request_thread,
                            NULL);
        if (rc != 0)
        {
            ERROR("Cannot create a connection request processing thread");
            return TE_OS_RC(iscsi_agent_type, rc);
        }
    }
    
    init_data->n_targets++;
    iscsi_init_default_tgt_parameters(&init_data->targets[tgt_id]);
    init_data->targets[tgt_id].target_id = tgt_id;

    IVERB("Adding %s with value %s, id=%d\n", oid, value, tgt_id);

    return 0;
}

static te_errno
iscsi_target_data_del(unsigned int gid, const char *oid,
                      const char *instance, ...)
{
    int tgt_id = atoi(instance + strlen("target_"));
    
    UNUSED(gid);
    UNUSED(oid);
    
    IVERB("Deletting %s\n", oid);
    init_data->targets[tgt_id].target_id = -1;
    if (--init_data->n_targets == 0)
    {
        /* to stop the thread and possibly a service daemon */
        iscsi_post_connection_request(ISCSI_ALL_CONNECTIONS, ISCSI_ALL_CONNECTIONS, 
                                      ISCSI_CONNECTION_REMOVED);
        pthread_join(init_data->request_thread, NULL);
    }
    
    return 0;
}

static te_errno
iscsi_target_data_list(unsigned int gid, const char *oid,
                       char **list, const char *instance, ...)
{
    int   id;
    char  targets_list[MAX_TARGETS_NUMBER * 15];
    char  tgt[15];
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    targets_list[0] = '\0';
    
    for (id = 0; id < MAX_TARGETS_NUMBER; id++)
    {
        if (init_data->targets[id].target_id != -1)
        {
            tgt[0] = '\0';
            sprintf(tgt,
                    "target_%d ", init_data->targets[id].target_id);
            strcat(targets_list, tgt);
        }
    }

    *list = strdup(targets_list);
    if (*list == NULL)
    {
        ERROR("Failed to allocate memory for the list of targets");
        return TE_RC(iscsi_agent_type, TE_ENOMEM);
    }
    return 0;
}

/* Target Authentication */
static te_errno
iscsi_initiator_target_auth_set(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    te_bool t = FALSE;
    
    UNUSED(gid);
    UNUSED(instance);
    
    if (*value == '1')
        t = TRUE;
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].chap.target_auth = t;

    return 0;
}

static te_errno
iscsi_initiator_target_auth_get(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.target_auth);

    return 0;
}

/* Peer Secret */
static te_errno
iscsi_initiator_peer_secret_set(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].chap.peer_secret, value);

    return 0;
}

static te_errno
iscsi_initiator_peer_secret_get(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.peer_secret);

    return 0;
}

/* Local Secret */
static te_errno
iscsi_initiator_local_secret_set(unsigned int gid, const char *oid,
                                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].chap.local_secret, 
           value);

    return 0;
}

static te_errno
iscsi_initiator_local_secret_get(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.local_secret);

    return 0;
}

/* MaxConnections */
static te_errno
iscsi_max_connections_set(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].max_connections = atoi(value);

    return 0;
}

static te_errno
iscsi_max_connections_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    sprintf(value, "%d", init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].max_connections);

    return 0;
}

/* InitialR2T */
static te_errno
iscsi_initial_r2t_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].initial_r2t, value);

    return 0;
}

static te_errno
iscsi_initial_r2t_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].initial_r2t);

    return 0;
}

/* HeaderDigest */
static te_errno
iscsi_header_digest_set(unsigned int gid, const char *oid,
                        char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].header_digest, value);

    return 0;
}

static te_errno
iscsi_header_digest_get(unsigned int gid, const char *oid,
                        char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].header_digest);

    return 0;
}

/* DataDigest */
static te_errno
iscsi_data_digest_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].data_digest, value);

    return 0;
}

static te_errno
iscsi_data_digest_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].data_digest);

    return 0;
}

/* ImmediateData */
static te_errno
iscsi_immediate_data_set(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].immediate_data, value);

    return 0;
}

static te_errno
iscsi_immediate_data_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].immediate_data);

    return 0;
}

/* MaxRecvDataSegmentLength */
static te_errno
iscsi_max_recv_data_segment_length_set(unsigned int gid, const char *oid,
                                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].max_recv_data_segment_length = atoi(value);

    return 0;
}

static te_errno
iscsi_max_recv_data_segment_length_get(unsigned int gid, const char *oid,
                                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].max_recv_data_segment_length);

    return 0;
}

/* FirstBurstLength */
static te_errno
iscsi_first_burst_length_set(unsigned int gid, const char *oid,
                             char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].first_burst_length = atoi(value);

    return 0;
}

static te_errno
iscsi_first_burst_length_get(unsigned int gid, const char *oid,
                             char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
   
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].first_burst_length);

    return 0;
}

/* MaxBurstLength */
static te_errno
iscsi_max_burst_length_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].max_burst_length = atoi(value);

    return 0;
}

static te_errno
iscsi_max_burst_length_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].max_burst_length);

    return 0;
}

/* DefaultTime2Wait */
static te_errno
iscsi_default_time2wait_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].default_time2wait = atoi(value);

    return 0;
}

static te_errno
iscsi_default_time2wait_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].default_time2wait);

    return 0;
}

/* DefaultTime2Retain */
static te_errno
iscsi_default_time2retain_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].default_time2retain = atoi(value);

    return 0;
}

static te_errno
iscsi_default_time2retain_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].default_time2retain);

    return 0;
}

/* MaxOutstandingR2T */
static te_errno
iscsi_max_outstanding_r2t_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].max_outstanding_r2t = atoi(value);

    return 0;
}

static te_errno
iscsi_max_outstanding_r2t_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].max_outstanding_r2t);

    return 0;
}

/* DataPDUInOrder */
static te_errno
iscsi_data_pdu_in_order_set(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].data_pdu_in_order, value);

    return 0;
}

static te_errno
iscsi_data_pdu_in_order_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].data_pdu_in_order);

    return 0;
}

/* DataSequenceInOrder */
static te_errno
iscsi_data_sequence_in_order_set(unsigned int gid, const char *oid,
                                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].data_sequence_in_order, 
           value);

    return 0;
}

static te_errno
iscsi_data_sequence_in_order_get(unsigned int gid, const char *oid,
                                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].data_sequence_in_order);

    return 0;
}

/* ErrorRecoveryLevel */
static te_errno
iscsi_error_recovery_level_set(unsigned int gid, const char *oid,
                                     char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].error_recovery_level = atoi(value);

    return 0;
}

static te_errno
iscsi_error_recovery_level_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].error_recovery_level);

    return 0;
}

/* SessionType */
static te_errno
iscsi_session_type_set(unsigned int gid, const char *oid,
                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].session_type, value);

    return 0;
}

static te_errno
iscsi_session_type_get(unsigned int gid, const char *oid,
                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].session_type);

    return 0;
}

/* TargetName */
static te_errno
iscsi_target_name_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].target_name, value);

    return 0;
}

static te_errno
iscsi_target_name_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].target_name);

    return 0;
}

/* InitiatorName */
static te_errno
iscsi_initiator_name_set(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].initiator_name, 
           value);

    return 0;
}

static te_errno
iscsi_initiator_name_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].initiator_name);

    return 0;
}

/* InitiatorAlias */
static te_errno
iscsi_initiator_alias_set(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].initiator_alias, 
           value);

    return 0;
}

static te_errno
iscsi_initiator_alias_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].initiator_alias);

    return 0;
}

/* TargetAddress */
static te_errno
iscsi_target_addr_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *(init_data->targets[iscsi_get_target_id(oid)].target_addr) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].target_addr, value);
    
    return 0;
}

static te_errno
iscsi_target_addr_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);

    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].target_addr);

    return 0;
}

/* TargetPort */
static te_errno
iscsi_target_port_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].target_port = atoi(value);

    return 0;
}

static te_errno
iscsi_target_port_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].target_port = 3260;
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].target_port);

    return 0;
}

static te_errno
iscsi_host_device_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int status;
    int tgt_id = iscsi_get_target_id(oid);
    int cid    = iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);

    pthread_mutex_lock(&init_data->initiator_mutex);
    status = init_data->targets[tgt_id].conns[cid].status;
    if (status != ISCSI_CONNECTION_UP)
    {
        *value = '\0';
    }
    else
    {
        strcpy(value, init_data->targets[tgt_id].conns[cid].device_name);
    }
    pthread_mutex_unlock(&init_data->initiator_mutex);
    return 0;
}

/* Initiator's path to scripts (for L5) */
static te_errno
iscsi_script_path_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(init_data->script_path, value);
    return 0;
}

static te_errno
iscsi_script_path_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, init_data->script_path);

    return 0;
}

/* Initiator type */
static te_errno
iscsi_type_set(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
#if 0
    int previous_type = init_data->init_type;
#endif

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (strcmp(value, "unh") == 0)
        init_data->init_type = UNH;
    else if (strcmp(value, "l5") == 0)
        init_data->init_type = L5;
    else if (strcmp(value, "microsoft") == 0)
        init_data->init_type = MICROSOFT;
    else if (strcmp(value, "open-iscsi") == 0)
        init_data->init_type = OPENISCSI;
    else
        return TE_RC(iscsi_agent_type, TE_EINVAL);

#if 0
    if (previous_type != init_data->init_type &&
        previous_type == OPENISCSI)
    {
        iscsi_openiscsi_stop_daemon();
    }
#endif

    return 0;
}

static te_errno
iscsi_type_get(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
    static char *types[] = {"unh", "l5", 
                            "open-iscsi", "microsoft"};
            
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, types[init_data->init_type]);

    return 0;
}


/* Host Bus Adapter */
static te_errno
iscsi_host_bus_adapter_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    init_data->host_bus_adapter = atoi(value);

    return 0;
}


static te_errno
iscsi_host_bus_adapter_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", init_data->host_bus_adapter);

    return 0;
}

/* Verbosity */
static te_errno
iscsi_initiator_verbose_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    init_data->verbosity = atoi(value);

    return 0;
}


static te_errno
iscsi_initiator_verbose_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", init_data->verbosity);

    return 0;
}

/* Local Name */
static te_errno
iscsi_initiator_local_name_set(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    strcpy(init_data->targets[iscsi_get_target_id(oid)].
           conns[iscsi_get_cid(oid)].chap.local_name, value);

    return 0;
}

static te_errno
iscsi_initiator_local_name_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);

    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].
            conns[iscsi_get_cid(oid)].chap.local_name);

    return 0;
}

static te_errno
iscsi_parameters2advertize_set(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    INFO("SETTING %s to %s", oid, value);
    init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].conf_params |= atoi(value);

    return 0;
}

static te_errno
iscsi_parameters2advertize_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    int tgt_id;
    int cid;
    
    UNUSED(gid);
    UNUSED(instance);

    tgt_id = iscsi_get_target_id(oid);
    cid    = iscsi_get_cid(oid);
    RING("iscsi_parameters2advertize_get: %d, %d", tgt_id, cid);
    sprintf(value, "%d", 
            init_data->targets[tgt_id].conns[cid].conf_params);
    
    return 0;
}

static te_errno
iscsi_status_set(unsigned int gid, const char *oid,
              char *value, const char *instance, ...)
{
    int tgt_id = iscsi_get_target_id(oid);
    int cid    = iscsi_get_cid(oid);
    int oper   = atoi(value);

    if (*value == '\0')
        return 0;
        
    UNUSED(gid);
    UNUSED(instance);

    return iscsi_post_connection_request(tgt_id, cid, oper);
}

static te_errno
iscsi_status_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    int status;
    UNUSED(gid);
    UNUSED(instance);

    pthread_mutex_lock(&init_data->initiator_mutex);
    status = init_data->targets[iscsi_get_target_id(oid)].
        conns[iscsi_get_cid(oid)].status;
    pthread_mutex_unlock(&init_data->initiator_mutex);    
    
    sprintf(value, "%d", status);

    return 0;
}

/* Configuration tree */

RCF_PCH_CFG_NODE_RW(node_iscsi_verbose, "verbose", NULL, 
                    NULL, iscsi_initiator_verbose_get,
                    iscsi_initiator_verbose_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_script_path, "script_path", NULL, 
                    &node_iscsi_verbose, iscsi_script_path_get,
                    iscsi_script_path_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_type, "type", NULL, 
                    &node_iscsi_script_path, iscsi_type_get,
                    iscsi_type_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_host_bus_adapter, "host_bus_adapter", NULL, 
                    &node_iscsi_type, iscsi_host_bus_adapter_get,
                    iscsi_host_bus_adapter_set);

RCF_PCH_CFG_NODE_RO(node_iscsi_initiator_host_device, "host_device",
                    NULL, NULL,
                    iscsi_host_device_get);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_port, "target_port", NULL, 
                    &node_iscsi_initiator_host_device,
                    iscsi_target_port_get, iscsi_target_port_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_addr, "target_addr", NULL, 
                    &node_iscsi_target_port,
                    iscsi_target_addr_get, iscsi_target_addr_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_name, "target_name", NULL,
                    &node_iscsi_target_addr, iscsi_target_name_get,
                    iscsi_target_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_cid, "status", NULL, NULL,
                    iscsi_status_get, iscsi_status_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_alias, "initiator_alias", NULL, 
                    &node_iscsi_cid, iscsi_initiator_alias_get,
                    iscsi_initiator_alias_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_name, "initiator_name", NULL, 
                    &node_iscsi_initiator_alias, iscsi_initiator_name_get,
                    iscsi_initiator_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_parameters2advertize, "parameters2advertize",
                    NULL, &node_iscsi_initiator_name,
                    iscsi_parameters2advertize_get, 
                    iscsi_parameters2advertize_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_session_type, "session_type", NULL,
                    &node_iscsi_parameters2advertize, iscsi_session_type_get,
                    iscsi_session_type_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_error_recovery_level, "error_recovery_level", 
                    NULL, &node_iscsi_session_type, 
                    iscsi_error_recovery_level_get,
                    iscsi_error_recovery_level_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_data_sequence_in_order, 
                    "data_sequence_in_order",
                    NULL, &node_iscsi_error_recovery_level,
                    iscsi_data_sequence_in_order_get,
                    iscsi_data_sequence_in_order_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_data_pdu_in_order, "data_pdu_in_order",
                    NULL, &node_iscsi_data_sequence_in_order,
                    iscsi_data_pdu_in_order_get,
                    iscsi_data_pdu_in_order_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_outstanding_r2t, "max_outstanding_r2t",
                    NULL, &node_iscsi_data_pdu_in_order,
                    iscsi_max_outstanding_r2t_get,
                    iscsi_max_outstanding_r2t_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_default_time2retain, "default_time2retain",
                    NULL, &node_iscsi_max_outstanding_r2t,
                    iscsi_default_time2retain_get,
                    iscsi_default_time2retain_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_default_time2wait, "default_time2wait",
                    NULL, &node_iscsi_default_time2retain,
                    iscsi_default_time2wait_get,
                    iscsi_default_time2wait_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_burst_length, "max_burst_length",
                    NULL, &node_iscsi_default_time2wait,
                    iscsi_max_burst_length_get,
                    iscsi_max_burst_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_first_burst_length, "first_burst_length",
                    NULL, &node_iscsi_max_burst_length,
                    iscsi_first_burst_length_get,
                    iscsi_first_burst_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_recv_data_segment_length,
                    "max_recv_data_segment_length",
                    NULL, &node_iscsi_first_burst_length,
                    iscsi_max_recv_data_segment_length_get,
                    iscsi_max_recv_data_segment_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_immediate_data, "immediate_data",
                    NULL, &node_iscsi_max_recv_data_segment_length,
                    iscsi_immediate_data_get,
                    iscsi_immediate_data_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_data_digest, "data_digest",
                    NULL, &node_iscsi_immediate_data,
                    iscsi_data_digest_get,
                    iscsi_data_digest_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_header_digest, "header_digest",
                    NULL, &node_iscsi_data_digest,
                    iscsi_header_digest_get,
                    iscsi_header_digest_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initial_r2t, "initial_r2t",
                    NULL, &node_iscsi_header_digest,
                    iscsi_initial_r2t_get,
                    iscsi_initial_r2t_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_connections, "max_connections",
                    NULL, &node_iscsi_initial_r2t,
                    iscsi_max_connections_get,
                    iscsi_max_connections_set);

/* CHAP related stuff */
RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_local_name, "local_name",
                   NULL, NULL,
                   iscsi_initiator_local_name_get,
                   iscsi_initiator_local_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_peer_secret, "peer_secret",
                    NULL, &node_iscsi_initiator_local_name,
                    iscsi_initiator_peer_secret_get,
                    iscsi_initiator_peer_secret_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_auth, "target_auth",
                    NULL,
                    &node_iscsi_initiator_peer_secret, 
                    iscsi_initiator_target_auth_get,
                    iscsi_initiator_target_auth_set);                   

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_enc_fmt, "enc_fmt",
                    NULL, &node_iscsi_target_auth,
                    iscsi_initiator_enc_fmt_get,
                    iscsi_initiator_enc_fmt_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_challenge_length, "challenge_length",
                    NULL, &node_iscsi_initiator_enc_fmt,
                    iscsi_initiator_challenge_length_get,
                    iscsi_initiator_challenge_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_peer_name, "peer_name",
                    NULL, &node_iscsi_initiator_challenge_length,
                    iscsi_initiator_peer_name_get,
                    iscsi_initiator_peer_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_local_secret, "local_secret",
                    NULL, &node_iscsi_initiator_peer_name,
                    iscsi_initiator_local_secret_get,
                    iscsi_initiator_local_secret_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_chap, "chap", 
                    &node_iscsi_initiator_local_secret,
                    &node_iscsi_max_connections, iscsi_initiator_chap_get,
                    iscsi_initiator_chap_set);

RCF_PCH_CFG_NODE_COLLECTION(node_iscsi_conn, "conn", 
                            &node_iscsi_chap, &node_iscsi_target_name,
                            iscsi_conn_add, iscsi_conn_del, iscsi_conn_list,
                            NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_iscsi_target_data, "target_data",
                            &node_iscsi_conn, &node_iscsi_host_bus_adapter, 
                            iscsi_target_data_add, iscsi_target_data_del,
                            iscsi_target_data_list, NULL);

/* Main object */
RCF_PCH_CFG_NODE_RW(node_ds_iscsi_initiator, "iscsi_initiator", 
                    &node_iscsi_target_data,
                    NULL, iscsi_initiator_get,
                    iscsi_initiator_set);


/**
 * Function to register the /agent/iscsi_initiator
 * object in the agent's tree.
 */
te_errno
iscsi_initiator_conf_init(int agent_type)
{
    /* On Init there is only one target configured on the Initiator */
    iscsi_init_default_ini_parameters(ISCSI_CONF_NO_TARGETS);

    iscsi_agent_type = agent_type;

    return rcf_pch_add_node("/agent", &node_ds_iscsi_initiator);
}
