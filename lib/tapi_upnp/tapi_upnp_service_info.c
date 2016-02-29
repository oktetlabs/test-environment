/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of Test API for DLNA UPnP Service features.
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
#define TE_LGR_USER     "TAPI UPnP Service Info"

#include "te_config.h"

#include <jansson.h>
#include "logger_api.h"
#include "tapi_upnp_cp.h"
#include "tapi_upnp_service_info.h"
#include "te_string.h"


/** State variable property accessors. */
typedef struct {
    const char *name;
    const char *(*get_value)(const tapi_upnp_state_variable *);
    te_errno    (*set_value)(tapi_upnp_state_variable *, const json_t *);
} upnp_state_variable_property_t;

/** Service property accessors. */
typedef struct {
    const char *name;
    const char *(*get_value)(const tapi_upnp_service_info *);
    te_errno    (*set_value)(tapi_upnp_service_info *, const json_t *);
} upnp_service_property_t;

/** Action argument property accessors. */
typedef struct {
    const char *name;
    const char *(*get_value)(const tapi_upnp_argument *);
    te_errno    (*set_value)(tapi_upnp_argument *, const json_t *,
                             const tapi_upnp_state_variables *);
} upnp_argument_property_t;

/**
 * Set a state variable property string value.
 *
 * @param variable          State variable.
 * @param property_idx      Index of certain property in properties array.
 * @param value             JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
tapi_upnp_set_sv_property_string(tapi_upnp_state_variable *variable,
                          te_upnp_state_variable_property_idx property_idx,
                          const json_t *value)
{
    const char *property;

    if (property_idx >= VPROPERTY_MAX ||
        property_idx == VPROPERTY_SEND_EVENTS ||
        property_idx == VPROPERTY_ALLOWED_VALUES)
    {
        ERROR("Invalid array index");
        return TE_EINVAL;
    }
    if (json_is_null(value))
    {
        variable->properties[property_idx] = NULL;
        return 0;
    }
    property = json_string_value(value);
    if (property == NULL)
    {
        ERROR("Invalid property. JSON string was expected");
        return TE_EINVAL;
    }
    variable->properties[property_idx] = strdup(property);
    return (variable->properties[property_idx] == NULL ? TE_ENOMEM : 0);
}

/**
 * Set a state variable property boolean value.
 *
 * @param variable          State variable.
 * @param property_idx      Index of certain property in properties array.
 * @param value             JSON boolean value to set.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
tapi_upnp_set_sv_property_boolean(tapi_upnp_state_variable *variable,
                          te_upnp_state_variable_property_idx property_idx,
                          const json_t *value)
{
    if (!json_is_boolean(value))
    {
        ERROR("Invalid property. JSON boolean was expected");
        return TE_EINVAL;
    }
    if (property_idx >= VPROPERTY_MAX ||
        property_idx != VPROPERTY_SEND_EVENTS)
    {
        ERROR("Invalid array index");
        return TE_EINVAL;
    }
    variable->properties[property_idx] = malloc(sizeof(te_bool));
    if (variable->properties[property_idx] == NULL)
    {
        ERROR("Cannot allocate memory");
        return TE_ENOMEM;
    }
    *(te_bool*)variable->properties[property_idx] =
                                            (te_bool)json_is_true(value);
    return 0;
}

/**
 * Set a Name state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_name(tapi_upnp_state_variable *variable,
                                  const json_t             *value)
{
    return tapi_upnp_set_sv_property_string(variable, VPROPERTY_NAME,
                                            value);
}

/**
 * Set a Type state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_type(tapi_upnp_state_variable *variable,
                                  const json_t             *value)
{
    return tapi_upnp_set_sv_property_string(variable, VPROPERTY_TYPE,
                                            value);
}

/**
 * Set a Send Events state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_send_events(tapi_upnp_state_variable *variable,
                                         const json_t             *value)
{
    return tapi_upnp_set_sv_property_boolean(variable,
                                             VPROPERTY_SEND_EVENTS,
                                             value);
}

/**
 * Set a Default Value state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_default_value(
                                tapi_upnp_state_variable *variable,
                                const json_t             *value)
{
    return tapi_upnp_set_sv_property_string(variable,
                                            VPROPERTY_DEFAULT_VALUE,
                                            value);
}

/**
 * Set a Minimum state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_minimum(tapi_upnp_state_variable *variable,
                                     const json_t             *value)
{
    return tapi_upnp_set_sv_property_string(variable, VPROPERTY_MINIMUM,
                                            value);
}

/**
 * Set a Maximum state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_maximum(tapi_upnp_state_variable *variable,
                                     const json_t             *value)
{
    return tapi_upnp_set_sv_property_string(variable, VPROPERTY_MAXIMUM,
                                            value);
}

/**
 * Set a Step state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_step(tapi_upnp_state_variable *variable,
                                  const json_t             *value)
{
    return tapi_upnp_set_sv_property_string(variable, VPROPERTY_STEP,
                                            value);
}

/**
 * Set an Allowed Values state variable property.
 *
 * @param variable      State variable.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_state_variable_allowed(tapi_upnp_state_variable *variable,
                                     const json_t             *value)
{
    tapi_upnp_sv_allowed_values *allowed;
    int     rc = 0;
    size_t  i;

    if (value == NULL || !json_is_array(value))
    {
        ERROR("Invalid input data. JSON array was expected");
        return TE_EINVAL;
    }
    allowed = calloc(1, sizeof(*allowed));
    if (allowed == NULL)
    {
        ERROR("Cannot allocate memory");
        return TE_ENOMEM;
    }
    allowed->len = json_array_size(value);
    allowed->values = calloc(allowed->len, sizeof(char *));
    if (allowed->values == NULL)
    {
        ERROR("Cannot allocate memory");
        return TE_ENOMEM;
    }
    for (i = 0; i < allowed->len; i++)
    {
        const char *str = json_string_value(json_array_get(value, i));

        if (str == NULL)
        {
            ERROR("Invalid property. JSON string was expected");
            rc = TE_EINVAL;
            break;
        }
        allowed->values[i] = strdup(str);
        if (allowed->values[i] == NULL)
        {
            ERROR("Invalid property. JSON string was expected");
            rc = TE_ENOMEM;
            break;
        }
    }

    if (rc == 0)
        variable->properties[VPROPERTY_ALLOWED_VALUES] = allowed;
    else
    {
        for (i = 0; i < allowed->len; i++)
            free(allowed->values[i]);
        free(allowed->values);
        free(allowed);
    }

    return rc;
}

/**
 * Get a Send Events flag of the certain state variable in string
 * representation.
 *
 * @param variable      State variable.
 *
 * @return Pointer to the static buffer which contains the string
 *         representation of Send Events flag, or @c NULL if the property
 *         is not set.
 */
