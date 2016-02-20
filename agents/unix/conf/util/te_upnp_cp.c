/** @file
 * @brief UPnP Control Point process
 *
 * UPnP Control Point process implementation.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#include <jansson.h>
#include <libgupnp/gupnp-control-point.h>
#include "te_defs.h"
#include "logger_api.h"
#include "te_dbuf.h"
#include "te_upnp.h"


/** Channel (socket) buffer size. */
#define CHANNEL_BUFFER_SIZE (4 * 1024)


/** Read status. */
typedef enum {
    RS_ERROR = -1,  /**< An error occurs due to read, and errno is set
                         appropriately. */
    RS_OK = 0,      /**< Success. */
    RS_PARTIAL,     /**< Partial read. */
    RS_EOF,         /**< Got @c EOF. */
    RS_NOMEM,       /**< Memory allocation error. */
} read_status;


static GMainLoop *main_loop = NULL;
static GIOChannel *ch_listen = NULL;
static GIOChannel *ch_read = NULL;
static guint ch_id_listen;
static guint ch_id_read;
static int fd_listen = -1;
static int fd_read = -1;
static GList *devices = NULL;
static GList *services = NULL;
static te_dbuf request = TE_DBUF_INIT(0);


/**
 * Close a file descriptor and print the error message if one occures. On
 * success, set file descriptor to @c -1.
 *
 * @param fd    File descriptor.
 */
static void
close_fd(int *fd)
{
    if (*fd >= 0)
    {
        if (close(*fd) == 0)
            *fd = -1;
        else
            ERROR("fd close error: %s", strerror(errno));
    }
}

/**
 * This is callback function. It is called whenever a UPnP device becomes
 * available.
 *
 * @param control_point     The GUPnPControlPoint that received the signal.
 * @param proxy             The now available GUPnPDeviceProxy.
 * @param user_data         User data set when the signal handler was
 *                          connected.
 */
static void
device_proxy_available_cb(GUPnPControlPoint *cp,
                          GUPnPServiceProxy *proxy,
                          gpointer           userdata)
{
    UNUSED(cp);
    UNUSED(userdata);

    devices = g_list_append(devices, GUPNP_DEVICE_INFO(proxy));
}

/**
 * This is callback function. It is called whenever a UPnP device becomes
 * unavailable.
 *
 * @param control_point     The GUPnPControlPoint that received the signal.
 * @param proxy             The now unavailable GUPnPDeviceProxy.
 * @param user_data         User data set when the signal handler was
 *                          connected.
 */
static void
device_proxy_unavailable_cb(GUPnPControlPoint *cp,
                            GUPnPDeviceProxy  *proxy,
                            gpointer           userdata)
{
    UNUSED(cp);
    UNUSED(userdata);

    devices = g_list_remove(devices, GUPNP_DEVICE_INFO(proxy));
}

/**
 * This is callback function. It is called whenever a UPnP service becomes
 * available.
 *
 * @param control_point     The GUPnPControlPoint that received the signal.
 * @param proxy             The now available GUPnPServiceProxy.
 * @param user_data         User data set when the signal handler was
 *                          connected.
 */
static void
service_proxy_available_cb(GUPnPControlPoint *cp,
                           GUPnPServiceProxy *proxy,
                           gpointer           userdata)
{
    UNUSED(cp);
    UNUSED(userdata);

    services = g_list_append(services, GUPNP_SERVICE_INFO(proxy));
}

/**
 * This is callback function. It is called whenever a UPnP service becomes
 * unavailable.
 *
 * @param control_point     The GUPnPControlPoint that received the signal.
 * @param proxy             The now unavailable GUPnPServiceProxy.
 * @param user_data         User data set when the signal handler was
 *                          connected.
 */
static void
service_proxy_unavailable_cb(GUPnPControlPoint *cp,
                             GUPnPDeviceProxy  *proxy,
                             gpointer           userdata)
{
    UNUSED(cp);
    UNUSED(userdata);

    services = g_list_remove(services, GUPNP_SERVICE_INFO(proxy));
}

