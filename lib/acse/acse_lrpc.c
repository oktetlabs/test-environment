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

/** Session */
typedef struct {
    session_state_t state;         /**< Session state                  */
    session_state_t target_state;  /**< Session desired state          */
    int             enabled;       /**< Whether a session may continue */
    int             hold_requests; /**< Whether to put "hold requests"
                                        in SOAP msg                    */
} session_t;

/** Device ID */
typedef struct {
    char const *manufacturer;  /**< Manufacturer                     */
    char const *oui;           /**< Organizational Unique Identifier */
    char const *product_class; /**< Product Class                    */
    char const *serial_number; /**< Serial Number                    */
} device_id_t;

/** CPE */
typedef struct {
    char const *name;          /**< CPE name          */
    char const *ip_addr;       /**< CPE IP address    */
    char const *url;           /**< CPE URL           */
    char const *cert;          /**< CPE certificate   */
    char const *user;          /**< CPE user name     */
    char const *pass;          /**< CPE user password */
    session_t   session;       /**< Session           */
    device_id_t device_id;     /**< Device Identifier */
} cpe_t;

/** CPE list */
typedef struct cpe_item_t
{
    STAILQ_ENTRY(cpe_item_t) link;
    cpe_t                    cpe;
} cpe_item_t;

/** ACS */
typedef struct {
    char const *name;          /**< ACS name                       */
    char const *url;           /**< ACS URL                        */
    char const *cert;          /**< ACS certificate                */
    char const *user;          /**< ACS user name                  */
    char const *pass;          /**< ACS user password              */
    STAILQ_HEAD(cpe_list_t, cpe_item_t)
                cpe_list;      /**< The list of CPEs being handled */
} acs_t;

/** ACS list */
typedef struct acs_item_t
{
    STAILQ_ENTRY(acs_item_t) link;
    acs_t                acs;
} acs_item_t;

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
static STAILQ_HEAD(acs_list_t, acs_item_t)
    acs_list = STAILQ_HEAD_INITIALIZER(&acs_list); 

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
    ERROR("Hi, I am rpc_test_" #_fun "!!! params->acse = %u", \
          params->acse);                                      \
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
            if (FD_SET(lrpc->sock, wr_set))
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
error_destroy(void *data)
{
    return destroy(data);
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

    channel->type          = lrpc_type;
    channel->before_select = &before_select;
    channel->after_select  = &after_select;
    channel->destroy       = &destroy;
    channel->error_destroy = &error_destroy;

#if 0
{
            unsigned int u;

            for (u = 0; u < 2; u++)
            {
              char buf[64];
              session_item_t *item;

              item = malloc(sizeof *item);

              sprintf(buf, "session_%u", u);
              item->session.name = strdup(buf);
              sprintf(buf, "/agent:Agt_A/acse:/acs:acs%u/cpe:cpe%u", u, u);
              item->session.link = strdup(buf);
              item->session.state = u + 3;
              item->session.target_state = u + 5;
              item->session.enabled = u;
              item->session.hold_requests = u;

              STAILQ_INSERT_TAIL(&session_list, item, link);
            }
}
#endif

    return 0;
}
