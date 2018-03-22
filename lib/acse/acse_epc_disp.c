/** @file
 * @brief ACSE EPC Dispathcer
 *
 * ACS Emulator support
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "ACSE EPC dispatcher"

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


extern int epc_socket;

extern int epc_listen_socket;

static epc_site_t *epc_disp_site = NULL;

/** struct to item of strint-integer convertor in enumerated sets */
typedef struct str_to_int_t {
    const char *s_val; /**< string value */
    int         i_val; /**< integer value */
} str_to_int_t;

/**
 * Convert string to integer corresponding with specified
 * enumerated set.
 *
 * @param tab    enumerate table
 * @param str    input string with value name
 *
 * @return converted value.
 */
int
str_to_int(str_to_int_t *tab, const char *str)
{
    int i;
    for (i = 0; tab[i].s_val != NULL; i++)
    {
        if (strcmp(str, tab[i].s_val) == 0)
            return tab[i].i_val;
    }
    return tab[i].i_val; /* value in the last record is default */
}


/**
 * Convert integer to string corresponding with specified
 * enumerated set.
 *
 * @param tab    enumerate table
 * @param i_val  input integer
 *
 * @return converted string value.
 */
const char *
int_to_str(str_to_int_t *tab, int i_val)
{
    int i;
    for (i = 0; tab[i].s_val != NULL; i++)
    {
        if (tab[i].i_val == i_val)
            return tab[i].s_val;
    }
    return "";
}


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
 * Utility functions for access to readonly configurator value
 * of string type.
 *
 * @return status
 */
static inline te_errno
cfg_string_readonly(const char *string, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_MODIFY)
        return TE_EACCES;

    if (NULL == string)
        params->value[0] = '\0';
    else
        strcpy(params->value, string);
    return 0;
}


/**
 * Utility functions for access to readwrite configurator value
 * of string type.
 *
 * @return status
 */
static inline te_errno
cfg_string_access(const char **pstring, acse_epc_config_data_t *params)
{
    assert(pstring);
    if (params->op.fun == EPC_CFG_MODIFY)
    {
        free_const(*pstring);

        if ((*pstring = strdup(params->value)) == NULL)
            return TE_RC(TE_ACSE, TE_ENOMEM);
    }
    else
    {
        if (*pstring != NULL)
            strcpy(params->value, *pstring);
        else
            params->value[0] = '\0';
    }
    return 0;
}


/**
 * Access to 'hold_requests' flag for CPE CWMP session.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_hold_requests(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_OBTAIN)
        sprintf(params->value, "%i", cpe->hold_requests);
    else
        cpe->hold_requests = atoi(params->value);
    return 0;
}

/**
 * Access to 'sync_mode' flag for CPE CWMP session.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_sync_mode(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_OBTAIN)
        sprintf(params->value, "%i", cpe->sync_mode);
    else
    {
        RING("CPE %p'%s' sync_mode set to %d", cpe,
             cpe->name, atoi(params->value));
        cpe->sync_mode = atoi(params->value);
    }
    return 0;
}

/**
 * Access to 'chunk_mode' flag for CPE CWMP session.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_chunk_mode(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_OBTAIN)
        sprintf(params->value, "%i", cpe->chunk_mode);
    else
        cpe->chunk_mode = atoi(params->value);
    return 0;
}


/**
 * Access to CPE 'traffic_log' flag
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_traffic_log(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_OBTAIN)
        sprintf(params->value, "%i", cpe->traffic_log);
    else
        cpe->traffic_log = atoi(params->value);
    return 0;
}



/**
 * Access to the 'enabled' flag.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_enabled(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_OBTAIN)
    {
        sprintf(params->value, "%i", cpe->enabled);
        return 0;
    }
    else
    {
        int new_value = atoi(params->value);

        if (new_value && !cpe->enabled)
            cpe->enabled = TRUE;

        if (!new_value && cpe->enabled)
            acse_disable_cpe(cpe);
    }
    return 0;
}


/**
 * Get the CWMP session state.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_cwmp_state(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_MODIFY)
        return TE_EACCES;
    if (cpe->session == NULL)
        sprintf(params->value, "%i", CWMP_NOP);
    else
        sprintf(params->value, "%i", cpe->session->state);
    return 0;
}

/**
 * Get the CPE Connection Request state.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
cpe_cr_state(cpe_t *cpe, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_MODIFY)
        return TE_EACCES;
    sprintf(params->value, "%i", cpe->cr_state);

    if (cpe->cr_state == CR_ERROR || cpe->cr_state == CR_DONE)
        cpe->cr_state = CR_NONE;

    return 0;
}


/**
 * Get the device ID serial nuber.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
device_id_serial_number(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_readonly(cpe->device_id.SerialNumber, params);
}

/**
 * Get the device ID product class.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
device_id_product_class(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_readonly(cpe->device_id.ProductClass, params);
}

/**
 * Get the device ID organizational unique ID.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
device_id_oui(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_readonly(cpe->device_id.OUI, params);
}

/**
 * Get the device ID manufacturer.
 *
 * @param cpe           CPE record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
device_id_manufacturer(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_readonly(cpe->device_id.Manufacturer, params);
}



/**
 * Get the list of CPE instances under ACS. Put result into passed
 * config data @p params.
 *
 * @param params        EPC parameters struct
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
    INFO("%s(): list of %s, result '%s'",
         __FUNCTION__, params->acs, params->list);
    return 0;
}


/**
 * Access to the ACS port value.
 *
 * @param acs           ACS record
 * @param params        EPC parameters struct
 *
 * @return      Status code.
 */
