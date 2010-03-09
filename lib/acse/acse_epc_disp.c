/** @file
 * @brief ACSE EPC Dispathcer
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_SYS_TYPE_H
#include <sys/type.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#else
#error <sys/socket.h> is definitely needed for acse_epc.c
#endif

#if HAVE_SYS_UN_H
#include <sys/un.h>
#else
#error <sys/un.h> is definitely needed for acse_epc.c
#endif

#include <string.h>
#include <ctype.h>

#include "acse_internal.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"


/**
 * Avoid warning when freeing pointers to const data
 *
 * @param p             Pointer to const data
 */
static void
free_const(void const *p)
{
 free((void *)p);
}

/**
 * Get the session 'hold_requests' flag.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
session_hold_requests_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", cpe_inst->hold_requests);
    return 0;
}

/**
 * Set the session 'hold_requests' flag.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
session_hold_requests_set(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    cpe_inst->hold_requests = atoi(params->value);
    return 0;
}

/**
 * Get the session 'enabled' flag.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
session_enabled_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", cpe_inst->enabled);
    return 0;
}

/**
 * Set the session 'enabled' flag.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
session_enabled_set(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    cpe_inst->enabled = atoi(params->value);
    return 0;
}


/**
 * Get the session state.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
session_state_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (cpe_inst->session == NULL)
        sprintf(params->value, "0");
    else
        sprintf(params->value, "%i", cpe_inst->session->state);
    return 0;
}

/**
 * Get the device ID serial nuber.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
device_id_serial_number_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.SerialNumber);
    return 0;
}

/**
 * Get the device ID product class.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
device_id_product_class_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.ProductClass);
    return 0;
}

/**
 * Get the device ID organizational unique ID.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
device_id_oui_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.OUI);
    return 0;
}

/**
 * Get the device ID manufacturer.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
device_id_manufacturer_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.Manufacturer);
    return 0;
}


/**
 * Get the cpe certificate.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
cpe_cert_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->cert);
    return 0;
}

/**
 * Set the cpe certificate.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
cpe_cert_set(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(cpe_inst->cert);

    if ((cpe_inst->cert = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get the cpe url.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
cpe_url_get(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->url);
    return 0;
}

/**
 * Set the cpe url.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
cpe_url_set(acse_epc_config_data_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(cpe_inst->url);

    if ((cpe_inst->url = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}


/**
 * Add the CPE instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_add(acse_epc_config_data_t *params)
{
    cpe_t *cpe_item;
    te_errno rc;

    rc = db_add_cpe(params->acs, params->cpe);
    if (rc)
        return rc;

    cpe_item = db_find_cpe(NULL, params->acs, params->cpe);
    if (cpe_item == NULL)
        return TE_RC(TE_ACSE, TE_EFAULT);

    cpe_item->session       = NULL;
    cpe_item->enabled       = FALSE;
    cpe_item->hold_requests = FALSE;

    return 0; 
}

/**
 * Delete the CPE instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_del(acse_epc_config_data_t *params)
{
    cpe_t *cpe_item = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return db_remove_cpe(cpe_item);
}

/**
 * Get the list of CPE instances under ACS. Put result into passed 
 * config data @p params.
 *
 * @param params        Parameters object (IN/OUT)
 *
 * @return              Status code
 */
static te_errno
acs_cpe_list(acse_epc_config_data_t *params)
{
    char        *ptr = params->list;
    unsigned int len = 0;
    acs_t  *acs_item;
    cpe_t  *cpe_item;

    acs_item = db_find_acs(params->acs);

    /* Calculate the whole length (plus 1 sym for trailing ' '/'\0') */
    if (acs_item != NULL)
    {
        LIST_FOREACH(cpe_item, &acs_item->cpe_list, links)
            len += strlen(cpe_item->name) + 1;
    }

    /* If no items, reserve 1 char for the trailing '\0' */
    if (len > sizeof params->list)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* Form the list */
    if (acs_item != NULL)
    {
        LIST_FOREACH(cpe_item, &acs_item->cpe_list, links)
        {
            if ((len = strlen(cpe_item->name)) > 0)
            {
                if (cpe_item != LIST_FIRST(&acs_item->cpe_list))
                  *ptr++ = ' ';

                memcpy(ptr, cpe_item->name, len);
                ptr += len;
            }
        }
    }

    *ptr = '\0';
    return 0;
}

/**
 * Get the ACS port value.
 *
 * @param params        Parameters object (IN/OUT)
 *
 * @return              Status code
 */
static te_errno
acs_port_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", acs_inst->port);
    return 0;
}

