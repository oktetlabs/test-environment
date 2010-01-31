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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "acse_internal.h"

/** LRPC mechanism state machine states */
typedef enum { want_read, want_write } epc_t;

/** LRPC mechanism state machine private data */
typedef struct {
    int       sock;   /**< The socket endpoint from TA to read/write    */
    struct sockaddr_un
              addr;   /**< The address of a requester to answer to      */
    socklen_t len;    /**< The length of the address of a requester     */
    acse_params_t *params;
                      /**< Parameters passed from TA over shared memory */
    te_errno  rc;     /**< Return code to be passed back to TA          */
    epc_t    state;  /**< LRPC mechanism state machine current state   */
} epc_data_t;


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
session_hold_requests_get(acse_params_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", cpe_inst->session.hold_requests);
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
session_hold_requests_set(acse_params_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    cpe_inst->session.hold_requests = atoi(params->value);
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
session_enabled_get(acse_params_t *params)
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
session_enabled_set(acse_params_t *params)
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
session_state_get(acse_params_t *params)
{
    cpe_t *cpe_inst = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", cpe_inst->session.state);
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
device_id_serial_number_get(acse_params_t *params)
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
device_id_product_class_get(acse_params_t *params)
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
device_id_oui_get(acse_params_t *params)
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
device_id_manufacturer_get(acse_params_t *params)
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
cpe_cert_get(acse_params_t *params)
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
cpe_cert_set(acse_params_t *params)
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
cpe_url_get(acse_params_t *params)
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
cpe_url_set(acse_params_t *params)
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
 * Add the acs cpe instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_add(acse_params_t *params)
{
    cpe_t *cpe_item;
    te_errno rc;

    rc = db_add_cpe(params->acs, params->cpe);
    if (rc)
        return rc;

    cpe_item = db_find_cpe(NULL, params->acs, params->cpe);
    if (cpe_item == NULL)
        return TE_RC(TE_ACSE, TE_EFAULT);

    cpe_item->session.state         = CWMP_NOP;
    cpe_item->enabled               = FALSE;
    cpe_item->session.hold_requests = FALSE;
    cpe_item->soap                  = NULL;

    return 0; 
}

/**
 * Delete the acs cpe instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_del(acse_params_t *params)
{
    cpe_t *cpe_item = db_find_cpe(NULL, params->acs, params->cpe);

    if (cpe_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return db_remove_cpe(cpe_item);
}

/**
 * Get the list of acs cpe instances.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_list(acse_params_t *params)
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
 * Get the acs port value.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_port_get(acse_params_t *params)
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
acs_port_set(acse_params_t *params)
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
acs_ssl_get(acse_params_t *params)
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
acs_ssl_set(acse_params_t *params)
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
acs_enabled_get(acse_params_t *params)
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
acs_enabled_set(acse_params_t *params)
{
    int    new_value = atoi(params->value);
    acs_t *acs_inst  = db_find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

#if 0
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
acs_pass_get(acse_params_t *params)
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
acs_pass_set(acse_params_t *params)
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
acs_user_get(acse_params_t *params)
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
acs_user_set(acse_params_t *params)
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
acs_cert_get(acse_params_t *params)
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
acs_cert_set(acse_params_t *params)
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
acs_url_get(acse_params_t *params)
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
acs_url_set(acse_params_t *params)
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
acse_acs_add(acse_params_t *params)
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
acse_acs_del(acse_params_t *params)
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
acse_acs_list(acse_params_t *params)
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
cpe_get_rpc_methods(acse_params_t *params)
{
    strcpy(params->method_list.list[0], "GetRPCMethods");
    strcpy(params->method_list.list[1], "SetParameterValues");
    strcpy(params->method_list.list[2], "GetParameterValues");
    strcpy(params->method_list.list[3], "GetParameterNames");
    strcpy(params->method_list.list[4], "AddObject");
    strcpy(params->method_list.list[5], "DeleteObject");
    strcpy(params->method_list.list[6], "ScheduleInform");
    params->method_list.len = 7;
    return 0;
}

#undef RPC_TEST

#define RPC_TEST(_fun) \
static te_errno                                               \
_fun(acse_params_t *params)                                        \
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

static te_errno
rpc_test(acse_params_t *params)
{
    UNUSED(params);
    RING("Hi, I am rpc_test!!! ");
    return 0;
}

/** Translation table for calculated goto
 * (shoud correspond to enum acse_fun_t in acse.h) */
static te_errno
    (*xlat[])(acse_params_t *) = {
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

static te_errno
epc_before_poll(void *data, struct pollfd *pfd)
{
    epc_data_t *epc = data;

    pfd->fd = epc->sock;
    pfd->revents = 0;

    switch (epc->state)
    {
        case want_read:
            pfd->events = POLLIN;
            break;
        case want_write:
            pfd->events = POLLOUT;
            break;
        default:
            pfd->events = 0;
            pfd->fd = -1;
            break;
    }

    return 0;
}

te_errno
epc_after_poll(void *data, struct pollfd *pfd)
{
    acse_fun_t   fun;
    epc_data_t *epc = data;

    switch (epc->state)
    {
        case want_read:
            if (pfd->revents & POLLIN)
            {
                epc->len = sizeof(epc->addr);

                switch (recvfrom(epc->sock, &fun, sizeof fun, 0,
                                 (struct sockaddr *)&epc->addr,
                                 &epc->len))
                {
                    case -1:
                        ERROR("Failed to get call over LRPC: %s",
                              strerror(errno));
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                    case sizeof fun:
                        epc->rc =
                          fun >= acse_fun_first && fun <= acse_fun_last ?
                            (*xlat[fun - acse_fun_first])(epc->params) :
                            TE_RC(TE_TA_UNIX, TE_ENOSYS);

                        epc->state = want_write;
                        break;
                    default:
                        ERROR("Failed to get call over LRPC");
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                }
            }

            break;
        case want_write:
            if (pfd->revents & POLLOUT)
            {
                switch (sendto(epc->sock, &epc->rc, sizeof epc->rc, 0,
                               (struct sockaddr *)&epc->addr, epc->len))
                {
                    case -1:
                        ERROR("Failed to return from call over LRPC: %s",
                              strerror(errno));
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                    case sizeof epc->rc:
                        epc->state = want_read;
                        break;
                    default:
                        ERROR("Failed to return from call over LRPC");
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                }
            }

            break;
        default:
            break;
    }

    return 0;
}

te_errno
epc_destroy(void *data)
{
    epc_data_t *epc = data;

    close(epc->sock);
    free(epc);
    return 0;
}

extern te_errno
acse_epc_create(channel_t *channel, acse_params_t *params, int sock)
{
    epc_data_t *epc = channel->data = malloc(sizeof(*epc));

    if (epc == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    epc->sock            = sock;
    epc->addr.sun_family = AF_UNIX;
    epc->len             = sizeof epc->addr;
    epc->params          = params;
    epc->rc              = 0;
    epc->state           = want_read;

    channel->before_poll  = &epc_before_poll;
    channel->after_poll   = &epc_after_poll;
    channel->destroy      = &epc_destroy;

    return 0;
}