static const char *
tapi_upnp_get_state_variable_send_events_to_string(
                                const tapi_upnp_state_variable *variable)
{
    te_bool     send_events;
    te_errno    rc;

    rc = tapi_upnp_get_state_variable_send_events(variable, &send_events);
    if (rc != 0)
        return NULL;
    return (send_events ? "true" : "false");
}

/**
 * Get an Allowed Values array of the certain state variable in string
 * representation. Memory pointed to by returned value must be freed by
 * user.
 *
 * @param variable      State variable.
 *
 * @return Pointer to the dynamic buffer which contains the string
 *         representation of Allowed Values, or @c NULL if the property is
 *         not set.
 */
static char *
tapi_upnp_get_state_variable_allowed_to_string(
                                const tapi_upnp_state_variable *variable)
{
    const tapi_upnp_sv_allowed_values *allowed_vals;
    te_string buf = TE_STRING_INIT;
    size_t    i;

    allowed_vals = tapi_upnp_get_state_variable_allowed(variable);
    if (allowed_vals == NULL)
        return NULL;
    te_string_append(&buf, "");
    for (i = 0; i < allowed_vals->len; i++)
    {
        if (i > 0)
            te_string_append(&buf, ", ");
        te_string_append(&buf, "%s", (char *)allowed_vals->values[i]);
    }
    return buf.ptr;
}

/** State variable's properties accessors (for internal use). */
static upnp_state_variable_property_t variable_property[] = {
    [VPROPERTY_NAME] = { "Name", tapi_upnp_get_state_variable_name,
                                 tapi_upnp_set_state_variable_name },
    [VPROPERTY_TYPE] = { "Type", tapi_upnp_get_state_variable_type,
                                 tapi_upnp_set_state_variable_type },
    [VPROPERTY_SEND_EVENTS] = { "Send Events",
                        tapi_upnp_get_state_variable_send_events_to_string,
                        tapi_upnp_set_state_variable_send_events },
    [VPROPERTY_DEFAULT_VALUE] = { "Default Value",
                        tapi_upnp_get_state_variable_default_value,
                        tapi_upnp_set_state_variable_default_value },
    [VPROPERTY_MINIMUM] = { "Minimum",
                            tapi_upnp_get_state_variable_minimum,
                            tapi_upnp_set_state_variable_minimum },
    [VPROPERTY_MAXIMUM] = { "Maximum",
                            tapi_upnp_get_state_variable_maximum,
                            tapi_upnp_set_state_variable_maximum },
    [VPROPERTY_STEP] = { "Step", tapi_upnp_get_state_variable_step,
                                 tapi_upnp_set_state_variable_step },
    [VPROPERTY_ALLOWED_VALUES] = { "Allowed Values",
                        tapi_upnp_get_state_variable_allowed_to_string,
                        tapi_upnp_set_state_variable_allowed }
};