static te_errno
acs_port(acs_t *acs, acse_epc_config_data_t *params)
{
    VERB("ACS-port config, fun %d, value '%s', acs ptr %p, old val %d",
            (int)params->op.fun, params->value, acs, (int)acs->port);
    if (params->op.fun == EPC_CFG_MODIFY)
        acs->port = atoi(params->value);
    else
        sprintf(params->value, "%i", acs->port);

    VERB("ACS-port config, value '%s', new val %d",
            params->value, (int)acs->port);
    return 0;
}

/**
 * Access to the ACS UDP port value.
 *
 * @param acs           ACS record
 * @param params        EPC parameters struct
 *
 * @return      Status code.
 */
static te_errno
acs_udp_port(acs_t *acs, acse_epc_config_data_t *params)
{
    VERB("ACS-udp-port config, fun %d, value '%s', acs ptr %p, old val %d",
            (int)params->op.fun, params->value, acs, (int)acs->udp_port);
    if (params->op.fun == EPC_CFG_MODIFY)
        acs->udp_port = atoi(params->value);
    else
        sprintf(params->value, "%i", acs->udp_port);

    VERB("ACS-udp-port config, value '%s', new val %d",
            params->value, (int)acs->udp_port);
    return 0;
}

/**
 * Set the ACS SSL flag.
 *
 * @param acs           ACS record
 * @param params        EPC parameters struct
 *
 * @return      Status code.
 */
static te_errno
acs_ssl(acs_t *acs, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_MODIFY)
        acs->ssl = atoi(params->value);
    else
        sprintf(params->value, "%i", acs->ssl);
    return 0;
}


/**
 * Access to ACS 'traffic_log' flag
 *
 * @param acs           ACS record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
acs_traffic_log(acs_t *acs, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_OBTAIN)
        sprintf(params->value, "%i", acs->traffic_log);
    else
        acs->traffic_log = atoi(params->value);
    return 0;
}


/**
 * Access to 'http_response' field
 *
 * @param acs           ACS record
 * @param params        EPC parameters struct
 *
 * @return      Status code.
 */
static te_errno
acs_http_resp(acs_t *acs, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_MODIFY)
    {
        int r;
        VERB("set http_response to <%s>", params->value);
        if (strlen(params->value) == 0)
        {
            free(acs->http_response);
            acs->http_response = NULL;
            return 0;
        }
        if (NULL == acs->http_response)
            acs->http_response = malloc(sizeof(acse_http_response_t));

        r = sscanf(params->value, "%d %s",
                   &(acs->http_response->http_code),
                    acs->http_response->location);
        if (1 == r)
            acs->http_response->location[0] = '\0';
        if (r <= 0)
        {
            WARN("HTTP response spec wrong, http code expected.");
            return TE_EINVAL;
        }
        VERB("set http_response to code %d, loc <%s>",
             acs->http_response->http_code,
             acs->http_response->location);
    }
    else
    {
        if (NULL == acs->http_response)
            params->value[0] = '\0';
        else
            sprintf(params->value, "%u %s",
                    acs->http_response->http_code,
                    acs->http_response->location);
    }
    return 0;
}
/**
 * Access to the ACS enabled flag.
 *
 * @param acs           ACS record
 * @param params        EPC parameters struct
 *
 * @return              Status code
 */
