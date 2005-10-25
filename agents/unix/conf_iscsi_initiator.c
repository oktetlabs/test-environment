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
#include <sys/types.h>
#include <stddef.h>
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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_config.h"
#include "config.h"
#include "conf_daemons.h"
#include "te_shell_cmd.h"
#include "ndn_iscsi.h"
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

/** Length of Open iSCSI record ID */
#define RECORD_ID_LENGTH 32

/** Maximum number of targets the Initiator can connect to */
#define MAX_TARGETS_NUMBER 3

/** Default targets port */
#define ISCSI_TARGET_DEFAULT_PORT 3260

/** 
 * Default lun number.
 * When LUN is not provided to UNH Initiator it starts
 * questioning the Target about it's LUNs.
 */
#define ISCSI_DEFAULT_LUN_NUMBER 1


/** 
 * Host bus adapter default value. If
 * the Initiator is the only scsi device on the
 * system and it is loaded the first time since
 * the last reboot than it is zero.
 */
#define DEFAULT_HOST_BUS_ADAPTER            0


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
    MICROSOFT,
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
    te_bool     target_auth; /**< If TRUE than Target authentication is
                                  required during the Security Phase */
} iscsi_tgt_chap_data_t;

/**
 * Per target data of the Initiator.
 * Most of the fields correspond to operational parameters
 * with the same name. See RFC3260 for allowed values.
 */
typedef struct iscsi_target_data {
    int               target_id; /**< Id of the Target */
    te_bool           is_active;

    int               number_of_open_connections;
    
    int               conf_params; /**< OR of OFFER_XXX flags */

    char              initiator_name[MAX_NAME_LENGTH]; /**< InitiatorName */ 
    char              initiator_alias[MAX_NAME_LENGTH]; /**< InitiatorAlias */

    char              target_name[MAX_NAME_LENGTH];  /**< TargetName */
    char              target_addr[MAX_ADDR_LENGTH];  /**< TargetAddr */
    int               target_port;                   /**< TargetPort */

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
    char              record_id[RECORD_ID_LENGTH]; 
                     /**< Open iSCSI db record id */

    iscsi_tgt_chap_data_t chap; /**< Serurity related data */

} iscsi_target_data_t;

/**
 * Initiator data structure.
 * Contains general information about the Initiator and
 * per target data.
 */
typedef struct iscsi_initiator_data {
    iscsi_initiator_type  init_type; /**< Type of the Initiator */

    char                  last_cmd[MAX_CMD_SIZE]; /**< Last cmd received by
                                                       the Initiator */

    int                   host_bus_adapter;  /**< Number of the host bus 
                                                  adapter. Usually 0 */
    char                  script_path[MAX_CMD_SIZE];
    
    iscsi_target_data_t   targets[MAX_TARGETS_NUMBER]; /**< Per target data */
} iscsi_initiator_data_t;


/** Initiator data */
static iscsi_initiator_data_t *init_data;

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
    char *c;
    int   tgt_id;
    
    c = strstr(oid, "target_");
    c = strstr(c + 1, "target_");
    c += strlen("target_");

    tgt_id = atoi(c);

    return tgt_id;
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
    memset(tgt_data, 0, sizeof(tgt_data));
    
    strcpy(tgt_data->initiator_name, DEFAULT_INITIATOR_NAME);
    strcpy(tgt_data->initiator_alias, DEFAULT_INITIATOR_ALIAS);
    
    strcpy(tgt_data->target_name, DEFAULT_TARGET_NAME);
    
    tgt_data->max_connections = DEFAULT_MAX_CONNECTIONS;
    strcpy(tgt_data->initial_r2t, DEFAULT_INITIAL_R2T);
    strcpy(tgt_data->header_digest, DEFAULT_HEADER_DIGEST);
    strcpy(tgt_data->data_digest, DEFAULT_DATA_DIGEST);
    strcpy(tgt_data->immediate_data, DEFAULT_IMMEDIATE_DATA);
    tgt_data->max_recv_data_segment_length = 
        DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH;
    tgt_data->first_burst_length = DEFAULT_FIRST_BURST_LENGTH;
    tgt_data->max_burst_length = DEFAULT_MAX_BURST_LENGTH;
    tgt_data->default_time2wait = DEFAULT_DEFAULT_TIME2WAIT;
    tgt_data->default_time2retain = DEFAULT_DEFAULT_TIME2RETAIN;
    tgt_data->max_outstanding_r2t = DEFAULT_MAX_OUTSTANDING_R2T;
    strcpy(tgt_data->data_pdu_in_order, DEFAULT_DATA_PDU_IN_ORDER);
    strcpy(tgt_data->data_sequence_in_order, 
           DEFAULT_DATA_SEQUENCE_IN_ORDER);
    tgt_data->error_recovery_level = DEFAULT_ERROR_RECOVERY_LEVEL;
    strcpy(tgt_data->session_type, DEFAULT_SESSION_TYPE);

    /* target's chap */
    tgt_data->chap.target_auth = FALSE;
    *(tgt_data->chap.peer_secret) = '\0';
    *(tgt_data->chap.local_name) = '\0';
    strcpy(tgt_data->chap.chap, "None");
    tgt_data->chap.enc_fmt = BASE_16;
    tgt_data->chap.challenge_length = DEFAULT_CHALLENGE_LENGTH;
    *(tgt_data->chap.peer_name) = '\0';
    *(tgt_data->chap.local_secret) = '\0';
}

/** Configure all targets (id: 0..MAX_TARGETS_NUMBER */
#define ISCSI_CONF_ALL_TARGETS -1