/**
 * Set a service property string value.
 *
 * @param service           Service context.
 * @param property_idx      Index of certain property in properties array.
 * @param value             JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
tapi_upnp_set_service_property_string(tapi_upnp_service_info *service,
                          te_upnp_service_property_idx property_idx,
                          const json_t *value)
{
    const char *property = json_string_value(value);

    if (property == NULL)
    {
        ERROR("Invalid property. JSON string was expected");
        return TE_EINVAL;
    }
    if (property_idx >= SPROPERTY_MAX)
    {
        ERROR("Invalid array index");
        return TE_EINVAL;
    }
    service->properties[property_idx] = strdup(property);
    return (service->properties[property_idx] == NULL ? TE_ENOMEM : 0);
}

/**
 * Set a Service ID.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_id(tapi_upnp_service_info *service,
                         const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service, SPROPERTY_ID,
                                                 value);
}

/**
 * Set a Service Unique Device Name.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_udn(tapi_upnp_service_info *service,
                          const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service, SPROPERTY_UDN,
                                                 value);
}

/**
 * Set a Service Type.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_type(tapi_upnp_service_info *service,
                           const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service, SPROPERTY_TYPE,
                                                 value);
}

/**
 * Set a Service Location.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_location(tapi_upnp_service_info *service,
                               const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service,
                                                 SPROPERTY_LOCATION,
                                                 value);
}

/**
 * Set a Service Control Protocol Document URL.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_scpd_url(tapi_upnp_service_info *service,
                               const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service,
                                                 SPROPERTY_SCPD_URL,
                                                 value);
}

/**
 * Set a Service Control URL.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_control_url(tapi_upnp_service_info *service,
                                  const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service,
                                                 SPROPERTY_CONTROL_URL,
                                                 value);
}

/**
 * Set a Service Event Subscription URL.
 *
 * @param service       Service context.
 * @param value         JSON string value to set.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_service_event_subscription_url(
                        tapi_upnp_service_info *service,
                        const json_t           *value)
{
    return tapi_upnp_set_service_property_string(service,
                                        SPROPERTY_EVENT_SUBSCRIPTION_URL,
                                        value);
}

/** Service's properties accessors (for internal use). */
static upnp_service_property_t service_property[] = {
    [SPROPERTY_ID] = { "ID", tapi_upnp_get_service_id,
                             tapi_upnp_set_service_id },
    [SPROPERTY_UDN] = { "UDN", tapi_upnp_get_service_udn,
                               tapi_upnp_set_service_udn },
    [SPROPERTY_TYPE] = { "Type", tapi_upnp_get_service_type,
                                 tapi_upnp_set_service_type },
    [SPROPERTY_LOCATION] = { "Location",
                             tapi_upnp_get_service_location,
                             tapi_upnp_set_service_location },
    [SPROPERTY_SCPD_URL] = { "SCPD URL",
                             tapi_upnp_get_service_scpd_url,
                             tapi_upnp_set_service_scpd_url },
    [SPROPERTY_CONTROL_URL] = { "Control URL",
                                tapi_upnp_get_service_control_url,
                                tapi_upnp_set_service_control_url },
    [SPROPERTY_EVENT_SUBSCRIPTION_URL] = { "Event subscription URL",
                        tapi_upnp_get_service_event_subscription_url,
                        tapi_upnp_set_service_event_subscription_url }
};


/**
 * Set an Argument name.
 *
 * @param argument      UPnP action argument.
 * @param value         JSON string value to set.
 * @param variables     List of static variables. It's unused here (can be
 *                      @c NULL).
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_argument_name(tapi_upnp_argument              *argument,
                            const json_t                    *value,
                            const tapi_upnp_state_variables *variables)
{
    UNUSED(variables);
    const char *property = json_string_value(value);

    if (property == NULL)
    {
        ERROR("Invalid property. JSON string was expected");
        return TE_EINVAL;
    }
    argument->name = strdup(property);
    return (argument->name == NULL ? TE_ENOMEM : 0);
}

/**
 * Set an Argument direction.
 *
 * @param argument      UPnP action argument.
 * @param value         JSON string value to set.
 * @param variables     List of static variables. It's unused here (can be
 *                      @c NULL).
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_argument_direction(tapi_upnp_argument              *argument,
                                 const json_t                    *value,
                                 const tapi_upnp_state_variables *variables)
{
    UNUSED(variables);
    json_int_t direction;

    if (!json_is_integer(value))
    {
        ERROR("Invalid property. JSON integer was expected");
        return TE_EINVAL;
    }
    direction = json_integer_value(value);
    if (direction != UPNP_ARG_DIRECTION_IN &&
        direction != UPNP_ARG_DIRECTION_OUT)
    {
        ERROR("Invalid property. Out of range. Here %" JSON_INTEGER_FORMAT
              ", but expected %d or %d", direction,
              UPNP_ARG_DIRECTION_IN, UPNP_ARG_DIRECTION_OUT);
        return TE_EINVAL;
    }
    argument->direction = (te_upnp_arg_direction)direction;
    return 0;
}

/**
 * Set an Argument variable.
 *
 * @param argument      UPnP action argument.
 * @param value         JSON string value to set.
 * @param variables     List of static variables.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_set_argument_variable(tapi_upnp_argument              *argument,
                                const json_t                    *value,
                                const tapi_upnp_state_variables *variables)
{
    const char *variable_name = json_string_value(value);
    tapi_upnp_state_variable *variable;

    if (variable_name == NULL)
    {
        ERROR("Invalid property. JSON string was expected");
        return TE_EINVAL;
    }
    SLIST_FOREACH(variable, variables, next)
    {
        const char *name = tapi_upnp_get_state_variable_name(variable);

        if (name && strcmp(name, variable_name) == 0)
        {
            argument->variable = variable;
            return 0;
        }
    }
    return TE_ENODATA;
}

/**
 * Get an Action direction in string representation.
 *
 * @param argument      UPnP action argument.
 *
 * @return Pointer to the static buffer which contains the string
 *         representation of Action direction.
 */
static const char *
tapi_upnp_get_argument_direction_to_string(
                                const tapi_upnp_argument *argument)
{
    te_upnp_arg_direction direction;

    direction = tapi_upnp_get_argument_direction(argument);
    return (direction == UPNP_ARG_DIRECTION_IN ? "in" : "out");
}

/**
 * Get an Action argument variable name.
 *
 * @param argument      UPnP action argument.
 *
 * @return The Action argument variable name.
 */
