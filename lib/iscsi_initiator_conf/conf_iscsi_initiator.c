/** @file
 * @brief iSCSI Initiator configuration
 *
 * Configurator tree
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
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER      "Configure iSCSI"

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
#ifdef __CYGWIN__
#include <regex.h>
#endif
#include <pthread.h>
#include <semaphore.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_shell_cmd.h"
#include "te_iscsi.h"
#include "iscsi_initiator.h"
#include "te_tools.h"


/**                                                                         
 * A template for connection-wide Initiator string parameter accessors.
 * Accessors are named `iscsi_parm_xxx_get' and `iscsi_parm_xxx_set',
 * where `xxx' is @p name and are of types rcf_ch_cfg_get and rcf_ch_cfg_set.
 *                                                                          
 * @param name  Parameter name                                              
 * @param field Field name in iscsi_connection_data_t
 * 
 */                                                                         
#define ISCSI_INITIATOR_STR_CONN_PARAM(name, field)                         \
static te_errno                                                             \
iscsi_parm_##name##_set(unsigned int gid, const char *oid,                  \
                             char *value, const char *instance, ...)        \
{                                                                           \
    iscsi_target_data_t    *target =                                        \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];          \
    iscsi_connection_data_t *conn  = &target->conns[iscsi_get_cid(oid)];    \
                                                                            \
    UNUSED(gid);                                                            \
    UNUSED(instance);                                                       \
                                                                            \
    strncpy(conn->field, value, sizeof(conn->field) - 1);                   \
                                                                            \
    return 0;                                                               \
}                                                                           \
                                                                            \
static te_errno                                                             \
iscsi_parm_##name##_get(unsigned int gid, const char *oid,                  \
                             char *value, const char *instance, ...)        \
{                                                                           \
    iscsi_target_data_t    *target =                                        \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];          \
    iscsi_connection_data_t *conn = &target->conns[iscsi_get_cid(oid)];     \
                                                                            \
                                                                            \
    UNUSED(gid);                                                            \
    UNUSED(instance);                                                       \
                                                                            \
    strcpy(value, conn->field);                                             \
                                                                            \
    return 0;                                                               \
}                                                                           \
struct iscsi_##name##_eating_semicolon

/**                                                                         
 * A template for connection-wide Initiator integral parameter accessors    
 * Accessors are named `iscsi_parm_xxx_get' and `iscsi_parm_xxx_set',
 * where `xxx' is @p name and are of types rcf_ch_cfg_get and rcf_ch_cfg_set.
 *                                                                          
 * @param name  Parameter name                                              
 * @param field Field name in iscsi_connection_data_t                       
 */                                                                         
#define ISCSI_INITIATOR_INT_CONN_PARAM(name, field)                         \
static te_errno                                                             \
iscsi_parm_##name##_set(unsigned int gid, const char *oid,                  \
                             char *value, const char *instance, ...)        \
{                                                                           \
    iscsi_target_data_t    *target =                                        \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];          \
    iscsi_connection_data_t *conn = &target->conns[iscsi_get_cid(oid)];     \
                                                                            \
    UNUSED(gid);                                                            \
    UNUSED(instance);                                                       \
                                                                            \
    conn->field = strtol(value, NULL, 10);                                  \
                                                                            \
    return 0;                                                               \
}                                                                           \
                                                                            \
static te_errno                                                             \
iscsi_parm_##name##_get(unsigned int gid, const char *oid,                  \
                             char *value, const char *instance, ...)        \
{                                                                           \
    iscsi_target_data_t    *target =                                        \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];          \
    iscsi_connection_data_t *conn = &target->conns[iscsi_get_cid(oid)];     \
                                                                            \
    UNUSED(gid);                                                            \
    UNUSED(instance);                                                       \
                                                                            \
    sprintf(value, "%d", (int)conn->field);                                 \
                                                                            \
    return 0;                                                               \
}                                                                           \
struct iscsi_##name##_eating_semicolon