static te_errno
acs_enabled(acs_t *acs, acse_epc_config_data_t *params)
{
    int prev_value, new_value;
    if (params->op.fun == EPC_CFG_OBTAIN)
    {
        sprintf(params->value, "%i", !(NULL == acs->conn_listen));
        return 0;
    }
    prev_value = !(NULL == acs->conn_listen);
    new_value = atoi(params->value);

    if (new_value != 0 && acs->port == 0)
    {
        WARN("Attempt to activate ACS '%s', but no net port provided",
              acs->name);
        return TE_EFAULT;
    }

    /* field acs->enabled will be set in these specific methods. */

    if (prev_value == 0 && new_value != 0)
        return acse_enable_acs(acs);

    if (prev_value != 0 && new_value == 0)
        return acse_disable_acs(acs);

    /* Here new and previous values are the same */

    return 0;
}



/**
 * Access to the url of ACS.
 *
 * @param acs           ACS object.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
acs_url(acs_t *acs, acse_epc_config_data_t *params)
{
    return cfg_string_access(&acs->url, params);
}


/**
 * Access to the http_root of ACS.
 *
 * @param acs           ACS object.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
acs_http_root(acs_t *acs, acse_epc_config_data_t *params)
{
    return cfg_string_access(&acs->http_root, params);
}


/**
 * Access to the ACS certificate value.
 *
 * @param acs           ACS object.
 * @param params        Parameters object
 *
 * @return      Status code.
 */
static te_errno
acs_cert(acs_t *acs, acse_epc_config_data_t *params)
{
    return cfg_string_access(&acs->cert, params);
}

/**
 * Access to the cert of CPE.
 *
 * @param cpe           CPE record.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
cpe_cert(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_access(&cpe->cert, params);
}

/**
 * Access to the CR url of CPE.
 *
 * @param cpe           CPE record.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
cpe_url(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_access(&cpe->url, params);
}

/**
 * Access to the login name of CPE to ACS.
 *
 * @param cpe           CPE record.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
cpe_acs_login(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_access(&cpe->acs_auth.login, params);
}

/**
 * Access to the passwd of CPE to ACS.
 *
 * @param cpe           CPE record.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
cpe_acs_passwd(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_access(&cpe->acs_auth.passwd, params);
}


/**
 * Access to the login name on CPE for Connection Request.
 *
 * @param cpe           CPE record.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
cpe_cr_login(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_access(&cpe->cr_auth.login, params);
}

/**
 * Access to the passwd on CPE for Connection Request.
 *
 * @param cpe           CPE record.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
cpe_cr_passwd(cpe_t *cpe, acse_epc_config_data_t *params)
{
    return cfg_string_access(&cpe->cr_auth.passwd, params);
}


/** Enumeration for authenticate modes */
str_to_int_t auth_mode_enum[] =
{
    {"noauth", ACSE_AUTH_NONE},
    {"basic",  ACSE_AUTH_BASIC},
    {"digest", ACSE_AUTH_DIGEST},
    {NULL, ACSE_AUTH_DIGEST} /* default */
};

/**
 * Access to the auth type of CPE.
 *
 * @param cpe           CPE object.
 * @param params        Parameters object.
 *
 * @return              Status code
 */
static te_errno
acs_auth_mode(acs_t *acs, acse_epc_config_data_t *params)
{
    if (params->op.fun == EPC_CFG_MODIFY)
        acs->auth_mode = str_to_int(auth_mode_enum, params->value);
    else
        strcpy(params->value, int_to_str(auth_mode_enum, acs->auth_mode));

    return 0;
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
    VERB("%s start", __FUNCTION__);

    *ptr = '\0';
    /* Calculate the whole length, plus 1 sym for trailing ' '|'\0' */
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
    VERB("%s stop, result is '%s'", __FUNCTION__, params->list);
    return 0;
}

/** Callback for access to ACS object conf.field */
typedef te_errno (*config_acs_fun_t)(acs_t *, acse_epc_config_data_t *);

