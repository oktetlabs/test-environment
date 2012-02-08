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


static char acse_epc_cfg_pipe[EPC_MAX_PATH] = {0,};


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

    rc = acse_epc_check();

    if (0 == rc)
    {
        rc = acse_conf_op(NULL, acs, NULL, NULL, EPC_CFG_LIST, &cfg_result);
        if (0 == rc && list != NULL)
            *list = strdup(cfg_result->list);

        free(cfg_result);
    }
    if (0 != rc)
    {
        acse_epc_close();
        acse_epc_cfg_pipe[0] = '\0';
        if (TE_RC_GET_ERROR(rc) != TE_ENOTCONN)
        {
           ERROR("call_list, for ACS '%s', rc %r", acs, rc);
        }
        else if (list != NULL)
            return empty_list(list);
    }

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
    te_errno rc;

    UNUSED(oid);
    UNUSED(gid);
    UNUSED(acse);

    rc = acse_epc_check();
    VERB("acse get called, pipe name '%s', check rc %r",
         acse_epc_cfg_pipe, rc);
    if (rc != 0)
    {
        acse_epc_close(); 
        acse_epc_cfg_pipe[0] = '\0';
        if (TE_RC_GET_ERROR(rc) != TE_ENOTCONN)
        {
            ERROR("check for EPC state fails %r", rc);
            return rc;
        }
    }

    strcpy(value, acse_epc_cfg_pipe);
    return 0;
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
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(acse);

    RING("Set to 'acse' node. old val %s, new val %s",
            acse_epc_cfg_pipe, value);

    if (strcmp(acse_epc_cfg_pipe, value) == 0)
        return 0;

    /* If there is already running ACSE, disconnect. */
    if (strlen(acse_epc_cfg_pipe) > 0)
    {
        rc = acse_epc_close();
        if (strlen(value) > 0)
        {
            WARN("reset ACSE pipe name when it is already connected"
                 " is dangerous, set to empty before");
        }
    }

    if (rc != 0)
    {
        ERROR("acse set failed %r", rc);
        return rc;
    }

    if (strlen(value) > 0)
    {
        rc = acse_epc_connect(value);

        if (rc != 0)
        {
            ERROR("acse set failed %r", rc);
            return rc;
        } 
        strcpy(acse_epc_cfg_pipe, value);
    }
    else
    {
        acse_epc_cfg_pipe[0] = '\0';
    }

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

RCF_PCH_CFG_NODE_RW(node_traffic_log, "traffic_log",
                    NULL, &node_session_state,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_chunk_mode, "chunk_mode",
                    NULL, &node_traffic_log,
                    &cfg_call_get, &cfg_call_set);

RCF_PCH_CFG_NODE_RW(node_sync_mode, "sync_mode",
                    NULL, &node_chunk_mode,
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

RCF_PCH_CFG_NODE_RW(node_acs_traffic_log, "traffic_log", 
                    NULL, &node_acs_port,
                    &cfg_acs_call_get, &cfg_acs_call_set);

RCF_PCH_CFG_NODE_RW(node_acs_auth_mode, "auth_mode", 
                    NULL, &node_acs_traffic_log,
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