/**                                                                     
 * A template for target-wide Initiator string parameter accessors.
 * Accessors are named `iscsi_parm_xxx_get' and `iscsi_parm_xxx_set',
 * where `xxx' is @p name and are of types rcf_ch_cfg_get and rcf_ch_cfg_set.
 *                                                                      
 * @param name  Parameter name and field name in iscsi_target_data_t    
 */                                                                     
#define ISCSI_INITIATOR_STR_TGT_PARAM(name)                             \
static te_errno                                                         \
iscsi_parm_##name##_set(unsigned int gid, const char *oid,              \
                             char *value, const char *instance, ...)    \
{                                                                       \
    iscsi_target_data_t    *target =                                    \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];      \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(instance);                                                   \
                                                                        \
    strncpy(target->name, value, sizeof(target->name) - 1);             \
                                                                        \
    return 0;                                                           \
}                                                                       \
                                                                        \
static te_errno                                                         \
iscsi_parm_##name##_get(unsigned int gid, const char *oid,              \
                             char *value, const char *instance, ...)    \
{                                                                       \
    iscsi_target_data_t    *target =                                    \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];      \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(instance);                                                   \
                                                                        \
    strcpy(value, target->name);                                        \
                                                                        \
    return 0;                                                           \
}                                                                       \
struct iscsi_##name##_eating_semicolon

/**                                                                     
 * A template for target-wide Initiator integral parameter accessors
 * Accessors are named `iscsi_parm_xxx_get' and `iscsi_parm_xxx_set',
 * where `xxx' is @p name and are of types rcf_ch_cfg_get and rcf_ch_cfg_set.
 *                                                                      
 * @param name  Parameter name and field name in iscsi_target_data_t    
 */                                                                     
#define ISCSI_INITIATOR_INT_TGT_PARAM(name)                             \
static te_errno                                                         \
iscsi_parm_##name##_set(unsigned int gid, const char *oid,              \
                             char *value, const char *instance, ...)    \
{                                                                       \
    iscsi_target_data_t    *target =                                    \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];      \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(instance);                                                   \
                                                                        \
    target->name = strtol(value, NULL, 10);                             \
                                                                        \
    return 0;                                                           \
}                                                                       \
                                                                        \
static te_errno                                                         \
iscsi_parm_##name##_get(unsigned int gid, const char *oid,              \
                             char *value, const char *instance, ...)    \
{                                                                       \
    iscsi_target_data_t    *target =                                    \
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];      \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(instance);                                                   \
                                                                        \
    sprintf(value, "%d", (int)target->name);                            \
                                                                        \
    return 0;                                                           \
}                                                                       \
struct iscsi_##name##_eating_semicolon


ISCSI_INITIATOR_STR_CONN_PARAM(chap, chap.chap);
ISCSI_INITIATOR_STR_CONN_PARAM(peer_name, chap.peer_name);
ISCSI_INITIATOR_STR_CONN_PARAM(peer_secret, chap.peer_secret);
ISCSI_INITIATOR_STR_CONN_PARAM(local_name, chap.local_name);
ISCSI_INITIATOR_STR_CONN_PARAM(local_secret, chap.local_secret);
ISCSI_INITIATOR_INT_CONN_PARAM(challenge_length, chap.challenge_length);
ISCSI_INITIATOR_INT_CONN_PARAM(enc_fmt, chap.enc_fmt);
ISCSI_INITIATOR_INT_CONN_PARAM(target_auth, chap.need_target_auth);
ISCSI_INITIATOR_INT_CONN_PARAM(max_connections, max_connections);
ISCSI_INITIATOR_STR_CONN_PARAM(initial_r2t, initial_r2t);
ISCSI_INITIATOR_STR_CONN_PARAM(immediate_data, immediate_data);
ISCSI_INITIATOR_STR_CONN_PARAM(header_digest, header_digest);
ISCSI_INITIATOR_STR_CONN_PARAM(data_digest, data_digest);
ISCSI_INITIATOR_INT_CONN_PARAM(max_recv_data_segment_length, 
                               max_recv_data_segment_length);
