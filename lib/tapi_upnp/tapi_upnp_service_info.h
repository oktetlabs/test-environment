/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Definition of Test API for DLNA UPnP Service features.
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TAPI_UPNP_SERVICE_INFO_H__
#define __TAPI_UPNP_SERVICE_INFO_H__

#include "rcf_rpc.h"
#include "tapi_upnp_device_info.h"


#ifdef __cplusplus
extern "C" {
#endif


/* State variable. */
/** UPnP state variable allowed values. */
typedef struct tapi_upnp_sv_allowed_values {
    char  **values;
    size_t  len;
} tapi_upnp_sv_allowed_values;

/** UPnP state variable parameters. */
typedef struct tapi_upnp_state_variable {
    void *properties[VPROPERTY_MAX];
    SLIST_ENTRY(tapi_upnp_state_variable) next;
} tapi_upnp_state_variable;

/** Head of the UPnP state variables list for particular service. */
typedef SLIST_HEAD(tapi_upnp_state_variables, tapi_upnp_state_variable)
    tapi_upnp_state_variables;

/* Argument. */
/**
 * UPnP action argument parameters.
 * Note! The only changable field is @b value. It can be set by user to
 * specify a value or read to get the value after an action execution,
 * other data is filled when service context is retrieved.
 */
typedef struct tapi_upnp_argument {
    const char               *name;
    te_upnp_arg_direction     direction;
    tapi_upnp_state_variable *variable; /**< Pointer to appropriate item in
                                             variables list (See
                                             tapi_upnp_service_info). */
    char                     *value; /**< IN/OUT value, set @c NULL to left
                                          the variable unspecified. */
    SLIST_ENTRY(tapi_upnp_argument) next;
} tapi_upnp_argument;

/** Head of the UPnP arguments list for particular service action. */
typedef SLIST_HEAD(tapi_upnp_action_arguments, tapi_upnp_argument)
    tapi_upnp_action_arguments;

/* Action. */
/** UPnP action. */
typedef struct tapi_upnp_action {
    const char                   *name;
    tapi_upnp_action_arguments    arguments;
    SLIST_ENTRY(tapi_upnp_action) next;
} tapi_upnp_action;

/** Head of the UPnP actions list for particular service. */
typedef SLIST_HEAD(tapi_upnp_actions, tapi_upnp_action)
    tapi_upnp_actions;

/* Service Info. */
/** UPnP service information. */
typedef struct tapi_upnp_service_info {
    void                               *properties[SPROPERTY_MAX];
    tapi_upnp_state_variables           variables;
    tapi_upnp_actions                   actions;
    SLIST_ENTRY(tapi_upnp_service_info) next;
} tapi_upnp_service_info;

/** Head of the UPnP services list. */
typedef SLIST_HEAD(tapi_upnp_services, tapi_upnp_service_info)
    tapi_upnp_services;


/* State variable properties. */

/**
 * Get a state variable property string value.
 *
 * @param variable          State variable.
 * @param property_idx      Index of certain property in properties array.
 *
 * @return The string property value or @c NULL if the property is not
 *         found.
 */
const char *tapi_upnp_get_state_variable_property_string(
                        const tapi_upnp_state_variable     *variable,
                        te_upnp_state_variable_property_idx property_idx);

/**
 * Get a state variable property boolean value.
 *
 * @param[in]  variable     State variable.
 * @param[in]  property_idx Index of certain property in properties array.
 * @param[out] value        Requested value.
 *
 * @return Status code. On success, @c 0.
 */
te_errno tapi_upnp_get_state_variable_property_boolean(
                        const tapi_upnp_state_variable     *variable,
                        te_upnp_state_variable_property_idx property_idx,
                        te_bool                            *value);

/**
 * Get a Name of the certain state variable.
 *
 * @param variable      State variable.
 *
 * @return The Name of state variable.
 */
static inline const char *
tapi_upnp_get_state_variable_name(const tapi_upnp_state_variable *variable)
{
    return tapi_upnp_get_state_variable_property_string(variable,
                                                        VPROPERTY_NAME);
}

/**
 * Get a Type of the certain state variable.
 *
 * @param variable      State variable.
 *
 * @return The Type of state variable.
 */
static inline const char *
tapi_upnp_get_state_variable_type(const tapi_upnp_state_variable *variable)
{
    return tapi_upnp_get_state_variable_property_string(variable,
                                                        VPROPERTY_TYPE);
}

/**
 * Get a Send Events flag of the certain state variable.
 *
 * @param[in]  variable     State variable.
 * @param[out] value        Value to return.
 *
 * @return Status code. On success, @c 0.
 */
static inline te_errno
tapi_upnp_get_state_variable_send_events(
                            const tapi_upnp_state_variable *variable,
                            te_bool *value)
{
    return tapi_upnp_get_state_variable_property_boolean(variable,
                                                  VPROPERTY_SEND_EVENTS,
                                                  value);
}

/**
 * Get a Default Value of the certain state variable.
 *
 * @param variable      State variable.
 *
 * @return The Default Value of state variable.
 */
static inline const char *
tapi_upnp_get_state_variable_default_value(
                            const tapi_upnp_state_variable *variable)
{
    return tapi_upnp_get_state_variable_property_string(variable,
                                                VPROPERTY_DEFAULT_VALUE);
}

/**
 * Get a Minimum value of the certain state variable. c@ NULL is valid value
 * for non-numerical state variable.
 *
 * @param variable      State variable.
 *
 * @return The Minimum value of state variable.
 */
static inline const char *
tapi_upnp_get_state_variable_minimum(
                            const tapi_upnp_state_variable *variable)
{
    return tapi_upnp_get_state_variable_property_string(variable,
                                                        VPROPERTY_MINIMUM);
}

/**
 * Get a Maximum value of the certain state variable. c@ NULL is valid value
 * for non-numerical state variable.
 *
 * @param variable      State variable.
 *
 * @return The Maximum value of state variable.
 */
static inline const char *
tapi_upnp_get_state_variable_maximum(
                            const tapi_upnp_state_variable *variable)
{
    return tapi_upnp_get_state_variable_property_string(variable,
                                                        VPROPERTY_MAXIMUM);
}

/**
 * Get a Step value of the certain state variable. c@ NULL is valid value
 * for non-numerical state variable.
 *
 * @param variable      State variable.
 *
 * @return The Step value of state variable.
 */
static inline const char *
tapi_upnp_get_state_variable_step(const tapi_upnp_state_variable *variable)
{
    return tapi_upnp_get_state_variable_property_string(variable,
                                                        VPROPERTY_STEP);
}

/**
 * Get a Allowed Values array of the certain state variable.
 *
 * @param variable      State variable.
 *
 * @return The Allowed Values array of state variable.
 */
const tapi_upnp_sv_allowed_values *tapi_upnp_get_state_variable_allowed(
                            const tapi_upnp_state_variable *variable);


/* Service properties. */

/**
 * Get a service property string value.
 *
 * @param service           Service context.
 * @param property_idx      Index of certain property in properties array.
 *
 * @return The string property value or @c NULL if the property is not
 *         found.
 */
const char *tapi_upnp_get_service_property_string(
                        const tapi_upnp_service_info *service,
                        te_upnp_service_property_idx  property_idx);

/**
 * Get a Service ID.
 *
 * @param service       Service context.
 *
 * @return The Service ID.
 */
static inline const char *
tapi_upnp_get_service_id(const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service, SPROPERTY_ID);
}

