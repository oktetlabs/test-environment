/** @file
 * @brief Unix Test Agent
 *
 * ACS Emulator support
 *
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Conf ACSE"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <sys/wait.h>

#include <string.h>
#include <fcntl.h>
#include <poll.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logfork.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "tarpc.h"
#include "te_cwmp.h"
#include "acse_epc.h"
#include "acse_user.h"
#include "cwmp_data.h"


/** Process ID of ACSE */
static pid_t acse_pid = -1;

/** Flag setting "at exit" callback */
static te_bool need_atexit = TRUE;

/** Shared memory name */
const char *epc_mmap_area = NULL;

/** Unix socket names for ACSE, TA and RPC */
const char *epc_acse_sock = NULL;

/* Methods forward declarations */
static te_errno stop_acse(void);
static te_errno start_acse(void);

/**
 * Determines whether ACSE is started and link to it is initialized.
 *
 * @return              TRUE is ACSE is started and link to it
                        is initialized, otherwise - FALSE
 */
static inline te_bool
acse_value(void)
{
#if 0
    return (acse_pid != -1) && (acse_epc_socket() > 0);
#else
    return (acse_pid != -1);
#endif
}



/**
 * Initializes the list of instances to be empty.
 *
 * @param list          The list of instances
 * 
 * @return              Status code
 */
static te_errno
empty_list(char **list)
{
    char *l = strdup("");

    if (l == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    *list = l;
    return 0;
}



/**
 * Perform ACSE Config Get operation via EPC interface.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier
 * @param value         The value of the instance to get (OUT)
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cfg_call_get(unsigned int gid, const char *oid, char *value,
             const char *acse, const char *acs, const char *cpe)
{
    te_errno rc;

    acse_epc_config_data_t *cfg_result = NULL;

    UNUSED(gid);
    UNUSED(acse);

    rc = acse_conf_op(oid, acs, cpe, NULL, EPC_CFG_OBTAIN, &cfg_result);

    if (TE_ENOTCONN == rc)
    { /* There is no connection with ACSE, cfg_result was not allocated. */
        value[0] = '\0'; /* return empty string */
        return 0;
    }

    if (0 != rc)
        WARN("ACSE config EPC failed %r", rc);

    if (0 == rc)
        strcpy(value, cfg_result->value);

    free(cfg_result);

    return rc;
}

static te_errno
cfg_acs_call_get(unsigned int gid, const char *oid, char *value,
                 const char *acse, const char *acs)
{
    return cfg_call_get(gid, oid, value, acse, acs, NULL);
}


/**
 * Perform ACSE Config Get operation via EPC interface.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier
 * @param value         The value of the instance to set 
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cfg_call_set(unsigned int gid, const char *oid, char *value,
             const char *acse, const char *acs, const char *cpe)
{
    te_errno rc;

    acse_epc_config_data_t *cfg_result = NULL;

    UNUSED(gid);
    UNUSED(acse);

    rc = acse_conf_op(oid, acs, cpe, value, EPC_CFG_MODIFY, &cfg_result);
    if (rc != 0)
    {
        WARN("ACSE config EPC failed %r", rc);
        return rc;
    }

    free(cfg_result);

    return 0;
}

static te_errno
cfg_acs_call_set(unsigned int gid, const char *oid, char *value,
                 const char *acse, const char *acs)
{
    return cfg_call_set(gid, oid, value, acse, acs, NULL);
}

/**
 * Get list of ACS ojects or CPE records from ACSE.
 *
 * @param list          The list of acses/cpes
 * @param acs           Name of the acs instance or NULL
 *
 * @return              Status code
 */
static te_errno
call_list(char **list, char const *acs)
{
    te_errno rc;
    acse_epc_config_data_t  *cfg_result = NULL;

    if (!acse_value())
        return empty_list(list);

    rc = acse_conf_op(NULL, acs, NULL, NULL, EPC_CFG_LIST, &cfg_result);
    if (0 == rc && list != NULL)
        *list = strdup(cfg_result->list);

    return rc;
}