ISCSI_INITIATOR_INT_CONN_PARAM(max_burst_length, max_burst_length);
ISCSI_INITIATOR_INT_CONN_PARAM(first_burst_length, first_burst_length);
ISCSI_INITIATOR_INT_CONN_PARAM(max_outstanding_r2t, max_outstanding_r2t);
ISCSI_INITIATOR_INT_CONN_PARAM(default_time2retain, default_time2retain);
ISCSI_INITIATOR_INT_CONN_PARAM(default_time2wait, default_time2wait);
ISCSI_INITIATOR_INT_CONN_PARAM(error_recovery_level, error_recovery_level);
ISCSI_INITIATOR_STR_CONN_PARAM(data_pdu_in_order, data_pdu_in_order);
ISCSI_INITIATOR_STR_CONN_PARAM(data_sequence_in_order, data_sequence_in_order);
ISCSI_INITIATOR_STR_CONN_PARAM(session_type, session_type);
ISCSI_INITIATOR_STR_CONN_PARAM(initiator_name, initiator_name);
ISCSI_INITIATOR_STR_CONN_PARAM(initiator_alias, initiator_alias);

ISCSI_INITIATOR_STR_TGT_PARAM(target_name);
ISCSI_INITIATOR_STR_TGT_PARAM(target_addr);
ISCSI_INITIATOR_INT_TGT_PARAM(target_port);

/* Connection structure */
static te_errno
iscsi_conn_add(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target =
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];
    iscsi_connection_data_t *conn = &target->conns[iscsi_get_cid(oid)];

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(value);

    VERB("Adding connection with id=%d to target with id %d", 
          conn - target->conns, target->target_id);

    pthread_mutex_lock(&conn->status_mutex);
    conn->status = (conn->status == ISCSI_CONNECTION_REMOVED || 
                    conn->status == ISCSI_CONNECTION_DOWN) ? 
        ISCSI_CONNECTION_DOWN :
        ISCSI_CONNECTION_ABNORMAL;
    pthread_mutex_unlock(&conn->status_mutex);

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
                                         ISCSI_CONNECTION_REMOVED, FALSE);
}