/**
 * Set the acs port.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_port_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    acs_inst->port = atoi(params->value);
    return 0;
}

/**
 * Get the acs ssl flag.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_ssl_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", acs_inst->ssl);
    return 0;
}

/**
 * Set the acs ssl flag.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_ssl_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    acs_inst->ssl = atoi(params->value);
    return 0;
}

/**
 * Get the acs enabled flag.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_enabled_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", acs_inst->enabled);
    return 0;
}

/**
 * Set the acs enabled flag.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_enabled_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst  = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

#if 0
    int    new_value = atoi(params->value);
    /* TODO: rewrite at all, is it need? */
    prev_value = acs_inst->enabled;

    if (prev_value == 0 && new_value != 0 && acs_inst->port != 0)
    {
        if ((acs_inst->soap = soap_new2(SOAP_IO_KEEPALIVE,
                                        SOAP_IO_DEFAULT)) != NULL)
        {
            acs_inst->soap->bind_flags = SO_REUSEADDR;

            if (soap_bind(acs_inst->soap, NULL,
                          acs_inst->port, 5) != SOAP_INVALID_SOCKET)
            {
                acs_inst->enabled = new_value;
                return 0;
            }
            else
            {
                te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

                soap_end(acs_inst->soap);
                soap_free(acs_inst->soap);
                acs_inst->soap = NULL;
                return rc;
            }
        }
        else
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if (prev_value != 0 && new_value == 0 && acs_inst->soap != NULL)
    {
        free(acs_inst->soap->user);
        soap_end(acs_inst->soap);
        soap_free(acs_inst->soap);
        acs_inst->soap = NULL;
        acs_inst->enabled = new_value;
    }
#endif
    return 0;
}

/**
 * Get the acs password.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_pass_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, acs_inst->password);
    return 0;
}

/**
 * Set the acs password.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_pass_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(acs_inst->password);

    if ((acs_inst->password = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get the acs user name.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_user_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, acs_inst->username);
    return 0;
}

/**
 * Set the acs user name.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_user_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(acs_inst->username);

    if ((acs_inst->username = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get the acs certificate value.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cert_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, acs_inst->cert);
    return 0;
}

/**
 * Set the acs certificate value.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_cert_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(acs_inst->cert);

    if ((acs_inst->cert = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get the acs url value.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_url_get(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, acs_inst->url);
    return 0;
}

/**
 * Set the acs url value.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_url_set(acse_epc_config_data_t *params)
{
    acs_t *acs_inst = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(acs_inst->url);

    if ((acs_inst->url = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Add an acs instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acse_acs_add(acse_epc_config_data_t *params)
{
    acs_t    *item;
    te_errno rc;

    rc = db_add_acs(params->acs);
    if (rc)
        return rc;
    /* Now fill ACS with some significant */

    item = db_find_acs(params->acs);

    item->enabled = 0;
    item->ssl     = 0;
    item->port    = 0;

    return 0; 
}

/**
 * Delete an acs instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acse_acs_del(acse_epc_config_data_t *params)
{
    acs_t *acs_item = db_find_acs(params->acs);

    if (acs_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return db_remove_acs(acs_item);
}

/**
 * Get the list of acs instances.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acse_acs_list(acse_epc_config_data_t *params)
{
    char        *ptr = params->list;
    unsigned int len = 0;
    acs_t       *item;

    /* Calculate the whole length (plus 1 sym for trailing ' '/'\0') */
    LIST_FOREACH(item, &acs_list, links)
        len += strlen(item->name) + 1;

    if (len > sizeof params->list)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* Form the list */
    LIST_FOREACH(item, &acs_list, links)
    {
        if ((len = strlen(item->name)) > 0)
        {
            if (item != LIST_FIRST(&acs_list))
              *ptr++ = ' ';

            memcpy(ptr, item->name, len);
            ptr += len;
        }
    }

    *ptr = '\0';
    return 0;
}

static te_errno
cpe_get_rpc_methods(acse_epc_config_data_t *params)
{
#if 0
    strcpy(params->method_list.list[0], "GetRPCMethods");
    strcpy(params->method_list.list[1], "SetParameterValues");
    strcpy(params->method_list.list[2], "GetParameterValues");
    strcpy(params->method_list.list[3], "GetParameterNames");
    strcpy(params->method_list.list[4], "AddObject");
    strcpy(params->method_list.list[5], "DeleteObject");
    strcpy(params->method_list.list[6], "ScheduleInform");
    params->method_list.len = 7;
#else
    UNUSED(params);
#endif
    return 0;
}

#if 0
#undef RPC_TEST

