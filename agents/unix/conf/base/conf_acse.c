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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: conf_acse.c 45424 2007-12-18 11:01:12Z edward $
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

#include <string.h>
#include <fcntl.h>

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
#include "acse.h"

/** The ACSE instance */
static struct {
    int       acse_value;
    pid_t     pid;
    params_t *params;
    long      params_size;
    int       rfd;
    int       wfd;
} acse_inst = { .acse_value = 0, .pid = -1,
                .params = NULL, .params_size = 0,
                .rfd = -1, .wfd = -1 };

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
 * Initializes the list of instances to be empty.
 *
 * @param fun           The particular fun to call
 *
 * @return              Status code
 */
static te_errno
call_fun(acse_fun_t fun)
{
    te_errno rc;

    if (acse_inst.acse_value == 0)
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    switch (write(acse_inst.wfd, &fun, sizeof fun))
    {
        case -1:
            ERROR("Failed to call ACSE over LRPC: %s", strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        case sizeof fun:
            break;
        default:
            ERROR("Failed to call ACSE over LRPC");
            return TE_RC(TE_TA_UNIX, TE_EUNKNOWN);
    }

    switch (read(acse_inst.rfd, &rc, sizeof rc))
    {
        case -1:
            ERROR("Failed to return from ACSE call over LRPC: %s",
                  strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        case sizeof rc:
            break;
        default:
            ERROR("Failed to return from ACSE call over LRPC");
            return TE_RC(TE_TA_UNIX, TE_EUNKNOWN);
    }

    return rc;
}

/**
 * Initializes acse params substructure with the supplied parameters.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param acs_session   Name of the acs/session instance
 * @param cpe           Name of the cpe instance
 *
 * @return              Status code
 */
static te_errno
prepare_params(unsigned int gid, char const *oid,
               char const *acs_session, char const *cpe)
{
    acse_inst.params->gid = gid;

    if (strlen(oid) >= sizeof acse_inst.params->oid)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(acse_inst.params->oid, oid);

    if (acs_session != NULL)
    {
        if (strlen(acs_session) >= sizeof acse_inst.params->acs)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        strcpy(acse_inst.params->acs, acs_session);
    }
    else
        *acse_inst.params->acs = '\0';

    if (cpe != NULL)
    {
        if (strlen(cpe) >= sizeof acse_inst.params->cpe)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        strcpy(acse_inst.params->cpe, cpe);
    }
    else
        *acse_inst.params->cpe = '\0';

    return 0;
}

/**
 * Initializes the list of instances to be empty.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param value         The value of the instance to get
 * @param acs_session   Name of the acs/session instance
 * @param cpe           Name of the cpe instance
 * @param fun           The particular fun to call
 *
 * @return              Status code
 */
static te_errno
call_get(unsigned int gid, char const *oid, char *value,
         char const *acs_session, char const *cpe, acse_fun_t fun)
{
    te_errno rc = prepare_params(gid, oid, acs_session, cpe);

    if (rc != 0 || (rc = call_fun(fun)) != 0)
        return rc;

    strcpy(value, acse_inst.params->value);
    return 0;
}

/**
 * Initializes the list of instances to be empty.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param value         The value of the instance to set
 * @param acs_session   Name of the acs/session instance
 * @param cpe           Name of the cpe instance
 * @param fun           The particular fun to call
 *
 * @return              Status code
 */
static te_errno
call_set(unsigned int gid, char const *oid, char const *value,
         char const *acs_session, char const *cpe, acse_fun_t fun)
{
    te_errno rc = prepare_params(gid, oid, acs_session, cpe);

    if (rc != 0)
        return rc;

    if (strlen(value) >= sizeof acse_inst.params->value)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(acse_inst.params->value, value);
    return call_fun(fun);
}

/**
 * Initializes the list of instances to be empty.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param value         The value of the instance to set
 * @param acs_session   Name of the acs/session instance
 * @param cpe           Name of the cpe instance
 * @param fun           The particular fun to call
 *
 * @return              Status code
 */
static te_errno
call_add(unsigned int gid, char const *oid, char const *value,
         char const *acs_session, char const *cpe, acse_fun_t fun)
{
    return call_set(gid, oid, value, acs_session, cpe, fun);
}

/**
 * Initializes the list of instances to be empty.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param acs_session   Name of the acs/session instance
 * @param cpe           Name of the cpe instance
 * @param fun           The particular fun to call
 *
 * @return              Status code
 */
static te_errno
call_del(unsigned int gid, char const *oid,
         char const *acs_session, char const *cpe, acse_fun_t fun)
{
    te_errno rc = prepare_params(gid, oid, acs_session, cpe);

    if (rc != 0)
        return rc;

    return call_fun(fun);
}

/**
 * Initializes the list of instances to be empty.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param list          The list of acses/cpes/sessions
 * @param acs           Name of the acs instance
 * @param fun           The particular fun to call
 *
 * @return              Status code
 */
static te_errno
call_list(unsigned int gid, char const *oid, char **list,
          char const *acs, acse_fun_t fun)
{
    if (acse_inst.acse_value != 0)
    {
        te_errno rc = prepare_params(gid, oid, acs, NULL);

        if (rc == 0 && (rc = call_fun(fun)) == 0)
        {
            char *l = strdup(acse_inst.params->list);

            if (l == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);

            *list = l;
        }

        return rc;
    }

    return empty_list(list);
}

/**
 * Get the session 'hold_requests' flag.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the sesion 'hold_requests' flag
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return              Status code
 */
static te_errno
session_hold_requests_get(unsigned int gid, char const *oid,
                          char *value, char const *acse,
                          char const *session)
{
    UNUSED(acse);

    return call_get(gid, oid, value, session, NULL,
                    session_hold_requests_get_fun);
}

/**
 * Set the session 'hold_requests' flag.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the session 'hold_requests' flag
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return      Status code.
 */
static te_errno
session_hold_requests_set(unsigned int gid, char const *oid,
                          char const *value, char const *acse,
                          char const *session)
{
    UNUSED(acse);

    return call_set(gid, oid, value, session, NULL,
                    session_hold_requests_set_fun);
}

/**
 * Get the session 'enabled' flag.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the session 'enabled' flag
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return              Status code
 */
static te_errno
session_enabled_get(unsigned int gid, char const *oid,
                    char *value, char const *acse,
                    char const *session)
{
    UNUSED(acse);

    return call_get(gid, oid, value, session, NULL,
                    session_enabled_get_fun);
}

/**
 * Set the session 'enabled' flag.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the session 'enabled' flag
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return      Status code.
 */
static te_errno
session_enabled_set(unsigned int gid, char const *oid,
                    char const *value, char const *acse,
                    char const *session)
{
    UNUSED(acse);

    return call_set(gid, oid, value, session, NULL,
                    session_enabled_set_fun);
}

/**
 * Get the session target_state.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the session target_state
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return              Status code
 */
static te_errno
session_target_state_get(unsigned int gid, char const *oid,
                         char *value, char const *acse,
                         char const *session)
{
    UNUSED(acse);

    return call_get(gid, oid, value, session, NULL,
                    session_target_state_get_fun);
}

/**
 * Set the session target_state.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the session target_state
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return      Status code.
 */
static te_errno
session_target_state_set(unsigned int gid, char const *oid,
                         char const *value, char const *acse,
                         char const *session)
{
    UNUSED(acse);

    return call_set(gid, oid, value, session, NULL,
                    session_target_state_set_fun);
}

/**
 * Get the session state.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the session state
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return              Status code
 */
static te_errno
session_state_get(unsigned int gid, char const *oid,
                  char *value, char const *acse,
                  char const *session)
{
    UNUSED(acse);

    return call_get(gid, oid, value, session, NULL,
                    session_state_get_fun);
}

/**
 * Get the session link to a cpe.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the session link to a cpe
 * @param acse          Name of the acse instance (unused)
 * @param session       Name of the session instance
 *
 * @return              Status code
 */
static te_errno
session_link_get(unsigned int gid, char const *oid,
                 char *value, char const *acse,
                 char const *session)
{
    UNUSED(acse);

    return call_get(gid, oid, value, session, NULL,
                    session_link_get_fun);
}

/**
 * Get the list of sessions.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param list          The list of sessions
 * @param acse          Name of the acse instance (unused)
 *
 * @return              Status code
 */
static te_errno
acse_session_list(unsigned int gid, char const *oid,
                  char **list, char const *acse)
{
    UNUSED(acse);

    return call_list(gid, oid, list, NULL, acse_session_list_fun);
}

/**
 * Get the device ID serial nuber.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the device ID serial number
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
device_id_serial_number_get(unsigned int gid, char const *oid,
                            char *value, char const *acse,
                            char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe,
                    device_id_serial_number_get_fun);
}

/**
 * Get the device ID product class.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the device ID product class
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
device_id_product_class_get(unsigned int gid, char const *oid,
                            char *value, char const *acse,
                            char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe,
                    device_id_product_class_get_fun);
}

/**
 * Get the device ID organizational unique ID.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the device ID organizational unique ID
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
device_id_oui_get(unsigned int gid, char const *oid,
                  char *value, char const *acse,
                  char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe,
                    device_id_oui_get_fun);
}

/**
 * Get the device ID manufacturer.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the device ID manufacturer
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
device_id_manufacturer_get(unsigned int gid, char const *oid,
                           char *value, char const *acse,
                           char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe,
                    device_id_manufacturer_get_fun);
}

/**
 * Get the cpe password.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the cpe password
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cpe_pass_get(unsigned int gid, char const *oid,
             char *value, char const *acse,
             char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe, cpe_pass_get_fun);
}

/**
 * Set the cpe password.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the cpe password
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return      Status code.
 */
static te_errno
cpe_pass_set(unsigned int gid, char const *oid,
             char const *value, char const *acse,
             char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, cpe, cpe_pass_set_fun);
}

/**
 * Get the cpe user name.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the cpe user name
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cpe_user_get(unsigned int gid, char const *oid,
             char *value, char const *acse,
             char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe, cpe_user_get_fun);
}

/**
 * Set the cpe user name.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the cpe user name
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return      Status code.
 */
static te_errno
cpe_user_set(unsigned int gid, char const *oid,
             char const *value, char const *acse,
             char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, cpe, cpe_user_set_fun);
}

