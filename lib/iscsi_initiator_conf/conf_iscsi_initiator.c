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

static te_errno iscsi_post_connection_request(int target_id, int cid, 
                                              int status, te_bool urgent);

/** Initiator data */
static iscsi_initiator_data_t *init_data = NULL;

static unsigned long iscsi_retry_timeout = 500000;
static int iscsi_retry_attempts = 30;

iscsi_initiator_data_t *
iscsi_configuration(void)
{
    return init_data;
}

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
int
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
int
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
static void
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
    pthread_mutex_init(&conn_data->status_mutex, NULL);
}
/**
 * Function initalize default parameters:
 * operational parameters and security parameters.
 *
 * @param tgt_data    Structure of the target data to
 *                    initalize.
 */
static void
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

#ifndef __CYGWIN__
/**
 * Executes ta_system.
 *
 * @param cmd    format of the command followed by parameters
 *
 * @return -1 or result of the ta_system
 */
int
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
#endif


te_bool
iscsi_is_param_needed(iscsi_target_param_descr_t *param,
                      iscsi_target_data_t *tgt_data,
                      iscsi_connection_data_t *conn_data,
                      iscsi_tgt_chap_data_t *auth_data)
{
    return (param->predicate != NULL ?
            param->predicate(tgt_data, conn_data, auth_data) : 
            TRUE);
}

void
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

void
iscsi_write_to_file(void *destination, char *what)
{
    fputs(what, destination);
}

void
iscsi_put_to_buf(void *destination, char *what)
{
    strcpy(destination, what);
}

void
iscsi_append_to_buf(void *destination, char *what)
{
    strcat(destination, "\"");
    strcat(destination, what);
    strcat(destination, "\"");
}

char *
iscsi_not_none (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "None") == 0 ? '0' : '1');
    return buf;
}

char *
iscsi_bool2int (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "Yes") == 0 ? '1' : '0');
    return buf;
}

te_bool
iscsi_when_tgt_auth(iscsi_target_data_t *target_data,
                    iscsi_connection_data_t *conn_data,
                    iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return auth_data->target_auth;
}

te_bool
iscsi_when_not_tgt_auth(iscsi_target_data_t *target_data,
                        iscsi_connection_data_t *conn_data,
                        iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return !auth_data->target_auth;
}

te_bool
iscsi_when_chap(iscsi_target_data_t *target_data,
                iscsi_connection_data_t *conn_data,
                iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return strstr(auth_data->chap, "CHAP") != NULL;
}

static const char *iscsi_status_names[] = {
    "REMOVED", 
    "DOWN",         
    "ESTABLISHING", 
    "WAITING_DEVICE", 
    "UP",           
    "CLOSING",      
    "ABNORMAL",     
    "RECOVER_DOWN",
    "RECOVER_UP"
};


void
iscsi_change_conn_status(iscsi_target_data_t *target,
                         iscsi_connection_data_t *conn, 
                         iscsi_connection_status status)
{
    iscsi_connection_status old_status;
    pthread_mutex_lock(&conn->status_mutex);
    old_status = conn->status;
    conn->status = status;
    if (old_status <= ISCSI_CONNECTION_DOWN &&
        status > ISCSI_CONNECTION_DOWN)
    {
        target->number_of_open_connections++;
    }
    else if (old_status > ISCSI_CONNECTION_DOWN && 
             status <= ISCSI_CONNECTION_DOWN)
    {
        target->number_of_open_connections--;
    }
    if (status == ISCSI_CONNECTION_UP)
    {
        init_data->n_connections++;
    }
    else if (old_status == ISCSI_CONNECTION_UP && 
             status == ISCSI_CONNECTION_CLOSING)
    {
        init_data->n_connections--;
    }
    pthread_mutex_unlock(&conn->status_mutex);
    RING("Connection %d,%d: %s -> %s", 
         target->target_id, conn - target->conns,
         iscsi_status_names[old_status + 1],
         iscsi_status_names[status     + 1]);
}