/** Configure only general Initiator data */
#define ISCSI_CONF_NO_TARGETS -2

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
static int
iscsi_initiator_get(const char *gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    *value = 0;
    sprintf(value, "%s", init_data->last_cmd);
    return 0;
}

/**
 * Executes te_shell_cmd() function.
 *
 * @param cmd    format of the command followed by
 *               parameters
 *
 * @return errno
 */
static int
te_shell_cmd_ex(const char *cmd, ...)
{
    char    cmdline[MAX_CMD_SIZE];
    va_list ap;

    va_start(ap, cmd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);

    IVERB("iSCSI Initiator: %s\n", cmdline);
    return te_shell_cmd(cmdline, -1, NULL, NULL) > 0 ? 0 : -1;
}

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
        return -1;
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

    return WIFEXITED(status) ? 0 : -1;
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

typedef struct iscsi_target_param_descr_t
{
    int      offer;
    char    *name;
    te_bool  is_string;
    int      offset;    
} iscsi_target_param_descr_t;

#define ISCSI_END_PARAM_TABLE {0, NULL, FALSE, 0}

/****** L5 Initiator specific stuff  ****/

static void
iscsi_l5_write_param(FILE *destination, 
                     iscsi_target_param_descr_t *param,
                     void *data)
{
    const char *n;
    for (n = param->name; *n != '\0'; n++)
    {
        if (*n != '_')
            fputc(*n, destination);
    }
    fputs(": ", destination);
    if (param->is_string)
    {
        RING("Setting %s = %s", param->name, 
             (char *)data + param->offset);
        fputs((char *)data + param->offset, destination);
    }
    else
    {
        RING("Setting %s = %d", param->name,
             *(int *)((char *)data + param->offset));
        fprintf(destination, "%d", 
                *(int *)((char *)data + param->offset));
    }
    fputc('\n', destination);
}

