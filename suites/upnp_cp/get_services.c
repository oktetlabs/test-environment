/** @file
 * @brief UPnP Control Point Test Suite
 *
 * Test to get UPnP services.
 *
 * Copyright (C) 2016 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

/** @page upnp_cp-get_services Test to get UPnP services
 *
 * @objective Get and print the list of UPnP services.
 *
 * @param device_name   UPnP device friendly name.
 * @param service_id    UPnP service ID.
 *
 * @par Scenario
 * -# Connect to UPnP Control Point.
 * -# Get UPnP device(s) info acording @p device_name.
 * -# Print UPnP device(s) info.
 * -# Get UPnP service(s) info acording @p device_name and
 *    @p service_id
 * -# Print UPnP service(s) info.
 * -# Free UPnP device(s) info.
 * -# Free UPnP service(s) info.
 * -# Disconnect from UPnP Control Point.
 */

#include "te_config.h"
#include "upnp_cp_ts.h"
#include "tapi_test.h"
#include "rcf_rpc.h"
#include "tapi_upnp.h"
#include "tapi_upnp_cp.h"
#include "tapi_upnp_service_info.h"


#define TE_TEST_NAME "upnp_cp/get_services"


int
main(int argc, char **argv)
{
    static tapi_upnp_devices  devices;
    static tapi_upnp_services services;
    rcf_rpc_server           *pco_iut = NULL;
    const char               *device_name = NULL;
    const char               *service_id = NULL;
    tapi_upnp_device_info    *device = NULL;

    TEST_START;
    TEST_GET_STRING_PARAM(device_name);
    TEST_GET_STRING_PARAM(service_id);
    TEST_GET_PCO(pco_iut);

    if (strcmp(device_name, "all") == 0)
        device_name = "";    /* or device_name = NULL */
    if (strcmp(service_id, "all") == 0)
        service_id = "";    /* or service_id = NULL */

    SLIST_INIT(&devices);
    SLIST_INIT(&services);

    rpc_upnp_cp_connect(pco_iut);

    if (strlen(device_name) > 0)
    {
        CHECK_RC(tapi_upnp_get_device_info(pco_iut, device_name, &devices));
        tapi_upnp_print_device_info(&devices);
        device = SLIST_FIRST(&devices);
        if (device == NULL)
            TEST_VERDICT("Specified device is not found");
    }

    CHECK_RC(
        tapi_upnp_get_service_info(pco_iut, device, service_id, &services));
    tapi_upnp_print_service_info(&services);

    TEST_SUCCESS;

cleanup:
    rpc_upnp_cp_disconnect(pco_iut);
    if (strlen(device_name) > 0)
        tapi_upnp_free_device_info(&devices);
    tapi_upnp_free_service_info(&services);
    TEST_END;
}