static te_errno
iscsi_post_connection_request(int target_id, int cid, int status, te_bool urgent)
{
    iscsi_connection_req *req;

    RING("Posting connection status change request: %d,%d -> %s",
         target_id, cid, 
         iscsi_status_names[status + 1]);

    switch (status)
    {
        case ISCSI_CONNECTION_UP:
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            /* status is ok */
            break;
        default:
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
    }

    req = malloc(sizeof(*req));
    if (req == NULL)
        return TE_OS_RC(ISCSI_AGENT_TYPE, errno);
    req->target_id = target_id;
    req->cid       = cid;
    req->status    = status;
    req->next      = NULL;
    pthread_mutex_lock(&init_data->initiator_mutex);
    if (urgent)
    {
        req->next = init_data->request_queue_head;
        init_data->request_queue_head = req;
        if (init_data->request_queue_tail == NULL)
            init_data->request_queue_tail = req;
    }
    else
    {
        if (init_data->request_queue_tail != NULL)
            init_data->request_queue_tail->next = req;
        else
            init_data->request_queue_head = req;
        init_data->request_queue_tail = req;
    }
    pthread_mutex_unlock(&init_data->initiator_mutex);
    sem_post(&init_data->request_sem);
    return 0;
}

static te_errno
iscsi_prepare_device (iscsi_connection_data_t *conn, int target_id)
{
    char        dev_pattern[128];
    int         rc = 0;

#ifndef __CYGWIN__

    char       *nameptr;
    FILE       *hba = NULL;
    glob_t      devices;

    switch (init_data->init_type)
    {
        case L5:
            hba = popen("T=`grep -l efabiscsi "
                        "/sys/class/scsi_host/host*/proc_name` && "
                        "B=${T%/proc_name} && "
                        "echo ${B##*/host}", "r");
            if (hba == NULL)
            {
                return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
            }
            
            rc = fscanf(hba, "%d", &init_data->host_bus_adapter);
            if (rc <= 0)
            {
                return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
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
                        return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
                    case GLOB_ABORTED:
                        ERROR("Cannot read a list of host bus adapters: read error");
                        return TE_RC(ISCSI_AGENT_TYPE, TE_EIO);
                    case GLOB_NOMATCH:
                        pthread_mutex_lock(&conn->status_mutex);
                        *conn->device_name = '\0';
                        pthread_mutex_unlock(&conn->status_mutex);
                        return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
                    default:
                        ERROR("unexpected error on glob()");
                        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
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
                                return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
                            }
                            break;
                        }
                    }
                }
                if (i == devices.gl_pathc)
                {
                    globfree(&devices);
                    return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
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
                return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
            case GLOB_ABORTED:
                ERROR("Cannot read a list of devices: read error");
                return TE_RC(ISCSI_AGENT_TYPE, TE_EIO);
            case GLOB_NOMATCH:
                RING("No items for %s", dev_pattern);
                pthread_mutex_lock(&conn->status_mutex);
                *conn->device_name = '\0';
                pthread_mutex_unlock(&conn->status_mutex);
                return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
            default:
                ERROR("unexpected error on glob()");
                return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
    }

    if (devices.gl_pathc > 1)
    {
        WARN("Stale devices detected; hoping we choose the right one");
    }

    if (realpath(devices.gl_pathv[devices.gl_pathc - 1], dev_pattern) == NULL)
    {
        rc = TE_OS_RC(ISCSI_AGENT_TYPE, errno);
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
            pthread_mutex_lock(&conn->status_mutex);
            strcpy(conn->device_name, "/dev/");
            strcat(conn->device_name, nameptr + 1);
            pthread_mutex_unlock(&conn->status_mutex);
        }
        /** Now checking that the device is active */
        strcpy(dev_pattern, devices.gl_pathv[0]);
        nameptr = strrchr(dev_pattern, '/');
        strcpy(nameptr + 1, "state");
        fd = open(dev_pattern, O_RDONLY);
        if (fd < 0)
        {
            rc = TE_OS_RC(ISCSI_AGENT_TYPE, errno);
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
                pthread_mutex_lock(&conn->status_mutex);
                *conn->device_name = '\0';
                pthread_mutex_unlock(&conn->status_mutex);
                rc = TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
            }
            close(fd);
        }
    }
    globfree(&devices);
    if (ta_system_ex("blockdev --setra 0 %s", conn->device_name) != 0)
    {
        WARN("Unable to disable read-ahead on %s", conn->device_name);
    }
#else
    rc = iscsi_win32_prepare_device(conn);
