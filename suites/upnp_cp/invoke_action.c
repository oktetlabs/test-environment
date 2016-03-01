/** @file
 * @brief UPnP Control Point Test Suite
 *
 * Test to invoke UPnP action.
 *
 * Copyright (C) 2016 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

/** @page upnp_cp-invoke_action Test to invoke UPnP action
 *
 * @objective Invoke the particular action on certain UPnP service.
 *
 * @param service_id        UPnP service ID.
 * @param action_name       Name of action to invoke.
 * @param in_arg_name       Name of IN argument.
 * @param in_arg_value      Value to set.
 *
 * @par Scenario
 * -# Get available services.
 * -# Find out service with @p service_id ID.
 * -# Find out action with @p action_name name.
 * -# Set the argument with @p in_arg_name name to @p in_arg_value value.
 * -# Invoke the action.
 * -# Print the services info with action results (values).
 */

#include "te_config.h"
#include "upnp_cp_ts.h"
#include "te_errno.h"
#include "tapi_test.h"
#include "rcf_rpc.h"
#include "tapi_upnp.h"
#include "tapi_upnp_cp.h"
#include "tapi_upnp_service_info.h"


#define TE_TEST_NAME "upnp_cp/invoke_action"


int
main(int argc, char **argv)
{
    static tapi_upnp_services services;
    rcf_rpc_server           *pco_iut = NULL;
    const char               *service_id = NULL;
    const char               *action_name = NULL;
    const char               *in_arg_name = NULL;
    const char               *in_arg_value = NULL;
    tapi_upnp_service_info   *service = NULL;
    tapi_upnp_action         *action = NULL;
    tapi_upnp_argument       *argument = NULL;

    TEST_START;
    TEST_GET_STRING_PARAM(service_id);
    TEST_GET_STRING_PARAM(action_name);
    TEST_GET_STRING_PARAM(in_arg_name);
    TEST_GET_STRING_PARAM(in_arg_value);
    TEST_GET_PCO(pco_iut);

    SLIST_INIT(&services);
    rpc_upnp_cp_connect(pco_iut);

    /* Get available services. */
    CHECK_RC(
        tapi_upnp_get_service_info(pco_iut, NULL, service_id, &services));
    tapi_upnp_print_service_info(&services);
    service = SLIST_FIRST(&services);
    if (service == NULL)
        TEST_VERDICT("UPnP service with id=\"%s\" not found", service_id);

    /* Search for particular action. */
    SLIST_FOREACH(action, &service->actions, next)
    {
        if (strcmp(action->name, action_name) == 0)
        {
            /* Assign IN arguments. */
            SLIST_FOREACH(argument, &action->arguments, next)
            {
                const char *name;
                te_upnp_arg_direction dir;

                dir = tapi_upnp_get_argument_direction(argument);
                if (dir != UPNP_ARG_DIRECTION_IN)
                    continue;

                name = tapi_upnp_get_argument_name(argument);
                if (strcmp(name, in_arg_name) == 0)
                {
                    tapi_upnp_set_argument_value(argument, in_arg_value);
                    break;
                }
            }
            break;
        }
    }
    if (action == NULL)
        TEST_VERDICT("UPnP action \"%s\" not found in service \"%s\"",
                     action_name, service_id);

    /* Invoke the action. */
    rc = tapi_upnp_invoke_action(pco_iut, service, action);
    if (rc != 0)
        TEST_VERDICT("UPnP \"%s(%s->%s)\" action invoke is failed",
                     action_name, in_arg_name, in_arg_value);

    /* Print the services info with action results (values). */
    tapi_upnp_print_service_info(&services);

    TEST_SUCCESS;

cleanup:
    rpc_upnp_cp_disconnect(pco_iut);
    tapi_upnp_free_service_info(&services);
    TEST_END;
}
