/** @file
 * @brief ACSE RPC Dispathcer
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
 * $Id: acse_lrpc.c 45424 2007-12-18 11:01:12Z edward $
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
#error <sys/socket.h> is definitely needed for acse_lrpc.c
#endif

#if HAVE_SYS_UN_H
#include <sys/un.h>
#else
#error <sys/un.h> is definitely needed for acse_lrpc.c
#endif

#include <string.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "acse_internal.h"

/** LRPC mechanism state machine states */
typedef enum { want_read, want_write } lrpc_t;

/** LRPC mechanism state machine private data */
typedef struct {
    int       sock;   /**< The socket endpoint from TA to read/write    */
    struct sockaddr_un
              addr;   /**< The address of a requester to answer to      */
    socklen_t len;    /**< The length of the address of a requester     */
    params_t *params; /**< Parameters passed from TA over shared memory */
    te_errno  rc;     /**< Return code to be passed back to TA          */
    lrpc_t    state;  /**< LRPC mechanism state machine current state   */
} lrpc_data_t;

/** The list af acs instances */
acs_list_t acs_list = STAILQ_HEAD_INITIALIZER(&acs_list); 

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
 * Find an acs instance from the acs list
 *
 * @param acs           Name of the acs instance
 *
 * @return              Acs instance address or NULL if not found
 */
static acs_t *
find_acs(char const *acs)
{
    acs_item_t *item;

    STAILQ_FOREACH(item, &acs_list, link)
    {
        if (strcmp(item->acs.name, acs) == 0)
            return &item->acs;
    }

    return NULL;
}

/**
 * Find a cpe instance from the cpe list of an acs instance
 *
 * @param acs           Name of the acs instance
 * @param cpe           Name of the cpe instance
 *
 * @return              Cpe instance address or NULL if not found
 */
static cpe_t *
find_cpe(char const *acs, char const *cpe)
{
    acs_item_t *acs_item;
    cpe_item_t *cpe_item;

    STAILQ_FOREACH(acs_item, &acs_list, link)
    {
        if (strcmp(acs_item->acs.name, acs) == 0)
            break;
    }

    if (acs_item != NULL)
    {
        STAILQ_FOREACH(cpe_item, &acs_item->acs.cpe_list, link)
        {
            if (strcmp(cpe_item->cpe.name, cpe) == 0)
                return &cpe_item->cpe;
        }
    }

    return NULL;
}