static te_errno
iscsi_conn_list(unsigned int gid, const char *oid,
                char **list, const char *instance, ...)
{
    int   cid;
    int   tgt_id = iscsi_get_target_id(oid);
    char  conns_list[ISCSI_MAX_CONNECTIONS_NUMBER * 15];
    char  conn[15];
    
    UNUSED(gid);
    UNUSED(instance);
    
    conns_list[0] = '\0';
    
    for (cid = 0; cid < ISCSI_MAX_CONNECTIONS_NUMBER; cid++)
    {
        if (iscsi_configuration()->targets[tgt_id].conns[cid].status != 
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
        return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
    }

    return 0;
}

/* Target Data */
static te_errno
iscsi_target_data_add(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int tgt_id = iscsi_get_target_id(oid);
    
    te_errno rc;

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(value);


    rc = iscsi_initiator_start_thread();
    if (rc != 0)
    {
        return rc;
    }
    
    iscsi_configuration()->n_targets++;
    iscsi_init_default_tgt_parameters(&iscsi_configuration()->targets[tgt_id]);
    iscsi_configuration()->targets[tgt_id].target_id = tgt_id;

    VERB("Adding %s with value %s, id=%d\n", oid, value, tgt_id);

    return 0;
}

static te_errno
iscsi_target_data_del(unsigned int gid, const char *oid,
                      const char *instance, ...)
{
    int tgt_id = atoi(instance + strlen("target_"));
    
    UNUSED(gid);
    UNUSED(oid);
    
    VERB("Deleting %s\n", oid);
    if (iscsi_configuration()->targets[tgt_id].target_id >= 0)
    {
        iscsi_configuration()->targets[tgt_id].target_id = -1;
        if (--iscsi_configuration()->n_targets == 0)
        {
            /* to stop the thread and possibly a service daemon */
            iscsi_post_connection_request(ISCSI_ALL_CONNECTIONS, ISCSI_ALL_CONNECTIONS, 
                                          ISCSI_CONNECTION_REMOVED, FALSE);
        }
    }

    return 0;
}

static te_errno
iscsi_target_data_list(unsigned int gid, const char *oid,
                       char **list, const char *instance, ...)
{
    int   id;
    char  targets_list[ISCSI_MAX_TARGETS_NUMBER * 15];
    char  tgt[15];
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    targets_list[0] = '\0';
    
    for (id = 0; id < ISCSI_MAX_TARGETS_NUMBER; id++)
    {
        if (iscsi_configuration()->targets[id].target_id != -1)
        {
            tgt[0] = '\0';
            sprintf(tgt,
                    "target_%d ", iscsi_configuration()->targets[id].target_id);
            strcat(targets_list, tgt);
        }
    }

    *list = strdup(targets_list);
    if (*list == NULL)
    {
        ERROR("Failed to allocate memory for the list of targets");
        return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
    }
    return 0;
}

/**
 *  Get host device name
 *
 */
static te_errno
iscsi_host_device_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int status;
    iscsi_target_data_t    *target = 
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];

    UNUSED(gid);
    UNUSED(instance);

    pthread_mutex_lock(&target->conns[0].status_mutex);
    status = target->conns[0].status;
    if (status != ISCSI_CONNECTION_UP)
    {
        WARN("Connection is not up, no host device name available");
        *value = '\0';
    }
    else
    {
        RING("%s(): device=%s", __FUNCTION__,
             target->conns[0].device_name);
        strcpy(value, target->conns[0].device_name);
    }
    pthread_mutex_unlock(&target->conns[0].status_mutex);

    return 0;
}

/**
 *  Get SCSI generic device name
 *
 */
static te_errno
iscsi_generic_device_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    int status;
    iscsi_target_data_t    *target = 
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];

    UNUSED(gid);
    UNUSED(instance);

    
    pthread_mutex_lock(&target->conns[0].status_mutex);
    status = target->conns[0].status;
    if (status != ISCSI_CONNECTION_UP)
    {
        WARN("Connection is not up, no generic device name available");
        *value = '\0';
    }
    else
    {
        if (target->conns[0].scsi_generic_device_name[0] == '\0')
        {
            int rc;
            pthread_mutex_unlock(&target->conns[0].status_mutex);
            rc = iscsi_get_device_name(&target->conns[0], 
                                       target->target_id,
                                       TRUE,
                                       target->conns[0]. \
                                       scsi_generic_device_name);
            if (rc != 0)
                return rc;
            pthread_mutex_lock(&target->conns[0].status_mutex);
        }
        strcpy(value, target->conns[0].scsi_generic_device_name);
    }
    pthread_mutex_unlock(&target->conns[0].status_mutex);

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

    strncpy(iscsi_configuration()->script_path, value, 
            sizeof(iscsi_configuration()->script_path) - 1);
    return 0;
}

static te_errno
iscsi_script_path_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, iscsi_configuration()->script_path);

    return 0;
}

/* Initiator type */
static te_errno
iscsi_type_set(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
#if 0
    int previous_type = iscsi_configuration()->init_type;
#endif
    static iscsi_initiator_control_t handlers[] = 
        {
            iscsi_initiator_dummy_set,
            iscsi_initiator_unh_set,
            iscsi_initiator_l5_set,
            iscsi_initiator_openiscsi_set,
            iscsi_initiator_win32_set,
            iscsi_initiator_win32_set
        };

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (*value == '\0')
        iscsi_configuration()->init_type = ISCSI_NO_INITIATOR;
    else if (strcmp(value, "unh") == 0)
        iscsi_configuration()->init_type = ISCSI_UNH;
    else if (strcmp(value, "l5") == 0)
        iscsi_configuration()->init_type = ISCSI_L5;
    else if (strcmp(value, "l5_win32") == 0)
        iscsi_configuration()->init_type = ISCSI_L5_WIN32;
    else if (strcmp(value, "microsoft") == 0)
        iscsi_configuration()->init_type = ISCSI_MICROSOFT;
    else if (strcmp(value, "open-iscsi") == 0)
        iscsi_configuration()->init_type = ISCSI_OPENISCSI;
    else
        return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);

    iscsi_configuration()->handler = handlers[iscsi_configuration()->init_type];