static inline const char *
tapi_upnp_get_argument_variable_to_string(
                                const tapi_upnp_argument *argument)
{
    return tapi_upnp_get_state_variable_name(argument->variable);
}

/** Action argument's properties accessors (for internal use). */
static upnp_argument_property_t argument_property[] = {
    [APROPERTY_NAME] = { "Name", tapi_upnp_get_argument_name,
                                 tapi_upnp_set_argument_name },
    [APROPERTY_DIRECTION] = { "Direction",
                              tapi_upnp_get_argument_direction_to_string,
                              tapi_upnp_set_argument_direction },
    [APROPERTY_STATE_VARIABLE] = { "State variable",
                                tapi_upnp_get_argument_variable_to_string,
                                tapi_upnp_set_argument_variable }
};


/**
 * Extract UPnP service's properties from JSON array.
 *
 * @param[in]  jarray       JSON array contains UPnP service properties.
 * @param[out] service      Service context.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_service_properties(const json_t           *jarray,
                         tapi_upnp_service_info *service)
{
    te_errno rc = 0;
    size_t   i;

    if (jarray == NULL || !json_is_array(jarray))
    {
        ERROR("Invalid input data. JSON array was expected");
        return TE_EINVAL;
    }
    for (i = 0; i < TE_ARRAY_LEN(service_property); i++)
    {
        rc = service_property[i].set_value(service,
                                           json_array_get(jarray, i));
        if (rc != 0)
            break;
    }
    if (rc != 0)
    {
        for (i = 0; i < TE_ARRAY_LEN(service->properties); i++)
        {
            free(service->properties[i]);
            service->properties[i] = NULL;
        }
    }
    return rc;
}

/**
 * Extract UPnP service's state variables from JSON array.
 *
 * @param[in]  jarray       JSON array contains UPnP service state
 *                          variables.
 * @param[out] service      Service context.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_service_state_variables(const json_t           *jarray,
                              tapi_upnp_service_info *service)
{
    json_t  *jvariable;
    te_errno rc = 0;
    size_t   i;
    size_t   j;
    size_t   num_variables;
    tapi_upnp_state_variable *variable = NULL;

    if (jarray == NULL || !json_is_array(jarray))
    {
        ERROR("Invalid input data. JSON array was expected");
        return TE_EINVAL;
    }
    num_variables = json_array_size(jarray);
    for (i = 0; i < num_variables; i++)
    {
        jvariable = json_array_get(jarray, i);
        if (jvariable == NULL || !json_is_array(jvariable))
        {
            ERROR("Invalid input data. JSON array was expected");
            rc = TE_EINVAL;
            break;
        }
        variable = calloc(1, sizeof(*variable));
        if (variable == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            break;
        }
        for (j = 0; j < TE_ARRAY_LEN(variable_property); j++)
        {
            rc = variable_property[j].set_value(variable,
                                            json_array_get(jvariable, j));
            if (rc != 0)
                break;
        }
        if (rc != 0)
            break;
        SLIST_INSERT_HEAD(&service->variables, variable, next);
        variable = NULL;
    }
    if (rc != 0)
    {
        tapi_upnp_state_variable *tmp = NULL;
        tapi_upnp_sv_allowed_values *allowed;

        if (variable != NULL)
        {
            for (i = 0; i < TE_ARRAY_LEN(variable->properties); i++)
            {
                if (i == VPROPERTY_ALLOWED_VALUES)
                {
                    allowed = variable->properties[i];
                    if (allowed != NULL)
                        for (j = 0; j < allowed->len; j++)
                            free(allowed->values[j]);
                }
                free(variable->properties[i]);
            }
            free(variable);
        }
        SLIST_FOREACH_SAFE(variable, &service->variables, next, tmp)
        {
            SLIST_REMOVE(&service->variables, variable,
                         tapi_upnp_state_variable, next);
            for (i = 0; i < TE_ARRAY_LEN(variable->properties); i++)
            {
                if (i == VPROPERTY_ALLOWED_VALUES)
                {
                    allowed = variable->properties[i];
                    if (allowed != NULL)
                        for (j = 0; j < allowed->len; j++)
                            free(allowed->values[j]);
                }
                free(variable->properties[i]);
            }
            free(variable);
        }
    }
    return rc;
}

/**
 * Extract UPnP service's actions from JSON object.
 *
 * @param[in]  jobject      JSON object contains UPnP service actions.
 * @param[out] service      Service context.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_service_actions(const json_t *jobject,
                      tapi_upnp_service_info *service)
{
    json_t     *jarg;
    void       *iter;
    const char *key;
    json_t     *value;
    te_errno    rc = 0;
    size_t      i;
    size_t      j;
    tapi_upnp_action   *action;
    tapi_upnp_argument *argument;

    if (jobject == NULL || !json_is_object(jobject))
    {
        ERROR("Invalid input data. JSON object was expected");
        return TE_EINVAL;
    }
    iter = json_object_iter((json_t *)jobject);
    while (iter != NULL)
    {
        /* Action. */
        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
        if (value == NULL || !json_is_array(value))
        {
            ERROR("Invalid input data. JSON array was expected");
            rc = TE_EINVAL;
            break;
        }
        action = calloc(1, sizeof(*action));
        if (action == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            break;
        }
        for (i = 0; i < json_array_size(value); i++)
        {
            /* Argument. */
            jarg = json_array_get(value, i);
            if (jarg == NULL || !json_is_array(jarg))
            {
                ERROR("Invalid input data. JSON array was expected");
                rc = TE_EINVAL;
                break;
            }
            argument = calloc(1, sizeof(*argument));
            if (argument == NULL)
            {
                ERROR("Cannot allocate memory");
                rc = TE_ENOMEM;
                break;
            }
            for (j = 0; j < TE_ARRAY_LEN(argument_property); j++)
            {
                rc = argument_property[j].set_value(argument,
                                                    json_array_get(jarg, j),
                                                    &service->variables);
                if (rc != 0)
                    break;
            }
            if (rc != 0)
                break;
            SLIST_INSERT_HEAD(&action->arguments, argument, next);
            argument = NULL;
        }
        if (rc != 0)
            break;
        action->name = strdup(key);
        if (action->name == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            break;
        }
        SLIST_INSERT_HEAD(&service->actions, action, next);
        action = NULL;
        /* To the next action. */
        iter = json_object_iter_next((json_t *)jobject, iter);
    }
    if (rc != 0)
    {
        tapi_upnp_action   *tmp_act;
        tapi_upnp_argument *tmp_arg;

        if (argument != NULL)
        {
            free((char *)argument->name);
            free(argument);
        }
        if (action != NULL)
        {
            free((char *)action->name);
            SLIST_FOREACH_SAFE(argument, &action->arguments, next, tmp_arg)
            {
                SLIST_REMOVE(&action->arguments, argument,
                             tapi_upnp_argument, next);
                free((char *)argument->name);
                free(argument);
            }
            free(action);
        }
        SLIST_FOREACH_SAFE(action, &service->actions, next, tmp_act)
        {
            SLIST_REMOVE(&service->actions, action,
                         tapi_upnp_action, next);
            free((char *)action->name);
            SLIST_FOREACH_SAFE(argument, &action->arguments, next, tmp_arg)
            {
                SLIST_REMOVE(&action->arguments, argument,
                             tapi_upnp_argument, next);
                free((char *)argument->name);
                free(argument);
            }
            free(action);
        }
    }
    return rc;
}

