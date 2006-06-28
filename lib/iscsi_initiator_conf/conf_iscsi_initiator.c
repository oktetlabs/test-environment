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

/** A time slice to wait for device readiness (usecs) */
static unsigned long iscsi_retry_timeout = 500000;

/** Number of times to probe iSCSI device readiness */
static int iscsi_retry_attempts = 30;

/**
 * See description in iscsi_initiator.h
 */
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
 * Initalize operational parameters and security parameters
 * to default values
 *
 * @param conn_data    Connection parameters
 */
static void
iscsi_init_default_connection_parameters(iscsi_connection_data_t *conn_data)
{
    memset(conn_data, 0, sizeof(*conn_data));

    conn_data->status = ISCSI_CONNECTION_REMOVED;
    strcpy(conn_data->initiator_name, ISCSI_DEFAULT_INITIATOR_NAME);
    strcpy(conn_data->initiator_alias, ISCSI_DEFAULT_INITIATOR_ALIAS);
    
    conn_data->max_connections = ISCSI_DEFAULT_MAX_CONNECTIONS;
    strcpy(conn_data->initial_r2t, ISCSI_DEFAULT_INITIAL_R2T);
    strcpy(conn_data->header_digest, ISCSI_DEFAULT_HEADER_DIGEST);
    strcpy(conn_data->data_digest, ISCSI_DEFAULT_DATA_DIGEST);
    strcpy(conn_data->immediate_data, ISCSI_DEFAULT_IMMEDIATE_DATA);
    conn_data->max_recv_data_segment_length = 
        ISCSI_DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH;
    conn_data->first_burst_length = ISCSI_DEFAULT_FIRST_BURST_LENGTH;
    conn_data->max_burst_length = ISCSI_DEFAULT_MAX_BURST_LENGTH;
    conn_data->default_time2wait = ISCSI_DEFAULT_ISCSI_DEFAULT_TIME2WAIT;
    conn_data->default_time2retain = ISCSI_DEFAULT_ISCSI_DEFAULT_TIME2RETAIN;
    conn_data->max_outstanding_r2t = ISCSI_DEFAULT_MAX_OUTSTANDING_R2T;
    strcpy(conn_data->data_pdu_in_order, ISCSI_DEFAULT_DATA_PDU_IN_ORDER);
    strcpy(conn_data->data_sequence_in_order, 
           ISCSI_DEFAULT_DATA_SEQUENCE_IN_ORDER);
    conn_data->error_recovery_level = ISCSI_DEFAULT_ERROR_RECOVERY_LEVEL;
    strcpy(conn_data->session_type, ISCSI_DEFAULT_SESSION_TYPE);

    /* target's chap */
    conn_data->chap.target_auth = FALSE;
    *(conn_data->chap.peer_secret) = '\0';
    *(conn_data->chap.local_name) = '\0';
    strcpy(conn_data->chap.chap, "None");
    conn_data->chap.enc_fmt = BASE_16;
    conn_data->chap.challenge_length = ISCSI_DEFAULT_CHALLENGE_LENGTH;
    *(conn_data->chap.peer_name) = '\0';
    *(conn_data->chap.local_secret) = '\0';
    pthread_mutex_init(&conn_data->status_mutex, NULL);
}

/**
 * Initalize default parameters for all possible connections
 * of a given target.
 *
 * @param tgt_data    Structure of the target data to
 *                    initalize.
 */
static void
iscsi_init_default_tgt_parameters(iscsi_target_data_t *tgt_data)
{
    int i;
    memset(tgt_data, 0, sizeof(*tgt_data));
    
    strcpy(tgt_data->target_name, ISCSI_DEFAULT_TARGET_NAME);
    
    for (i = 0; i < ISCSI_MAX_CONNECTIONS_NUMBER; i++)
    {
        iscsi_init_default_connection_parameters(&tgt_data->conns[i]);
    }
}

static void *iscsi_initiator_conn_request_thread(void *arg);