/** struct for ACS configuration fields descriptors */
struct config_acs_item_t {
    const char       *label; /**< name of conf.field */
    config_acs_fun_t  fun;   /**< function to access conf.field */
} cfg_acs_array [] =
{
    {"url", acs_url},
    {"http_root", acs_http_root},
    {"auth_mode", acs_auth_mode},
    {"cert", acs_cert},
    {"ssl",  acs_ssl},
    {"traffic_log",  acs_traffic_log},
    {"port", acs_port},
    {"udp_port", acs_udp_port},
    {"enabled", acs_enabled},
    {"http_response", acs_http_resp},
};

/** Callback for access to CPE record conf.field */
typedef te_errno (*config_cpe_fun_t)(cpe_t *, acse_epc_config_data_t *);

/** struct for CPE configuration fields descriptors */
struct config_cpe_item_t {
    const char       *label; /**< name of conf.field */
    config_cpe_fun_t  fun;   /**< function to access conf.field */
} cfg_cpe_array [] =
{
    {"cr_url",    cpe_url},
    {"cert",    cpe_cert},
    {"cr_login",  cpe_cr_login},
    {"cr_passwd", cpe_cr_passwd},
    {"login",  cpe_acs_login},
    {"passwd", cpe_acs_passwd},
    {"manufacturer", device_id_manufacturer},
    {"oui", device_id_oui},
    {"product_class", device_id_product_class},
    {"serial_number", device_id_serial_number},
    {"cwmp_state", cpe_cwmp_state},
    {"sync_mode", cpe_sync_mode},
    {"chunk_mode", cpe_chunk_mode},
    {"traffic_log",  cpe_traffic_log},
    {"enabled", cpe_enabled},
    {"hold_requests", cpe_hold_requests},
    {"cr_state", cpe_cr_state},
};

/**
 * Perform configuration EPC request on CPE level.
 *
 * @param cfg_pars      input EPC parameters
 *
 * @return status code
 */
te_errno
config_cpe(acse_epc_config_data_t *cfg_pars)
{
    acs_t  *acs;
    cpe_t  *cpe;
    size_t  i;

    acs = db_find_acs(cfg_pars->acs);
    if (acs == NULL || (cpe = db_find_cpe(acs, cfg_pars->cpe)) == NULL)
        return TE_ENOENT;

    VERB("epc_config_cpe, CR URL %s", cpe->url);

    for (i = 0; i < TE_ARRAY_LEN(cfg_cpe_array); i++)
        if (strcmp(cfg_cpe_array[i].label, cfg_pars->oid) == 0)
            return cfg_cpe_array[i].fun(cpe, cfg_pars);

    WARN("config CPE, param '%s' not found", cfg_pars->oid);
    return TE_EINVAL;
}

static te_errno
config_acs(acse_epc_config_data_t *cfg_pars)
{
    acs_t  *acs = db_find_acs(cfg_pars->acs);
    size_t  i;

    if (acs == NULL)
        return TE_ENOENT;

    for (i = 0; i < TE_ARRAY_LEN(cfg_acs_array); i++)
        if (strcmp(cfg_acs_array[i].label, cfg_pars->oid) == 0)
            return cfg_acs_array[i].fun(acs, cfg_pars);

    WARN("config ACS, param '%s' not found", cfg_pars->oid);
    return TE_EINVAL;
}

/**
 * Process EPC related to local configuration: DB, etc.
 * This function do not blocks, and fills @p cfg_pars
 * with immediately result of operation, if any.
 * Usually config operations may be performed without blocking,
 * and they are performed during this call.
 *
 * @param cfg_pars     struct with EPC parameters.
 *
 * @return status code.
 */