/**
 * Create a reply of @p reply_type and containing data pointed by @p jin in
 * JSON format and then serialize it.
 *
 * @param[in]  reply_type   Reply type (see description of Request Type).
 * @param[in]  jin          JSON data to wrap.
 * @param[out] reply        Buffer to write serialized reply message to.
 *                          Note: this buffer is allocated in this function
 *                          and should be freed afrer use.
 * @param[out] size         Size of reply message.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
create_reply(te_upnp_cp_request_type reply_type, const json_t *jin,
             char **reply, size_t *size)
{
    json_t  *jreply = NULL;
    char    *dump = NULL;
    te_errno rc = 0;

#define REPLY_JSON_ARRAY_CREATE(_arr) \
    JSON_ARRAY_CREATE(_arr, rc, create_reply_cleanup)
#define REPLY_JSON_ARRAY_APPEND_NEW(_arr, _val) \
    JSON_ARRAY_APPEND_NEW(_arr, _val, rc, create_reply_cleanup)
#define REPLY_JSON_ARRAY_APPEND(_arr, _val) \
    JSON_ARRAY_APPEND(_arr, _val, rc, create_reply_cleanup)

    REPLY_JSON_ARRAY_CREATE(jreply);
    REPLY_JSON_ARRAY_APPEND_NEW(jreply, json_integer(reply_type));
    REPLY_JSON_ARRAY_APPEND(jreply, (json_t *)jin);

    dump = json_dumps(jreply, JSON_ENCODING_FLAGS);
    if (dump != NULL)
    {
        *reply = dump;
        *size = strlen(dump) + 1;
    }
    else
    {
        ERROR("json_dumps fails");
        rc = TE_EFMT;
    }

create_reply_cleanup:
    json_decref(jreply);

    return rc;

#undef REPLY_JSON_ARRAY_CREATE
#undef REPLY_JSON_ARRAY_APPEND_NEW
#undef REPLY_JSON_ARRAY_APPEND
}

/**
 * Extract the devices from the list and put them into JSON object.
 *
 * @param[in]  name    A friendly name of the device. If zero-length
 *                     string or @c NULL then all devices will be extracted.
 * @param[out] jout    JSON type output data with discovered devices info.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
get_devices(const char *name, json_t **jout)
{
    typedef const char *(*static_getter)(GUPnPDeviceInfo *);
    typedef char *(*dynamic_getter)(GUPnPDeviceInfo *);
    static_getter static_value[] = {
        [DPROPERTY_LOCATION] = gupnp_device_info_get_location,
        [DPROPERTY_UDN] = gupnp_device_info_get_udn,
        [DPROPERTY_TYPE] = gupnp_device_info_get_device_type
    };
    dynamic_getter dynamic_value[] = {
        [DPROPERTY_MANUFACTURER] = gupnp_device_info_get_manufacturer,
        [DPROPERTY_MANUFACTURER_URL] =
                        gupnp_device_info_get_manufacturer_url,
        [DPROPERTY_MODEL_DESCRIPTION] =
                        gupnp_device_info_get_model_description,
        [DPROPERTY_MODEL_NAME] = gupnp_device_info_get_model_name,
        [DPROPERTY_MODEL_NUMBER] = gupnp_device_info_get_model_number,
        [DPROPERTY_MODEL_URL] = gupnp_device_info_get_model_url,
        [DPROPERTY_SERIAL_NUMBER] = gupnp_device_info_get_serial_number,
        [DPROPERTY_UPC] = gupnp_device_info_get_upc,
        [DPROPERTY_PRESENTATION_URL] =
                        gupnp_device_info_get_presentation_url
    };
    json_t *jdevices = NULL;
    json_t *jparams = NULL;
    json_t *params[DPROPERTY_MAX] = {};
    int     rc = 0;
    char   *str;
    char   *friendly_name = NULL;
    GList  *l;
    size_t  i;

    if (name == NULL)
        name = "";

    jdevices = json_array();
    if (jdevices == NULL)
    {
        ERROR("json_array fails");
        return TE_ENOMEM;
    }

    for (l = devices; l != NULL; l = l->next)
    {
        friendly_name = gupnp_device_info_get_friendly_name(l->data);
        if (*name == '\0' /* Any device. */ ||
            (friendly_name != NULL && strcmp(friendly_name, name) == 0))
        {
            /* Parameters: <index><value>. */
            /* Friendly name. */
            params[DPROPERTY_FRIENDLY_NAME] = json_string(
                (friendly_name ? friendly_name : ""));
            /* Icon URL. */
            str = gupnp_device_info_get_icon_url(l->data, NULL, -1, -1, -1,
                                                 TRUE, NULL, NULL, NULL,
                                                 NULL);
            params[DPROPERTY_ICON_URL] = json_string(str ? str : "");
            g_free(str);
            /* Static values. */
            for (i = 0; i < TE_ARRAY_LEN(static_value); i++)
            {
                if (static_value[i] != NULL)
                {
                    const char *tmp_str = static_value[i](l->data);
                    params[i] = json_string(tmp_str ? tmp_str : "");
                }
            }
            /* Dynamic values. */
            for (i = 0; i < TE_ARRAY_LEN(dynamic_value); i++)
            {
                if (dynamic_value[i] != NULL)
                {
                    str = dynamic_value[i](l->data);
                    params[i] = json_string(str ? str : "");
                    g_free(str);
                }
            }

            /* Put parameters to the device's array. */
            jparams = json_array();
            if (jparams == NULL)
            {
                ERROR("json_array fails");
                rc = TE_ENOMEM;
                break;
            }
            for (i = 0; i < TE_ARRAY_LEN(params); i++)
            {
                if (json_array_append(jparams, params[i]) == -1)
                {
                    ERROR("json_array_append fails");
                    rc = TE_EFAIL;
                    break;
                }
            }
            if (rc != 0)
                break;
            if (json_array_append(jdevices, jparams) == -1)
            {
                ERROR("json_array_append fails");
                rc = TE_EFAIL;
                break;
            }
            for (i = 0; i < TE_ARRAY_LEN(params); i++)
                json_decref(params[i]);
            json_decref(jparams);
        }
        g_free(friendly_name);
    }

    if (rc == 0)
        *jout = jdevices;
    else
    {
        for (i = 0; i < TE_ARRAY_LEN(params); i++)
            json_decref(params[i]);
        g_free(friendly_name);
        json_decref(jparams);
        json_decref(jdevices);
    }

    return rc;
}