/**
 * Get the cpe certificate.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the cpe certificate
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cpe_cert_get(unsigned int gid, char const *oid,
             char *value, char const *acse,
             char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe, cpe_cert_get_fun);
}

/**
 * Set the cpe certificate.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the cpe certificate
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return      Status code.
 */
static te_errno
cpe_cert_set(unsigned int gid, char const *oid,
             char const *value, char const *acse,
             char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, cpe, cpe_cert_set_fun);
}

/**
 * Get the cpe url.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the cpe url
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cpe_url_get(unsigned int gid, char const *oid,
            char *value, char const *acse,
            char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe, cpe_url_get_fun);
}

/**
 * Set the cpe url.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the cpe url
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return      Status code.
 */
static te_errno
cpe_url_set(unsigned int gid, char const *oid,
            char const *value, char const *acse,
            char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, cpe, cpe_url_set_fun);
}

/**
 * Get the cpe IP address.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the cpe IP address
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
cpe_ip_addr_get(unsigned int gid, char const *oid,
                char *value, char const *acse,
                char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, cpe, cpe_ip_addr_get_fun);
}

/**
 * Set the cpe IP address.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the cpe IP address
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return      Status code.
 */
static te_errno
cpe_ip_addr_set(unsigned int gid, char const *oid,
                char const *value, char const *acse,
                char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, cpe, cpe_ip_addr_set_fun);
}