te_errno
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

    VERB("epc_cb config, %s/%s, EPC op %s, oid '%s'",
         cfg_pars->acs,
         cfg_pars->op.level == EPC_CFG_CPE ? cfg_pars->cpe : "-",
         cwmp_epc_cfg_op_string(cfg_pars->op.fun), cfg_pars->oid);

    if (EPC_CFG_MODIFY != cfg_pars->op.fun)
        memset(cfg_pars->value, 0, sizeof(cfg_pars->value));

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
            if ((cpe_item = db_find_cpe(acs_item, cfg_pars->cpe)) == NULL)
                return TE_ENOENT;
            return db_remove_cpe(cpe_item);

        case EPC_CFG_MODIFY:
        case EPC_CFG_OBTAIN:
            if (cfg_pars->op.level == EPC_CFG_ACS)
                return config_acs(cfg_pars);
            return config_cpe(cfg_pars);

        case EPC_CFG_LIST:
            VERB("acse_epc_config(): list, level is %d ",
                 (int)cfg_pars->op.level);
            if (EPC_CFG_ACS == cfg_pars->op.level)
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
    acs_t    *acs;
    cpe_t    *cpe;
    te_errno  rc = 0;

    acs = db_find_acs(cwmp_pars->acs);
    if (acs == NULL || (cpe = db_find_cpe(acs, cwmp_pars->cpe)) == NULL)
    {
        ERROR("EPC op %d fails, '%s':'%s' not found\n",
               cwmp_pars->op, cwmp_pars->acs, cwmp_pars->cpe);
        return TE_ENOENT;
    }
    VERB("epc_cb CWMP, %s/%s, EPC op %s, RPC %s, CR URL %s",
         cwmp_pars->acs, cwmp_pars->cpe,
         cwmp_epc_cwmp_op_string(cwmp_pars->op),
         cwmp_rpc_cpe_string(cwmp_pars->rpc_cpe),
         cpe->url);

    switch (cwmp_pars->op)
    {
        case EPC_RPC_CALL:
        {
            te_bool need_call = FALSE;
            /* Insert RPC to queue, ACSE will deliver it during first
               established CWMP session with CPE. */
            cpe_rpc_item_t *rpc_item = calloc(1, sizeof(*rpc_item));

            rpc_item->params = cwmp_pars;
            rpc_item->request_id = cwmp_pars->request_id =
                                        ++cpe->last_queue_index;

            need_call = (cpe->sync_mode &&
                         TAILQ_EMPTY(&cpe->rpc_queue) &&
                         (cpe->session != NULL) &&
                         (CWMP_PENDING == cpe->session->state));

            TAILQ_INSERT_TAIL(&cpe->rpc_queue, rpc_item, links);
            cwmp_pars->from_cpe.p = NULL; /* nothing yet.. */

            RING("EPC CWMP, session %p RPC call %s to '%s', ind %d, "
                 "sync %d need %d",
                 cpe->session,
                 cwmp_rpc_cpe_string(cwmp_pars->rpc_cpe),
                 cwmp_pars->cpe,
                 rpc_item->request_id, cpe->sync_mode, need_call);

            if (need_call)
                acse_cwmp_send_rpc(&(cpe->session->m_soap), cpe->session);
        }
        break;
        case EPC_RPC_CHECK:
        {
            cpe_rpc_item_t *rpc_item;
            void *result = NULL;
            te_bool is_cpe_response = (0 != cwmp_pars->request_id);

            if (!is_cpe_response && 0 == cwmp_pars->rpc_acs)
                return TE_EINVAL;

            TAILQ_FOREACH(rpc_item, &cpe->rpc_queue, links)
            {
                if (rpc_item->request_id == cwmp_pars->request_id)
                    break;
            }

            if (rpc_item != NULL)
                return TE_EPENDING;

            TAILQ_FOREACH(rpc_item, &cpe->rpc_results, links)
            {
                if (CWMP_RPC_ACS_NONE == cwmp_pars->rpc_acs)
                {
                    if (rpc_item->request_id == cwmp_pars->request_id)
                        break;
                }
                else
                {
                    if (rpc_item->params->rpc_acs == cwmp_pars->rpc_acs)
                        break;
                }
            }

            if (rpc_item == NULL)
                return TE_ENOENT;

            cwmp_pars->rpc_cpe = rpc_item->params->rpc_cpe;
            cwmp_pars->request_id = rpc_item->params->request_id;

            result = rpc_item->params->from_cpe.p;
            if (is_cpe_response && cwmp_pars->rpc_cpe == CWMP_RPC_NONE)
            {
                /* Response is received, but unexpected or invalid one */
                rc = TE_EBADMSG;
            }
            else if (result == NULL)
                return TE_EPENDING;

            cwmp_pars->from_cpe.p = result;
            mheap_add_user(rpc_item->heap, cwmp_pars);

            TAILQ_REMOVE(&cpe->rpc_results, rpc_item, links);
            acse_rpc_item_free(rpc_item);

            if (CWMP_RPC_FAULT == cwmp_pars->rpc_cpe)
                rc = TE_CWMP_FAULT;
        }
        break;
        case EPC_GET_INFORM:
        {
            cpe_inform_t *inform_rec;
            if (0 == cwmp_pars->request_id)
                inform_rec = LIST_FIRST(&(cpe->inform_list));
            else
                LIST_FOREACH(inform_rec, &(cpe->inform_list), links)
                {
                    if (inform_rec->request_id == cwmp_pars->request_id)
                        break;
                }
            /* If not found, error */
            if (inform_rec == NULL)
            {
                cwmp_pars->request_id = 0;
                cwmp_pars->from_cpe.inform = NULL;
                return TE_ENOENT;
            }
            cwmp_pars->request_id = inform_rec->request_id;
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
            VERB("EPC CWMP Issue ConnReq to '%s'", cwmp_pars->cpe);
        break;
        case EPC_CONN_REQ_CHECK:
            cwmp_pars->from_cpe.cr_state = cpe->cr_state;
            if (cpe->cr_state == CR_ERROR || cpe->cr_state == CR_DONE)
                cpe->cr_state = CR_NONE;
        break;

        case EPC_HTTP_RESP:
        {
            const char *location = (const char *)cwmp_pars->enc_start;
            size_t loc_len = strlen(location);
            int r;

            if ((cpe->session != NULL) &&
                (CWMP_PENDING == cpe->session->state))
            {
                r = acse_cwmp_send_http(&(cpe->session->m_soap),
                                        cpe->session,
                                        cwmp_pars->to_cpe.http_code,
                                        location);
                if (0 != r)
                {
                    ERROR("send HTTP resp, gSOAP internal error %d", r);
                    return TE_GSOAP_ERROR;
                }
            }
            else /* Save http code */
            {
                if (loc_len > sizeof(cpe->http_response->location))
                {
                    ERROR("HTTP location too long, %u bytes", loc_len);
                    return TE_EINVAL;
                }
                if (NULL == cpe->http_response)
                    cpe->http_response =
                        malloc(sizeof(*cpe->http_response));
                cpe->http_response->http_code = cwmp_pars->to_cpe.http_code;
                strncpy(cpe->http_response->location,
                        (const char *)cwmp_pars->enc_start,
                        sizeof(cpe->http_response->location));
            }
        }
        break;
    }
    return rc;
}