/**
 * Get a Service Unique Device Name.
 *
 * @param service       Service context.
 *
 * @return The Service Unique Device Name.
 */
static inline const char *
tapi_upnp_get_service_udn(const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service, SPROPERTY_UDN);
}

/**
 * Get a Service Type.
 *
 * @param service       Service context.
 *
 * @return The Service Type.
 */
static inline const char *
tapi_upnp_get_service_type(const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service, SPROPERTY_TYPE);
}

/**
 * Get a Service Location.
 *
 * @param service       Service context.
 *
 * @return The Service Location.
 */
static inline const char *
tapi_upnp_get_service_location(const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service,
                                                 SPROPERTY_LOCATION);
}

/**
 * Get a Service Control Protocol Document URL.
 *
 * @param service       Service context.
 *
 * @return The Service Control Protocol Document URL.
 */
static inline const char *
tapi_upnp_get_service_scpd_url(const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service,
                                                 SPROPERTY_SCPD_URL);
}

/**
 * Get a Service Control URL.
 *
 * @param service       Service context.
 *
 * @return The Service Control URL.
 */
static inline const char *
tapi_upnp_get_service_control_url(const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service,
                                                 SPROPERTY_CONTROL_URL);
}

/**
 * Get a Service Event Subscription URL.
 *
 * @param service       Service context.
 *
 * @return The Service Event Subscription URL.
 */