#endif

    if (rc == 0)
    {
        int fd = open(conn->device_name, O_WRONLY | O_SYNC);
        
        if (fd < 0)
        {
            rc = TE_OS_RC(ISCSI_AGENT_TYPE, errno);
            if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            {
                RING("Device %s is not ready :(", conn->device_name);
                pthread_mutex_lock(&conn->status_mutex);
                *conn->device_name = '\0';
                pthread_mutex_unlock(&conn->status_mutex);
                rc = TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
            }
            else
            {
                ERROR("Cannot open a device %s: %r",
                      conn->device_name, rc);
            }
        }
        else
        {
            const char buf[512] = "testing";
            errno = 0;
            if (write(fd, buf, sizeof(buf)) != sizeof(buf))
            {
                rc = (errno == 0 ? TE_RC(ISCSI_AGENT_TYPE, TE_ENOSPC) :
                      TE_OS_RC(ISCSI_AGENT_TYPE, errno));
                ISCSI_WIN32_REPORT_ERROR();
                ERROR("Cannot write to device %s: %r",
                      conn->device_name, rc);
                close(fd);
            }
            else
            {
                if (close(fd) != 0)
                {
                    rc = TE_OS_RC(ISCSI_AGENT_TYPE, errno);
                    ERROR("Error syncing data to %s: %r",
                          conn->device_name, rc);
                }
            }
        }
    }
    return rc;
}

static void *
iscsi_initiator_timer_thread(void *arg)
{
    int i, j;

    UNUSED(arg);
    
    for (;;)
    {
        for (i = 0; i < init_data->n_targets; i++)
        {
            for (j = 0; j < MAX_CONNECTIONS_NUMBER; j++)
            {
                pthread_mutex_lock(&init_data->targets[i].conns[j].status_mutex);
                switch (init_data->targets[i].conns[j].status)
                {
                    case ISCSI_CONNECTION_ABNORMAL:
                        iscsi_post_connection_request(i, j, 
                                                      ISCSI_CONNECTION_REMOVED, TRUE);
                        break;
                    case ISCSI_CONNECTION_RECOVER_DOWN:
                        iscsi_post_connection_request(i, j, 
                                                      ISCSI_CONNECTION_DOWN, TRUE);
                        break;
                    case ISCSI_CONNECTION_WAITING_DEVICE:
                        iscsi_post_connection_request(i, j, 
                                                      ISCSI_CONNECTION_UP, TRUE);
                        break;
                    case ISCSI_CONNECTION_RECOVER_UP:
                        iscsi_post_connection_request(i, j, 
                                                      ISCSI_CONNECTION_UP, TRUE);
                        iscsi_post_connection_request(i, j, 
                                                      ISCSI_CONNECTION_DOWN, TRUE);
                        break;
                    default:
                        /* do nothing */
                        break;
                }
                pthread_mutex_unlock(&init_data->targets[i].conns[j].status_mutex);
            }
        }
        te_usleep(iscsi_retry_timeout);
    }
    return NULL;
}