/**
 * Extract UPnP services from JSON array.
 *
 * @param[in]  jarray       JSON array contains UPnP services.
 * @param[out] services     Services context list.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_services(const json_t *jarray, tapi_upnp_services *services)
{
    json_t  *jobject;
    json_t  *jitem;
    te_errno rc = 0;
    ssize_t  num_services;
    ssize_t  i;
    tapi_upnp_service_info *service = NULL;
    tapi_upnp_service_info *tmp = NULL;

    if (jarray == NULL || !json_is_array(jarray))
    {
        ERROR("Invalid input data. JSON array was expected");
        return TE_EINVAL;
    }
    if (!SLIST_EMPTY(services))
        VERB("Services list is not empty");
    num_services = json_array_size(jarray);
    for (i = 0; i < num_services; i++)
    {
        jobject = json_array_get(jarray, i);
        if (jobject == NULL || !json_is_object(jobject))
        {
            ERROR("Invalid input data. JSON object was expected");
            service = NULL;
            rc = TE_EINVAL;
            break;
        }
        service = calloc(1, sizeof(*service));
        if (service == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            break;
        }
        jitem = json_object_get(jobject, "Parameters");
        rc = parse_service_properties(jitem, service);
        if (rc != 0)
        {
            ERROR("Fail to extract properties");
            break;
        }
        jitem = json_object_get(jobject, "StateVariables");
        rc = parse_service_state_variables(jitem, service);
        if (rc != 0)
        {
            ERROR("Fail to extract state variables");
            break;
        }
        jitem = json_object_get(jobject, "Actions");
        rc = parse_service_actions(jitem, service);
        if (rc != 0)
        {
            ERROR("Fail to extract actions");
            break;
        }
        SLIST_INSERT_HEAD(services, service, next);
    }
    if (rc != 0)
    {
        if (service != NULL)
            free(service);
        SLIST_FOREACH_SAFE(service, services, next, tmp)
        {
            SLIST_REMOVE(services, service, tapi_upnp_service_info, next);
            free(service);
        }
    }
    return rc;
}

/**
 * Extract results of invoked action on certain UPnP service from JSON
 * object.
 *
 * @param[in]    jin        JSON object contains results of invoked action.
 * @param[inout] action     Action context.
 *
 * @return Status code. On success, @c 0.
 */
static te_errno
parse_action(const json_t *jin, tapi_upnp_action *action)
{
    json_t     *jarg;
    void       *iter;
    const char *key;
    const char *value;
    char       *str;
    te_errno    rc = 0;
    tapi_upnp_argument *argument;

    if (jin == NULL || !json_is_object(jin))
    {
        ERROR("Invalid input data. JSON object was expected");
        return TE_EINVAL;
    }
    if (action == NULL)
    {
        ERROR("Invalid input data. Action is not provided");
        return TE_EINVAL;
    }
    str = (char *)json_string_value(json_object_get(jin, "name"));
    if (str == NULL || strcmp(str, action->name) != 0)
    {
        ERROR("Unexpected action name");
        return TE_EINVAL;
    }
    jarg = json_object_get(jin, "out");
    if (jarg == NULL || !json_is_object(jarg))
    {
        ERROR("Invalid input data. JSON object was expected");
        return TE_EINVAL;
    }

    iter = json_object_iter((json_t *)jarg);
    while (iter != NULL)
    {
        /* Action. */
        key = json_object_iter_key(iter);
        value = json_string_value(json_object_iter_value(iter));
        if (value == NULL)
        {
            ERROR("Invalid input data. JSON string was expected");
            return TE_EINVAL;
        }
        SLIST_FOREACH(argument, &action->arguments, next)
        {
            str = (char *)tapi_upnp_get_argument_name(argument);
            if (strcmp(str, key) == 0)
            {
                tapi_upnp_set_argument_value(argument, value);
                break;
            }
        }
        /* To the next action. */
        iter = json_object_iter_next((json_t *)jarg, iter);
    }

    return rc;
}