/**
 * Callback for I/O ACSE channel, called before poll().
 * It fills @p pfd according with specific channel situation.
 * Its prototype matches with field #channel_t::before_poll.
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct (OUT)
 * @param deadline  Timestamp until wait, keep untouched if not need (OUT)
 *
 * @return status code.
 */
static te_errno
epc_cfg_before_poll(void *data, struct pollfd *pfd,
                    struct timeval *deadline)
{
    UNUSED(data);
    UNUSED(deadline);
    if (epc_listen_socket >= 0)
        pfd->fd = epc_listen_socket;
    else
        pfd->fd = acse_epc_socket();
    VERB("EPC before poll, fd %d", pfd->fd);

    pfd->revents = 0;
    if (pfd->fd >= 0)
        pfd->events = POLLIN;

    return 0;
}


/**
 * Callback for I/O ACSE channel, called after poll()
 * Its prototype matches with field #channel_t::after_poll.
 * This function should process detected event (usually, incoming data).
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct with marks, which
 *                  event happen.
 *
 * @return status code.
 */
te_errno
epc_cfg_after_poll(void *data, struct pollfd *pfd)
{
    static acse_epc_config_data_t msg;
    te_errno       rc, status;

    UNUSED(data);

    if (pfd == NULL)
    {
        WARN("%s(): pfd is NULL, timeout should not occure!", __FUNCTION__);
        return 0;
    }

    if (!(pfd->revents & POLLIN))
        return 0;

    if (epc_listen_socket >= 0)
    {
        epc_socket = accept(epc_listen_socket, NULL, NULL);
        epc_listen_socket = -1;
        return 0;
    }

    rc = acse_epc_conf_recv(&msg);

    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) != TE_ENOTCONN)
            ERROR("%s(): failed to get EPC message %r",
                  __FUNCTION__, rc);
        fprintf(stderr, "ACSE dispatcher: EPC recv returned error %s\n",
                te_rc_err2str(rc));

        return TE_RC(TE_ACSE, rc);
    }

    status = acse_epc_config(&msg);

    /* Now send response, all data prepared in specific calls above. */
    rc = acse_epc_conf_send(&msg);

    VERB("%s(): send EPC response rc %r", __FUNCTION__, rc);

    if (rc != 0)
        ERROR("%s(): send EPC failed %r", __FUNCTION__, rc);

    return rc;
}