/**
 * Add the acs cpe instance.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of acs cpe being added
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 * @param cpe           Name of the acs cpe instance
 *
 * @return              Status code
 */
static te_errno
acs_cpe_add(unsigned int gid, char const *oid,
            char const *value, char const *acse,
            char const *acs, char const *cpe)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, cpe, acs_cpe_add_fun);
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
    UNUSED(acse);

    return call_del(gid, oid, acs, cpe, acs_cpe_del_fun);
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
    UNUSED(acse);

    return call_list(gid, oid, list, acs, acs_cpe_list_fun);
}

/**
 * Get the acs password.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the acs password
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return              Status code
 */
static te_errno
acs_pass_get(unsigned int gid, char const *oid,
             char *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, NULL, acs_pass_get_fun);
}

/**
 * Set the acs password.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the acs password
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return      Status code.
 */
static te_errno
acs_pass_set(unsigned int gid, char const *oid,
             char const *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, NULL, acs_pass_set_fun);
}

/**
 * Get the acs user name.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the acs user name
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return              Status code
 */
static te_errno
acs_user_get(unsigned int gid, char const *oid,
             char *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, NULL, acs_user_get_fun);
}

/**
 * Set the acs user name.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the acs user name
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return      Status code.
 */
static te_errno
acs_user_set(unsigned int gid, char const *oid,
             char const *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, NULL, acs_user_set_fun);
}