/**
 * Add the CPE record on ACSE
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier
 * @param value         The value of acs cpe being added
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
acs_cpe_add(unsigned int gid, char const *oid,
            char const *value, char const *acse, char const *acs,
            char const *cpe)
{
    te_errno                 rc; 
    acse_epc_config_data_t  *cfg_result = NULL;

    UNUSED(gid);
    UNUSED(acse);

    rc = acse_conf_op(oid, acs, cpe, value, EPC_CFG_ADD, &cfg_result);

    free(cfg_result);

    return rc;
}

/**
 * Delete the acs cpe instance.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
acs_cpe_del(unsigned int gid, char const *oid,
            char const *acse, char const *acs, char const *cpe)
{
    te_errno rc; 
    acse_epc_config_data_t *cfg_result = NULL;

    UNUSED(gid);
    UNUSED(acse);

    rc = acse_conf_op(oid, acs, cpe, NULL, EPC_CFG_DEL, &cfg_result);
    free(cfg_result);

    return rc;
}

/**
 * Get the list of acs cpe instances.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param list          The list of acs cpe instances
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return              Status code
 */
static te_errno
acs_cpe_list(unsigned int gid, char const *oid,
             char **list, char const *acse, char const *acs)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(acse);

    return call_list(list, acs);
}


/**
 * Add an acs instance.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of an acs instance being added (unused)
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return              Status code
 */
static te_errno
acse_acs_add(unsigned int gid, char const *oid,
             char const *value, char const *acse, char const *acs)
{
    te_errno                 rc; 
    acse_epc_config_data_t  *cfg_result = NULL;

    UNUSED(gid);
    UNUSED(acse);

    rc = acse_conf_op(oid, acs, NULL, value, EPC_CFG_ADD, &cfg_result);

    free(cfg_result);

    return rc;
}

/**
 * Delete an acs instance.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param acse          Name of the acse instance (unused)
 *
 * @return              Status code
 */
static te_errno
acse_acs_del(unsigned int gid, char const *oid,
             char const *acse, char const *acs)
{
    te_errno                 rc; 
    acse_epc_config_data_t  *cfg_result = NULL;

    UNUSED(gid);
    UNUSED(acse);

    rc = acse_conf_op(oid, acs, NULL, NULL, EPC_CFG_DEL, &cfg_result);

    free(cfg_result);

    return rc;
}

/**
 * Get the list of acs instances.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param list          The list of acs instances
 * @param acse          Name of the acse instance (unused)
 *
 * @return              Status code
 */
static te_errno
acse_acs_list(unsigned int gid, char const *oid,
              char **list, char const *acse)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(acse);

    return call_list(list, NULL);
}

/**
 * Get the unique_id value.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of unique_id
 * @param acse          Name of the acse instance (unused)
 *
 * @return              Status code
 */
static te_errno
acse_unique_id_get(unsigned int gid, char const *oid,
                   char *value, char const *acse)
{
    static unsigned int acse_unique_id = 0;

    UNUSED(oid);
    UNUSED(gid);
    UNUSED(acse);

    sprintf(value, "%u", acse_unique_id++);
    return 0;
}

/**
 * Get the acse instance value (whether ASE is started up/shut down).
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the acse object instance
 * @param acse          Name of the acse instance (unused)
 *
 * @return              Status code
 */
static te_errno
acse_get(unsigned int gid, char const *oid,
         char *value, char const *acse)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(acse);

    strcpy(value, acse_value() ? "1" : "0");
    return 0;
}



/**
 * Initialize necessary entities and start ACSE.
 *
 * @return              Status
 */
static te_errno
start_acse(void)
{
    te_errno rc = 0;

    RING("Start ACSE process");

    acse_pid = fork();
    if (acse_pid == 0) /* we are in child */
    {
        rcf_pch_detach();
        freopen("/tmp/acse_stderr", "a", stderr);
        setpgid(0, 0);
        logfork_register_user("ACSE");
        if ((rc = acse_epc_disp_init(NULL, NULL)) != 0)
        {
            ERROR("Fail create EPC dispatcher %r", rc);
            exit(1);
        }
        acse_loop();
        RING("Exit from ACSE process.");
        exit(0); 
    }
    if (acse_pid == -1)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s: fork() failed: %r", __FUNCTION__, rc);
        return rc;
    }
    sleep(1);

    RING("ACSE process was started with PID %d", (int)acse_pid);
    if ((rc = acse_epc_open(NULL, NULL, ACSE_EPC_CLIENT))
        != 0)
    {
        int res;
        ERROR("open EPC failed %r", rc);

#if 1
        kill(acse_pid, SIGTERM);
        waitpid(acse_pid, &res, 0);
#endif
        acse_pid = -1;
        return rc;
    }

    if (need_atexit)
    {
        atexit(stop_acse);
        need_atexit = FALSE; /* To register stop_acse only once */
    } 

    return rc;
}