static void *
iscsi_initator_conn_request_thread(void *arg)
{
    iscsi_connection_req *current_req = NULL;
    iscsi_target_data_t *target = NULL;
    iscsi_connection_data_t *conn = NULL;
    
    int old_status;
    int rc = 0;

    pthread_t timer_thread;

    UNUSED(arg);
    

    if (pthread_create(&timer_thread, NULL, 
                       iscsi_initiator_timer_thread, NULL))
    {
        ERROR("Unable to start watchdog thread");
        return NULL;
    }
    for (;;)
    {
        sem_wait(&init_data->request_sem);
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

        RING("Got connection status change request: %d,%d %s",
             current_req->target_id, current_req->cid,
             iscsi_status_names[current_req->status + 1]);
        if (current_req->cid == ISCSI_ALL_CONNECTIONS)
        {
#ifndef __CYGWIN__
            if (init_data->init_type == OPENISCSI)
            {
                iscsi_openiscsi_stop_daemon();
            }
#endif
            pthread_cancel(timer_thread);
            pthread_join(timer_thread, NULL);
            return NULL;
        }

        target = init_data->targets + current_req->target_id;
        conn   = target->conns + current_req->cid;

        old_status = conn->status;

        switch (old_status)
        {
            case ISCSI_CONNECTION_DOWN:
            case ISCSI_CONNECTION_REMOVED:
                if (current_req->status == ISCSI_CONNECTION_DOWN ||
                    current_req->status == ISCSI_CONNECTION_REMOVED)
                {
                    RING("Connection %d,%d is already down, nothing to do",
                         current_req->target_id, current_req->cid);
                    iscsi_change_conn_status(target, conn, current_req->status);
                    free(current_req);
                    continue;
                }
                break;
            case ISCSI_CONNECTION_UP:
            {
                if (current_req->status == ISCSI_CONNECTION_UP)
                {
                    WARN("Connection %d:%d is already up",
                         current_req->target_id, current_req->cid);
                    free(current_req);
                    continue;
                }
                break;
            }
            case ISCSI_CONNECTION_ABNORMAL:
            {
                if (current_req->status != ISCSI_CONNECTION_REMOVED)
                {
                    WARN("Connection %d,%d is in inconsistent state, "
                         "trying to shut down first", 
                         current_req->target_id, current_req->cid);
                    iscsi_change_conn_status(target, conn,
                                             (current_req->status == ISCSI_CONNECTION_UP ? 
                                              ISCSI_CONNECTION_RECOVER_UP :
                                              ISCSI_CONNECTION_RECOVER_DOWN));
                    free(current_req);
                    continue;
                }
                break;
            }
            case ISCSI_CONNECTION_WAITING_DEVICE:
            {
                if (current_req->status == ISCSI_CONNECTION_UP)
                {
                    int rc;
                    
                    rc = iscsi_prepare_device(conn, current_req->target_id);
                    if (rc != 0)
                    {
                        if (TE_RC_GET_ERROR(rc) != TE_EAGAIN || 
                            conn->prepare_device_attempts == iscsi_retry_attempts)
                        {
                            ERROR("Cannot prepare SCSI device for connection %d,%d: %r",
                                  current_req->target_id, 
                                  current_req->cid,
                                  rc);
                            conn->prepare_device_attempts = 0;
                            iscsi_change_conn_status(target, conn, ISCSI_CONNECTION_ABNORMAL);
                        }
                        else
                        {
                            conn->prepare_device_attempts++;
                        }
                    }
                    else
                    {
                        conn->prepare_device_attempts = 0;
                        iscsi_change_conn_status(target, conn, ISCSI_CONNECTION_UP);
                    }
                    free(current_req);
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

        iscsi_change_conn_status(target, conn, 
                                 current_req->status == ISCSI_CONNECTION_UP ? 
                                 ISCSI_CONNECTION_ESTABLISHING :
                                 ISCSI_CONNECTION_CLOSING);

        switch (init_data->init_type)
        {
#ifndef __CYGWIN__
            case UNH:
                rc = iscsi_initiator_unh_set(current_req);
                break;
            case L5:
                rc = iscsi_initiator_l5_set(current_req);
                break;
            case OPENISCSI:
                rc = iscsi_initiator_openiscsi_set(current_req);
                break;
#endif
#ifdef __CYGWIN__
            case MICROSOFT:
            case L5:
                rc = iscsi_initiator_win32_set(current_req);
                break;
#endif
            default:
                ERROR("Corrupted init_data->init_type: %d",
                      init_data->init_type);
                continue;
        }
        if (rc != 0)
        {
            ERROR("Unable to change connection %d,%d status: %r",
                  current_req->target_id, current_req->cid, rc);
            iscsi_change_conn_status(target, conn, ISCSI_CONNECTION_ABNORMAL);
        }
        else
        {
            if (current_req->status == ISCSI_CONNECTION_UP)
            {
                iscsi_change_conn_status(target, conn, 
                                         current_req->cid > 0 ? 
                                         ISCSI_CONNECTION_UP :
                                         ISCSI_CONNECTION_WAITING_DEVICE);
                if (current_req->cid == 0)
                {
                    iscsi_post_connection_request(current_req->target_id,
                                                  current_req->cid,
                                                  ISCSI_CONNECTION_UP,
                                                  TRUE);
                }
            }
            else
            {
                iscsi_change_conn_status(target, conn, current_req->status);
                pthread_mutex_lock(&conn->status_mutex);
                if (conn->status == ISCSI_CONNECTION_REMOVED)
                {
                    iscsi_init_default_connection_parameters(conn);
                }
                pthread_mutex_unlock(&conn->status_mutex);
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
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);

    strcpy(conn->chap.chap, value);

    return 0;
}

static te_errno
iscsi_initiator_chap_get(unsigned int gid, const char *oid,
                         char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);


    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", conn->chap.chap);

    return 0;
}

/* Peer Name */
static te_errno
iscsi_initiator_peer_name_set(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);


    UNUSED(gid);
    UNUSED(instance);
    
    strcpy(conn->chap.peer_name, 
           value);

    return 0;
}

static te_errno
iscsi_initiator_peer_name_get(unsigned int gid, const char *oid,
                              char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);
    
    sprintf(value, "%s", conn->chap.peer_name);

    return 0;
}

/* Challenge Length */
static te_errno
iscsi_initiator_challenge_length_set(unsigned int gid, const char *oid,
                                     char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    conn->chap.challenge_length = atoi(value);

    return 0;
}

static te_errno
iscsi_initiator_challenge_length_get(unsigned int gid, const char *oid,
                                      char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);
    
    sprintf(value, "%d", 
            conn->chap.challenge_length);

    return 0;
}

/* Encoding Format */
static te_errno
iscsi_initiator_enc_fmt_set(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    conn->chap.enc_fmt = atoi(value);

    return 0;
}

static te_errno
iscsi_initiator_enc_fmt_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", conn->chap.enc_fmt);

    return 0;
}