/**
 * Extract the actions from the service introspection and put them into JSON
 * object.
 *
 * @param[in]  introspection    Service introspection.
 * @param[out] jout             JSON type output data with actions.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
get_service_actions(GUPnPServiceIntrospection *introspection,
                    json_t **jout)
{
    const GList *actions;
    const GList *act;
    const GList *arg;
    json_t      *jactions = NULL;
    json_t      *params[APROPERTY_MAX] = {};
    int          rc = 0;
    size_t       i;
    GUPnPServiceActionInfo *act_info;

    jactions = json_object();
    if (jactions == NULL)
    {
        ERROR("json_object fails");
        return TE_ENOMEM;
    }

    if (introspection == NULL)
    {
        *jout = jactions;
        return 0;
    }

    actions = gupnp_service_introspection_list_actions(introspection);
    for (act = actions; act; act = act->next)
    {
        json_t *jarguments = NULL;

        /* Arguments. */
        jarguments = json_array();
        if (jarguments == NULL)
        {
            ERROR("json_array fails");
            rc = TE_ENOMEM;
            break;
        }

        act_info = (GUPnPServiceActionInfo *)act->data;
        for (arg = act_info->arguments; arg; arg = arg->next)
        {
            GUPnPServiceActionArgInfo *arg_info;
            json_t *jargument = NULL;

            arg_info = (GUPnPServiceActionArgInfo *)arg->data;
            /* Argument name. */
            params[APROPERTY_NAME] =
                        json_string(arg_info->name ? arg_info->name : "");
            /* Argument direction. */
            params[APROPERTY_DIRECTION] = json_integer(
                arg_info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN
                ? UPNP_ARG_DIRECTION_IN
                : UPNP_ARG_DIRECTION_OUT);
            /* Argument variable name. */
            params[APROPERTY_STATE_VARIABLE] =
                    json_string(arg_info->related_state_variable ?
                                arg_info->related_state_variable : "");
            /* Argument retval arg_info->retval looks as unnecessary. */

            jargument = json_array();
            if (jargument == NULL)
            {
                ERROR("json_array fails");
                rc = TE_ENOMEM;
                break;
            }
            for (i = 0; i < TE_ARRAY_LEN(params); i++)
            {
                if (json_array_append(jargument, params[i]) == -1)
                {
                    ERROR("json_array_append fails");
                    rc = TE_EFAIL;
                    break;
                }
            }
            if (rc != 0)
                break;
            /* Put argument to the arguments' array. */
            rc = json_array_append(jarguments, jargument);
            json_decref(jargument);
            if (rc != 0)
            {
                ERROR("json_array_append fails");
                rc = TE_EFAIL;
                break;
            }
            for (i = 0; i < TE_ARRAY_LEN(params); i++)
                json_decref(params[i]);
        }
        if (rc != 0)
        {
            for (i = 0; i < TE_ARRAY_LEN(params); i++)
                json_decref(params[i]);
            json_decref(jarguments);
            break;
        }

        /* Put action name and arguments to the actions' object. */
        rc = json_object_set(jactions,
                             (act_info->name ? act_info->name : "nameless"),
                             jarguments);
        json_decref(jarguments);
        if (rc != 0)
        {
            ERROR("json_object_set fails");
            rc = TE_EFAIL;
            break;
        }
    }

    if (rc == 0)
        *jout = jactions;
    else
        json_decref(jactions);

    return rc;
}

/**
 * Cast the G_TYPE_* GValue variable to G_TYPE_STRING GValue variable and
 * put the contents of a G_TYPE_STRING GValue to the created JSON string.
 *
 * @param gvalue    GValue value to cast.
 *
 * @return JSON string or @c NULL on error.
 */
static json_t *
get_jstring_from_gvalue(const GValue *gvalue)
{
    GValue      gval = G_VALUE_INIT;
    json_t     *jstring = NULL;
    const char *str;

    g_value_init(&gval, G_TYPE_STRING);
    if (!g_value_transform(gvalue, &gval))
        return NULL;
    str = g_value_get_string(&gval);
    jstring = json_string(str ? str : "");
    g_value_unset(&gval);
    return jstring;
}

/**
 * Extract the state variables from the service introspection and put them
 * into JSON object.
 *
 * @param[in]  introspection    Service introspection.
 * @param[out] jout             JSON type output data with state variables.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
get_service_state_variables(GUPnPServiceIntrospection *introspection,
                            json_t **jout)
{
    const GList *variables;
    const GList *var;
    const char  *str;
    json_t      *jstate_variables = NULL;
    json_t      *jvariable = NULL;
    json_t      *jallowed_values = NULL;
    json_t      *params[VPROPERTY_MAX] = {};
    int          rc = 0;
    size_t       i;
    GUPnPServiceStateVariableInfo *variable;

    jstate_variables = json_array();
    if (jstate_variables == NULL)
    {
        ERROR("json_array fails");
        return TE_ENOMEM;
    }

    if (introspection == NULL)
    {
        *jout = jstate_variables;
        return 0;
    }

    variables =
        gupnp_service_introspection_list_state_variables(introspection);
    for (var = variables; var; var = var->next)
    {
        variable = (GUPnPServiceStateVariableInfo *)var->data;

        /* Variable parameters: <name><value>. */
        /* State variable name. */
        params[VPROPERTY_NAME] =
                        json_string(variable->name ? variable->name : "");
        /* Send events. */
#if JANSSON_VERSION_HEX >= 0x020400
        params[VPROPERTY_SEND_EVENTS] = json_boolean(variable->send_events);
#else /* JANSSON_VERSION_HEX */
        params[VPROPERTY_SEND_EVENTS] =
                    (variable->send_events ? json_true() : json_false());
