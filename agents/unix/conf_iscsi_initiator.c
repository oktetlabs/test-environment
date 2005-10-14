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
              

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_config.h"
#include "config.h"
#include "conf_daemons.h"
#include "te_shell_cmd.h"

/* Debug logs */
#ifdef ISCSI_DEBUG_LOG
#define IVERB(args...) fprintf(stderr, args); fprintf(stderr, "\n")
#else
#define IVERB VERB
#endif

#define MAX_NAME_LENGTH 256

/* Yes or No */
#define YES_NO_LENGTH 4

/* CHAP or None or CHAP,None */
#define AUTH_METHOD_LENGTH 11

/* Normal or Discovery */
#define SESSION_TYPE_LENGTH 10

/* HeaderDigest length CRC32,None */
#define HEADER_DIGEST_LENGTH 15

/* Maximum length of the list of cids of the initiator */
#define MAX_CID_LIST_LENGTH 100

/* Maximum length of the CLI command */
#define MAX_CMD_SIZE 1024

/* Length of the peer_secret, peer_name, local_secret, local_name */
#define SECURITY_COMMON_LENGTH 256

/* Maximum number of targets the Initiator can connect to */
#define MAX_TARGETS_NUMBER 3

#define ISCSI_TARGET_DEFAULT_PORT 3260

#define DEFAULT_TARGET_NAME                  "iqn.2004-01.com:0"
#define DEFAULT_MAX_CONNECTIONS              1
#define DEFAULT_INITIAL_R2T                  "Yes"
#define DEFAULT_HEADER_DIGEST                "None"
#define DEFAULT_DATA_DIGEST                  "None"
#define DEFAULT_IMMEDIATE_DATA               "Yes"
#define DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH 8192
#define DEFAULT_FIRST_BURST_LENGTH           65536
#define DEFAULT_MAX_BURST_LENGTH             262144
#define DEFAULT_DEFAULT_TIME2WAIT            2
#define DEFAULT_DEFAULT_TIME2RETAIN          20
#define DEFAULT_MAX_OUTSTANDING_R2T          1
#define DEFAULT_DATA_PDU_IN_ORDER            "Yes"
#define DEFAULT_DATA_SEQUENCE_IN_ORDER       "Yes"
#define DEFAULT_ERROR_RECOVERY_LEVEL         0
#define DEFAULT_SESSION_TYPE                 "Normal"
#define DEFAULT_CHAP                         "None"
#define DEFAULT_CHALLENGE_LENGTH             256

#define DEFAULT_HOST_BUS_ADAPTER            0
#define DEFAULT_INITIATOR_NAME              "iqn.1999-11.edu.unh.iol.iscsi-initiator"
#define DEFAULT_INITIATOR_ALIAS             "UNH"

#define OFFER_MAX_CONNECTIONS                   (1 << 0)
#define OFFER_INITIAL_R2T                       (1 << 1)
#define OFFER_HEADER_DIGEST                     (1 << 2)
#define OFFER_DATA_DIGEST                       (1 << 3)
#define OFFER_IMMEDIATE_DATA                    (1 << 4)
#define OFFER_MAX_RECV_DATA_SEGMENT_LENGTH      (1 << 5)
#define OFFER_FIRST_BURST_LENGTH                (1 << 6)
#define OFFER_MAX_BURST_LENGTH                  (1 << 7)
#define OFFER_DEFAULT_TIME2WAIT                 (1 << 8)
#define OFFER_DEFAULT_TIME2RETAIN               (1 << 9)
#define OFFER_MAX_OUTSTANDING_R2T               (1 << 10)
#define OFFER_DATA_PDU_IN_ORDER                 (1 << 11)
#define OFFER_DATA_SEQUENCE_IN_ORDER            (1 << 12)
#define OFFER_ERROR_RECOVERY_LEVEL              (1 << 13)

typedef enum {
    UNH,
    L5,
} iscsi_initiator_type;

/* Encoding of challange and response */
typedef enum {
    BASE_16,
    BASE_64
} enc_fmt_e;

typedef struct iscsi_tgt_chap_data {
    char        chap[AUTH_METHOD_LENGTH];

    enc_fmt_e   enc_fmt;
    int         challenge_length;
    char        peer_name[SECURITY_COMMON_LENGTH];
    char        local_secret[SECURITY_COMMON_LENGTH];

    te_bool target_auth;
    char peer_secret[SECURITY_COMMON_LENGTH];
    char local_name[SECURITY_COMMON_LENGTH];
} iscsi_tgt_chap_data_t;

