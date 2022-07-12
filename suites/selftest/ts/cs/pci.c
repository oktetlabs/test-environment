/** @file
 * @brief TAPI PCI functions testing
 *
 * TAPI PCI functions testing
 *
 * Copyright (C) 2022 OKTET Labs Ltd., St.-Petersburg, Russia
 */

/** @page cs-pci A sample of PCI management TAPI
 *
 * @objective Check that PCI management routines work correctly
 *
 * @par Scenario:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_TEST_NAME "cs/pci"


#ifndef TEST_START_VARS
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"
#include "conf_api.h"
#include "tapi_cfg_pci.h"
#include "tapi_cfg_base.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "te_vector.h"

static void
list_pci_devices_by_vendor(const char *ta, te_vec *list)
{
    cfg_handle *instances = NULL;
    unsigned int n_instances = 0;
    unsigned int i;

    CHECK_RC(cfg_find_pattern_fmt(&n_instances, &instances,
                                  "/agent:%s/hardware:/pci:"
                                  "/vendor:*/device:*/instance:*",
                                  ta));

    for (i = 0; i < n_instances; i++)
    {
        char *oid = NULL;

        CHECK_RC(cfg_get_oid_str(instances[i], &oid));
        CHECK_RC(TE_VEC_APPEND(list, oid));
    }
    free(instances);
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    te_vec vnd_oids = TE_VEC_INIT(char *);
    te_vec pci_oids = TE_VEC_INIT(char *);
    char **oid;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Get list of PCI devices");
    list_pci_devices_by_vendor(pco_iut->ta, &vnd_oids);
    if (te_vec_size(&vnd_oids) == 0)
        TEST_SKIP("No PCI devices grabbed");

    TEST_STEP("Check the resolving of PCI instance OIDs");
    TE_VEC_FOREACH(&vnd_oids, oid)
    {
        char *pci_oid = NULL;
        CHECK_RC(tapi_cfg_pci_resolve_device_oid(*oid, &pci_oid));
        CHECK_NOT_NULL(pci_oid);
        CHECK_RC(TE_VEC_APPEND(&pci_oids, pci_oid));
    }

    TEST_STEP("Check that resolving of PCI OIDs is idempotent");
    TE_VEC_FOREACH(&pci_oids, oid)
    {
        char *pci_oid = NULL;
        CHECK_RC(tapi_cfg_pci_resolve_device_oid(*oid, &pci_oid));
        CHECK_NOT_NULL(pci_oid);
        if (strcmp(pci_oid, *oid) != 0)
        {
            TEST_VERDICT("PCI OID '%s' is not resolved to itself, "
                         "got '%s' instead", pci_oid, *oid);
        }
        free(pci_oid);
    }

    TEST_SUCCESS;

cleanup:
    te_vec_deep_free(&vnd_oids);
    te_vec_deep_free(&pci_oids);

    TEST_END;
}
