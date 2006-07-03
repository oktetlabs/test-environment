/** @file
 * @brief iSCSI Initiator configuration
 *
 * Initiator-independent control functions
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

/** Initiator data */
static iscsi_initiator_data_t *init_data = NULL;

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
    conn_data->chap.need_target_auth = FALSE;
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
void
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
 *  Dummy initiator controlling function
 *
 * @param req   Connection request (unused)
 * @return      TE_ENOSYS
 */
te_errno
iscsi_initiator_dummy_set(iscsi_connection_req *req)
{
    UNUSED(req);
    return TE_RC(ISCSI_AGENT_TYPE, TE_ENOSYS);
}


/**
 * Initialize all Initiator-related structures
 */
void
iscsi_init_default_ini_parameters(void)
{
    int i;
    
    init_data = TE_ALLOC(sizeof(*init_data));

    pthread_mutex_init(&init_data->mutex, NULL);
    sem_init(&init_data->request_sem, 0, 0);

    init_data->init_type        = ISCSI_NO_INITIATOR;
    init_data->handler          = iscsi_initiator_dummy_set;
    init_data->host_bus_adapter = ISCSI_DEFAULT_HOST_BUS_ADAPTER;
    init_data->retry_timeout    = ISCSI_DEFAULT_RETRY_TIMEOUT;
    init_data->retry_attempts   = ISCSI_DEFAULT_RETRY_ATTEMPTS;
    
    for (i = 0; i < ISCSI_MAX_TARGETS_NUMBER; i++)
    {
        iscsi_init_default_tgt_parameters(&init_data->targets[i]);
        init_data->targets[i].target_id = -1;
    }
}


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
te_bool
iscsi_when_tgt_auth(iscsi_target_data_t *target_data,
                    iscsi_connection_data_t *conn_data,
                    iscsi_tgt_chap_data_t *auth_data)
{
    UNUSED(target_data);
    UNUSED(conn_data);

    return auth_data->need_target_auth;
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

    return !auth_data->need_target_auth;
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
te_errno
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
        te_usleep(init_data->retry_timeout);
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
                            conn->prepare_device_attempts == init_data->retry_attempts)
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

        /* Doing actual Initiator-specific work */
        rc = init_data->handler(current_req);

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


/**
 * Start iSCSI initiator managing thread
 *
 * @return Status code
 */
te_errno
iscsi_initiator_start_thread(void)
{
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
    return 0;
}