typedef struct iscsi_target_data {
    int               target_id;
    
    int               conf_params;

    char              initiator_name[MAX_NAME_LENGTH];
    char              initiator_alias[MAX_NAME_LENGTH];
    /* Target parameters */
    char              target_name[MAX_NAME_LENGTH];
    /* TODO: here the struct sockaddr should be used */
    char              target_addr[MAX_NAME_LENGTH];
    int               target_port;

    /* Initiator parameters */
    int               max_connections;
    char              initial_r2t[YES_NO_LENGTH];
    char              header_digest[HEADER_DIGEST_LENGTH];
    char              data_digest[HEADER_DIGEST_LENGTH];
    char              immediate_data[YES_NO_LENGTH];
    int               max_recv_data_segment_length;
    int               first_burst_length;
    int               max_burst_length;
    int               default_time2wait;
    int               default_time2retain;
    int               max_outstanding_r2t;
    char              data_pdu_in_order[YES_NO_LENGTH];
    char              data_sequence_in_order[YES_NO_LENGTH];
    int               error_recovery_level;
    char              session_type[SESSION_TYPE_LENGTH];

    iscsi_tgt_chap_data_t chap;

} iscsi_target_data_t;

/* Auxiliary variables used for during configuration request processing */
typedef struct iscsi_initiator_data {
    /* Auxiliary data of the initiator */
    iscsi_initiator_type  init_type;

    char                  last_cmd[MAX_CMD_SIZE];

    int                   host_bus_adapter;    
    
    iscsi_target_data_t   targets[MAX_TARGETS_NUMBER];
} iscsi_initiator_data_t;


/* List of all targets on this agent */
iscsi_initiator_data_t *init_data;

static int
iscsi_get_target_id(const char *oid)
{
    char *c;
    
    c = strstr(oid, "target_");
    c = strstr(c + 1, "target_");
    c += strlen("target_");

    return atoi(c);
}

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

/**
 * Function configures init_data and target number i.
 * If i == -1 it configures all targets with default parameters.
 */

#define CONF_ALL_TARGETS -1
#define CONF_NO_TARGETS -2

static void
iscsi_init_default_ini_parameters(int i)
{
    init_data = malloc(sizeof(iscsi_initiator_data_t));
    memset(init_data, 0, sizeof(init_data));

    /* init_id */
    /* init_type */
    /* initiator name */
    init_data->host_bus_adapter = DEFAULT_HOST_BUS_ADAPTER;
    
    if (i == CONF_NO_TARGETS)
    {
        int j;

        for (j = 0;j < MAX_TARGETS_NUMBER;j++)
        {
            init_data->targets[j].target_id = -1;
        }

        VERB("No targets were configured");
    }
    else if (i == CONF_ALL_TARGETS)
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
         iscsi_init_default_tgt_parameters(&init_data->targets[i]);
         init_data->targets[i].target_id = i;
    }
}

/**
 * Returns list of the CIDs of all open connections:
 * "5 3 2 6"
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

#define CHECK_SHELL_CONFIG_RC(rc_, parameter_) \
    do {                                                                    \
        if (rc_ != 0)                                                       \
        {                                                                   \
            ERROR("Setting %s parameter failed, rc=%d",                     \
                  (parameter_), (rc_));                                     \
            return (rc_);                                                   \
        }                                                                   \
    } while (0)

static const char *conf_iscsi_unh_set_fmt =
    "iscsi_manage init set %s=%s target=%d host=%d";

static const char *conf_iscsi_unh_set_int_fmt =
    "iscsi_manage init set %s=%d target=%d host=%d";

static const char *conf_iscsi_unh_force_fmt =
    "iscsi_manage init force %s=%s target=%d host=%d";

static const char *conf_iscsi_unh_force_string_fmt =
    "iscsi_manage init force %s=\"%s\" target=%d host=%d";
    
static const char *conf_iscsi_unh_force_int_fmt =
    "iscsi_manage init force %s=%d target=%d host=%d";

static const char *conf_iscsi_unh_force_flag_fmt =
    "iscsi_manage init force %s target=%d host=%d";

static const char *conf_iscsi_unh_unset_flag_fmt =
    "iscsi_manage init unset %s target=%d host=%d";

#define ISCSI_UNH_SET(param_, value_, target_id_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_SET(%s,0x%x,%d)\n",                                \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_fmt, (param_),                  \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (param_));                                                      \
    } while (0)

#define ISCSI_UNH_SET_INT(param_, value_, target_id_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_SET_INT(%s,0x%x,%d)\n",                            \
                param_, value_, target_id_);                                \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_set_int_fmt, (param_),              \
                         (value_), (target_id_),                            \
                         init_data->host_bus_adapter),                      \
            (param_));                                                      \
    } while (0)

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

#define ISCSI_UNH_UNSET_FLAG(flag_, target_id_, info_) \
    do {                                                                    \
        IVERB("ISCSI_UNH_UNSET_FLAG(%s,%d)\n",                              \
                flag_, target_id_);                                         \
        CHECK_SHELL_CONFIG_RC(                                              \
            ta_system_ex(conf_iscsi_unh_unset_flag_fmt, (flag_),            \
                         (target_id_),                                      \
                         init_data->host_bus_adapter),                      \
            (info_));                                                       \
    } while (0)



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
 * Format of the value:
 * up 5 7 - establish the connection with id = 5 and target 7
 * down 5 3- destroy the connection with id = 5 and target 3
 */