#if 0
    if (previous_type != iscsi_configuration()->init_type &&
        previous_type == ISCSI_OPENISCSI)
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
    static char *types[] = {"", "unh", "l5", 
                            "open-iscsi", "microsoft",
                            "l5_win32"};
            
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, types[iscsi_configuration()->init_type]);

    return 0;
}


static te_errno
iscsi_host_bus_adapter_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", iscsi_configuration()->host_bus_adapter);

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

    iscsi_configuration()->verbosity = atoi(value);

    return 0;
}


static te_errno
iscsi_initiator_verbose_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", iscsi_configuration()->verbosity);

    return 0;
}

/* Win32 iSCSI Service Restart Requirement */
static te_errno
iscsi_initiator_win32_service_restart_set(unsigned int gid, const char *oid,
                                          char *value, const char *instance,
                                          ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    iscsi_configuration()->win32_service_restart = atoi(value);

    return 0;
}

static te_errno
iscsi_initiator_win32_service_restart_get(unsigned int gid, const char *oid,
                                          char *value, const char *instance,
                                          ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", iscsi_configuration()->win32_service_restart);

    return 0;
}

/* Retry timeout */
static te_errno
iscsi_initiator_retry_timeout_set(unsigned int gid, const char *oid,
                                  char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    iscsi_configuration()->retry_timeout = strtoul(value, NULL, 10);

    return 0;
}


static te_errno
iscsi_initiator_retry_timeout_get(unsigned int gid, const char *oid,
                                  char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%lu", iscsi_configuration()->retry_timeout);

    return 0;
}


/* Retry attempts */
static te_errno
iscsi_initiator_retry_attempts_set(unsigned int gid, const char *oid,
                                   char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    iscsi_configuration()->retry_attempts = strtoul(value, NULL, 10);

    return 0;
}


static te_errno
iscsi_initiator_retry_attempts_get(unsigned int gid, const char *oid,
                                  char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", iscsi_configuration()->retry_attempts);

    return 0;
}


static te_errno
iscsi_parameters2advertize_set(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    INFO("SETTING %s to %s", oid, value);
    iscsi_configuration()->targets[iscsi_get_target_id(oid)].
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
            iscsi_configuration()->targets[tgt_id].conns[cid].conf_params);
    
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

    return iscsi_post_connection_request(tgt_id, cid, oper, FALSE);
}

static te_errno
iscsi_status_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    int status;
    iscsi_target_data_t    *target = 
        &iscsi_configuration()->targets[iscsi_get_target_id(oid)];
    iscsi_connection_data_t *conn  = &target->conns[iscsi_get_cid(oid)];

    UNUSED(gid);
    UNUSED(instance);

    pthread_mutex_lock(&conn->status_mutex);
    status = conn->status;
    pthread_mutex_unlock(&conn->status_mutex);    
    
    sprintf(value, "%d", status);

    return 0;
}

/* Configuration tree */

RCF_PCH_CFG_NODE_RW(node_iscsi_retry_attempts, "retry_attempts", NULL, 
                    NULL, 
                    iscsi_initiator_retry_attempts_get,
                    iscsi_initiator_retry_attempts_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_retry_timeout, "retry_timeout", NULL, 
                    &node_iscsi_retry_attempts, 
                    iscsi_initiator_retry_timeout_get,
                    iscsi_initiator_retry_timeout_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_verbose, "verbose", NULL, 
                    &node_iscsi_retry_timeout, 
                    iscsi_initiator_verbose_get,
                    iscsi_initiator_verbose_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_win32_service_restart,
                    "win32_service_restart", NULL, 
                    &node_iscsi_verbose, 
                    iscsi_initiator_win32_service_restart_get,
                    iscsi_initiator_win32_service_restart_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_script_path, "script_path", NULL, 
                    &node_iscsi_win32_service_restart,
                    iscsi_script_path_get,
                    iscsi_script_path_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_type, "type", NULL, 
                    &node_iscsi_script_path, iscsi_type_get,
                    iscsi_type_set);