/**
 * Stop ACSE and clean up previously initialized entities.
 *
 * @return              Status
 */
static te_errno
stop_acse(void)
{
    te_errno rc = 0;
    int acse_status = 0;
    int r = 0;

#if 0
    fprintf(stderr, "Stop ACSE process, pid %d\n", acse_pid);
#endif
    if (-1 == acse_pid || 0 == acse_pid)
        return 0; /* nothing to do */

    RING("Stop ACSE process, pid %d", acse_pid);

    if ((rc = acse_epc_close()) != 0)
    {
        ERROR("Stop ACSE, EPC close failed %r, now try to kill", rc);
        if (kill(acse_pid, SIGTERM))
        {
            int saved_errno = errno;
            ERROR("ACSE kill failed %s", strerror(saved_errno));
            /* failed to stop ACSE, just return ... */
            return TE_OS_RC(TE_TA_UNIX, saved_errno);
        }
    }
    sleep(1); /* Time to stop ACSE itself */

    r = waitpid(acse_pid, &acse_status, WNOHANG);
    RING("waitpid rc %d, errno %s", r, strerror(errno));
    if (r != acse_pid)
    {
        int saved_errno = errno;
        if (r < 0)
        {
            if (saved_errno != ESRCH)
            {
                ERROR("waitpid ACSE failed %s", strerror(saved_errno));
                /* failed to stop ACSE, just return ... */
                acse_pid = -1;
                return TE_OS_RC(TE_TA_UNIX, saved_errno);
            }
        }
        if (r == 0)
        {
            /* ACSE was not stopped after EPC closed, terminate it.*/
            if (kill(acse_pid, SIGTERM))
            {
                int saved_errno = errno;
                ERROR("ACSE kill after waitpid failed %s",
                      strerror(saved_errno));
                /* failed to stop ACSE, just return ... */
                acse_pid = -1;
                return TE_OS_RC(TE_TA_UNIX, saved_errno);
            }
        }
    }

    acse_pid = -1;

    if (acse_status)
        WARN("ACSE exit status %d", acse_status);

    return rc;
}

/**
 * Set the acse instance value (startup/shutdown ACSE).
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the acse object instance
 * @param acse          Name of the acse instance (unused)
 *
 * @return      Status code.
 */
static te_errno
acse_set(unsigned int gid, char const *oid,
         char const *value, char const *acse)
{
    te_bool new_acse_value = !!atoi(value);
    te_bool old_acse_value = acse_value();

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(acse);

    RING("Set to 'acse' node. old val %d, new val %d",
            old_acse_value, new_acse_value);
    if (new_acse_value && !old_acse_value) 
        return start_acse();
    if (!new_acse_value && old_acse_value)
        return stop_acse();

    return 0;
}

RCF_PCH_CFG_NODE_RO(node_cpe_connreq_status, "cr_state",
                    NULL, NULL,
                    &cfg_call_get);