#endif /* JANSSON_VERSION_HEX */

        /* Type. */
        str = g_type_name(variable->type);
        params[VPROPERTY_TYPE] = json_string(str ? str : "");
        /* Default value. */
        params[VPROPERTY_DEFAULT_VALUE] =
                        get_jstring_from_gvalue(&variable->default_value);
        if (variable->is_numeric)
        {
            /* Minimum. */
            params[VPROPERTY_MINIMUM] =
                            get_jstring_from_gvalue(&variable->minimum);
            /* Maximum. */
            params[VPROPERTY_MAXIMUM] =
                            get_jstring_from_gvalue(&variable->maximum);
            /* Step. */
            params[VPROPERTY_STEP] =
                            get_jstring_from_gvalue(&variable->step);
        }
        else
        {
            /* Minimum. */
            params[VPROPERTY_MINIMUM] = json_null();
            /* Maximum. */
            params[VPROPERTY_MAXIMUM] = json_null();
            /* Step. */
            params[VPROPERTY_STEP] = json_null();
        }

        /* Allowed_values. */
        jallowed_values = json_array();
        params[VPROPERTY_ALLOWED_VALUES] = jallowed_values;
        if (jallowed_values == NULL)
        {
            ERROR("json_array fails");
            rc = TE_ENOMEM;
            break;
        }
        if (variable->allowed_values)
        {
            GList *alval;

            for (alval = variable->allowed_values;
                 alval;
                 alval = alval->next)
            {
                rc = json_array_append_new(jallowed_values,
                       json_string(alval->data ? (char *)alval->data : ""));
                if (rc != 0)
                {
                    ERROR("json_array_append_new fails");
                    rc = TE_EFAIL;
                    break;
                }
            }
        }
        if (rc != 0)
            break;

        jvariable = json_array();
        if (jvariable == NULL)
        {
            ERROR("json_array fails");
            rc = TE_ENOMEM;
            break;
        }
        for (i = 0; i < TE_ARRAY_LEN(params); i++)
        {
            if (json_array_append(jvariable, params[i]) == -1)
            {
                ERROR("json_array_append fails");
                rc = TE_EFAIL;
                break;
            }
        }
        if (rc != 0)
            break;

        /* Put state variable to the jstate_variables' array. */
        if (json_array_append(jstate_variables, jvariable) == -1)
        {
            ERROR("json_array_append fails");
            rc = TE_EFAIL;
            break;
        }
        for (i = 0; i < TE_ARRAY_LEN(params); i++)
            json_decref(params[i]);
        json_decref(jvariable);
        jvariable = NULL;
    }

    if (rc == 0)
        *jout = jstate_variables;
    else
    {
        for (i = 0; i < TE_ARRAY_LEN(params); i++)
            json_decref(params[i]);
        json_decref(jvariable);
        json_decref(jstate_variables);
    }

    return rc;
}

/**
 * Extract the services from the list and put them into JSON object.
 *
 * @param[in]  udn      UDN of the device which services looks for. If zero
 *                      length string or @c NULL then certain services of
 *                      any device will be extracted.
 * @param[in]  id       ID of the service. If zero length string or @c NULL
 *                      then every service of certain device will be
 *                      extracted.
 * @param[out] jout     JSON type output data with discovered services info.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
get_services(const char *udn, const char *id, json_t **jout)
{
    typedef const char *(*static_getter)(GUPnPServiceInfo *);
    typedef char *(*dynamic_getter)(GUPnPServiceInfo *);
    static_getter static_value[] = {
        [SPROPERTY_LOCATION] = gupnp_service_info_get_location,
        [SPROPERTY_TYPE] = gupnp_service_info_get_service_type
    };
    dynamic_getter dynamic_value[] = {
        [SPROPERTY_SCPD_URL] = gupnp_service_info_get_scpd_url,
        [SPROPERTY_CONTROL_URL] = gupnp_service_info_get_control_url,
        [SPROPERTY_EVENT_SUBSCRIPTION_URL] =
                        gupnp_service_info_get_event_subscription_url
    };
    json_t     *jservices = NULL;
    json_t     *jservice = NULL;
    json_t     *jparams = NULL;
    json_t     *jactions = NULL;
    json_t     *jvariables = NULL;
    json_t     *params[SPROPERTY_MAX] = {};
    int         rc = 0;
    char       *service_id = NULL;
    const char *service_udn = NULL;
    GList      *l;
    size_t      i;
    GUPnPServiceIntrospection *introspection = NULL;
    GError     *error = NULL;

    if (id == NULL)
        id = "";
    if (udn == NULL)
        udn = "";

    jservices = json_array();
    if (jservices == NULL)
    {
        ERROR("json_array fails");
        return TE_ENOMEM;
    }

    for (l = services; l != NULL; l = l->next)
    {
        service_id = gupnp_service_info_get_id(l->data);
        service_udn = gupnp_service_info_get_udn(l->data);

        if ((*id == '\0' /* Any service. */ ||
             (service_id != NULL && strcmp(service_id, id) == 0)) &&
            (*udn == '\0' /* For any device. */ ||
             strcmp(service_udn, udn) == 0))
        {
            /* Parameters: <index><value>. */
            /* Service ID. */
            params[SPROPERTY_ID] = json_string(
                                        (service_id ? service_id : ""));
            /* Service UDN. */
            params[SPROPERTY_UDN] = json_string(
                                        (service_udn ? service_udn : ""));
            /* Static values. */
            for (i = 0; i < TE_ARRAY_LEN(static_value); i++)
            {
                if (static_value[i] != NULL)
                {
                    const char *str = static_value[i](l->data);
                    params[i] = json_string(str ? str : "");
                }
            }
            /* Dynamic values. */
            for (i = 0; i < TE_ARRAY_LEN(dynamic_value); i++)
            {
                if (dynamic_value[i] != NULL)
                {
                    char *str = dynamic_value[i](l->data);
                    params[i] = json_string(str ? str : "");
                    g_free(str);
                }
            }

            /* Introspection. */
            introspection = gupnp_service_info_get_introspection(l->data,
                                                                 &error);
            if (error != NULL)
            {
                ERROR("Fail to get introspection for service \"%s\": %s",
                      service_udn, error->message);
                g_error_free(error);
                error = NULL;
                introspection = NULL;
            }
            /* Actions. */
            rc = get_service_actions(introspection, &jactions);
            if (rc != 0)
            {
                ERROR("Fail to get actions for service \"%s\"",
                      service_udn);
                break;
            }
            /* State variables. */
            rc = get_service_state_variables(introspection, &jvariables);
            if (rc != 0)
            {
                ERROR("Fail to get state variables for service \"%s\"",
                      service_udn);
                break;
            }
            if (introspection != NULL)
            {
                g_object_unref(introspection);
                introspection = NULL;
            }

            /* Put parameters to the service's array. */
            jparams = json_array();
            if (jparams == NULL)
            {
                ERROR("json_array fails");
                rc = TE_ENOMEM;
                break;
            }
            for (i = 0; i < TE_ARRAY_LEN(params); i++)
            {
                if (json_array_append(jparams, params[i]) == -1)
                {
                    ERROR("json_array_append fails");
                    rc = TE_EFAIL;
                    break;
                }
            }
            if (rc != 0)
                break;

            jservice = json_object();
            if (jservice == NULL)
            {
                ERROR("json_object fails");
                return -1;
            }
            if (json_object_set(jservice, "Parameters", jparams) == -1)
            {
                ERROR("json_object_set fails");
                rc = TE_EFAIL;
                break;
            }
            if (json_object_set(jservice, "Actions", jactions) == -1)
            {
                ERROR("json_object_set fails");
                rc = TE_EFAIL;
                break;
            }
            if (json_object_set(jservice, "StateVariables",
                                jvariables) == -1)
            {
                ERROR("json_object_set fails");
                rc = TE_EFAIL;
                break;
            }
            if (json_array_append(jservices, jservice) == -1)
            {
                ERROR("json_array_append fails");
                rc = TE_EFAIL;
                break;
            }

            for (i = 0; i < TE_ARRAY_LEN(params); i++)
                json_decref(params[i]);
            json_decref(jparams);
            jparams = NULL;
            json_decref(jactions);
            jactions = NULL;
            json_decref(jvariables);
            jvariables = NULL;
            json_decref(jservice);
            jservice = NULL;
        }
        g_free(service_id);
    }

    if (rc == 0)
        *jout = jservices;
    else
    {
        g_free(service_id);
        if (introspection != NULL)
            g_object_unref(introspection);
        json_decref(jactions);
        json_decref(jvariables);
        for (i = 0; i < TE_ARRAY_LEN(params); i++)
            json_decref(params[i]);
        json_decref(jparams);
        json_decref(jservices);
        json_decref(jservice);
    }

    return rc;
}