/* See description in tapi_upnp_service_info.h. */
te_errno
tapi_upnp_set_argument_value(tapi_upnp_argument *argument,
                             const char         *value)
{
    free(argument->value);
    if (value == NULL)
    {
        argument->value = NULL;
        return 0;
    }
    argument->value = strdup(value);
    if (argument->value == NULL)
    {
        ERROR("Cannot allocate memory");
        return TE_ENOMEM;
    }
    return 0;
}

/* See description in tapi_upnp_service_info.h. */
const tapi_upnp_sv_allowed_values *
tapi_upnp_get_state_variable_allowed(
                            const tapi_upnp_state_variable *variable)
{
    return (tapi_upnp_sv_allowed_values *)variable->
                                       properties[VPROPERTY_ALLOWED_VALUES];
}

/* See description in tapi_upnp_service_info.h. */
const char *
tapi_upnp_get_state_variable_property_string(
                        const tapi_upnp_state_variable     *variable,
                        te_upnp_state_variable_property_idx property_idx)
{
    return (property_idx < VPROPERTY_MAX &&
            property_idx != VPROPERTY_SEND_EVENTS &&
            property_idx != VPROPERTY_ALLOWED_VALUES
            ? (const char *)variable->properties[property_idx]
            : NULL);
}

/* See description in tapi_upnp_service_info.h. */
te_errno
tapi_upnp_get_state_variable_property_boolean(
                        const tapi_upnp_state_variable     *variable,
                        te_upnp_state_variable_property_idx property_idx,
                        te_bool                            *value)
{
    if (property_idx >= VPROPERTY_MAX ||
        property_idx != VPROPERTY_SEND_EVENTS)
        return TE_EINVAL;
    if (variable->properties[property_idx] == NULL)
        return TE_ENODATA;
    *value = *(te_bool *)variable->properties[property_idx];
    return 0;
}

/* See description in tapi_upnp_service_info.h. */
const char *
tapi_upnp_get_service_property_string(const tapi_upnp_service_info *service,
                                te_upnp_service_property_idx property_idx)
{
    return (property_idx < SPROPERTY_MAX
            ? (const char *)service->properties[property_idx]
            : NULL);
}

/* See description in tapi_upnp_service_info.h. */
te_errno
tapi_upnp_get_service_info(rcf_rpc_server        *rpcs,
                           tapi_upnp_device_info *device,
                           const char            *service_id,
                           tapi_upnp_services    *services)
{
    json_error_t error;
    json_t      *jrequest = NULL;
    json_t      *jreply = NULL;
    char        *request;
    char        *reply = NULL;
    size_t       reply_len = 0;
    te_upnp_cp_request_type reply_type;
    const char  *device_udn;
    te_errno     rc;

    if (service_id == NULL)
        service_id = "";
    if (device == NULL)
        device_udn = "";
    else
        device_udn = tapi_upnp_get_device_udn(device);

    /* Prepare request. */
    jrequest = json_pack("[i,s,s]", UPNP_CP_REQUEST_SERVICE, device_udn,
                         service_id);
    if (jrequest == NULL)
    {
        ERROR("json_pack fails");
        return TE_ENOMEM;
    }
    request = json_dumps(jrequest, JSON_ENCODING_FLAGS);
    if (request == NULL)
    {
        ERROR("json_dumps fails");
        json_decref(jrequest);
        return TE_ENOMEM;
    }

    /* Send request. */
    rc = rpc_upnp_cp_action(rpcs, request, strlen(request) + 1,
                            (void **)&reply, &reply_len);
    if (rc != 0)
        goto get_service_info_cleanup;

    /* Parse reply. */
    jreply = json_loads(reply, 0, &error);
    if (jreply == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        rc = TE_EINVAL;
        goto get_service_info_cleanup;
    }
    if (!json_is_integer(json_array_get(jreply, 0)))
    {
        ERROR("Invalid reply type. JSON integer was expected");
        rc = TE_EINVAL;
        goto get_service_info_cleanup;
    }
    reply_type = json_integer_value(json_array_get(jreply, 0));
    if (reply_type != UPNP_CP_REQUEST_SERVICE)
    {
        ERROR("Unexpected reply type");
        rc = TE_EINVAL;
        goto get_service_info_cleanup;
    }

    rc = parse_services(json_array_get(jreply, 1), services);
    if (rc != 0)
    {
        ERROR("parse_services fails");
        tapi_upnp_free_service_info(services);
    }

get_service_info_cleanup:
    free(reply);
    free(request);
    json_decref(jreply);
    json_decref(jrequest);
    return rc;
}