static int
iscsi_l5_write_target_params(FILE *destination, 
                             iscsi_target_data_t *target)
{
    iscsi_target_param_descr_t *p;
#define PARAMETER(field, offer, type) \
    {OFFER_##offer, #field, type, offsetof(iscsi_target_data_t, field)}
    static iscsi_target_param_descr_t params[] =
        {
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(initial_r2t, INITIAL_R2T,  TRUE),
            PARAMETER(header_digest, HEADER_DIGEST,  TRUE),
            PARAMETER(data_digest, DATA_DIGEST,  TRUE),
            PARAMETER(immediate_data, IMMEDIATE_DATA,  TRUE),
            PARAMETER(max_recv_data_segment_length, MAX_RECV_DATA_SEGMENT_LENGTH,  
                      FALSE),
            PARAMETER(first_burst_length, FIRST_BURST_LENGTH,  FALSE),
            PARAMETER(max_burst_length, MAX_BURST_LENGTH,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            PARAMETER(max_outstanding_r2t, MAX_OUTSTANDING_R2T,  FALSE),
            PARAMETER(data_pdu_in_order, DATA_PDU_IN_ORDER,  TRUE),
            PARAMETER(data_sequence_in_order, DATA_SEQUENCE_IN_ORDER,  TRUE),
            PARAMETER(error_recovery_level, ERROR_RECOVERY_LEVEL,  FALSE),
            ISCSI_END_PARAM_TABLE
        };
#define AUTH_PARAM(field, name, type) \
    static iscsi_target_param_descr_t authp_##field = \
        {0, name, type, offsetof(iscsi_tgt_chap_data_t, field)}
#define WRITE_AUTH(field) \
    iscsi_l5_write_param(destination, &authp_##field, &target->chap)

    AUTH_PARAM(chap, "AuthMethod", TRUE);
    
    for (p = params; p->name != NULL; p++)
    {
        if ((target->conf_params & p->offer) == p->offer)
        {
            iscsi_l5_write_param(destination, p, target);
        }
    }
    WRITE_AUTH(chap);
    /** Other authentication parameters are not supported by
     *  L5 initiator, both on the level of script and on 
     *  the level of ioctls
     */
    fprintf(destination, "\n\n[target%d_conn0]\n"
                         "Host: %s\n"
                         "Port: %d\n\n",
            target->target_id,
            target->target_addr, target->target_port);
    return 0;
#undef PARAMETER
#undef AUTH_PARAM
#undef WRITE_AUTH
}

static int
iscsi_l5_write_config(iscsi_initiator_data_t *iscsi_data)
{
    static char filename[MAX_CMD_SIZE];
    FILE *destination;
    iscsi_target_data_t *target;
    
    snprintf(filename, sizeof(filename),
             "%s/configs/te",
             *iscsi_data->script_path != '\0' ? 
             iscsi_data->script_path : ".");
    destination = fopen(filename, "w");
    if (destination == NULL)
    {
        ERROR("Cannot open '%s' for writing: %s",
              filename, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
             
    target = iscsi_data->targets;
    if (target->target_id < 0)
    {
        ERROR("No targets configured");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    /** NOTE: L5 Initator seems to be unable 
     *  to have different Initator names for
     *  different Targets/Connections.
     *  So we just using the first one
     */
    fprintf(destination, "[INITIATOR]\n"
                         "Name: %s\n"
                         "Targets:",
            target->initiator_name);
    for (; target < iscsi_data->targets + MAX_TARGETS_NUMBER; target++)
    {
        if (target->target_id >= 0 && target->is_active)
        {
            fprintf(destination, " target%d", target->target_id);
        }
    }

    target = iscsi_data->targets;
    /** NOTE: Currently L5 initiator supports only a single
     *  _local_ secret per initiator 
     */
    if (*target->chap.local_secret != '\0')
    {
        fprintf(destination, "\nCHAPSecret: %s\n", target->chap.local_secret);
    }
    for (; target < iscsi_data->targets + MAX_TARGETS_NUMBER; target++)
    {
        if (target->target_id >= 0 && target->is_active)
        {
            fprintf(destination, "\n\n[target%d]\n"
                                 "TargetName: %s\n"
                                 "Connections: target%d_conn0\n", 
                    target->target_id,
                    target->target_name,
                    target->target_id);
            iscsi_l5_write_target_params(destination, target);
        }
    }
    fclose(destination);
    return 0;
}

static int
iscsi_openiscsi_set_param(const char *recid,
                          iscsi_target_param_descr_t *param,
                          void *data)
{
    int rc = 0;

    if (param->is_string)
    {
        rc = te_shell_cmd_ex("iscsiadm -m node --record=%s "
                             "--name=%s --value=%s", recid,
                             param->name,
                             (char *)data + param->offset);
    }
    else
    {
        rc = te_shell_cmd_ex("iscsiadm -m node --record=%s "
                        "--name=%s --value=%d", recid,
                        param->name,
                        *(int *)((char *)data + param->offset));
    }
    return rc;
}

static int
iscsi_openiscsi_start_daemon(iscsi_target_data_t *target)
{
    int   rc;
    FILE *name_file;
    
    name_file = fopen("/tmp/initiatorname.iscsi", "w");
    if (name_file == NULL)
    {
        rc = errno;
        ERROR("Cannot open /tmp/initiatorname.iscsi: %s",
              strerror(rc));
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    fprintf(name_file, "InitiatorName=%s\n", target->initiator_name);
    if (*target->initiator_alias != '\0')
        fprintf(name_file, "InitiatorAlias=%s\n", target->initiator_alias);
    fclose(name_file);

    return te_shell_cmd("iscsid -c /dev/null -i /tmp/initiatorname.iscsi "
                        "-p /tmp/openiscsi.pid", 
                        -1, NULL, NULL);
}

static int
iscsi_openiscsi_stop_daemon(void)
{
    FILE *name_file;
    int   pid = 0;
    int   unused;
    
    name_file = fopen("/tmp/openiscsi.pid", "r");
    if (name_file == NULL)
    {
        WARN("Cannot open PID file, assuming no iscsid running");
        return 0;
    }
    fscanf(name_file, "%d", &pid);
    if (pid == 0)
    {
        ERROR("Malformed PID file");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }
    if (kill(pid, SIGTERM))
    {
        WARN("Unable to kill iscsid: %s",
             strerror(errno));
    }
    else
    if (waitpid(pid, &unused, 0) == -1)
    {
        WARN("Unable to wait for iscsid: %s",
             strerror(errno));
    }
    return 0;
}

static int
iscsi_openiscsi_set_target_params(iscsi_target_data_t *target)
{
    iscsi_target_param_descr_t *p;
#define PARAMETER(field, name, offer, type) \
    {OFFER_##offer, name, type, offsetof(iscsi_target_data_t, field)}
    static iscsi_target_param_descr_t params[] =
        {
            PARAMETER(target_name, "node.name", 0, TRUE),
            PARAMETER(target_addr, "node.conn[0].address", 0, TRUE),
            PARAMETER(target_port, "node.conn[0].port", 0, FALSE),
            PARAMETER(max_connections, 
                      "node.session.iscsi.MaxConnections", MAX_CONNECTIONS, FALSE),
            PARAMETER(initial_r2t, 
                      "node.session.iscsi.InitialR2T", INITIAL_R2T,  TRUE),
            PARAMETER(header_digest, 
                      "node.conn[0].iscsi.HeaderDigest", HEADER_DIGEST,  TRUE),
            PARAMETER(data_digest, 
                      "node.conn[0].icsi.DataDigest", DATA_DIGEST,  TRUE),
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
            ISCSI_END_PARAM_TABLE
        };
#define AUTH_PARAM(field, name) \
        {0, "node.session.auth." name, TRUE, offsetof(iscsi_tgt_chap_data_t, field)}
    static iscsi_target_param_descr_t auth_params[] =
        {
            AUTH_PARAM(chap, "authmethod"),
            AUTH_PARAM(local_name, "username"),
            AUTH_PARAM(local_secret, "password"),
            AUTH_PARAM(peer_name, "username_in"),
            AUTH_PARAM(peer_secret, "password_in"),
            ISCSI_END_PARAM_TABLE
        };

    int   rc;
    
    for (p = params; p->name != NULL; p++)
    {
        if (p->offer == 0 || 
            ((target->conf_params & p->offer) == p->offer))
        {
            if ((rc = iscsi_openiscsi_set_param(target->record_id, 
                                                p, target)) != 0)
            {
                ERROR("Unable to set param %s: %r", p->name, rc);
                return rc;
            }
        }
    }
    for (p = auth_params; p->name != NULL; p++)
    {
        if ((rc = iscsi_openiscsi_set_param(target->record_id,
                                            p, &target->chap)) != 0)
        {
            ERROR("Unable to set param %s: %r", p->name, rc);
            return rc;
        }
    }

    return 0;
#undef PARAMETER
#undef AUTH_PARAM
#undef WRITE_AUTH
}

static const char *
iscsi_openiscsi_find_free_recid (iscsi_initiator_data_t *data)
{
    char  buffer[80];
    FILE *nodelist;
    int   i;

    static char recid[RECORD_ID_LENGTH];

    
    nodelist = popen("iscsiadm -m node", "r");
    if (nodelist == NULL)
    {
        ERROR("Unable to get the list of records: %s",
              strerror(errno));
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), nodelist) != NULL)
    {
        *recid = '\0';
        if (sscanf(buffer, "[%[^]]] %*s %*s", recid) != 1)
        {
            ERROR("Spurious record line '%s'", buffer);
            continue;
        }
        for (i = 0; i < MAX_TARGETS_NUMBER; i++)
        {
            if (strcmp(data->targets[i].record_id, recid) == 0)
            {
                *recid = '\0';
                break;
            }
        }
        if (*recid != '\0')
        {
            pclose(nodelist);
            return recid;
        }
    }
    pclose(nodelist);
    ERROR("No available record ids!");
    return NULL;
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
        IVERB("ISCSI_UNH_SET_UNNEGOTIATED(%s,0x%x,%d)\n",                   \
                param_, value_, target_id_);                                \
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
        IVERB("ISCSI_UNH_SET(%s,0x%x,%d)\n",                                \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_fmt,                            \
                         SHOULD_OFFER((offered_params_), (mask_)) ? "r":"p",\
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
        IVERB("ISCSI_UNH_SET_INT(%s,0x%x,%d)\n",                            \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_int_fmt,                        \
                         SHOULD_OFFER((offered_params_), (mask_)) ? "r":"p",\
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
        IVERB("ISCSI_UNH_FORCE(%s,0x%x,%d)\n",                              \
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
        IVERB("ISCSI_UNH_FORCE(%s,0x%x,%d)\n",                              \
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
        IVERB("ISCSI_UNH_FORCE_INT(%s,0x%x,%d)\n",                          \
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

/**
 * Parses the command received by the Initiator and retreives
 * connection ID and target ID from it.
 *
 * @param cmdline     Comman to retreive from
 * @param cid         (OUT) Connection ID
 * @param target      (OUT) Target ID
 * 
 * @return 0 or errno
 */
static int
iscsi_get_cid_and_target(const char *cmdline, int *cid, int *target)
{
    char *tmp = strdup(cmdline);
    char *c;

    if (tmp == NULL)
        return ENOMEM;
    
    IVERB("COMMAND: %s", cmdline);

    c = strrchr(tmp, ' ');
    if (c == NULL)
        return EINVAL;
    *c = '\0';

    if (*cmdline == 'd')
    {
        *cid = atoi(tmp + strlen("down") + 1);
        *target = atoi(c + 1);
    }
    else if (*cmdline == 'u')
    {
        *cid = atoi(tmp + strlen("up") + 1);
        *target = atoi(c + 1);
    }

    free(tmp);
    return 0;
}

/**
 * Handles set command for the Instance
 * /agent:Agt_X/iscsi_initiator:
 * 
 * Format of the value:
 * up 5 7 - establish the connection with id = 5 and target 7
 * down 5 3- destroy the connection with id = 5 and target 3
 */
static int
iscsi_initiator_unh_set(const char *value)
{
    int                     rc = -1;
    int                     cid = 0;
    int                     target_id = 0;
    iscsi_target_data_t    *target;

    int                     offer;

    rc = iscsi_get_cid_and_target(value, &cid, &target_id);
    if (rc == EINVAL)
    {
        sprintf(init_data->last_cmd, value);
        return 0;
    }

    target = &init_data->targets[target_id];
    
    offer = target->conf_params;

    IVERB("Current number of open connections: %d", 
          target->number_of_open_connections);
    
    if (strncmp(value, "down", strlen("down")) == 0)
    {
        init_data->targets[target_id].is_active = FALSE;
        /* We should down the connection */
        rc = ta_system_ex("iscsi_config down cid=%d target=%d host=%d",
                          cid, target_id, 
                          init_data->host_bus_adapter);
        if (rc != 0)
        {
            ERROR("Failed to close the connection "
                  "with CID = %d", cid);
            return TE_RC(TE_TA_UNIX, EINVAL);
        }
        target->number_of_open_connections--;
        INFO("Connections with ID %d is closed", cid);

        return 0;
    }
    /* We should open new connection */
    /* 1: configurating the Initiator */

    CHECK_SHELL_CONFIG_RC(
        ta_system_ex("iscsi_manage init restore target=%d host=%d",
                     target_id, init_data->host_bus_adapter),
        "Restoring");
    
    ISCSI_UNH_SET_UNNEGOTIATED("TargetName", target->target_name, target_id);
    
    if (target->number_of_open_connections == 0)
    {
        ISCSI_UNH_SET_INT("MaxConnections", target->max_connections,
                          target_id, OFFER_MAX_CONNECTIONS, offer);

        ISCSI_UNH_SET("InitialR2T", target->initial_r2t, target_id, 
                      OFFER_INITIAL_R2T, offer);

        ISCSI_UNH_SET("HeaderDigest", target->header_digest, target_id,
                      OFFER_HEADER_DIGEST, offer);

        ISCSI_UNH_SET("DataDigest", target->data_digest, target_id,
                      OFFER_DATA_DIGEST, offer);

        ISCSI_UNH_SET("ImmediateData", target->immediate_data, target_id,
                      OFFER_IMMEDIATE_DATA, offer);

        ISCSI_UNH_SET_INT("MaxRecvDataSegmentLength", 
                          target->max_recv_data_segment_length,
                          target_id, OFFER_MAX_RECV_DATA_SEGMENT_LENGTH,
                          offer);

        ISCSI_UNH_SET_INT("MaxBurstLength", 
                          target->max_burst_length, target_id,
                          OFFER_FIRST_BURST_LENGTH, offer);

        ISCSI_UNH_SET_INT("FirstBurstLength", 
                          target->first_burst_length, target_id,
                          OFFER_MAX_BURST_LENGTH, offer);

        ISCSI_UNH_SET_INT("DefaultTime2Wait", 
                          target->default_time2wait, target_id,
                          OFFER_DEFAULT_TIME2WAIT, offer);

        ISCSI_UNH_SET_INT("DefaultTime2Retain", 
                          target->default_time2retain, target_id,
                          OFFER_DEFAULT_TIME2RETAIN, offer);

        ISCSI_UNH_SET_INT("MaxOutstandingR2T", 
                          target->max_outstanding_r2t, target_id,
                          OFFER_MAX_OUTSTANDING_R2T, offer);

        ISCSI_UNH_SET("DataPDUInOrder", 
                      target->data_pdu_in_order, target_id,
                      OFFER_DATA_PDU_IN_ORDER, offer);

        ISCSI_UNH_SET("DataSequenceInOrder", 
                      target->data_sequence_in_order, target_id,
                      OFFER_DATA_SEQUENCE_IN_ORDER, offer);

        ISCSI_UNH_SET_INT("ErrorRecoveryLevel", 
                          target->error_recovery_level, target_id,
                          OFFER_ERROR_RECOVERY_LEVEL, offer);

        ISCSI_UNH_SET_UNNEGOTIATED("SessionType", target->session_type, target_id);

        /* Target' CHAP */
        if (init_data->targets[target_id].chap.target_auth)
        {
            ISCSI_UNH_FORCE_FLAG("t", target_id,
                                 "Target Authentication");
        }

        ISCSI_UNH_FORCE_STRING("px", target->chap.peer_secret, target_id,
                               "Peer Secret");

        ISCSI_UNH_FORCE("ln", target->chap.local_name, target_id,
                        "Local Name");

        ISCSI_UNH_SET_UNNEGOTIATED("AuthMethod", target->chap.chap, target_id);

        if (target->chap.enc_fmt == BASE_64)
            ISCSI_UNH_FORCE_FLAG("b", target_id,
                                 "Encoding Format");

        ISCSI_UNH_FORCE_INT("cl", target->chap.challenge_length, target_id,
                            "Challenge Length");

        ISCSI_UNH_FORCE("pn", target->chap.peer_name, target_id,
                        "Peer Name");

        ISCSI_UNH_FORCE_STRING("lx", target->chap.local_secret, target_id,
                               "Local Secret");

    }
    /* Initiator itself */
    ISCSI_UNH_SET_UNNEGOTIATED("InitiatorName", 
                  target->initiator_name,
                  target_id);

    ISCSI_UNH_SET_UNNEGOTIATED("InitiatorAlias", 
                  target->initiator_alias, 
                  target_id);

    /* Now the connection should be opened */
    rc = te_shell_cmd_ex("iscsi_config up ip=%s port=%d "
                         "cid=%d target=%d host=%d lun=%d" ,
                         target->target_addr,
                         target->target_port,
                         cid, target_id, init_data->host_bus_adapter,
                         ISCSI_DEFAULT_LUN_NUMBER);
    if (rc != 0)
    {
        ERROR("Failed to establish connection with cid=%d", cid);
        return rc;
    }
    init_data->targets[target_id].is_active = TRUE;
    target->number_of_open_connections++;
    return 0;
}

static int
iscsi_initiator_l5_set(const char *value)
{
    int                     rc = -1;
    int                     target_id = 0;
    int                     cid = 0;
    te_bool                 anything_to_stop = FALSE;
    te_bool                 anything_to_start = FALSE;
 
    for (target_id = 0; target_id < MAX_TARGETS_NUMBER; target_id++)
    {
        if (init_data->targets[target_id].is_active)
        {
            anything_to_stop = TRUE;
            break;
        }
    }
    iscsi_get_cid_and_target(value, &cid, &target_id);
    if (strncmp(value, "down", strlen("down")) == 0)
    {
        init_data->targets[target_id].is_active = FALSE;
    }
    else
    {
        init_data->targets[target_id].is_active = TRUE;
    }

    for (target_id = 0; target_id < MAX_TARGETS_NUMBER; target_id++)
    {
        if (init_data->targets[target_id].is_active)
        {
            anything_to_start = TRUE;
            break;
        }
    }
    rc = iscsi_l5_write_config(init_data);
    if (rc != 0)
        return rc;
    if (anything_to_stop)
    {
        rc = te_shell_cmd_ex("cd %s; ./iscsi_stop te", 
                             init_data->script_path);
        if (rc != 0)
        {
            ERROR("Unable to stop initiator connections");
            return rc;
        }
    }
    if (anything_to_start)
    {
        rc = te_shell_cmd_ex("cd %s; ./iscsi_start te", 
                         init_data->script_path);    
        if (rc != 0)
        {
            ERROR("Unable to start initiator connections");
            return rc;
        }
    }
    return 0;
}

static int
iscsi_initiator_openiscsi_set(const char *value)
{
    int                     rc = -1;
    int                     target_id = 0;
    int                     cid = 0;

    iscsi_get_cid_and_target(value, &cid, &target_id);
    if (strncmp(value, "down", strlen("down")) == 0)
    {
        if (*init_data->targets[target_id].record_id != '\0')
        {
            ERROR("Target %d has no associated record id", target_id);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        rc = te_shell_cmd_ex("iscsiadm -m node --record=%s --logout",
                             init_data->targets[target_id].record_id);
        if (cid == 0 && target_id == 0)
            iscsi_openiscsi_stop_daemon();
        return rc;
    }
    else
    {
        if (cid == 0 && target_id == 0)
        {
            rc = iscsi_openiscsi_start_daemon(init_data->targets);
            if (rc != 0)
                return rc;
        }
        if (*init_data->targets[target_id].record_id == '\0')
        {
            const char *id = iscsi_openiscsi_find_free_recid(init_data);
            if (id == NULL)
                return TE_RC(TE_TA_UNIX, TE_ETOOMANY);
            strcpy(init_data->targets[target_id].record_id, id);
        }
        rc = iscsi_openiscsi_set_target_params(init_data->targets + 
                                               target_id);
        if (rc != 0)
            return rc;
        return te_shell_cmd_ex("iscsiadm -m node --record=%s --login",
                               init_data->targets[target_id].record_id);
    }
    
    return 0;
}

static int
iscsi_initiator_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    switch (init_data->init_type)
    {
        case UNH:
            return iscsi_initiator_unh_set(value);
        case L5:
            return iscsi_initiator_l5_set(value);
        case OPENISCSI:
            return iscsi_initiator_openiscsi_set(value);
        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
}

/* AuthMethod */
static int
iscsi_initiator_chap_set(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *(init_data->targets[iscsi_get_target_id(oid)].chap.chap) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].chap.chap, 
           value);

    return 0;
}

static int
iscsi_initiator_chap_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].chap.chap);

    return 0;
}

/* Peer Name */
static int
iscsi_initiator_peer_name_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *(init_data->targets[iscsi_get_target_id(oid)].chap.peer_name) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].chap.peer_name, 
           value);

    return 0;
}

static int
iscsi_initiator_peer_name_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].chap.peer_name);

    return 0;
}

/* Challenge Length */
static int
iscsi_initiator_challenge_length_set(unsigned int gid, const char *oid,
                                     char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    init_data->targets[iscsi_get_target_id(oid)].chap.challenge_length = 
        atoi(value);

    return 0;
}

static int
iscsi_initiator_challenge_length_get(unsigned int gid, const char *oid,
                                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].chap.challenge_length);

    return 0;
}