#if !GLIB_CHECK_VERSION(2,28,0)
/* Only for GLib version < 2.28.*/
/**
 * Remove the all elements from the @p list and free it.
 * Elements will be freed with @b free function, i.e. they should not be
 * allocated with GLib allocators.
 *
 * @param list      List to free.
 *
 * @sa free, g_list_free
 */
static void
glist_free(GList *list)
{
    GList *l;

    for (l = list; l != NULL; l = l->next)
        free(l->data);
    g_list_free(list);
}
#else /* !GLIB_CHECK_VERSION(2,28,0) */
/**
 * Remove the all elements from the @p list and free it.
 * Elements will be freed with @b free function, i.e. they should not be
 * allocated with GLib allocators.
 *
 * @param list      List to free.
 *
 * @sa free, g_list_free_full
 */
static inline void
glist_free(GList *list)
{
    g_list_free_full(list, free);
}
#endif /* !GLIB_CHECK_VERSION(2,28,0) */

/**
 * Remove and free all GValue elements of the @p list and free the list.
 *
 * @param list      List of GValues to free.
 *
 * @sa g_value_unset, g_slice_free, g_list_free
 */
static inline void
gvaluelist_free(GList *list)
{
    GList *l;

    for (l = list; l != NULL; l = l->next)
    {
        g_value_unset(l->data);
        g_slice_free(GValue, l->data);
    }
    g_list_free(list);
}