RCF_PCH_CFG_NODE_RO(node_iscsi_host_bus_adapter, "host_bus_adapter", NULL, 
                    &node_iscsi_type, iscsi_host_bus_adapter_get);

RCF_PCH_CFG_NODE_RO(node_iscsi_initiator_generic_device, "generic_device",
                    NULL, NULL,
                    iscsi_generic_device_get);

RCF_PCH_CFG_NODE_RO(node_iscsi_initiator_host_device, "host_device",
                    NULL, &node_iscsi_initiator_generic_device,
                    iscsi_host_device_get);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_port, "target_port", NULL, 
                    &node_iscsi_initiator_host_device,
                    iscsi_parm_target_port_get, 
                    iscsi_parm_target_port_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_addr, "target_addr", NULL, 
                    &node_iscsi_target_port,
                    iscsi_parm_target_addr_get, 
                    iscsi_parm_target_addr_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_name, "target_name", NULL,
                    &node_iscsi_target_addr, 
                    iscsi_parm_target_name_get,
                    iscsi_parm_target_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_cid, "status", NULL, NULL,
                    iscsi_status_get, iscsi_status_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_alias, "initiator_alias", NULL, 
                    &node_iscsi_cid, 
                    iscsi_parm_initiator_alias_get,
                    iscsi_parm_initiator_alias_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_name, "initiator_name", NULL, 
                    &node_iscsi_initiator_alias, 
                    iscsi_parm_initiator_name_get,
                    iscsi_parm_initiator_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_parameters2advertize, "parameters2advertize",
                    NULL, &node_iscsi_initiator_name,
                    iscsi_parameters2advertize_get, 
                    iscsi_parameters2advertize_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_session_type, "session_type", NULL,
                    &node_iscsi_parameters2advertize, 
                    iscsi_parm_session_type_get,
                    iscsi_parm_session_type_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_error_recovery_level, "error_recovery_level", 
                    NULL, &node_iscsi_session_type, 
                    iscsi_parm_error_recovery_level_get,
                    iscsi_parm_error_recovery_level_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_data_sequence_in_order, 
                    "data_sequence_in_order",
                    NULL, &node_iscsi_error_recovery_level,
                    iscsi_parm_data_sequence_in_order_get,
                    iscsi_parm_data_sequence_in_order_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_data_pdu_in_order, "data_pdu_in_order",
                    NULL, &node_iscsi_data_sequence_in_order,
                    iscsi_parm_data_pdu_in_order_get,
                    iscsi_parm_data_pdu_in_order_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_outstanding_r2t, "max_outstanding_r2t",
                    NULL, &node_iscsi_data_pdu_in_order,
                    iscsi_parm_max_outstanding_r2t_get,
                    iscsi_parm_max_outstanding_r2t_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_default_time2retain, "default_time2retain",
                    NULL, &node_iscsi_max_outstanding_r2t,
                    iscsi_parm_default_time2retain_get,
                    iscsi_parm_default_time2retain_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_default_time2wait, "default_time2wait",
                    NULL, &node_iscsi_default_time2retain,
                    iscsi_parm_default_time2wait_get,
                    iscsi_parm_default_time2wait_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_burst_length, "max_burst_length",
                    NULL, &node_iscsi_default_time2wait,
                    iscsi_parm_max_burst_length_get,
                    iscsi_parm_max_burst_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_first_burst_length, "first_burst_length",
                    NULL, &node_iscsi_max_burst_length,
                    iscsi_parm_first_burst_length_get,
                    iscsi_parm_first_burst_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_recv_data_segment_length,
                    "max_recv_data_segment_length",
                    NULL, &node_iscsi_first_burst_length,
                    iscsi_parm_max_recv_data_segment_length_get,
                    iscsi_parm_max_recv_data_segment_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_immediate_data, "immediate_data",
                    NULL, &node_iscsi_max_recv_data_segment_length,
                    iscsi_parm_immediate_data_get,
                    iscsi_parm_immediate_data_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_data_digest, "data_digest",
                    NULL, &node_iscsi_immediate_data,
                    iscsi_parm_data_digest_get,
                    iscsi_parm_data_digest_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_header_digest, "header_digest",
                    NULL, &node_iscsi_data_digest,
                    iscsi_parm_header_digest_get,
                    iscsi_parm_header_digest_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initial_r2t, "initial_r2t",
                    NULL, &node_iscsi_header_digest,
                    iscsi_parm_initial_r2t_get,
                    iscsi_parm_initial_r2t_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_max_connections, "max_connections",
                    NULL, &node_iscsi_initial_r2t,
                    iscsi_parm_max_connections_get,
                    iscsi_parm_max_connections_set);