/* Encoding Format */
static int
iscsi_initiator_enc_fmt_set(unsigned int gid, const char *oid,
                                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    init_data->targets[iscsi_get_target_id(oid)].chap.enc_fmt = atoi(value);

    return 0;
}

static int
iscsi_initiator_enc_fmt_get(unsigned int gid, const char *oid,
                                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].chap.enc_fmt);

    return 0;
}

/* Target Data */
static int
iscsi_target_data_add(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int tgt_id = iscsi_get_target_id(oid);
    
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(value);
 
    iscsi_init_default_tgt_parameters(&init_data->targets[tgt_id]);
    init_data->targets[tgt_id].target_id = tgt_id;
    init_data->targets[tgt_id].conf_params = 0;

    IVERB("Adding %s with value %s, id=%d\n", oid, value, tgt_id);

    return 0;
}

static int
iscsi_target_data_del(unsigned int gid, const char *oid,
                      const char *instance, ...)
{
    int tgt_id = atoi(instance + strlen("target_"));
    
    UNUSED(gid);
    UNUSED(oid);
    
    IVERB("Deletting %s\n", oid);
    init_data->targets[tgt_id].target_id = -1;
    return 0;
}

static int
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
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    return 0;
}

/* Target Authentication */
static int
iscsi_initiator_target_auth_set(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    te_bool t = FALSE;
    
    UNUSED(gid);
    UNUSED(instance);
    
    if (*value == '1')
        t = TRUE;
    
    init_data->targets[iscsi_get_target_id(oid)].chap.target_auth = t;

    return 0;
}