/**
 * Invoke the action on particular UPnP device and service.
 *
 * @param[in]  udn      UDN of the service to invoke action on.
 * @param[in]  id       ID of the service to invoke action on.
 * @param[in]  jaction  JSON type contains action to invoke.
 * @param[out] jout     JSON type output data with invoke action result.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
invoke_action(const json_t *jaction, json_t **jout)
{
    GUPnPServiceProxy *service = NULL;
    const char *udn;
    const char *id;
    const char *action_name = NULL;
    GError     *error = NULL;
    GList      *in_names = NULL;
    GList      *in_values = NULL;
    GList      *out_names = NULL;
    GList      *out_types = NULL;
    GList      *out_values = NULL;
    GList      *l;
    GList      *n;
    json_t     *jval;
    json_t     *joutval = NULL;
    json_t     *jres = NULL;
    void       *iter;
    te_errno    rc = 0;
    size_t      i;

    if (!json_is_object(jaction))
    {
        ERROR("Invalid input arguments");
        return TE_EINVAL;
    }

    /* Retrieve the service UDN. */
    udn = json_string_value(json_object_get(jaction, "udn"));
    if (udn == NULL)
    {
        ERROR("Invalid service UDN. JSON string was expected");
        return TE_EFMT;
    }
    /* Retrieve the service ID. */
    id = json_string_value(json_object_get(jaction, "id"));
    if (id == NULL)
    {
        ERROR("Invalid service ID. JSON string was expected");
        return TE_EFMT;
    }

    /* Search for particular service. */
    for (l = services; l != NULL; l = l->next)
    {
        const char *service_udn;
        char       *service_id;

        service_id = gupnp_service_info_get_id(l->data);
        service_udn = gupnp_service_info_get_udn(l->data);
        if (service_udn != NULL && strcmp(service_udn, udn) == 0 &&
            service_id != NULL && strcmp(service_id, id) == 0)
        {
            service = l->data;
            g_free(service_id);
            break;
        }
        g_free(service_id);
    }
    if (service == NULL)
    {
        ERROR("Service not found");
        return TE_EINVAL;
    }

    /* Prepare action. */
    /* Retrieve the action name. */
    action_name = json_string_value(json_object_get(jaction, "name"));
    if (action_name == NULL)
    {
        ERROR("Invalid action name. JSON string was expected");
        return TE_EFMT;
    }
    /* Retrieve the action input arguments. */
    jval = json_object_get(jaction, "in");
    if (jval == NULL || !json_is_object(jval))
    {
        ERROR("Invalid action input argument. JSON object was expected");
        return TE_EFMT;
    }
    iter = json_object_iter(jval);
    while (iter != NULL)
    {
        const char *key;
        const char *value;
        char       *str;
        GValue     *gval;

        /* Argument name. */
        key = json_object_iter_key(iter);
        str = strdup(key);
        if (str == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            break;
        }
        in_names = g_list_append(in_names, (gpointer)str);

        /* Argument value. */
        value = json_string_value(json_object_iter_value(iter));
        if (value == NULL)
        {
            ERROR("Invalid argument value. JSON string was expected");
            rc = TE_EINVAL;
            break;
        }
        gval = g_slice_new0 (GValue);
        g_value_init(gval, G_TYPE_STRING);
        g_value_set_string(gval, value);
        in_values = g_list_append(in_values, (gpointer)gval);

        /* Go to next argument. */
        iter = json_object_iter_next(jval, iter);
    }
    if (rc != 0)
        goto invoke_action_cleanup;

    /* Retrieve the action output arguments. */
    jval = json_object_get(jaction, "out");
    if (jval == NULL || !json_is_array(jval))
    {
        ERROR("Invalid action output argument. JSON array was expected");
        rc = TE_EFMT;
        goto invoke_action_cleanup;
    }
    for (i = 0; i < json_array_size(jval); i++)
    {
        const char *name;
        char       *str;

        /* Argument name. */
        name = json_string_value(json_array_get(jval, i));
        if (name == NULL)
        {
            ERROR("Invalid argument value. JSON string was expected");
            rc = TE_EINVAL;
            break;
        }
        str = strdup(name);
        if (str == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            break;
        }
        out_names = g_list_append(out_names, (gpointer)str);

        /* Argument type. */
        out_types = g_list_append(out_types, (gpointer)G_TYPE_STRING);
    }
    if (rc != 0)
        goto invoke_action_cleanup;

    if (!gupnp_service_proxy_send_action_list(service, action_name, &error,
                                              in_names, in_values,
                                              out_names, out_types,
                                              &out_values))
    {
        if (error != NULL)
        {
            ERROR("Send action \"%s\" finished with error: %s",
                  action_name, error->message);
            g_error_free(error);
            error = NULL;
        }
        else
            ERROR("Send action \"%s\" fails", action_name);
        rc = TE_EFAIL;
        goto invoke_action_cleanup;
    }

    /* Parse result and pack it in JSON object. */
#define ACT_JSON_OBJECT_CREATE(_obj) \
    JSON_OBJECT_CREATE(_obj, rc, invoke_action_cleanup)
#define ACT_JSON_OBJECT_SET_NEW(_obj, _key, _val) \
    JSON_OBJECT_SET_NEW(_obj, _key, _val, rc, invoke_action_cleanup)

    ACT_JSON_OBJECT_CREATE(joutval);
    for (l = out_values, n = out_names;
         l != NULL && n != NULL;
         l = l->next, n = n->next)
    {
        ACT_JSON_OBJECT_SET_NEW(joutval, n->data,
                                json_string(g_value_get_string(l->data)));
    }
    ACT_JSON_OBJECT_CREATE(jres);
    ACT_JSON_OBJECT_SET_NEW(jres, "udn", json_string(udn));
    ACT_JSON_OBJECT_SET_NEW(jres, "id", json_string(id));
    ACT_JSON_OBJECT_SET_NEW(jres, "name", json_string(action_name));
    ACT_JSON_OBJECT_SET_NEW(jres, "out", joutval);

#undef ACT_JSON_OBJECT_CREATE
#undef ACT_JSON_OBJECT_SET_NEW

invoke_action_cleanup:
    glist_free(in_names);
    glist_free(out_names);
    g_list_free(out_types);
    gvaluelist_free(in_values);
    gvaluelist_free(out_values);

    if (rc == 0)
        *jout = jres;
    else
    {
        json_decref(joutval);
        json_decref(jres);
    }

    return rc;
}

/**
 * Try to read data from the socket (get a request).
 *
 * @param[in]  fd       File descriptor.
 * @param[out] dbuf     Dynamic buffer to collect the read data.
 *
 * @return Result of reading @sa read_status.
 */
static int
get_request(int fd, te_dbuf *dbuf)
{
    static char tmp_buf[128];
    ssize_t     rc;

    rc = read(fd, tmp_buf, sizeof(tmp_buf));
    if (rc > 0)
    {
        if (te_dbuf_append(dbuf, tmp_buf, rc) != 0)
            return RS_NOMEM;
        if (tmp_buf[rc - 1] == '\0') /* Wait for null-terminated string */
            return RS_OK;
    }
    else if (rc == -1)
    {
        ERROR("Read error: %s", strerror(errno));
        return RS_ERROR;
    }
    else if (rc == 0)
    {
        WARN("Got EOF");
        return RS_EOF;
    }
    return RS_PARTIAL;
}

