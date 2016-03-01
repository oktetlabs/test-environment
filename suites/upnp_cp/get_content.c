/** @file
 * @brief UPnP Control Point Test Suite
 *
 * Test to get UPnP content directory.
 *
 * Copyright (C) 2016 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

/** @page upnp_cp-get_content Test to get UPnP content directory
 *
 * @objective Invoke the particular action on ContentDirectory compatible
 *            UPnP service.
 *
 * @param service_id        UPnP service ID.
 *
 * @par Scenario
 * -# Connect to UPnP Control Point.
 * -# Get particular UPnP service(s) aconrding to @p service_id.
 * -# Invoke action Browse on ContentDirectory service and build tree.
 * -# Search and print URI of media items in tree.
 * -# Destroy tree (free memory).
 * -# Free UPnP service(s) info.
 * -# Disconnect from UPnP Control Point.
 */

#include "te_config.h"
#include "upnp_cp_ts.h"
#include "tapi_test.h"
#include "rcf_rpc.h"
#include "tapi_upnp.h"
#include "tapi_upnp_cp.h"
#include "tapi_upnp_content_directory.h"
#include "tapi_upnp_resources.h"


#define TE_TEST_NAME "upnp_cp/get_content"

/** Enable print of obtained service info. Set to @c 0 to disable. */
#define PRINT_OF_SERVICES_ENABLED 0
/** Get either full tree or only root item and it direct children. */
#define GET_FULL_TREE 1


int
main(int argc, char **argv)
{
    static tapi_upnp_services     services;
    static tapi_upnp_cd_container_node content;
    static tapi_upnp_media_uri    media;
    rcf_rpc_server               *pco_iut = NULL;
    const char                   *service_id = NULL;
    tapi_upnp_service_info       *service = NULL;
    tapi_upnp_resources_uri_node *m = NULL;
    size_t                        media_count = 0;

    TEST_START;
    TEST_GET_STRING_PARAM(service_id);
    TEST_GET_PCO(pco_iut);

    SLIST_INIT(&services);
    SLIST_INIT(&media);

    rpc_upnp_cp_connect(pco_iut);

    /* Get available services. */
    CHECK_RC(
        tapi_upnp_get_service_info(pco_iut, NULL, service_id, &services));

#if PRINT_OF_SERVICES_ENABLED
    tapi_upnp_print_service_info(&services);
#endif /* PRINT_OF_SERVICES_ENABLED */

    service = SLIST_FIRST(&services);
    if (service == NULL)
        TEST_VERDICT("UPnP service with id=\"%s\" not found", service_id);

    /* Get Content Directory. */
#if GET_FULL_TREE == 0
    CHECK_RC(tapi_upnp_cd_get_root(pco_iut, service, &content));
    CHECK_RC(tapi_upnp_cd_get_children(pco_iut, service, &content));
#else /* GET_FULL_TREE */
    CHECK_RC(tapi_upnp_cd_get_tree(pco_iut, service, &content));
#endif /* GET_FULL_TREE */

#if PRINT_OF_SERVICES_ENABLED
    /* Print the services info with action results (values). */
    tapi_upnp_print_service_info(&services);
#endif /* PRINT_OF_SERVICES_ENABLED */

    CHECK_RC(tapi_upnp_resources_get_media_uri(&content,
                                               TAPI_UPNP_CD_RESOURCE_OTHER,
                                               &media));
    SLIST_FOREACH(m, &media, next)
    {
        RING("URI: %s", m->uri);
        media_count++;
    }
    RING("===\n total media count: %u", media_count);

    TEST_SUCCESS;

cleanup:
    rpc_upnp_cp_disconnect(pco_iut);
    tapi_upnp_resources_free_media_uri(&media);
    tapi_upnp_cd_remove_tree(&content);
    tapi_upnp_free_service_info(&services);
    TEST_END;
}