/* Connection structure */
static te_errno
iscsi_conn_add(unsigned int gid, const char *oid,
               char *value, const char *instance, ...)
{
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);


    UNUSED(gid);
    UNUSED(instance);
    UNUSED(value);

    IVERB("Adding connection with id=%d to target with id %d", 
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
            return TE_OS_RC(ISCSI_AGENT_TYPE, rc);
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
    
    IVERB("Deleting %s\n", oid);
    if (init_data->targets[tgt_id].target_id >= 0)
    {
        init_data->targets[tgt_id].target_id = -1;
        if (--init_data->n_targets == 0)
        {
            /* to stop the thread and possibly a service daemon */
            iscsi_post_connection_request(ISCSI_ALL_CONNECTIONS, ISCSI_ALL_CONNECTIONS, 
                                          ISCSI_CONNECTION_REMOVED, FALSE);
            pthread_join(init_data->request_thread, NULL);
        }
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
        return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
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
    iscsi_target_data_t    *target = init_data->targets + 
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

    UNUSED(gid);
    UNUSED(instance);

    pthread_mutex_lock(&conn->status_mutex);
    status = conn->status;
    if (status != ISCSI_CONNECTION_UP)
    {
        *value = '\0';
    }
    else
    {
        strcpy(value, conn->device_name);
    }
    pthread_mutex_unlock(&conn->status_mutex);
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
        return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);

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

/* Retry timeout */
static te_errno
iscsi_initiator_retry_timeout_set(unsigned int gid, const char *oid,
                                  char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    iscsi_retry_timeout = strtoul(value, NULL, 10);

    return 0;
}


static te_errno
iscsi_initiator_retry_timeout_get(unsigned int gid, const char *oid,
                                  char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%lu", iscsi_retry_timeout);

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

    iscsi_retry_attempts = strtoul(value, NULL, 10);

    return 0;
}


static te_errno
iscsi_initiator_retry_attempts_get(unsigned int gid, const char *oid,
                                  char *value, const char *instance, ...)
{

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", iscsi_retry_attempts);

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

    return iscsi_post_connection_request(tgt_id, cid, oper, FALSE);
}

static te_errno
iscsi_status_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    int status;
    iscsi_target_data_t    *target = init_data->targets +
        iscsi_get_target_id(oid);
    iscsi_connection_data_t *conn  = target->conns + iscsi_get_cid(oid);

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
iscsi_initiator_conf_init(void)
{
    /* On Init there is only one target configured on the Initiator */
    iscsi_init_default_ini_parameters(ISCSI_CONF_NO_TARGETS);

#ifdef __CYGWIN__
    if (!iscsi_win32_init_regexps())
        return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);

#endif

    return rcf_pch_add_node("/agent", &node_ds_iscsi_initiator);
}