/**
 * Get the acs certificate value.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the acse certificate
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return              Status code
 */
static te_errno
acs_cert_get(unsigned int gid, char const *oid,
             char *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, NULL, acs_cert_get_fun);
}

/**
 * Set the acs certificate value.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the acs certificate
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return      Status code.
 */
static te_errno
acs_cert_set(unsigned int gid, char const *oid,
             char const *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, NULL, acs_cert_set_fun);
}

/**
 * Get the acs url value.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         The value of the acs url
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return              Status code
 */
static te_errno
acs_url_get(unsigned int gid, char const *oid,
            char *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_get(gid, oid, value, acs, NULL, acs_url_get_fun);
}

/**
 * Set the acs url value.
 *
 * @param gid           Group identifier (unused)
 * @param oid           Object identifier (unused)
 * @param value         New value of the acs url
 * @param acse          Name of the acse instance (unused)
 * @param acs           Name of the acs instance
 *
 * @return      Status code.
 */
static te_errno
acs_url_set(unsigned int gid, char const *oid,
            char const *value, char const *acse, char const *acs)
{
    UNUSED(acse);

    return call_set(gid, oid, value, acs, NULL, acs_url_set_fun);
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
    UNUSED(acse);

    return call_add(gid, oid, value, acs, NULL, acse_acs_add_fun);
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
    UNUSED(acse);

    return call_del(gid, oid, acs, NULL, acse_acs_del_fun);
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
    UNUSED(acse);

    return call_list(gid, oid, list, NULL, acse_acs_list_fun);
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

    sprintf(value, "%i", acse_inst.acse_value);
    return 0;
}