/**
 * Get the session 'hold_requests' flag.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
session_hold_requests_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

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
session_hold_requests_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

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
session_enabled_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", cpe_inst->session.enabled);
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
session_enabled_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    cpe_inst->session.enabled = atoi(params->value);
    return 0;
}

/**
 * Get the session target_state.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
session_target_state_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(params->value, "%i", cpe_inst->session.target_state);
    return 0;
}

/**
 * Set the session target_state.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
session_target_state_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    cpe_inst->session.target_state = atoi(params->value);
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
session_state_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

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
device_id_serial_number_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.serial_number);
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
device_id_product_class_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.product_class);
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
device_id_oui_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.oui);
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
device_id_manufacturer_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->device_id.manufacturer);
    return 0;
}

/**
 * Get the cpe password.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
cpe_pass_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->pass);
    return 0;
}

/**
 * Set the cpe password.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
cpe_pass_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(cpe_inst->pass);

    if ((cpe_inst->pass = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get the cpe user name.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
cpe_user_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->user);
    return 0;
}

/**
 * Set the cpe user name.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
cpe_user_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(cpe_inst->user);

    if ((cpe_inst->user = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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
cpe_cert_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

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
cpe_cert_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

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
cpe_url_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

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
cpe_url_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(cpe_inst->url);

    if ((cpe_inst->url = strdup(params->value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get the cpe IP address.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
cpe_ip_addr_get(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, cpe_inst->ip_addr);
    return 0;
}

/**
 * Set the cpe IP address.
 *
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
cpe_ip_addr_set(params_t *params)
{
    cpe_t *cpe_inst = find_cpe(params->acs, params->cpe);

    if (cpe_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(cpe_inst->ip_addr);

    if ((cpe_inst->ip_addr = strdup(params->value)) == NULL)
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
acs_cpe_add(params_t *params)
{
    acs_item_t *acs_item;
    cpe_item_t *cpe_item;

    /* Find the acs item */
    STAILQ_FOREACH(acs_item, &acs_list, link)
    {
        if (strcmp(acs_item->acs.name, params->acs) == 0)
            break;
    }

    if (acs_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    STAILQ_FOREACH(cpe_item, &acs_item->acs.cpe_list, link)
    {
        if (strcmp(cpe_item->cpe.name, params->cpe) == 0)
            break;
    }

    if (cpe_item != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((cpe_item = malloc(sizeof *cpe_item)) == NULL)
        goto enomem_0;

    if ((cpe_item->cpe.name = strdup(params->cpe)) == NULL)
        goto enomem_1;

    if ((cpe_item->cpe.ip_addr  = strdup("0.0.0.0")) == NULL)
        goto enomem_2;

    if ((cpe_item->cpe.url  = strdup("")) == NULL)
        goto enomem_3;

    if ((cpe_item->cpe.cert = strdup("")) == NULL)
        goto enomem_4;

    if ((cpe_item->cpe.user = strdup("")) == NULL)
        goto enomem_5;

    if ((cpe_item->cpe.pass = strdup("")) == NULL)
        goto enomem_6;

    if ((cpe_item->cpe.device_id.manufacturer = strdup("")) == NULL)
        goto enomem_7;

    if ((cpe_item->cpe.device_id.oui = strdup("")) == NULL)
        goto enomem_8;

    if ((cpe_item->cpe.device_id.product_class = strdup("")) == NULL)
        goto enomem_9;

    if ((cpe_item->cpe.device_id.serial_number = strdup("")) == NULL)
        goto enomem_A;

    cpe_item->cpe.session.state         = session_no_state;
    cpe_item->cpe.session.target_state  = session_no_state;
    cpe_item->cpe.session.enabled       = FALSE;
    cpe_item->cpe.session.hold_requests = FALSE;
    cpe_item->cpe.soap                  = NULL;

    STAILQ_INSERT_TAIL(&acs_item->acs.cpe_list, cpe_item, link);
    return 0;

enomem_A:
    free_const(cpe_item->cpe.device_id.product_class);

enomem_9:
    free_const(cpe_item->cpe.device_id.oui);

enomem_8:
    free_const(cpe_item->cpe.device_id.manufacturer);

enomem_7:
    free_const(cpe_item->cpe.pass);

enomem_6:
    free_const(cpe_item->cpe.user);

enomem_5:
    free_const(cpe_item->cpe.cert);

enomem_4:
    free_const(cpe_item->cpe.url);

enomem_3:
    free_const(cpe_item->cpe.ip_addr);

enomem_2:
    free_const(cpe_item->cpe.name);

enomem_1:
    free(cpe_item);

enomem_0:
    return TE_RC(TE_TA_UNIX, TE_ENOMEM);
}

/**
 * Delete the acs cpe instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_del(params_t *params)
{
    acs_item_t *acs_item;

    STAILQ_FOREACH(acs_item, &acs_list, link)
    {
        if (strcmp(acs_item->acs.name, params->acs) == 0)
        {
            cpe_item_t *cpe_item;

            STAILQ_FOREACH(cpe_item, &acs_item->acs.cpe_list, link)
            {
                if (strcmp(cpe_item->cpe.name, params->cpe) == 0)
                {
                    STAILQ_REMOVE(&acs_item->acs.cpe_list,
                                  cpe_item, cpe_item_t, link);
                    free_const(cpe_item->cpe.name);
                    free_const(cpe_item->cpe.url);
                    free_const(cpe_item->cpe.cert);
                    free_const(cpe_item->cpe.user);
                    free_const(cpe_item->cpe.pass);
                    free(cpe_item);

                    return 0;
                }
            }
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Get the list of acs cpe instances.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acs_cpe_list(params_t *params)
{
    char        *ptr = params->list;
    unsigned int len = 0;
    acs_item_t  *acs_item;
    cpe_item_t  *cpe_item;

    /* Find the acs item */
    STAILQ_FOREACH(acs_item, &acs_list, link)
    {
        if (strcmp(acs_item->acs.name, params->acs) == 0)
            break;
    }

    /* Calculate the whole length (plus 1 sym for trailing ' '/'\0') */
    if (acs_item != NULL)
    {
        STAILQ_FOREACH(cpe_item, &acs_item->acs.cpe_list, link)
            len += strlen(cpe_item->cpe.name) + 1;
    }

    /* If no items, reserve 1 char for the trailing '\0' */
    if (len > sizeof params->list)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* Form the list */
    if (acs_item != NULL)
    {
        STAILQ_FOREACH(cpe_item, &acs_item->acs.cpe_list, link)
        {
            if ((len = strlen(cpe_item->cpe.name)) > 0)
            {
                if (cpe_item != STAILQ_FIRST(&acs_item->acs.cpe_list))
                  *ptr++ = ' ';

                memcpy(ptr, cpe_item->cpe.name, len);
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
acs_port_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_port_set(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_ssl_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_ssl_set(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_enabled_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_enabled_set(params_t *params)
{
    int    prev_value;
    int    new_value = atoi(params->value);
    acs_t *acs_inst  = find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
acs_pass_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, acs_inst->pass);
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
acs_pass_set(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(acs_inst->pass);

    if ((acs_inst->pass = strdup(params->value)) == NULL)
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
acs_user_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(params->value, acs_inst->user);
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
acs_user_set(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

    if (acs_inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    free_const(acs_inst->user);

    if ((acs_inst->user = strdup(params->value)) == NULL)
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
acs_cert_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_cert_set(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_url_get(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acs_url_set(params_t *params)
{
    acs_t *acs_inst = find_acs(params->acs);

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
acse_acs_add(params_t *params)
{
    acs_item_t *item;

    STAILQ_FOREACH(item, &acs_list, link)
    {
        if (strcmp(item->acs.name, params->acs) == 0)
            break;
    }

    if (item != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((item = malloc(sizeof *item)) == NULL)
        goto enomem_0;

    if ((item->acs.name = strdup(params->acs)) == NULL)
        goto enomem_1;

    if ((item->acs.url  = strdup("")) == NULL)
        goto enomem_2;

    if ((item->acs.cert = strdup("")) == NULL)
        goto enomem_3;

    if ((item->acs.user = strdup("")) == NULL)
        goto enomem_4;

    if ((item->acs.pass = strdup("")) == NULL)
        goto enomem_5;

    item->acs.enabled = 0;
    item->acs.ssl     = 0;
    item->acs.port    = 0;
    item->acs.soap    = NULL;

    STAILQ_INIT(&item->acs.cpe_list);
    STAILQ_INSERT_TAIL(&acs_list, item, link);
    return 0;

enomem_5:
    free_const(item->acs.user);

enomem_4:
    free_const(item->acs.cert);

enomem_3:
    free_const(item->acs.url);

enomem_2:
    free_const(item->acs.name);

enomem_1:
    free(item);

enomem_0:
    return TE_RC(TE_TA_UNIX, TE_ENOMEM);
}

/**
 * Delete an acs instance.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acse_acs_del(params_t *params)
{
    acs_item_t *item;

    STAILQ_FOREACH(item, &acs_list, link)
    {
        if (strcmp(item->acs.name, params->acs) == 0)
        {
            if (!STAILQ_EMPTY(&item->acs.cpe_list))
                return TE_RC(TE_TA_UNIX, TE_EBUSY);

            STAILQ_REMOVE(&acs_list, item, acs_item_t, link);
            free_const(item->acs.name);
            free_const(item->acs.url);
            free_const(item->acs.cert);
            free_const(item->acs.user);
            free_const(item->acs.pass);

            if (item->acs.soap != NULL)
            {
                free(item->acs.soap->user);
                soap_end(item->acs.soap);
                soap_free(item->acs.soap);
            }

            free(item);
            return 0;
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Get the list of acs instances.
 *
 * @param params        Parameters object
 *
 * @return              Status code
 */
static te_errno
acse_acs_list(params_t *params)
{
    char        *ptr = params->list;
    unsigned int len = 0;
    acs_item_t  *item;

    /* Calculate the whole length (plus 1 sym for trailing ' '/'\0') */
    STAILQ_FOREACH(item, &acs_list, link)
        len += strlen(item->acs.name) + 1;

    if (len > sizeof params->list)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* Form the list */
    STAILQ_FOREACH(item, &acs_list, link)
    {
        if ((len = strlen(item->acs.name)) > 0)
        {
            if (item != STAILQ_FIRST(&acs_list))
              *ptr++ = ' ';

            memcpy(ptr, item->acs.name, len);
            ptr += len;
        }
    }

    *ptr = '\0';
    return 0;
}

static te_errno
cpe_get_rpc_methods(params_t *params)
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
_fun(params_t *params)                                        \
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
rpc_test(params_t *params)
{
    UNUSED(params);
    ERROR("Hi, I am rpc_test!!! params->acse = %u", params->acse);
    return 0;
}

/** Translation table for calculated goto
 * (shoud correspond to enum acse_fun_t in acse.h) */
static te_errno
    (*xlat[])(params_t *) = {
        &acse_acs_add, &acse_acs_del, &acse_acs_list,
        &acs_url_get, &acs_url_set,
        &acs_cert_get, &acs_cert_set,
        &acs_user_get, &acs_user_set,
        &acs_pass_get, &acs_pass_set,
        &acs_enabled_get, &acs_enabled_set,
        &acs_ssl_get, &acs_ssl_set,
        &acs_port_get, &acs_port_set,
        &acs_cpe_add, &acs_cpe_del, &acs_cpe_list,
        &cpe_ip_addr_get, &cpe_ip_addr_set,
        &cpe_url_get, &cpe_url_set,
        &cpe_cert_get, &cpe_cert_set,
        &cpe_user_get, &cpe_user_set,
        &cpe_pass_get, &cpe_pass_set,
        &device_id_manufacturer_get,
        &device_id_oui_get,
        &device_id_product_class_get,
        &device_id_serial_number_get,
        &session_state_get,
        &session_target_state_get, &session_target_state_set,
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
before_select(void *data, fd_set *rd_set, fd_set *wr_set, int *fd_max)
{
    lrpc_data_t *lrpc = data;

    switch (lrpc->state)
    {
        case want_read:
            FD_SET(lrpc->sock, rd_set);

            if (*fd_max < lrpc->sock + 1)
                *fd_max = lrpc->sock + 1;

            break;
        case want_write:
            FD_SET(lrpc->sock, wr_set);

            if (*fd_max < lrpc->sock + 1)
                *fd_max = lrpc->sock + 1;

            break;
        default:
            break;
    }

    return 0;
}

static te_errno
after_select(void *data, fd_set *rd_set, fd_set *wr_set)
{
    acse_fun_t   fun;
    lrpc_data_t *lrpc = data;

    switch (lrpc->state)
    {
        case want_read:
            if (FD_ISSET(lrpc->sock, rd_set))
            {
                lrpc->len = sizeof lrpc->addr;

                switch (recvfrom(lrpc->sock, &fun, sizeof fun, 0,
                                 (struct sockaddr *)&lrpc->addr,
                                 &lrpc->len))
                {
                    case -1:
                        ERROR("Failed to get call over LRPC: %s",
                              strerror(errno));
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                    case sizeof fun:
                        lrpc->rc =
                            fun >= acse_fun_first && fun <= acse_fun_last ?
                                (*xlat[fun - acse_fun_first])(lrpc->params) :
                                TE_RC(TE_TA_UNIX, TE_ENOSYS);

                        lrpc->state = want_write;
                        break;
                    default:
                        ERROR("Failed to get call over LRPC");
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                }
            }

            break;
        case want_write:
            if (FD_ISSET(lrpc->sock, wr_set))
            {
                switch (sendto(lrpc->sock, &lrpc->rc, sizeof lrpc->rc, 0,
                               (struct sockaddr *)&lrpc->addr, lrpc->len))
                {
                    case -1:
                        ERROR("Failed to return from call over LRPC: %s",
                              strerror(errno));
                        return TE_RC(TE_TA_UNIX, TE_EFAIL);
                    case sizeof lrpc->rc:
                        lrpc->state = want_read;
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

static te_errno
destroy(void *data)
{
    lrpc_data_t *lrpc = data;

    close(lrpc->sock);
    free(lrpc);
    return 0;
}

static te_errno
recover_fds(void *data)
{
    UNUSED(data);
    return -1;
}

extern te_errno
acse_lrpc_create(channel_t *channel, params_t *params, int sock)
{
    lrpc_data_t *lrpc = channel->data = malloc(sizeof *lrpc);

    if (lrpc == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    lrpc->sock            = sock;
    lrpc->addr.sun_family = AF_UNIX;
    lrpc->len             = sizeof lrpc->addr;
    lrpc->params          = params;
    lrpc->rc              = 0;
    lrpc->state           = want_read;

    channel->before_select = &before_select;
    channel->after_select  = &after_select;
    channel->destroy       = &destroy;
    channel->recover_fds   = &recover_fds;

    return 0;
}