/* TODO: write normal handler */
static int
iscsi_initiator_set(unsigned int gid, const char *oid, const char *value,
                    const char *instance, ...)
{
    int                     rc = -1;
    int                     cid = 0;
    int                     target_id = 0;
    iscsi_target_data_t    *target;

    int                     offer;
 
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    /* TODO: init_type = UNH */
    init_data->init_type = UNH;

    iscsi_get_cid_and_target(value, &cid, &target_id);
    if (strncmp(value, "down", strlen("down")) == 0)
    {
        /* We should down the connection */
        switch (init_data->init_type)
        {
            case UNH:
                rc = ta_system_ex("iscsi_config down cid=%d target=%d host=%d",
                                   cid, target_id, 
                                  init_data->host_bus_adapter);
                if (rc != 0)
                {
                    ERROR("Failed to close the connection "
                          "with CID = %d", cid);
                    return TE_RC(TE_TA_UNIX, EINVAL);
                }
                break;
            case L5:
                break;
            default:
                ERROR("Unknown initiator type");
                return TE_RC(TE_TA_UNIX, EINVAL);
        }
        INFO("Connections with ID %d is closed", cid);
        return 0;
    }
    /* We should open new connection */
    /* 1: configurating the Initiator */

    target = &init_data->targets[target_id];
    
    switch (init_data->init_type)
    {
        case UNH:
            offer = target->conf_params;
            
            IVERB("Offer: %d", (int)offer);

            CHECK_SHELL_CONFIG_RC(
                ta_system_ex("iscsi_manage init restore target=%d host=%d",
                             target_id, init_data->host_bus_adapter),
                "Restoring");

            ISCSI_UNH_SET("TargetName", target->target_name, target_id);

            if ((offer & OFFER_MAX_CONNECTIONS) == OFFER_MAX_CONNECTIONS)
                ISCSI_UNH_SET_INT("MaxConnections", target->max_connections,
                                  target_id);
            if ((offer & OFFER_INITIAL_R2T) == OFFER_INITIAL_R2T)
                ISCSI_UNH_SET("InitialR2T", target->initial_r2t, target_id);

            if ((offer & OFFER_HEADER_DIGEST) == OFFER_INITIAL_R2T)
                ISCSI_UNH_SET("HeaderDigest", target->header_digest, target_id);

            if ((offer & OFFER_DATA_DIGEST) == OFFER_DATA_DIGEST)
                ISCSI_UNH_SET("DataDigest", target->data_digest, target_id);

            if ((offer & OFFER_IMMEDIATE_DATA) == OFFER_IMMEDIATE_DATA)
                ISCSI_UNH_SET("ImmediateData", target->immediate_data, target_id);

            if ((offer & OFFER_MAX_RECV_DATA_SEGMENT_LENGTH) ==
                OFFER_MAX_RECV_DATA_SEGMENT_LENGTH)
                ISCSI_UNH_SET_INT("MaxRecvDataSegmentLength", 
                                  target->max_recv_data_segment_length,
                                  target_id);

            if ((offer & OFFER_FIRST_BURST_LENGTH) == OFFER_FIRST_BURST_LENGTH)
                ISCSI_UNH_SET_INT("MaxBurstLength", 
                                  target->max_burst_length, target_id);

            if ((offer & OFFER_MAX_BURST_LENGTH) == OFFER_MAX_BURST_LENGTH)
                ISCSI_UNH_SET_INT("FirstBurstLength", 
                                  target->first_burst_length, target_id);

            if ((offer & OFFER_DEFAULT_TIME2WAIT) == OFFER_DEFAULT_TIME2WAIT)
                ISCSI_UNH_SET_INT("DefaultTime2Wait", 
                                  target->default_time2wait, target_id);

            if ((offer & OFFER_DEFAULT_TIME2RETAIN) == OFFER_DEFAULT_TIME2RETAIN)
                ISCSI_UNH_SET_INT("DefaultTime2Retain", 
                                  target->default_time2retain, target_id);

            if ((offer & OFFER_MAX_OUTSTANDING_R2T) == OFFER_MAX_OUTSTANDING_R2T)
                ISCSI_UNH_SET_INT("MaxOutstandingR2T", 
                                  target->max_outstanding_r2t, target_id);

            if ((offer & OFFER_DATA_PDU_IN_ORDER) == OFFER_DATA_PDU_IN_ORDER)
                ISCSI_UNH_SET("DataPDUInOrder", 
                              target->data_pdu_in_order, target_id);

            if ((offer & OFFER_DATA_SEQUENCE_IN_ORDER) == 
                OFFER_DATA_SEQUENCE_IN_ORDER)
                ISCSI_UNH_SET("DataSequenceInOrder", 
                              target->data_sequence_in_order, target_id);

            if ((offer & OFFER_ERROR_RECOVERY_LEVEL) == 
                OFFER_ERROR_RECOVERY_LEVEL)
                ISCSI_UNH_SET_INT("ErrorRecoveryLevel", 
                                  target->error_recovery_level, target_id);

            ISCSI_UNH_SET("SessionType", target->session_type, target_id);
    
            /* Target' CHAP */
            if (init_data->targets[target_id].chap.target_auth)
            {
                ISCSI_UNH_FORCE_FLAG("t", target_id,
                                     "Target Authentication");
            }
            else
            {
                ISCSI_UNH_UNSET_FLAG("t", target_id,
                                     "Target Authentication");
            }

            ISCSI_UNH_FORCE_STRING("px", target->chap.peer_secret, target_id,
                                   "Peer Secret");

            ISCSI_UNH_FORCE("ln", target->chap.local_name, target_id,
                            "Local Name");

            /* Initiator itself */
            ISCSI_UNH_SET("InitiatorName", 
                          target->initiator_name,
                          target_id);

            ISCSI_UNH_SET("InitiatorAlias", 
                          target->initiator_alias, 
                          target_id);

            ISCSI_UNH_SET("AuthMethod", target->chap.chap, target_id);

            if (target->chap.enc_fmt == BASE_64)
                ISCSI_UNH_FORCE_FLAG("b", target_id,
                                     "Encoding Format");
            else
                ISCSI_UNH_UNSET_FLAG("b", target_id,
                                     "Encoding Format");

            ISCSI_UNH_FORCE_INT("cl", target->chap.challenge_length, target_id,
                                "Challenge Length");

            ISCSI_UNH_FORCE("pn", target->chap.peer_name, target_id,
                            "Peer Name");

            ISCSI_UNH_FORCE_STRING("lx", target->chap.local_secret, target_id,
                                   "Local Secret");


            /* Now the connection should be opened */
            rc = te_shell_cmd_ex("iscsi_config up ip=%s port=%d "
                                 "cid=%d target=%d host=%d",
                                 target->target_addr,
                                 target->target_port,
                                 cid, target_id, init_data->host_bus_adapter);
            if (rc != 0)
            {
                ERROR("Failed to establish connection with cid=%d", cid);
                return rc;
            }
            break;
        case L5:
            break;
        default:
            ERROR("Wrong Initiator Type");
    }

    return 0;
}