static inline const char *
tapi_upnp_get_service_event_subscription_url(
                        const tapi_upnp_service_info *service)
{
    return tapi_upnp_get_service_property_string(service,
                                        SPROPERTY_EVENT_SUBSCRIPTION_URL);
}


/* Argument properties. */

/**
 * Get an Argument name.
 *
 * @param argument      UPnP action argument.
 *
 * @return The Argument name.
 */
static inline const char *
tapi_upnp_get_argument_name(const tapi_upnp_argument *argument)
{
    return argument->name;
}

/**
 * Get an Argument direction.
 *
 * @param argument      UPnP action argument.
 *
 * @return The Argument direction.
 */
static inline te_upnp_arg_direction
tapi_upnp_get_argument_direction(const tapi_upnp_argument *argument)
{
    return argument->direction;
}

/**
 * Get an Argument variable.
 *
 * @param argument      UPnP action argument.
 *
 * @return The Argument variable.
 */
static inline const tapi_upnp_state_variable *
tapi_upnp_get_argument_variable(const tapi_upnp_argument *argument)
{
    return argument->variable;
}

/**
 * Get an Argument value.
 *
 * @param argument      UPnP action argument.
 *
 * @return The Argument value.
 */
static inline const char *
tapi_upnp_get_argument_value(const tapi_upnp_argument *argument)
{
    return argument->value;
}

/**
 * Set an Argument value. Note, previous set value will be freed, i.e. it is
 * no need to free argument before to set it.
 *
 * @param argument      UPnP action argument.
 * @param value         Null-terminated string value to set, or @c NULL to
 *                      free argument and set it to @c NULL.
 *
 * @return Status code. On success, @c 0.
 */
te_errno tapi_upnp_set_argument_value(tapi_upnp_argument *argument,
                                      const char         *value);

/* Services. */

/**
 * Retrieve information about available UPnP services.
 * The posted @p services should be empty, otherwise the new services will
 * be appended to the it and there is no garantee that the list will
 * contains no duplicates.
 *
 * @param[in]    rpcs         RPC server handle.
 * @param[in]    device       The device which provides the service, can be
 *                            unspecified (@c NULL).
 * @param[in]    service_id   Service ID string or @c NULL.
 * @param[inout] services     Service context list.
 *
 * @return Status code. On success, @c 0.
 */
extern te_errno tapi_upnp_get_service_info(
                                        rcf_rpc_server        *rpcs,
                                        tapi_upnp_device_info *device,
                                        const char            *service_id,
                                        tapi_upnp_services    *services);

/**
 * Empty the list of UPnP services (free allocated memory).
 *
 * @param services      List of services.
 */
extern void tapi_upnp_free_service_info(tapi_upnp_services *services);

/**
 * Execute a UPnP action. Note! @b IN arguments @b value of the @p action
 * should be initialized by user or set to @c NULL, @b OUT arguments
 * @b value must be initialized with @c NULL.
 *
 * @param[in]    rpcs       RPC server handle.
 * @param[in]    service    Service context.
 * @param[inout] action     Requested action to invoke.
 *
 * @return Status code.
 */
extern te_errno tapi_upnp_invoke_action(
                                    rcf_rpc_server               *rpcs,
                                    const tapi_upnp_service_info *service,
                                    tapi_upnp_action             *action);

/**
 * Print UPnP services context using VERB function.
 * This function should be used for debugging purpose.
 *
 * @param services      Services context list.
 */
extern void tapi_upnp_print_service_info(
                            const tapi_upnp_services *services);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_SERVICE_INFO_H__ */
