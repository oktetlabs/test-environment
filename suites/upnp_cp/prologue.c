/** @file
 * @brief UPnP Control Point Test Suite
 *
 * UPnP Control Point Test Suite prologue.
 *
 * Copyright (C) 2016 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

/** @page upnp_cp-prologue UPnP Control Point Test Suite prologue
 *
 * @objective Start UPnP Control Point with particular search target.
 *
 * @param target    Search Target.
 * @param iface     Network interface.
 *
 * @par Scenario
 * -# Stop currently started UPnP Control Point.
 * -# Start UPnP Control Point with particular search target @p target.
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME "upnp_cp/prologue"

#include "te_config.h"
#include "upnp_cp_ts.h"
#include "rcf_rpc.h"
#include "tapi_test.h"
#include "tapi_upnp_cp.h"

/**
 * Timeout to make sure that UPnP CP finished search for devices and/or
 * services (it depends on number of available devices).
 */
#define TIME_TO_WAIT    5


int
main(int argc, char *argv[])
{
    const char     *target;
    rcf_rpc_server *pco_iut = NULL;
    const struct if_nameindex *iut_if = NULL;

    TEST_START;
    TEST_GET_STRING_PARAM(target);
    TEST_GET_PCO(pco_iut);
    TEST_GET_IF(iut_if);

    if (tapi_upnp_cp_started(pco_iut->ta))
        CHECK_RC(tapi_upnp_cp_stop(pco_iut->ta));
    CHECK_RC(tapi_upnp_cp_start(pco_iut->ta, target, iut_if->if_name));
    SLEEP(TIME_TO_WAIT);
    RING("UPnP enabled: %d", tapi_upnp_cp_started(pco_iut->ta));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