/* Operational parameters */
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

    fprintf(stderr, "Adding %s with value %s, id=%d\n", oid, value, tgt_id);

    return 0;
}

static int
iscsi_target_data_del(unsigned int gid, const char *oid,
                      const char *instance, ...)
{
    int tgt_id = atoi(instance + strlen("target_"));
    
    UNUSED(gid);
    UNUSED(oid);
    
    fprintf(stderr, "Deletting %s\n", oid);
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

    IVERB("Offer E: %d", init_data->targets[iscsi_get_target_id(oid)].conf_params);
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

/* Configuration tree */

RCF_PCH_CFG_NODE_RW(node_iscsi_host_bus_adapter, "host_bus_adapter", NULL, 
                    NULL, iscsi_host_bus_adapter_get,
                    iscsi_host_bus_adapter_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_alias, "initiator_alias", NULL, 
                    NULL, iscsi_initiator_alias_get,
                    iscsi_initiator_alias_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_name, "initiator_name", NULL, 
                    &node_iscsi_initiator_alias, iscsi_initiator_name_get,
                    iscsi_initiator_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_port, "target_port", NULL, 
                    &node_iscsi_initiator_name,
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


int
ta_unix_iscsi_initiator_init(rcf_pch_cfg_object **last)
{
    /* On Init there is only one target configured on the Initiator */
    iscsi_init_default_ini_parameters(CONF_NO_TARGETS);

    DS_REGISTER(iscsi_initiator);

    return 0;
}