/**
 * Perform a request processing and prepare a reply.
 *
 * @param[in]  request  Request message.
 * @param[out] reply    Buffer to write reply message to. Note that buffer
 *                      is allocated in this function.
 * @param[out] size     Size of reply message.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
process_request(const te_dbuf *request, char **reply, size_t *size)
{
    json_t       *jrequest = NULL;
    json_t       *jdata = NULL;
    json_error_t  error;
    te_upnp_cp_request_type req_type;
    int           rc = 0;

    jrequest = json_loads((const char*)request->ptr, 0, &error);
    if (jrequest == NULL)
    {
        ERROR("json_loads fails on position %u with massage: %s",
              error.position, error.text);
        return TE_EFMT;
    }
    if (!json_is_integer(json_array_get(jrequest, 0)))
    {
        ERROR("Invalid request type. JSON integer was expected");
        json_decref(jrequest);
        return TE_EFMT;
    }
    req_type = json_integer_value(json_array_get(jrequest, 0));

    switch(req_type)
    {
        case UPNP_CP_REQUEST_DEVICE:
        {
            const char *device_name;

            device_name = json_string_value(json_array_get(jrequest, 1));
            if (device_name == NULL)
            {
                ERROR("Invalid request argument");
                rc = TE_EINVAL;
            }
            else
                rc = get_devices(device_name, &jdata);
            break;
        }

        case UPNP_CP_REQUEST_SERVICE:
        {
            const char *udn;
            const char *id;

            udn = json_string_value(json_array_get(jrequest, 1));
            id = json_string_value(json_array_get(jrequest, 2));
            if (udn == NULL || id == NULL)
            {
                ERROR("Invalid request arguments");
                rc = TE_EINVAL;
            }
            else
                rc = get_services(udn, id, &jdata);
            break;
        }

        case UPNP_CP_REQUEST_ACTION:
            rc = invoke_action(json_array_get(jrequest, 1), &jdata);
            break;

        default:
            ERROR("Unknown request type: %s", req_type);
            rc = TE_EBADMSG;
    }

    if (rc == 0)
    {
        rc = create_reply(req_type, jdata, reply, size);
        if (rc != 0)
            ERROR("create_reply fails");
    }
    json_decref(jdata);
    json_decref(jrequest);
    return rc;
}

/**
 * Write data to the socket (send a reply).
 *
 * @param fd    Socket descriptor.
 * @param buf   Buffer to send data from.
 * @param size  Buffer size.
 *
 * @return @c 0 on success and @c -1 on error.
 */
static int
send_reply(int fd, const void *buf, size_t size)
{
    ssize_t     remaining = size;
    ssize_t     written;
    const char *data = buf;

    while (remaining > 0)
    {
        written = write(fd, data, remaining);
        if (written != remaining)
        {
            if (written >= 0)
            {
                if (written == 0)
                    VERB("Write error: nothing was written");
                else
                    VERB("Write error: partial write. "
                         "Written: %u/%u bytes.", written, remaining);
            }
            else /* if (wc == -1) */
            {
                ERROR("Write error: %s", strerror(errno));
                return -1;
            }
        }
        remaining -= written;
        data += written;
    }
    return 0;
}

/**
 * This is callback function. It is called if the data is available on unix
 * socket to read. Perform the protocol message exchange with server.
 *
 * @param source        The GIOChannel event source.
 * @param condition     The condition which has been satisfied.
 * @param data          User data set in g_io_add_watch() or
 *                      g_io_add_watch_full().
 *
 * @return @c FALSE if the event source should be removed and @c TRUE
 *         otherwise.
 */
static gboolean
read_client_cb(GIOChannel  *source,
               GIOCondition condition,
               gpointer     data)
{
    UNUSED(condition);
    UNUSED(data);
    int          s;
    read_status  rs;
    gboolean     rc = TRUE;
    char        *reply = NULL;
    size_t       reply_size;

    /*
     * Get the file descriptor from the channel. Must be the same as
     * fd_read.
     */
    s = g_io_channel_unix_get_fd(source);

    /* Get request stage. */
#if UPNP_DEBUG > 1
    VERB("* Read request");
#endif /* UPNP_DEBUG */
    rs = get_request(s, &request);
    switch (rs)
    {
        case RS_PARTIAL:
            return TRUE;    /* To continue a data waiting. */

        case RS_OK:
#if UPNP_DEBUG > 1
            te_dbuf_print(&request);
#endif /* UPNP_DEBUG */

            /* Processing stage. */
#if UPNP_DEBUG > 1
            VERB("* Process request, prepare reply");
#endif /* UPNP_DEBUG */
            if (process_request(&request, &reply, &reply_size) != 0)
            {
                rc = FALSE;
                break;
            }

            /* Send reply stage. */
#if UPNP_DEBUG > 1
            VERB("* Write reply: %lu bytes", reply_size);
#endif /* UPNP_DEBUG */
            if (send_reply(s, reply, reply_size) == -1)
                rc = FALSE;
            break;

        default:
            rc = FALSE;
    }
    te_dbuf_reset(&request);
    free(reply);
    if (!rc)
    {
        g_io_channel_unref(ch_read);
        ch_read = NULL;
        close_fd(&fd_read);
    }
    return rc;
}

/**
 * This is callback function. It is called when a client try to connect to
 * certain socket.
 *
 * @param source        The GIOChannel event source.
 * @param condition     The condition which has been satisfied.
 * @param data          User data set in g_io_add_watch() or
 *                      g_io_add_watch_full().
 *
 * @return The function return @c FALSE if the event source should be
 *         removed and @c TRUE otherwise.
 */
static gboolean
wait_for_client_cb(GIOChannel  *source,
                   GIOCondition condition,
                   gpointer     data)
{
    UNUSED(condition);
    UNUSED(data);
    GError   *error = NULL;
    GIOStatus giost;
    int       client;

    if (g_io_channel_get_buffer_condition(source) == 0)
    {
        client = accept(fd_listen, NULL, NULL);
        if (client == -1)
            ERROR("Accept error: %s", strerror(errno));
        else
        {
            if (ch_read != NULL)
            {
                ERROR("Too many clients");
                close_fd(&client);
            }
            else
            {
                ch_read = g_io_channel_unix_new(client);
                giost = g_io_channel_set_encoding(ch_read, NULL, &error);
                if (giost != G_IO_STATUS_NORMAL)
                {
                    ERROR("Set channel encoding error: %s", error->message);
                    g_error_free(error);
                    error = NULL;
                    g_io_channel_unref(ch_read);
                    ch_read = NULL;
                    close_fd(&client);
                }
                else
                {
                    fd_read = client;
                    ch_id_read = g_io_add_watch(ch_read, G_IO_IN,
                                    (GIOFunc)read_client_cb, NULL);
#if UPNP_DEBUG > 1
                    VERB("* Client accepted");
#endif /* UPNP_DEBUG */
                }
            }
        }
    }
    return TRUE;
}