/**
 * Callback for I/O ACSE channel, called at channel destroy.
 * Its prototype matches with field #channel_t::destroy.
 *
 * @param data      Channel-specific private data.
 */
void
epc_cfg_destroy(void *data)
{
    UNUSED(data);
    VERB("EPC dispatcher destroy, pid %d\n", getpid());
    acse_epc_close();
}

/**
 * Callback for I/O ACSE channel, called before poll().
 * It fills @p pfd according with specific channel situation.
 * Its prototype matches with field #channel_t::before_poll.
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct (OUT)
 * @param deadline  Timestamp until wait, keep untouched if not need (OUT)
 *
 * @return status code.
 */
static te_errno
epc_cwmp_before_poll(void *data, struct pollfd *pfd,
                     struct timeval *deadline)
{
    UNUSED(data);
    UNUSED(deadline);
    pfd->fd = epc_disp_site->fd_in;
    VERB("EPC before poll, fd %d", pfd->fd);

    pfd->revents = 0;
    if (pfd->fd >= 0)
        pfd->events = POLLIN;

    return 0;
}


/**
 * Callback for I/O ACSE channel, called after poll()
 * Its prototype matches with field #channel_t::after_poll.
 * This function should process detected event (usually, incoming data).
 *
 * @param data      Channel-specific private data.
 * @param pfd       Poll file descriptor struct with marks, which
 *                  event happen.
 *
 * @return status code.
 */
te_errno
epc_cwmp_after_poll(void *data, struct pollfd *pfd)
{
    acse_epc_cwmp_data_t *msg;
    te_errno       rc;

    UNUSED(data);

    if (pfd == NULL)
    {
        WARN("%s(): pfd is NULL, timeout should not occure!", __FUNCTION__);
        return 0;
    }

    if (!(pfd->revents & POLLIN))
        return 0;

    rc = acse_epc_cwmp_recv(epc_disp_site, &msg, NULL);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) != TE_ENOTCONN)
        {
            ERROR("%s(): failed to get EPC message %r",
                  __FUNCTION__, rc);
            fprintf(stderr, "ACSE dispatcher: EPC recv returned error %s\n",
                    te_rc_err2str(rc));
        }

        return TE_RC(TE_ACSE, rc);
    }

    msg->status = acse_epc_cwmp(msg);

    VERB("%s(): status of operation: %r", __FUNCTION__, msg->status);

    rc = acse_epc_cwmp_send(epc_disp_site, msg);

    VERB("%s(): send EPC cwmp response rc %r", __FUNCTION__, rc);

    /* Do NOT free cwmp message for RPC call operation - they are stored
       in queue, and will be free'd after recieve RPC response and
       report about it. */

    if (rc != 0)
        ERROR("%s(): send EPC failed %r", __FUNCTION__, rc);

    return rc;
}

/**
 * Callback for I/O ACSE channel, called at channel destroy.
 * Its prototype matches with field #channel_t::destroy.
 *
 * @param data      Channel-specific private data.
 */
void
epc_cwmp_destroy(void *data)
{
    UNUSED(data);
    /* if we destroy channel for CWMP operations, ACSE should be stopped */
    if (acse_epc_socket() >= 0)
        acse_epc_close();
}

/* see description in acse_internal.h */
te_errno
acse_epc_disp_init(int listen_sock, epc_site_t *s)
{
    channel_t  *channel_cfg = malloc(sizeof(channel_t));
    channel_t  *channel_cwmp = malloc(sizeof(channel_t));

    if (channel_cfg == NULL || channel_cwmp == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    channel_cfg->data = NULL;
    channel_cwmp->data = NULL;

    epc_listen_socket = listen_sock;
    epc_disp_site = s;

    channel_cfg->before_poll  = &epc_cfg_before_poll;
    channel_cfg->after_poll   = &epc_cfg_after_poll;
    channel_cfg->destroy      = &epc_cfg_destroy;
    channel_cfg->name         = strdup("EPC-config");

    acse_add_channel(channel_cfg);

    channel_cwmp->before_poll  = &epc_cwmp_before_poll;
    channel_cwmp->after_poll   = &epc_cwmp_after_poll;
    channel_cwmp->destroy      = &epc_cwmp_destroy;
    channel_cwmp->name         = strdup("EPC-cwmp");

    acse_add_channel(channel_cwmp);

    return 0;
}