static te_errno
start_acse(char const *lrpc_area)
{
    te_errno rc = 0;
    int      shm_fd;
    int      fd[2][2];

    shm_unlink(lrpc_area);

    if ((shm_fd = shm_open(lrpc_area,
                           O_CREAT | O_EXCL | O_RDWR,
                           S_IRWXU | S_IRWXG | S_IRWXO)) != -1)
    {
        long size = sysconf(_SC_PAGESIZE);

        if (size != -1)
        {
            if (size > 0)
            {
                size = (sizeof *acse_inst.params / size +
                           (sizeof *acse_inst.params % size ? 1 : 0)) *
                               size;

                if (ftruncate(shm_fd, size) != -1)
                {
                    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd, 0);

                    if (p != MAP_FAILED)
                    {
                        memset(p, 0, acse_inst.params_size = size);

                        if (pipe(fd[0]) != -1)
                        {
                            if (pipe(fd[1]) != -1)
                            {
                                switch (acse_inst.pid = fork())
                                {
                                    case -1:
                                        rc = TE_OS_RC(TE_TA_UNIX, errno);
                                        ERROR("%s: fork() failed: %r",
                                              __FUNCTION__, rc);
                                        close(fd[0][0]);
                                        close(fd[0][1]);
                                        close(fd[1][0]);
                                        close(fd[1][1]);
                                        break;
                                    case 0:
                                        rcf_pch_detach();
                                        setpgid(0, 0);
                                        logfork_register_user("ACSE");
                                        close(fd[1][0]);
                                        close(fd[0][1]);
                                        close(shm_fd);
                                        acse_loop(p, fd[0][0], fd[1][1]);
                                        close(fd[0][0]);
                                        close(fd[1][1]);
                                        munmap(acse_inst.params,
                                               acse_inst.params_size);
                                        exit(0);
                                    default:
                                        acse_inst.params = p;
                                        acse_inst.rfd = fd[1][0];
                                        acse_inst.wfd = fd[0][1];
                                        close(fd[0][0]);
                                        close(fd[1][1]);
                                        break;
                                }
                            }
                            else
                                rc = TE_OS_RC(TE_TA_UNIX, errno);
                        }
                        else
                            rc = TE_OS_RC(TE_TA_UNIX, errno);
                    }
                    else
                        rc = TE_OS_RC(TE_TA_UNIX, errno);
                }
                else
                    rc = TE_OS_RC(TE_TA_UNIX, errno);
            }
            else
                rc = TE_RC(TE_TA_UNIX, TE_EUNKNOWN);
        }
        else
            rc = TE_OS_RC(TE_TA_UNIX, errno);

        close(shm_fd);
    }
    else
        rc = TE_OS_RC(TE_TA_UNIX, errno);

    return rc;
}

static te_errno
stop_acse(char const *lrpc_area)
{
    te_errno rc = 0;

    if (acse_inst.pid != -1)
    {
        if (kill(-acse_inst.pid, SIGTERM) == 0)
        {
            RING("Sent SIGTERM to the process with PID = %u",
                 acse_inst.pid);
            acse_inst.pid = -1;
        }
        else
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("Failed to send SIGTERM to the process with "
                  "PID = %u: %r", acse_inst.pid, rc);

            if (kill(-acse_inst.pid, SIGKILL) == 0)
            {
                RING("Sent SIGKILL to the process with PID = %u",
                     acse_inst.pid);
                acse_inst.pid = -1;
            }
            else
            {
                rc = TE_OS_RC(TE_TA_UNIX, errno);
                ERROR("Failed to send SIGKILL to the process with "
                      "PID = %u: %r", acse_inst.pid, rc);
            }
        }
    }

    if (rc == 0)
    {
        munmap(acse_inst.params, acse_inst.params_size);
        shm_unlink(lrpc_area);
        close(acse_inst.rfd);
        acse_inst.rfd = -1;
        close(acse_inst.wfd);
        acse_inst.wfd = -1;
    }

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
    te_errno rc             = 0;
    int      new_acse_value = atoi(value);

    static char lrpc_area[] = "/lrpc_area";

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(acse);

    if (acse_inst.acse_value != new_acse_value)
    {
        te_errno (*action)(char const *) =
            acse_inst.acse_value == 0 ? &start_acse : &stop_acse;

        if ((rc = (*action)(lrpc_area)) == 0)
            acse_inst.acse_value = new_acse_value;
    }

    return rc;
}