/* See description in tapi_upnp_service_info.h. */
void
tapi_upnp_free_service_info(tapi_upnp_services *services)
{
    tapi_upnp_service_info *service, *tmp_service;
    tapi_upnp_state_variable *variable, *tmp_variable;
    tapi_upnp_action *action, *tmp_action;
    tapi_upnp_argument *argument, *tmp_argument;
    tapi_upnp_sv_allowed_values *allowed;
    size_t i, j;

    SLIST_FOREACH_SAFE(service, services, next, tmp_service)
    {
        SLIST_REMOVE(services, service, tapi_upnp_service_info, next);
        SLIST_FOREACH_SAFE(action, &service->actions, next, tmp_action)
        {
            SLIST_REMOVE(&service->actions, action,
                         tapi_upnp_action, next);
            free((char *)action->name);
            SLIST_FOREACH_SAFE(argument, &action->arguments, next,
                               tmp_argument)
            {
                SLIST_REMOVE(&action->arguments, argument,
                             tapi_upnp_argument, next);
                free((char *)argument->name);
                free(argument->value);
                free(argument);
            }
            free(action);
        }
        SLIST_FOREACH_SAFE(variable, &service->variables, next,
                           tmp_variable)
        {
            SLIST_REMOVE(&service->variables, variable,
                         tapi_upnp_state_variable, next);
            for (i = 0; i < TE_ARRAY_LEN(variable->properties); i++)
            {
                if (i == VPROPERTY_ALLOWED_VALUES)
                {
                    allowed = variable->properties[i];
                    if (allowed != NULL)
                        for (j = 0; j < allowed->len; j++)
                            free(allowed->values[j]);
                }
                free(variable->properties[i]);
            }
            free(variable);
        }
        for (i = 0; i < TE_ARRAY_LEN(service->properties); i++)
            free(service->properties[i]);
        free(service);
    }
}

/* See description in tapi_upnp_service_info.h. */
te_errno
tapi_upnp_invoke_action(rcf_rpc_server               *rpcs,
                        const tapi_upnp_service_info *service,
                        tapi_upnp_action             *action)
{
    json_error_t error;
    json_t      *jrequest = NULL;
    json_t      *jaction = NULL;
    json_t      *jin = NULL;
    json_t      *jout = NULL;
    json_t      *jreply = NULL;
    json_t      *jobj = NULL;
    char        *request = NULL;
    char        *reply = NULL;
    char        *str = NULL;
    size_t       reply_len = 0;
    te_upnp_cp_request_type reply_type;
    int          rc = 0;
    const char  *service_id;
    const char  *service_udn;
    tapi_upnp_argument *argument;

#define ACT_JSON_ARRAY_CREATE(_arr) \
    JSON_ARRAY_CREATE(_arr, rc, invoke_action_cleanup)
#define ACT_JSON_ARRAY_APPEND_NEW(_arr, _val) \
    JSON_ARRAY_APPEND_NEW(_arr, _val, rc, invoke_action_cleanup)
#define ACT_JSON_ARRAY_APPEND(_arr, _val) \
    JSON_ARRAY_APPEND(_arr, _val, rc, invoke_action_cleanup)

#define ACT_JSON_OBJECT_CREATE(_obj) \
    JSON_OBJECT_CREATE(_obj, rc, invoke_action_cleanup)
#define ACT_JSON_OBJECT_SET_NEW(_obj, _key, _val) \
    JSON_OBJECT_SET_NEW(_obj, _key, _val, rc, invoke_action_cleanup)
#define ACT_JSON_OBJECT_SET(_obj, _key, _val) \
    JSON_OBJECT_SET(_obj, _key, _val, rc, invoke_action_cleanup)

    /* Check for correctness of input arguments. */
    if (service == NULL || action == NULL)
        return TE_EINVAL;
    service_id = tapi_upnp_get_service_id(service);
    service_udn = tapi_upnp_get_service_udn(service);
    if (service_id == NULL || service_udn == NULL)
    {
        ERROR("Service ID or UDN is not given");
        return TE_EINVAL;
    }

    /* Prepare request. */
    ACT_JSON_ARRAY_CREATE(jrequest);
    ACT_JSON_ARRAY_APPEND_NEW(jrequest,
                              json_integer(UPNP_CP_REQUEST_ACTION));
    ACT_JSON_OBJECT_CREATE(jin);
    ACT_JSON_ARRAY_CREATE(jout);
    SLIST_FOREACH(argument, &action->arguments, next)
    {
        if (argument->direction == UPNP_ARG_DIRECTION_IN)
        {
            if (argument == NULL || argument->value == NULL)
            {
                ERROR("Invalid IN argument");
                rc = TE_EINVAL;
                goto invoke_action_cleanup;
            }
            /**
             * @todo Perhaps it is need to check for valid value using
             * allowed values list.
             */
            ACT_JSON_OBJECT_SET_NEW(jin, argument->name,
                                    json_string(argument->value));
        }
        else /* if (argument->direction == TAPI_UPNP_ARG_DIRECTION_OUT) */
        {
            ACT_JSON_ARRAY_APPEND_NEW(jout, json_string(argument->name));
        }
    }
    ACT_JSON_OBJECT_CREATE(jaction);
    ACT_JSON_OBJECT_SET_NEW(jaction, "udn", json_string(service_udn));
    ACT_JSON_OBJECT_SET_NEW(jaction, "id", json_string(service_id));
    ACT_JSON_OBJECT_SET_NEW(jaction, "name", json_string(action->name));
    ACT_JSON_OBJECT_SET(jaction, "in", jin);
    ACT_JSON_OBJECT_SET(jaction, "out", jout);

    ACT_JSON_ARRAY_APPEND(jrequest, jaction);

    request = json_dumps(jrequest, JSON_ENCODING_FLAGS);
    if (request == NULL)
    {
        ERROR("json_dumps fails");
        rc = TE_EFMT;
        goto invoke_action_cleanup;
    }

    /* Send request. */
    rc = rpc_upnp_cp_action(rpcs, request, strlen(request) + 1,
                            (void **)&reply, &reply_len);
    if (rc != 0)
        goto invoke_action_cleanup;

    /* Parse reply. */
    jreply = json_loads(reply, 0, &error);
    if (jreply == NULL)
    {
        ERROR("json_loads fails with massage: \"%s\", position: %u",
              error.text, error.position);
        rc = TE_EFMT;
        goto invoke_action_cleanup;
    }
    if (!json_is_integer(json_array_get(jreply, 0)))
    {
        ERROR("Invalid reply type. JSON integer was expected");
        rc = TE_EINVAL;
        goto invoke_action_cleanup;
    }
    reply_type = json_integer_value(json_array_get(jreply, 0));
    if (reply_type != UPNP_CP_REQUEST_ACTION)
    {
        ERROR("Unexpected reply type");
        rc = TE_EINVAL;
        goto invoke_action_cleanup;
    }

    /* Check for valid of reply. */
    jobj = json_array_get(jreply, 1);
    if (!json_is_object(jobj))
    {
        ERROR("Invalid reply message. JSON object was expected");
        rc = TE_EINVAL;
        goto invoke_action_cleanup;
    }
    str = (char *)json_string_value(json_object_get(jobj, "udn"));
    if (str == NULL || strcmp(str, service_udn) != 0)
    {
        ERROR("Unexpected UDN passed from reply message");
        rc = TE_EINVAL;
        goto invoke_action_cleanup;
    }
    str = (char *)json_string_value(json_object_get(jobj, "id"));
    if (str == NULL || strcmp(str, service_id) != 0)
    {
        ERROR("Unexpected service ID passed from reply message");
        rc = TE_EINVAL;
        goto invoke_action_cleanup;
    }

    rc = parse_action(json_array_get(jreply, 1), action);

invoke_action_cleanup:
    free(reply);
    free(request);
    json_decref(jin);
    json_decref(jout);
    json_decref(jaction);
    json_decref(jreply);
    json_decref(jrequest);

    return rc;

#undef ACT_JSON_ARRAY_CREATE
#undef ACT_JSON_ARRAY_APPEND_NEW
#undef ACT_JSON_ARRAY_APPEND
#undef ACT_JSON_OBJECT_CREATE
#undef ACT_JSON_OBJECT_SET_NEW
#undef ACT_JSON_OBJECT_SET
}