/**
 * Initialize all Initiator-related structures
 */
static void
iscsi_init_default_ini_parameters(void)
{
    int i;
    
    init_data = TE_ALLOC(sizeof(*init_data));

    pthread_mutex_init(&init_data->mutex, NULL);
    sem_init(&init_data->request_sem, 0, 0);

    init_data->init_type = ISCSI_DEFAULT_INITIATOR_TYPE;
    init_data->host_bus_adapter = ISCSI_DEFAULT_HOST_BUS_ADAPTER;
    
    for (i = 0; i < ISCSI_MAX_TARGETS_NUMBER; i++)
    {
        iscsi_init_default_tgt_parameters(&init_data->targets[i]);
        init_data->targets[i].target_id = -1;
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
    char    cmdline[ISCSI_MAX_CMD_SIZE];
    va_list ap;

    va_start(ap, cmd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);

    VERB("iSCSI Initiator: %s\n", cmdline);
    return te_shell_cmd(cmdline, -1, NULL, NULL) > 0 ? 0 : TE_ESHCMD;
}
#endif

#ifndef __CYGWIN__
/** See description in iscsi_initiator.h */
int
iscsi_unix_cli(const char *cmd, ...)
{
    char   *cmdline;
    int     status = 0;
    va_list ap;

    if ((cmdline = (char *)TE_ALLOC(ISCSI_MAX_CMD_SIZE)) == NULL)
    {
        return TE_ENOMEM;
    }

    va_start(ap, cmd);
    vsnprintf(cmdline, ISCSI_MAX_CMD_SIZE, cmd, ap);
    va_end(ap);

    VERB("%s() command line: %s\n", __FUNCTION__, cmdline);
    status = ta_system(cmdline);
    VERB("%s(): ta_system() call returns 0x%x\n", __FUNCTION__, status);

    free(cmdline);

    return WIFEXITED(status) ? 0 : TE_ESHCMD;
}
#endif


/**
 * See description in iscsi_initiator.h
 */

/**
 * See description in iscsi_initiator.h
 */
void
iscsi_write_param(void (*outfunc)(void *, char *),
                  void *destination,
                  iscsi_target_param_descr_t *param,
                  iscsi_target_data_t *tgt_data,
                  iscsi_connection_data_t *conn_data,
                  iscsi_tgt_chap_data_t *auth_data)
{
    void *dataptr;

    switch(param->kind)
    {
        case ISCSI_FIXED_PARAM:
            if (param->formatter == NULL || param->offset != 0)
            {
                ERROR("Invalid fixed parameter description");
                return;
            }
            dataptr = NULL;
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

/**
 * See description in iscsi_initiator.h
 */
void
iscsi_write_to_file(void *destination, char *what)
{
    fputs(what, destination);
}

/**
 * See description in iscsi_initiator.h
 */
void
iscsi_put_to_buf(void *destination, char *what)
{
    strcpy(destination, what);
}

/**
 * See description in iscsi_initiator.h
 */
void
iscsi_append_to_buf(void *destination, char *what)
{
    strcat(destination, "\"");
    strcat(destination, what);
    strcat(destination, "\"");
}

/**
 * See description in iscsi_initiator.h
 */
char *
iscsi_not_none (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "None") == 0 ? '0' : '1');
    return buf;
}

/**
 * See description in iscsi_initiator.h
 */
char *
iscsi_bool2int (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "Yes") == 0 ? '1' : '0');
    return buf;
}

/**
 * See description in iscsi_initiator.h
 */
te_bool
iscsi_when_tgt_auth(iscsi_target_data_t *target_data,
                    iscsi_connection_data_t *conn_data,
                    iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return auth_data->target_auth;
}

/**
 * See description in iscsi_initiator.h
 */
te_bool
iscsi_when_not_tgt_auth(iscsi_target_data_t *target_data,
                        iscsi_connection_data_t *conn_data,
                        iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return !auth_data->target_auth;
}