RCF_PCH_CFG_NODE_RW(node_session_hold_requests, "hold_requests",
                    NULL, NULL,
                    &session_hold_requests_get, &session_hold_requests_set);

RCF_PCH_CFG_NODE_RW(node_session_enabled, "enabled",
                    NULL, &node_session_hold_requests,
                    &session_enabled_get, &session_enabled_set);

RCF_PCH_CFG_NODE_RW(node_session_target_state, "target_state",
                    NULL, &node_session_enabled,
                    &session_target_state_get, &session_target_state_set);

RCF_PCH_CFG_NODE_RO(node_session_state, "state",
                    NULL, &node_session_target_state,
                    &session_state_get);

RCF_PCH_CFG_NODE_RO(node_session_link, "link",
                    NULL, &node_session_state,
                    &session_link_get);

RCF_PCH_CFG_NODE_COLLECTION(node_acse_session, "session", 
                            &node_session_link, NULL,
                            NULL, NULL,
                            &acse_session_list, NULL);

RCF_PCH_CFG_NODE_RO(node_device_id_serial_number, "serial_number",
                    NULL, NULL,
                    &device_id_serial_number_get);

RCF_PCH_CFG_NODE_RO(node_device_id_product_class, "product_class",
                    NULL, &node_device_id_serial_number,
                    &device_id_product_class_get);

RCF_PCH_CFG_NODE_RO(node_device_id_oui, "oui",
                    NULL, &node_device_id_product_class,
                    &device_id_oui_get);

RCF_PCH_CFG_NODE_RO(node_device_id_manufacturer, "manufacturer",
                    NULL, &node_device_id_oui,
                    &device_id_manufacturer_get);

RCF_PCH_CFG_NODE_NA(node_cpe_device_id, "device_id", 
                    &node_device_id_manufacturer, NULL);

RCF_PCH_CFG_NODE_RW(node_cpe_pass, "pass", 
                    NULL, &node_cpe_device_id,
                    &cpe_pass_get, &cpe_pass_set);

RCF_PCH_CFG_NODE_RW(node_cpe_user, "user", 
                    NULL, &node_cpe_pass,
                    &cpe_user_get, &cpe_user_set);

RCF_PCH_CFG_NODE_RW(node_cpe_cert, "cert", 
                    NULL, &node_cpe_user,
                    &cpe_cert_get, &cpe_cert_set);

RCF_PCH_CFG_NODE_RW(node_cpe_url, "url", 
                    NULL, &node_cpe_cert,
                    &cpe_url_get, &cpe_url_set);

RCF_PCH_CFG_NODE_RW(node_cpe_ip_addr, "ip_addr", 
                    NULL, &node_cpe_url,
                    &cpe_ip_addr_get, &cpe_ip_addr_set);

RCF_PCH_CFG_NODE_COLLECTION(node_acs_cpe, "cpe",
                            &node_cpe_ip_addr, NULL,
                            &acs_cpe_add, &acs_cpe_del,
                            &acs_cpe_list, NULL);

RCF_PCH_CFG_NODE_RW(node_acs_pass, "pass", 
                    NULL, &node_acs_cpe,
                    &acs_pass_get, &acs_pass_set);

RCF_PCH_CFG_NODE_RW(node_acs_user, "user", 
                    NULL, &node_acs_pass,
                    &acs_user_get, &acs_user_set);

RCF_PCH_CFG_NODE_RW(node_acs_cert, "cert", 
                    NULL, &node_acs_user,
                    &acs_cert_get, &acs_cert_set);

RCF_PCH_CFG_NODE_RW(node_acs_url, "url", 
                    NULL, &node_acs_cert,
                    &acs_url_get, &acs_url_set);

RCF_PCH_CFG_NODE_COLLECTION(node_acse_acs, "acs",
                            &node_acs_url, &node_acse_session,
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
ta_unix_conf_acse_init()
{
    return rcf_pch_add_node("/agent", &node_acse);
}