#define RPC_TEST(_fun) \
static te_errno                                               \
_fun(acse_epc_config_data_t *params)                                  \
{                                                             \
    UNUSED(params);                                           \
    ERROR("Hi, I am " #_fun "!!!");                           \
    return 0;                                                 \
}

RPC_TEST(cpe_set_parameter_values)
RPC_TEST(cpe_get_parameter_values)
RPC_TEST(cpe_get_parameter_names)
RPC_TEST(cpe_set_parameter_attributes)
RPC_TEST(cpe_get_parameter_attributes)
RPC_TEST(cpe_add_object)
RPC_TEST(cpe_delete_object)
RPC_TEST(cpe_reboot)
RPC_TEST(cpe_download)
RPC_TEST(cpe_upload)
RPC_TEST(cpe_factory_reset)
RPC_TEST(cpe_get_queued_transfers)
RPC_TEST(cpe_get_all_queued_transfers)
RPC_TEST(cpe_schedule_inform)
RPC_TEST(cpe_set_vouchers)
RPC_TEST(cpe_get_options)


#undef RPC_TEST
#endif


/**
 * Process EPC related to local configuration: DB, etc. 
 * This function do not blocks, and fills @p cwmp_pars 
 * with immediately result of operation, if any.
 * Usually config operations may be performed without blocking, 
 * and they are performed during this call.
 *
 * @param cfg_pars     struct with EPC parameters.
 *
 * @return status code.
 */
static te_errno
acse_epc_config(acse_epc_config_data_t *cfg_pars)
{
    acs_t *acs_item = NULL;
    cpe_t *cpe_item = NULL;

    if (cfg_pars->op.level != EPC_CFG_ACS && 
        cfg_pars->op.level != EPC_CFG_CPE)
    {
        ERROR("%s(): wrong op.level %d", __FUNCTION__, cfg_pars->op.level);
        return TE_RC(TE_ACSE, TE_EINVAL);
    }

    switch (cfg_pars->op.fun) 
    { 
    case EPC_CFG_ADD:
        if (cfg_pars->op.level == EPC_CFG_ACS)
            return db_add_acs(cfg_pars->acs);

        return db_add_cpe(cfg_pars->acs, cfg_pars->cpe);

    case EPC_CFG_DEL:
        if ((acs_item = db_find_acs(cfg_pars->acs)) == NULL)
            return TE_ENOENT;
        if (cfg_pars->op.level == EPC_CFG_ACS)
            return db_remove_acs(acs_item);
        if ((cpe_item = 
                db_find_cpe(acs_item, cfg_pars->acs, cfg_pars->cpe)) 
            == NULL)
            return TE_ENOENT;
        return db_remove_cpe(cpe_item);

    case EPC_CFG_MODIFY:
    case EPC_CFG_OBTAIN:
        RING("%s():%d TODO", __FUNCTION__, __LINE__);
        /* TODO */
        break;
    case EPC_CFG_LIST:
        RING("acse_epc_config(): LIST... ");
        if (cfg_pars->op.level == EPC_CFG_ACS)
            return acse_acs_list(cfg_pars);
        return acs_cpe_list(cfg_pars);
    }

    return 0;
}

/**
 * Process EPC related to CWMP.
 * This function do not blocks, and fills @p cwmp_pars 
 * with immediately result of operation, if any.
 *
 * @param cwmp_pars     struct with EPC parameters.
 *
 * @return status code.
 */
static te_errno
acse_epc_cwmp(acse_epc_cwmp_data_t *cwmp_pars)
{
    cpe_t    *cpe;
    te_errno  rc;

    cpe = db_find_cpe(NULL, cwmp_pars->acs, cwmp_pars->cpe);
    if (cpe == NULL)
    {
        ERROR("EPC CONN_REQ fails, '%s':'%s' not found\n",
               cwmp_pars->acs, cwmp_pars->cpe);
        return TE_ENOENT;
    }

    switch(cwmp_pars->op)
    {
    case EPC_RPC_CALL:
    case EPC_RPC_CHECK:
    RING("%s():%d TODO", __FUNCTION__, __LINE__);
    /* TODO */
        break;
    case EPC_GET_INFORM:
        {
            cpe_inform_t *inform_rec;
            if (cwmp_pars->index == -1)
                inform_rec = LIST_FIRST(&(cpe->inform_list));
            else 
                LIST_FOREACH(inform_rec, &(cpe->inform_list), links)
                {
                    if (inform_rec->index == cwmp_pars->index)
                        break;
                }
            /* If not found, error */
            if (inform_rec == NULL)
            {
                cwmp_pars->index = -1;
                cwmp_pars->from_cpe.inform = NULL;
                return TE_ENOENT;
            }
            cwmp_pars->index = inform_rec->index;
            cwmp_pars->from_cpe.inform = inform_rec->inform;
        }
        break;
    case EPC_CONN_REQ:
        rc = acse_init_connection_request(cpe);
        if (rc != 0)
        {
            ERROR("CONN_REQ failed: %r", rc);
            return rc;
        }
        cwmp_pars->from_cpe.cr_state = cpe->cr_state;
        break;
    case EPC_CONN_REQ_CHECK:
        cwmp_pars->from_cpe.cr_state = cpe->cr_state;
        if (cpe->cr_state == CR_ERROR)
            cpe->cr_state = CR_NONE;
        break;
    }
    return 0;
}

#if 0
#if 1
/** Translation table for calculated goto
 * (shoud correspond to enum acse_fun_t in acse.h) */
static te_errno
    (*xlat[])(acse_epc_msg_t *) = {
        &acse_epc_db,
        &acse_epc_config,
        &acse_epc_cwmp,
        &acse_epc_test };
#else
/** Translation table for calculated goto
 * (shoud correspond to enum acse_fun_t in acse.h) */
static te_errno
    (*xlat[])(acse_epc_msg_t *) = {
        &acse_acs_add, &acse_acs_del, &acse_acs_list,
        &acs_url_get, &acs_url_set,
        &acs_cert_get, &acs_cert_set,
        &acs_user_get, &acs_user_set,
        &acs_pass_get, &acs_pass_set,
        &acs_enabled_get, &acs_enabled_set,
        &acs_ssl_get, &acs_ssl_set,
        &acs_port_get, &acs_port_set,
        &acs_cpe_add, &acs_cpe_del, &acs_cpe_list,
        &cpe_url_get, &cpe_url_set,
        &cpe_cert_get, &cpe_cert_set,
        &device_id_manufacturer_get,
        &device_id_oui_get,
        &device_id_product_class_get,
        &device_id_serial_number_get,
        &session_state_get,
        &session_enabled_get, &session_enabled_set,
        &session_hold_requests_get, &session_hold_requests_set,
        &cpe_get_rpc_methods,
        &cpe_set_parameter_values,
        &cpe_get_parameter_values,
        &cpe_get_parameter_names,
        &cpe_set_parameter_attributes,
        &cpe_get_parameter_attributes,
        &cpe_add_object,
        &cpe_delete_object,
        &cpe_reboot,
        &cpe_download,
        &cpe_upload,
        &cpe_factory_reset,
        &cpe_get_queued_transfers,
        &cpe_get_all_queued_transfers,
        &cpe_schedule_inform,
        &cpe_set_vouchers,
        &cpe_get_options,
        &rpc_test };
#endif
#endif

static te_errno
epc_before_poll(void *data, struct pollfd *pfd)
{
    UNUSED(data);
    pfd->fd = acse_epc_sock();
    pfd->revents = 0;
    if (pfd->fd > 0)
        pfd->events = POLLIN;

    return 0;
}


te_errno
epc_after_poll(void *data, struct pollfd *pfd)
{
    acse_epc_msg_t *msg;
    te_errno     rc; 

    if (!(pfd->revents & POLLIN))
        return 0;

    rc = acse_epc_recv(&msg);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) != TE_ENOTCONN)
            ERROR("%s(): failed to get EPC message %r",
                  __FUNCTION__, rc);

        return TE_RC(TE_ACSE, rc);
    }
    else if (msg == NULL || msg->data.p == NULL)
    {
        ERROR("%s(): NULL in 'msg' after 'epc_recv'", 
              __FUNCTION__);
        return TE_RC(TE_ACSE, TE_EFAIL);
    }

    switch(msg->opcode)
    {
    case EPC_CONFIG_CALL:
        msg->status = acse_epc_config(msg->data.cfg);
        msg->opcode = EPC_CONFIG_RESPONSE;
        break;

    case EPC_CWMP_CALL:
        msg->status = acse_epc_cwmp(msg->data.cwmp);
        msg->opcode = EPC_CWMP_RESPONSE;
        break;

    default:
        ERROR("%s(): unexpected msg opcode 0x%x",
                __FUNCTION__, msg->opcode);
        return TE_RC(TE_ACSE, TE_EFAIL);
    }

    /* Now send response, all data prepared in specific calls above. */
    rc = acse_epc_send(msg);
    RING("%s(): send EPC rc %r", __FUNCTION__, rc);

    free(msg->data.p);
    free(msg);

    if (rc != 0)
        ERROR("%s(): send EPC failed %r", __FUNCTION__, rc);

    return rc;
}

te_errno
epc_destroy(void *data)
{
    UNUSED(data);
    return acse_epc_close();
}

extern te_errno
acse_epc_disp_init(const char *msg_sock_name, const char *shmem_name)
{
    channel_t  *channel = malloc(sizeof(channel_t));
    te_errno rc;

    channel->data = NULL; 

    if (channel == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    if ((rc = acse_epc_open(msg_sock_name, shmem_name, ACSE_EPC_SERVER))
        != 0)
        return TE_RC(TE_ACSE, rc);

    channel->before_poll  = &epc_before_poll;
    channel->after_poll   = &epc_after_poll;
    channel->destroy      = &epc_destroy;

    acse_add_channel(channel);

    return 0;
}