RCF_PCH_CFG_NODE_RW(node_session_hold_requests, "hold_requests",
                    NULL, &node_cpe_connreq_status,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_session_enabled, "enabled",
                    NULL, &node_session_hold_requests,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RO(node_session_state, "cwmp_state",
                    NULL, &node_session_enabled,
                    &cfg_call_get);

RCF_PCH_CFG_NODE_RW(node_sync_mode, "sync_mode",
                    NULL, &node_session_state,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RO(node_device_id_serial_number, "serial_number",
                    NULL, NULL,
                    &cfg_call_get);

RCF_PCH_CFG_NODE_RO(node_device_id_product_class, "product_class",
                    NULL, &node_device_id_serial_number,
                    &cfg_call_get);

RCF_PCH_CFG_NODE_RO(node_device_id_oui, "oui",
                    NULL, &node_device_id_product_class,
                    &cfg_call_get);

RCF_PCH_CFG_NODE_RO(node_device_id_manufacturer, "manufacturer",
                    NULL, &node_device_id_oui,
                    &cfg_call_get);

RCF_PCH_CFG_NODE_NA(node_cpe_device_id, "device_id", 
                    &node_device_id_manufacturer, &node_sync_mode);

RCF_PCH_CFG_NODE_RW(node_cpe_passwd, "passwd", 
                    NULL, &node_cpe_device_id,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_cpe_login, "login", 
                    NULL, &node_cpe_passwd,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_cpe_cr_passwd, "cr_passwd", 
                    NULL, &node_cpe_login,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_cpe_cr_login, "cr_login", 
                    NULL, &node_cpe_cr_passwd,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_cpe_cert, "cert", 
                    NULL, &node_cpe_cr_login,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_cpe_url, "cr_url", 
                    NULL, &node_cpe_cert,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_COLLECTION(node_acs_cpe, "cpe",
                            &node_cpe_url, NULL,
                            &acs_cpe_add, &acs_cpe_del,
                            &acs_cpe_list, NULL);




RCF_PCH_CFG_NODE_RW(node_acs_cert, "ssl_cert", 
                    NULL, &node_acs_cpe,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_ssl, "ssl", 
                    NULL, &node_acs_cert,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_port, "port", 
                    NULL, &node_acs_ssl,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_auth_mode, "auth_mode", 
                    NULL, &node_acs_port,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_http_root, "http_root", 
                    NULL, &node_acs_auth_mode,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_http_response, "http_response", 
                    NULL, &node_acs_http_root,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_url, "url", 
                    NULL, &node_acs_http_response,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_enabled, "enabled", 
                    NULL, &node_acs_url,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_COLLECTION(node_acse_acs, "acs",
                            &node_acs_enabled, NULL,
                            &acse_acs_add, &acse_acs_del,
                            &acse_acs_list, NULL);

RCF_PCH_CFG_NODE_RO(node_acse_unique_id, "unique_id", 
                    NULL, &node_acse_acs,
                    &acse_unique_id_get);

RCF_PCH_CFG_NODE_RW(node_acse, "acse", 
                    &node_acse_unique_id, NULL,
                    &acse_get, &acse_set);

/**
 * Initializes ta_unix_conf_acse support.
 *
 * @return Status code (see te_errno.h)
 */
te_errno
ta_unix_conf_acse_init(void)
{
    return rcf_pch_add_node("/agent", &node_acse);
}

#if 0
static te_errno
cwmp_conn_req_util(const char *acs, const char *cpe, 
                   acse_epc_cwmp_op_t op, int *request_id)
{
    acse_epc_msg_t msg;
    acse_epc_msg_t msg_resp;
    acse_epc_cwmp_data_t c_data;

    te_errno rc;

    if (NULL == acs || NULL == cpe || NULL == request_id)
        return TE_EINVAL;

    if (!acse_value())
    { 
        return TE_EFAIL;
    }

    RING("Issue CWMP Connection Request to %s/%s, op %d ", 
         acs, cpe, op);

    msg.opcode = EPC_CWMP_CALL;
    msg.data.cwmp = &c_data;
    msg.length = sizeof(c_data);

    memset(&c_data, 0, sizeof(c_data));

    c_data.op = op;
        
    strcpy(c_data.acs, acs);
    strcpy(c_data.cpe, cpe);
        
    rc = acse_epc_send(&msg);
    if (rc != 0)
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);

    rc = acse_epc_recv(&msg_resp);
    if (rc != 0)
        ERROR("%s(): EPC recv failed %r", __FUNCTION__, rc);

    out->status = msg_resp.status;

    return 0;
}
#endif

int
cwmp_conn_req(tarpc_cwmp_conn_req_in *in,
              tarpc_cwmp_conn_req_out *out)
{
    te_errno rc;

    if (!acse_value())
    {
        RING("%s(): no ACSE started, pid %d", __FUNCTION__, (int)acse_pid);
        return -1;
    }


    INFO("Issue CWMP Connection Request to %s/%s ", 
         in->acs_name, in->cpe_name);

    rc = acse_cwmp_connreq(in->acs_name, in->cpe_name, NULL);
    if (rc)
        RING("issue CWMP ConnReq failed %r", rc);

    out->status = rc;

    return 0;
}

int
cwmp_op_call(tarpc_cwmp_op_call_in *in,
             tarpc_cwmp_op_call_out *out)
{
    te_errno rc, status;
    acse_epc_cwmp_data_t *cwmp_data = NULL;

    if (!acse_value())
        return -1;

    INFO("cwmp RPC %s to %s/%s called", 
         cwmp_rpc_cpe_string(in->cwmp_rpc), in->acs_name, in->cpe_name);

    rc = acse_cwmp_prepare(in->acs_name, in->cpe_name,
                           EPC_RPC_CALL, &cwmp_data);
    cwmp_data->rpc_cpe = in->cwmp_rpc; 

    if (in->buf.buf_len > 0)
    {
        rc = epc_unpack_call_data(in->buf.buf_val, in->buf.buf_len,
                                    cwmp_data);
        if (rc != 0)
        {
            ERROR("%s(): unpack cwmp data failed %r", __FUNCTION__, rc);
            out->status = rc;
            return 0;
        }
    }

    rc = acse_cwmp_call(&status, NULL, &cwmp_data);
    if (0 != rc)
    {
        ERROR("%s(): ACSE call failed %r", __FUNCTION__, rc);
        out->status = TE_RC(TE_TA_ACSE, rc);
    }
    else
    { 
        out->request_id = cwmp_data->request_id;
        out->status = TE_RC(TE_ACSE, status);
    }

    return 0;
}


int
cwmp_op_check(tarpc_cwmp_op_check_in *in,
              tarpc_cwmp_op_check_out *out)
{
    te_errno rc, status;
    acse_epc_cwmp_data_t *cwmp_data = NULL;
    size_t d_len;

    if (!acse_value())
        return -1;

    INFO("cwmp check operation No %d (rpc %s) to %s/%s called ", 
         (int)in->request_id, 
         cwmp_rpc_cpe_string(in->cwmp_rpc), in->acs_name, in->cpe_name);

    rc = acse_cwmp_prepare(in->acs_name, in->cpe_name,
                           EPC_RPC_CHECK, &cwmp_data);
    cwmp_data->request_id = in->request_id;

    if (in->cwmp_rpc != CWMP_RPC_ACS_NONE)
        cwmp_data->rpc_acs = in->cwmp_rpc;

    rc = acse_cwmp_call(&status, &d_len, &cwmp_data);
    if (rc != 0)
    {
        ERROR("%s(): EPC recv failed %r", __FUNCTION__, rc);
        out->status = TE_RC(TE_TA_ACSE, rc);
    }
    else
    {
        ssize_t packed_len;

        out->status = TE_RC(TE_ACSE, status);
        INFO("%s(): status is %r", __FUNCTION__, status);

        if (0 == status || TE_CWMP_FAULT == status)
        { 
            out->buf.buf_val = malloc(d_len);
            out->buf.buf_len = d_len;
            packed_len = epc_pack_response_data(out->buf.buf_val, 
                                                d_len, cwmp_data);
#if 0 /* Debug print */
            if (TE_CWMP_FAULT == msg_resp.status)
            {
                _cwmp__Fault *f = cwmp_data->from_cpe.fault;
                RING("pass Fault %s (%s)", f->FaultCode, f->FaultString);
            }
#endif
        }
        else
        {
            out->buf.buf_val = NULL;
            out->buf.buf_len = 0;
        }
        if (cwmp_data->rpc_cpe != CWMP_RPC_NONE)
            out->cwmp_rpc = cwmp_data->rpc_cpe;
        else if (cwmp_data->rpc_acs != CWMP_RPC_ACS_NONE)
            out->cwmp_rpc = cwmp_data->rpc_acs;
    }

    return 0;
}


#if 0
int
cwmp_get_inform(tarpc_cwmp_get_inform_in *in,
                tarpc_cwmp_get_inform_out *out)
{
    return 0;
}
#endif