/**
 * See description in iscsi_initiator.h
 */
te_bool
iscsi_when_chap(iscsi_target_data_t *target_data,
                iscsi_connection_data_t *conn_data,
                iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return strstr(auth_data->chap, "CHAP") != NULL;
}

/** Names of iSCSI connection states */
static inline const char *
iscsi_status_name(iscsi_connection_status status)
{
    static const char *names[] = {
        "REMOVED", 
        "DOWN",         
        "ESTABLISHING", 
        "WAITING_DEVICE", 
        "UP",           
        "CLOSING",      
        "ABNORMAL",     
        "RECOVER_DOWN",
        "RECOVER_UP",
        "DISCOVERING"
    };
    return names[status + 1];
}


/**
 * Changes the status of a connection. 
 * Counters for active connections are updated if
 * necessary.
 *
 * @param target        Target data
 * @param conn          Connection data
 * @param status        New status
 */
static void
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
         iscsi_status_name(old_status),
         iscsi_status_name(status));
}

/**
 * Asynchronously posts a request to change a state of a given connection.
 * The request will be handled by iscsi_initiator_conn_request_thread().
 * The only status values acceptable by this function are:
 * - ISCSI_CONNECTION_UP
 * - ISCSI_CONNECTION_DOWN
 * - ISCSI_CONNECTION_REMOVED
 *
 * @return              Status code
 * @param target_id     Target number
 * @param cid           Connection number
 * @param status        New status
 * @param urgent        If TRUE, the request will be put into the head 
 *                      of the queue, instead of the tail
 */
static te_errno
iscsi_post_connection_request(int target_id, int cid, int status, te_bool urgent)
{
    iscsi_connection_req *req;

    RING("Posting connection status change request: %d,%d -> %s",
         target_id, cid, 
         iscsi_status_name(status));

    switch (status)
    {
        case ISCSI_CONNECTION_UP:
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            /* status is ok */
            break;
        default:
            ERROR("Invalid connection status change request");
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
    }

    req = TE_ALLOC(sizeof(*req));
    if (req == NULL)
        return TE_OS_RC(ISCSI_AGENT_TYPE, errno);
    req->target_id = target_id;
    req->cid       = cid;
    req->status    = status;
    req->next      = NULL;
    pthread_mutex_lock(&init_data->mutex);
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
    pthread_mutex_unlock(&init_data->mutex);
    sem_post(&init_data->request_sem);
    return 0;
}

#ifndef __CYGWIN__
/**
 *  Builds a glob_t list of files matching @p pattern.
 *  Does appropriate logging in case of an error.
 *
 *  @return             Status code
 *  @retval TE_EGAIN    No matching files have been found
 *
 *  @param  conn        Connection data
 *  @param  pattern     glob()-style filename pattern
 *  @param  entity_name Kind of logical entity being listed
 *                      (used for logging)
 *  @param  list (OUT)  Resulting list of files
 */