/* See description in tapi_upnp_service_info.h. */
void
tapi_upnp_print_service_info(const tapi_upnp_services *services)
{
    size_t    i;
    size_t    num_services = 0;
    te_string dump = TE_STRING_INIT;
    tapi_upnp_service_info   *service = NULL;
    tapi_upnp_state_variable *variable = NULL;
    tapi_upnp_action         *action = NULL;
    tapi_upnp_argument       *argument = NULL;

    if (SLIST_EMPTY(services))
    {
        VERB("List of services is empty");
        return;
    }

    SLIST_FOREACH(service, services, next)
    {
        num_services++;
        te_string_append(&dump, "[\n");
        /* Service properties. */
        for (i = 0; i < TE_ARRAY_LEN(service_property); i++)
            te_string_append(&dump, " %s: %s\n",
                             service_property[i].name,
                             service_property[i].get_value(service));
        /* Service state variables. */
        te_string_append(&dump, " variables {\n");
        SLIST_FOREACH(variable, &service->variables, next)
        {
            te_string_append(&dump, "  [\n");
            for (i = 0; i < TE_ARRAY_LEN(variable_property); i++)
                te_string_append(&dump, "   %s: %s\n",
                                 variable_property[i].name,
                                 variable_property[i].get_value(variable));
            te_string_append(&dump, "  ],\n");
        }
        te_string_append(&dump, " },\n");
        /* Service actions. */
        te_string_append(&dump, " actions {\n");
        SLIST_FOREACH(action, &service->actions, next)
        {
            te_string_append(&dump, "  %s: {\n", action->name);
            /* Service action arguments. */
            SLIST_FOREACH(argument, &action->arguments, next)
            {
                for (i = 0; i < TE_ARRAY_LEN(argument_property); i++)
                    te_string_append(&dump, "   %s: %s\n",
                                argument_property[i].name,
                                argument_property[i].get_value(argument));
                te_string_append(&dump, "   Value: %s\n",
                                 (argument->value ? argument->value : ""));
            }
            te_string_append(&dump, "  },\n");
        }
        te_string_append(&dump, " }\n");
        te_string_append(&dump, "],\n");
    }
    te_string_append(&dump, "---\n");
    te_string_append(&dump, "Total number of services: %u\n", num_services);
    VERB(dump.ptr);
    te_string_free(&dump);
}
