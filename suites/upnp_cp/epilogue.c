/** @file
 * @brief UPnP Control Point Test Suite
 *
 * UPnP Control Point Test Suite epilogue.
 *
 * Copyright (C) 2016 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

/** @page upnp_cp-epilogue UPnP Control Point Test Suite epilogue
 *
 * @objective Stop currently run UPnP Control Point.
 *
 * @par Scenario
 * -# Stop currently run UPnP Control Point.
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME "upnp_cp/epilogue"

#include "te_config.h"
#include "upnp_cp_ts.h"
#include "rcf_rpc.h"
#include "tapi_test.h"
#include "tapi_upnp_cp.h"


int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    if (tapi_upnp_cp_started(pco_iut->ta))
        CHECK_RC(tapi_upnp_cp_stop(pco_iut->ta));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