static te_errno
iscsi_scan_directory(iscsi_connection_data_t *conn,
                     const char *pattern, const char *entity_name,
                     glob_t *list)
{
    int rc = 0;

    if ((rc = glob(pattern, GLOB_ERR, NULL, list)) != 0)
    {
        switch(rc)
        {
            case GLOB_NOSPACE:
                ERROR("Cannot read a list of %s: no memory", 
                      entity_name);
                return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
            case GLOB_ABORTED:
                ERROR("Cannot read a list of %s: read error",
                      entity_name);
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
    return 0;
}
                     

/**
 * Probe a SCSI device associated with a given iSCSI connection.
 * This function looks inside /sys for necessary information.
 * If the device is ready, its name is stored into @p conn->device_name,
 *
 * FIXME: there are different mechanisms of SCSI device discovery
 * for L5 and non-L5 initiators. This really should be unified.
 *
 * @return              Status code
 * @retval 0            Device is ready
 * @retval TE_EGAIN     Device is not yet ready, try again
 * 
 * @param conn          Connection data
 * @param target_id     Target ID
 */
static te_errno
iscsi_linux_prepare_device(iscsi_connection_data_t *conn, int target_id)
{
    int         rc = 0;
    char        dev_pattern[128];
    char       *nameptr;
    FILE       *hba = NULL;
    glob_t      devices;

    switch (init_data->init_type)
    {
        case ISCSI_L5:
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
            rc = iscsi_scan_directory(conn, 
                                      "/sys/bus/scsi/devices/*/vendor",
                                      "host bus adapters",
                                      &devices);
            if (rc != 0)
            {
                return rc;
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
                        continue;
                    }
                    *dev_pattern = '\0';
                    fgets(dev_pattern, sizeof(dev_pattern) - 1, hba);
                    RING("Vendor reported is %s", dev_pattern);
                    fclose(hba);
                    if (strstr(dev_pattern, "UNH") == NULL)
                        continue;
                    
                    rc = sscanf(devices.gl_pathv[i], 
                                "/sys/bus/scsi/devices/%d:", 
                                &init_data->host_bus_adapter);
                    if (rc != 1)
                    {
                        ERROR("Something strange with "
                              "/sys/bus/scsi/devices");
                        globfree(&devices);
                        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
                    }
                    break;
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
    rc = iscsi_scan_directory(conn, dev_pattern, "devices", &devices);
    if (rc != 0)
        return rc;

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
    if (rc == 0)
    {
        if (iscsi_unix_cli("blockdev --setra 0 %s", conn->device_name) != 0)
        {
            WARN("Unable to disable read-ahead on %s", conn->device_name);
        }
    }
    return rc;
}

#endif /* ! __CYGWIN__ */

/**
 * Attempts to write a sample data to a SCSI device associated with @p conn.
 *
 * @return              Status code
 * @retval TE_EAGAIN    Device is not yet ready, try again
 *
 * @param conn  Connection data
 */
static te_errno
iscsi_write_sample_to_device(iscsi_connection_data_t *conn)
{
    te_errno rc = 0;
    int      fd = open(conn->device_name, O_WRONLY | O_SYNC);
        
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
    return rc;
}

/**
 * Probe for a device readiness and obtain its name.
 * Then attempts to write to the device to notify tests that the
 * device creation has completed. 
 *
 * @return              Status code
 * @retval TE_EAGAIN    The device is not yet ready,
 *                      another attempt is to be made
 *
 * @param conn          Connection data
 * @param target_id     Target number 
 *
 * @sa iscsi_linux_prepare_device()
 * @sa iscsi_win32_prepare_device()
 * @sa iscsi_write_sample_to_device()
 */
static te_errno
iscsi_prepare_device(iscsi_connection_data_t *conn, int target_id)
{
    int         rc = 0;

#ifndef __CYGWIN__
    rc = iscsi_linux_prepare_device(conn, target_id);
#else
    UNUSED(target_id);
    rc = iscsi_win32_prepare_device(conn);
#endif

    return rc != 0 ? rc : iscsi_write_sample_to_device(conn);
}

/**
 * This thread wakes up from time to time 
 * (namely, every @p iscsi_retry_timeout usecs, and attempts:
 * - to shutdown all connections in abnormal state 
 *   (possibly bringing some them up again, if needed)
 * - to probe for SCSI devices that are not yet ready
 */
static void *
iscsi_initiator_timer_thread(void *arg)
{
    int i, j;

    UNUSED(arg);
    
    for (;;)
    {
        for (i = 0; i < init_data->n_targets; i++)
        {
            for (j = 0; j < ISCSI_MAX_CONNECTIONS_NUMBER; j++)
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

/**
 * This is the main thread for handling connection status change requests.
 * The behaviour is described by the following state machine
 * (connection requests are in parentheses):
 * 
 * REMOVED 
 * |
 * V (ISCSI_CONNECTION_DOWN)
 * DOWN
 * |
 * V (ISCSI_CONNECTION_UP)
 * ESTABLISHING -(Error)-> ABNORMAL (see below)
 * |
 * V (Login Phase successful)
 * WAITING_DEVICE  -(Error)-> ABNORMAL
 *                 -(ISCSI_CONNECTION_UP by iscsi_initiator_timer_thread)
 *                  -> WAITING_DEVICE
 * |
 * V (device is ready)
 * UP - (ISCSI_CONNECTION_UP) -> UP
 * |
 * V (ISCSI_CONNECTION_DOWN)
 * CLOSING -(Error)-> ABNORMAL
 * |
 * V (Logout is successful)
 * DOWN - (ISCSI_CONNECTION_DOWN) -> DOWN
 * |
 * V (ISCSI_CONNECTION_REMOVED)
 * REMOVED
 *
 * For discovery sessions states are a bit different:
 * ...
 * DOWN
 * |
 * V (ISCSI_CONNECTION_UP)
 * DISCOVERING -(Error) -> ABNORMAL
 * |
 * V (Discovery session complete)
 * DOWN
 * ...
 *
 * Errorneous states are as follows:
 *
 * (a) ABNORMAL
 *     |
 *     V (by iscsi_initiator_timer_thread)
 *     CLOSING - (Error) -> ABNORMAL
 *     |
 *     V 
 *     DOWN
 *     |
 *     V (ISCSI_CONNECTION_REMOVED by iscsi_initiator_timer_thread)
 *     REMOVED
 *
 * (b) ABNORMAL
 *     |
 *     V (ISCSI_CONNECTION_UP)
 *     RECOVERY_UP
 *     |
 *     V (by iscsi_initiator_timer_thread)
 *     ESTABLISHING
 *
 * (c) ABNORMAL
 *     |
 *     V (ISCSI_CONNECTION_DOWN)
 *     |
 *     V
 *     RECOVERY_DOWN
 *     |
 *     V (by iscsi_initiator_timer_thread)
 *     CLOSING
 */
static void *
iscsi_initiator_conn_request_thread(void *arg)
{
    iscsi_connection_req *current_req = NULL;
    iscsi_target_data_t *target = NULL;
    iscsi_connection_data_t *conn = NULL;
    
    int old_status;
    int rc = 0;

    UNUSED(arg);
    

    if (pthread_create(&init_data->timer_thread, NULL, 
                       iscsi_initiator_timer_thread, NULL))
    {
        ERROR("Unable to start watchdog thread");
        return NULL;
    }
    for (;;)
    {
        sem_wait(&init_data->request_sem);
        pthread_mutex_lock(&init_data->mutex);
        current_req = init_data->request_queue_head;
        if (current_req != NULL)
        {
            init_data->request_queue_head = current_req->next;
            if (init_data->request_queue_head == NULL)
                init_data->request_queue_tail = NULL;
        }
        pthread_mutex_unlock(&init_data->mutex);
        if (current_req == NULL)
            continue;

        RING("Got connection status change request: %d,%d %s",
             current_req->target_id, current_req->cid,
             iscsi_status_name(current_req->status));
        /** A request with cid == ISCSI_ALL_CONNECTIONS is sent
         *  when a test is done so that the thread could do any
         *  clean-up.
         *  Currently, the only action performed 
         *  is stopping Open-iSCSI managing daemon, if 
         *  Open iSCSI is the initiator used.
         */
        if (current_req->cid == ISCSI_ALL_CONNECTIONS)
        {
#ifndef __CYGWIN__
            if (init_data->init_type == ISCSI_OPENISCSI)
            {
                iscsi_openiscsi_stop_daemon();
            }
#endif
            free(current_req);
            continue;
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
            case ISCSI_CONNECTION_DISCOVERING:
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

        if (current_req->status == ISCSI_CONNECTION_UP)
        {
            iscsi_change_conn_status(target, conn, 
                                     strcmp(conn->session_type, "Discovery") != 0 ? 
                                     ISCSI_CONNECTION_ESTABLISHING :
                                     ISCSI_CONNECTION_DISCOVERING);
        }
        else
        {
            iscsi_change_conn_status(target, conn, 
                                     ISCSI_CONNECTION_CLOSING);
        }

        switch (init_data->init_type)
        {
#ifndef __CYGWIN__
            case ISCSI_UNH:
                rc = iscsi_initiator_unh_set(current_req);
                break;
            case ISCSI_L5:
                rc = iscsi_initiator_l5_set(current_req);
                break;
            case ISCSI_OPENISCSI:
                rc = iscsi_initiator_openiscsi_set(current_req);
                break;
#endif
#ifdef __CYGWIN__
            case ISCSI_MICROSOFT:
            case ISCSI_L5_WIN32:
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
            if (conn->status == ISCSI_CONNECTION_DISCOVERING)
            {
                iscsi_change_conn_status(target, conn, 
                                         ISCSI_CONNECTION_DOWN);
            }
            else if (current_req->status == ISCSI_CONNECTION_UP)
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
                /* no mutex protection needed here, because conn->status is
                 * never modified in other threads
                 */
                if (conn->status == ISCSI_CONNECTION_REMOVED)
                {
                    iscsi_init_default_connection_parameters(conn);
                }
            }
        }
        free(current_req);    
    }
}

/** 
 * Kill connection request handling threads
 */
static void
kill_request_thread(void)
{
    if (init_data->request_thread)
    {
        pthread_cancel(init_data->timer_thread);
        pthread_join(init_data->timer_thread, NULL);
        pthread_cancel(init_data->request_thread);
        pthread_join(init_data->request_thread, NULL);
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

    
    if (!init_data->request_thread_started ||
        pthread_kill(init_data->request_thread, 0) != 0)
    {
        int rc;
        rc = pthread_create(&init_data->request_thread, NULL, 
                            iscsi_initiator_conn_request_thread,
                            NULL);
        if (rc != 0)
        {
            ERROR("Cannot create a connection request processing thread");
            return TE_OS_RC(ISCSI_AGENT_TYPE, rc);
        }
        if (!init_data->request_thread_started)
        {
            atexit(kill_request_thread);
            init_data->request_thread_started = TRUE;
        }
    }
    
    init_data->n_targets++;
    iscsi_init_default_tgt_parameters(&init_data->targets[tgt_id]);
    init_data->targets[tgt_id].target_id = tgt_id;

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
    if (init_data->targets[tgt_id].target_id >= 0)
    {
        init_data->targets[tgt_id].target_id = -1;
        if (--init_data->n_targets == 0)
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

    UNUSED(gid);
    UNUSED(instance);

    pthread_mutex_lock(&target->conns[0].status_mutex);
    status = target->conns[0].status;
    if (status != ISCSI_CONNECTION_UP)
    {
        *value = '\0';
    }
    else
    {
        strcpy(value, target->conns[0].device_name);
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
        init_data->init_type = ISCSI_UNH;
    else if (strcmp(value, "l5") == 0)
        init_data->init_type = ISCSI_L5;
    else if (strcmp(value, "l5_win32") == 0)
        init_data->init_type = ISCSI_L5_WIN32;
    else if (strcmp(value, "microsoft") == 0)
        init_data->init_type = ISCSI_MICROSOFT;
    else if (strcmp(value, "open-iscsi") == 0)
        init_data->init_type = ISCSI_OPENISCSI;
    else
        return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);

#if 0
    if (previous_type != init_data->init_type &&
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
    static char *types[] = {"unh", "l5", 
                            "open-iscsi", "microsoft",
                            "l5_win32"};
            
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, types[init_data->init_type]);

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

RCF_PCH_CFG_NODE_RO(node_iscsi_host_bus_adapter, "host_bus_adapter", NULL, 
                    &node_iscsi_type, iscsi_host_bus_adapter_get);

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