static int
iscsi_initiator_target_auth_get(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].chap.target_auth);

    return 0;
}

/* Peer Secret */
static int
iscsi_initiator_peer_secret_set(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *init_data->targets[iscsi_get_target_id(oid)].chap.peer_secret = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].chap.peer_secret, value);

    return 0;
}

static int
iscsi_initiator_peer_secret_get(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].chap.peer_secret);

    return 0;
}

/* Local Secret */
static int
iscsi_initiator_local_secret_set(unsigned int gid, const char *oid,
                                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    *init_data->targets[iscsi_get_target_id(oid)].chap.local_secret = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].chap.local_secret, 
           value);

    return 0;
}

static int
iscsi_initiator_local_secret_get(unsigned int gid, const char *oid,
                                char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].chap.local_secret);

    return 0;
}

/* MaxConnections */
static int
iscsi_max_connections_set(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_MAX_CONNECTIONS;

    init_data->targets[iscsi_get_target_id(oid)].
        max_connections = atoi(value);

    return 0;
}

static int
iscsi_max_connections_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *value = '\0';
    sprintf(value, "%d", init_data->targets[iscsi_get_target_id(oid)].
            max_connections);

    return 0;
}

/* InitialR2T */
static int
iscsi_initial_r2t_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_INITIAL_R2T;
    *(init_data->targets[iscsi_get_target_id(oid)].initial_r2t) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].initial_r2t, value);

    return 0;
}

