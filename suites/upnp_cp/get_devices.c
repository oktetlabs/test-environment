/** @file
 * @brief UPnP Control Point Test Suite
 *
 * Test to get UPnP devices.
 *
 * Copyright (C) 2016 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

/** @page upnp_cp-get_devices Test to get UPnP devices
 *
 * @objective Get and print the list of UPnP devices.
 *
 * @param device        UPnP device friendly name.
 *
 * @par Scenario
 * -# Connect to UPnP Control Point.
 * -# Get UPnP device(s) info.
 * -# Print UPnP device(s) info.
 * -# Free UPnP device(s) info.
 * -# Disconnect from UPnP Control Point.
 */

#include "te_config.h"
#include "upnp_cp_ts.h"
#include "tapi_test.h"
#include "rcf_rpc.h"
#include "tapi_upnp.h"
#include "tapi_upnp_cp.h"
#include "tapi_upnp_device_info.h"


#define TE_TEST_NAME "upnp_cp/get_devices"


int
main(int argc, char **argv)
{
    static tapi_upnp_devices devices;
    rcf_rpc_server          *pco_iut = NULL;
    const char              *device = NULL;

    TEST_START;
    TEST_GET_STRING_PARAM(device);
    TEST_GET_PCO(pco_iut);

    if (strcmp(device, "all") == 0)
        device = "";    /* or device = NULL */

    SLIST_INIT(&devices);
    rpc_upnp_cp_connect(pco_iut);
    CHECK_RC(tapi_upnp_get_device_info(pco_iut, device, &devices));
    tapi_upnp_print_device_info(&devices);

    TEST_SUCCESS;

cleanup:
    rpc_upnp_cp_disconnect(pco_iut);
    tapi_upnp_free_device_info(&devices);
    TEST_END;
}