/**
 * Quit from the main loop when a signal comes.
 *
 * @param signum    Signal number.
 */
static void
terminate(int signum)
{
    UNUSED(signum);

    if (g_main_loop_is_running(main_loop))
        g_main_loop_quit(main_loop);
}

/**
 * Hook for SIGPIPE signal.
 * Do nothing. Catch SIGPIPE signal to avoid termination of process if write
 * operation fails on socket.
 *
 * @param signum    Signal number.
 */
static void
sigpipe_hook(int signum)
{
    UNUSED(signum);
}

/**
 * Remove the source from the default main context.
 *
 * @param source_id     Source ID.
 */
static void
remove_source_from_main_context(guint source_id)
{
    GSource *source;

    if (source_id == 0)
        return;

    source =
        g_main_context_find_source_by_id(g_main_loop_get_context(main_loop),
                                         source_id);
    if (source != NULL)
        g_source_destroy(source);
}

/* See definition in te_upnp_cp.h. */
int
te_upnp_cp(int argc, char *argv[])
{
    GUPnPContext      *context = NULL;
    GUPnPControlPoint *cp = NULL;
    GError            *error = NULL;
    GIOStatus          giost;
    struct sockaddr_un addr;
    const char        *target = NULL;
    const char        *iface = NULL;
    const char        *socket_path = NULL;
    int                res = 0;

    if (argc > 3)
    {
        target = argv[1];
        socket_path = argv[2];
        iface = argv[3];
    }
    else
    {
        ERROR("Usage error, see description in te_upnp_cp.h");
        return -1;
    }

    /* Required initialisation. */
#if !GLIB_CHECK_VERSION(2,35,0)
    /*
     * Initialise the type system only for GLib version < 2.35.
     * Since GLib 2.35, the type system is initialised automatically and
     * g_type_init() is deprecated.
     */
    g_type_init();
#endif /* !GLIB_CHECK_VERSION(2,35,0) */

    /*
     * Create a new GUPnP Context. By here we are using the default GLib
     * main context, and connecting to the current machine's network
     * interface @p iface on an automatically generated port.
     */
    context = gupnp_context_new(NULL, iface, 0, &error);
    if (context == NULL)
    {
        ERROR("Creation of new GUPnP Context was failed with error: %s",
              error->message);
        g_error_free(error);
        return -1;
    }

    /* Create a Control Point with the specified context and target. */
    cp = gupnp_control_point_new(context, target);

    /* Connect to certain signals. */
    g_signal_connect(cp, "device-proxy-available",
                     G_CALLBACK(device_proxy_available_cb), NULL);
    g_signal_connect(cp, "device-proxy-unavailable",
                     G_CALLBACK(device_proxy_unavailable_cb), NULL);
    g_signal_connect(cp, "service-proxy-available",
                     G_CALLBACK(service_proxy_available_cb), NULL);
    g_signal_connect(cp, "service-proxy-unavailable",
                     G_CALLBACK(service_proxy_unavailable_cb), NULL);

    /* Tell the Control Point to start searching. */
    gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cp), TRUE);

    /* Prepare the socket to receive a te_upnp protocol commands. */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    fd_listen = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_listen == -1)
    {
        ERROR("Socket error: %s", strerror(errno));
        return -1;
    }
    if (bind(fd_listen, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        ERROR("Bind error: %s", strerror(errno));
        close_fd(&fd_listen);
        return -1;
    }
    if (listen(fd_listen, 1) == -1)
    {
        ERROR("Listen error: %s", strerror(errno));
        close_fd(&fd_listen);
        return -1;
    }
    ch_listen = g_io_channel_unix_new(fd_listen);
    giost = g_io_channel_set_encoding(ch_listen, NULL, &error);
    if (giost != G_IO_STATUS_NORMAL)
    {
        ERROR("Set channel encoding error: %s", error->message);
        g_error_free(error);
        error = NULL;
        res = -1;
        goto cleanup;
    }
    g_io_channel_set_buffer_size(ch_listen, CHANNEL_BUFFER_SIZE);

    ch_id_listen = g_io_add_watch(ch_listen, G_IO_IN | G_IO_PRI,
                                  (GIOFunc)wait_for_client_cb, NULL);
    /* Because of g_io_add_watch() adds extra reference to channel. */
    g_io_channel_unref(ch_listen);

    /* POSIX signals hooks. */
    signal(SIGTERM, terminate);
    signal(SIGPIPE, sigpipe_hook);

    /*
     * Enter the main loop. This will start the search and result in
     * callbacks.
     */
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

cleanup:
    remove_source_from_main_context(ch_id_read);
    remove_source_from_main_context(ch_id_listen);
    if (ch_read != NULL)
        g_io_channel_unref(ch_read);
    g_io_channel_unref(ch_listen);
    close_fd(&fd_read);
    close_fd(&fd_listen);
    if (remove(socket_path) == -1)
    {
        ERROR("Removing of \"%s\" file fails: %s",
              socket_path, strerror(errno));
    }
    g_main_loop_unref(main_loop);
#if 0
    /** @todo Periodically fails. Why? */
    g_object_unref(cp);
#endif
    g_object_unref(context);
    return res;
}