static int
iscsi_initial_r2t_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].initial_r2t);

    return 0;
}

/* HeaderDigest */
static int
iscsi_header_digest_set(unsigned int gid, const char *oid,
                        char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_HEADER_DIGEST;
    *(init_data->targets[iscsi_get_target_id(oid)].header_digest) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].header_digest, value);

    return 0;
}

static int
iscsi_header_digest_get(unsigned int gid, const char *oid,
                        char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].header_digest);

    return 0;
}

/* DataDigest */
static int
iscsi_data_digest_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_DATA_DIGEST;
    *(init_data->targets[iscsi_get_target_id(oid)].data_digest) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].data_digest, value);

    return 0;
}

static int
iscsi_data_digest_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *value = '\0';
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].data_digest);

    return 0;
}

/* ImmediateData */
static int
iscsi_immediate_data_set(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_IMMEDIATE_DATA;
    *(init_data->targets[iscsi_get_target_id(oid)].immediate_data) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].immediate_data, value);

    return 0;
}

static int
iscsi_immediate_data_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].immediate_data);

    return 0;
}

/* MaxRecvDataSegmentLength */
static int
iscsi_max_recv_data_segment_length_set(unsigned int gid, const char *oid,
                                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_MAX_RECV_DATA_SEGMENT_LENGTH;
    init_data->targets[iscsi_get_target_id(oid)].
        max_recv_data_segment_length = atoi(value);

    return 0;
}