/* CHAP related stuff */
RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_local_name, "local_name",
                   NULL, NULL,
                   iscsi_parm_local_name_get,
                   iscsi_parm_local_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_peer_secret, "peer_secret",
                    NULL, &node_iscsi_initiator_local_name,
                    iscsi_parm_peer_secret_get,
                    iscsi_parm_peer_secret_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_auth, "target_auth",
                    NULL,
                    &node_iscsi_initiator_peer_secret, 
                    iscsi_parm_target_auth_get,
                    iscsi_parm_target_auth_set);                   

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_enc_fmt, "enc_fmt",
                    NULL, &node_iscsi_target_auth,
                    iscsi_parm_enc_fmt_get,
                    iscsi_parm_enc_fmt_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_challenge_length, "challenge_length",
                    NULL, &node_iscsi_initiator_enc_fmt,
                    iscsi_parm_challenge_length_get,
                    iscsi_parm_challenge_length_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_peer_name, "peer_name",
                    NULL, &node_iscsi_initiator_challenge_length,
                    iscsi_parm_peer_name_get,
                    iscsi_parm_peer_name_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_initiator_local_secret, "local_secret",
                    NULL, &node_iscsi_initiator_peer_name,
                    iscsi_parm_local_secret_get,
                    iscsi_parm_local_secret_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_chap, "chap", 
                    &node_iscsi_initiator_local_secret,
                    &node_iscsi_max_connections, 
                    iscsi_parm_chap_get,
                    iscsi_parm_chap_set);

RCF_PCH_CFG_NODE_COLLECTION(node_iscsi_conn, "conn", 
                            &node_iscsi_chap, &node_iscsi_target_name,
                            iscsi_conn_add, iscsi_conn_del, iscsi_conn_list,
                            NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_iscsi_target_data, "target_data",
                            &node_iscsi_conn, &node_iscsi_host_bus_adapter, 
                            iscsi_target_data_add, iscsi_target_data_del,
                            iscsi_target_data_list, NULL);

/* Main object */
RCF_PCH_CFG_NODE_NA(node_ds_iscsi_initiator, "iscsi_initiator", 
                    &node_iscsi_target_data, NULL);

/**
 * Function to register the /agent/iscsi_initiator
 * object in the agent's tree.
 */
te_errno
iscsi_initiator_conf_init(void)
{
    te_errno rc;

    iscsi_init_default_ini_parameters();

#ifdef __CYGWIN__
    if (iscsi_win32_init_regexps() != 0)
    {
        ERROR("Unable to compile regexps");
        return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
    }

#endif

    rc = rcf_pch_add_node("/agent", &node_ds_iscsi_initiator);
    if (rc != 0)
    {
        ERROR("Unable to add /agent/iscsi_initiator tree: %r", rc);
    }
    return rc;
}