static int
iscsi_max_recv_data_segment_length_get(unsigned int gid, const char *oid,
                                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].
            max_recv_data_segment_length);

    return 0;
}

/* FirstBurstLength */
static int
iscsi_first_burst_length_set(unsigned int gid, const char *oid,
                             char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_FIRST_BURST_LENGTH;
    init_data->targets[iscsi_get_target_id(oid)].first_burst_length = 
        atoi(value);

    return 0;
}

static int
iscsi_first_burst_length_get(unsigned int gid, const char *oid,
                             char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
   
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].first_burst_length);

    return 0;
}

/* MaxBurstLength */
static int
iscsi_max_burst_length_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_MAX_BURST_LENGTH;
    init_data->targets[iscsi_get_target_id(oid)].max_burst_length = 
        atoi(value);

    return 0;
}

static int
iscsi_max_burst_length_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].max_burst_length);

    return 0;
}

/* DefaultTime2Wait */
static int
iscsi_default_time2wait_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_DEFAULT_TIME2WAIT;
    init_data->targets[iscsi_get_target_id(oid)].default_time2wait = atoi(value);

    return 0;
}

static int
iscsi_default_time2wait_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].default_time2wait);

    return 0;
}

/* DefaultTime2Retain */
static int
iscsi_default_time2retain_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_DEFAULT_TIME2RETAIN;
    init_data->targets[iscsi_get_target_id(oid)].default_time2retain = atoi(value);

    return 0;
}

static int
iscsi_default_time2retain_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].default_time2retain);

    return 0;
}

/* MaxOutstandingR2T */
static int
iscsi_max_outstanding_r2t_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_MAX_OUTSTANDING_R2T;
    init_data->targets[iscsi_get_target_id(oid)].max_outstanding_r2t = atoi(value);

    return 0;
}

static int
iscsi_max_outstanding_r2t_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", init_data->targets[iscsi_get_target_id(oid)].max_outstanding_r2t);

    return 0;
}

/* DataPDUInOrder */
static int
iscsi_data_pdu_in_order_set(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_DATA_PDU_IN_ORDER;
    *(init_data->targets[iscsi_get_target_id(oid)].data_pdu_in_order) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].data_pdu_in_order, value);

    return 0;
}

static int
iscsi_data_pdu_in_order_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].data_pdu_in_order);

    return 0;
}

/* DataSequenceInOrder */
static int
iscsi_data_sequence_in_order_set(unsigned int gid, const char *oid,
                                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_DATA_SEQUENCE_IN_ORDER;
    *(init_data->targets[iscsi_get_target_id(oid)].data_sequence_in_order) =
        '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].data_sequence_in_order, 
           value);

    return 0;
}

static int
iscsi_data_sequence_in_order_get(unsigned int gid, const char *oid,
                                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].data_sequence_in_order);

    return 0;
}

/* ErrorRecoveryLevel */
static int
iscsi_error_recovery_level_set(unsigned int gid, const char *oid,
                                     char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params |=
        OFFER_ERROR_RECOVERY_LEVEL;

    init_data->targets[iscsi_get_target_id(oid)].error_recovery_level = atoi(value);

    return 0;
}

static int
iscsi_error_recovery_level_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].error_recovery_level);

    return 0;
}

/* SessionType */
static int
iscsi_session_type_set(unsigned int gid, const char *oid,
                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *(init_data->targets[iscsi_get_target_id(oid)].session_type) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].session_type, value);

    return 0;
}

static int
iscsi_session_type_get(unsigned int gid, const char *oid,
                       char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].session_type);

    return 0;
}

/* TargetName */
static int
iscsi_target_name_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *(init_data->targets[iscsi_get_target_id(oid)].target_name) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].target_name, value);

    return 0;
}

static int
iscsi_target_name_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *value = '\0';
    sprintf(value, "%s", init_data->targets[iscsi_get_target_id(oid)].target_name);

    return 0;
}

/* InitiatorName */
static int
iscsi_initiator_name_set(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *(init_data->targets[iscsi_get_target_id(oid)].initiator_name) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].initiator_name, 
           value);

    return 0;
}

static int
iscsi_initiator_name_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].initiator_name);

    return 0;
}

/* InitiatorAlias */
static int
iscsi_initiator_alias_set(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    *(init_data->targets[iscsi_get_target_id(oid)].initiator_alias) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].initiator_alias, 
           value);

    return 0;
}

static int
iscsi_initiator_alias_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].initiator_alias);

    return 0;
}

/* TargetAddress */
static int
iscsi_target_addr_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *(init_data->targets[iscsi_get_target_id(oid)].target_addr) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].target_addr, value);
    
    return 0;
}

static int
iscsi_target_addr_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);

    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].target_addr);

    return 0;
}

/* TargetPort */
static int
iscsi_target_port_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    init_data->targets[iscsi_get_target_id(oid)].target_port = atoi(value);

    return 0;
}

static int
iscsi_target_port_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);

    *value = '\0';
    init_data->targets[iscsi_get_target_id(oid)].target_port = 3260;
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].target_port);

    return 0;
}

static int
iscsi_host_device_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    char        dev_pattern[128];
    glob_t      devices;
    int         rc;
    char       *nameptr;
    unsigned    i;


    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(dev_pattern, "/sys/bus/scsi/devices/%d:0:0:*/block", 
            init_data->host_bus_adapter);
    if ((rc = glob(dev_pattern, GLOB_ERR, NULL, &devices)) != 0)
    {
        switch(rc)
        {
            case GLOB_NOSPACE:
                ERROR("Cannot read a list of devices: no memory");
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            case GLOB_ABORTED:
                ERROR("Cannot read a list of devices: read error");
                return TE_RC(TE_TA_UNIX, TE_EIO);
            case GLOB_NOMATCH:
                *value = '\0';
                return 0;
            default:
                ERROR("unexpected error on glob()");
                return TE_RC(TE_TA_UNIX, TE_EFAIL);
        }
    }

    *value = '\0';
    for (i = 0; i < devices.gl_pathc; i++)
    {
        if (realpath(devices.gl_pathv[i], dev_pattern) == NULL)
        {
            WARN("Cannot resolve %s: %s", devices.gl_pathv[i],
                 strerror(errno));
        }
        else
        {
            nameptr = strrchr(dev_pattern, '/');
            if (nameptr == NULL)
                WARN("Strange sysfs name: %s", dev_pattern);
            else
            {
                if (*value != '\0')
                    strcat(value, " ");
                strcat(value, "/dev/");
                strcat(value, nameptr + 1);
            }
        }
    }

    globfree(&devices);
    return 0;
}

/* Initiator's path to scripts (for L5) */
static int
iscsi_script_path_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(init_data->script_path, value);
    return 0;
}

static int
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
static int
iscsi_type_set(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
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
        return TE_RC(TE_TA_UNIX, EINVAL);

    return 0;
}

static int
iscsi_type_get(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
    static char *types[] = {"unh", "l5", 
                            "microsoft", "open-iscsi"};
            
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, types[init_data->init_type]);

    return 0;
}


/* Host Bus Adapter */
static int
iscsi_host_bus_adapter_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    init_data->host_bus_adapter = atoi(value);

    return 0;
}


static int
iscsi_host_bus_adapter_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    *value = '\0';
    sprintf(value, "%d", init_data->host_bus_adapter);

    return 0;
}

/* Local Name */
static int
iscsi_initiator_local_name_set(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    *(init_data->targets[iscsi_get_target_id(oid)].chap.local_name) = '\0';
    strcpy(init_data->targets[iscsi_get_target_id(oid)].chap.local_name, value);

    return 0;
}

static int
iscsi_initiator_local_name_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);

    *value = '\0';
    sprintf(value, "%s", 
            init_data->targets[iscsi_get_target_id(oid)].chap.local_name);

    return 0;
}

static int
iscsi_parameters2advertize_set(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    init_data->targets[iscsi_get_target_id(oid)].conf_params = atoi(value);

    return 0;
}

static int
iscsi_parameters2advertize_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%d", 
            init_data->targets[iscsi_get_target_id(oid)].conf_params);
    
    return 0;
}

/* Configuration tree */

RCF_PCH_CFG_NODE_RO(node_iscsi_host_device, "host_device", NULL, 
                    NULL, iscsi_host_device_get);

RCF_PCH_CFG_NODE_RW(node_iscsi_script_path, "script_path", NULL, 
                    &node_iscsi_host_device, iscsi_script_path_get,
                    iscsi_script_path_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_type, "type", NULL, 
                    &node_iscsi_script_path, iscsi_type_get,
                    iscsi_type_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_host_bus_adapter, "host_bus_adapter", NULL, 
                    &node_iscsi_type, iscsi_host_bus_adapter_get,
                    iscsi_host_bus_adapter_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_alias, "initiator_alias", NULL, 
                    NULL, iscsi_initiator_alias_get,
                    iscsi_initiator_alias_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_name, "initiator_name", NULL, 
                    &node_iscsi_initiator_alias, iscsi_initiator_name_get,
                    iscsi_initiator_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_parameters2advertize, "parameters2advertize",
                    NULL, &node_iscsi_initiator_name,
                    iscsi_parameters2advertize_get, 
                    iscsi_parameters2advertize_set);
                    
RCF_PCH_CFG_NODE_RW(node_iscsi_target_port, "target_port", NULL, 
                    &node_iscsi_parameters2advertize,
                    iscsi_target_port_get, iscsi_target_port_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_addr, "target_addr", NULL, 
                    &node_iscsi_target_port,
                    iscsi_target_addr_get, iscsi_target_addr_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_name, "target_name", NULL,
                    &node_iscsi_target_addr, iscsi_target_name_get,
                    iscsi_target_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_session_type, "session_type", NULL,
                    &node_iscsi_target_name, iscsi_session_type_get,
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

RCF_PCH_CFG_NODE_COLLECTION(node_iscsi_target_data, "target_data",
                            &node_iscsi_chap, &node_iscsi_host_bus_adapter, 
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
int
ta_unix_iscsi_initiator_init(rcf_pch_cfg_object **last)
{
    /* On Init there is only one target configured on the Initiator */
    iscsi_init_default_ini_parameters(ISCSI_CONF_NO_TARGETS);

    DS_REGISTER(iscsi_initiator);

    return 0;
}
