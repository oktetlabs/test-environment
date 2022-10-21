/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI PCI functions testing
 *
 * TAPI PCI functions testing.
 */

/** @page cs-pci A sample of PCI management TAPI
 *
 * @objective Check that PCI management routines work correctly.
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
#include "tapi_file.h"
#include "te_vector.h"

static te_errno
for_each_pci_instance(cfg_handle handle, void *data)
{
    char *oid = NULL;
    te_vec *list = data;

    CHECK_RC(cfg_get_oid_str(handle, &oid));
    CHECK_RC(TE_VEC_APPEND(list, oid));

    return 0;
}

static void
check_spdk_config(const char *ta, const char *pci_oid, const char *vnd_oid)
{
    char *filename;
    char *filename2;
    char *filename3;
    te_errno rc;
    unsigned int class_id;
    char *contents1;
    char *contents2;

    CHECK_RC(tapi_cfg_pci_get_class(pci_oid, &class_id, NULL, NULL));
    if (class_id != TE_PCI_CLASS_MASS_STORAGE_CONTROLLER)
    {
        rc = tapi_cfg_pci_get_spdk_config_filename(pci_oid, "Test",
                                                   TRUE, &filename);
        if (rc != TE_RC(TE_TA_UNIX, TE_ENOTBLK))
            TEST_VERDICT("SPDK config added to a non-storage device");

        rc = tapi_cfg_pci_get_spdk_config_filename(vnd_oid, "Test",
                                                   TRUE, &filename);
        if (rc != TE_RC(TE_TA_UNIX, TE_ENOTBLK))
            TEST_VERDICT("SPDK config added to a non-storage device");
        return;
    }

    CHECK_RC(tapi_cfg_pci_get_spdk_config_filename(pci_oid, "Test",
                                                   TRUE, &filename));
    rc = tapi_cfg_pci_get_spdk_config_filename(pci_oid, "Test",
                                               TRUE, &filename);
    if (rc != TE_RC(TE_CS, TE_EEXIST))
        TEST_VERDICT("Duplicate SPDK config was added");

    CHECK_RC(tapi_cfg_pci_get_spdk_config_filename(vnd_oid, "Test",
                                                   FALSE, &filename2));
    if (strcmp(filename, filename2) != 0)
    {
        ERROR("Two filenames of the same SPDK config differ: '%s' vs '%s'",
              filename, filename2);
        TEST_VERDICT("Config filenames differ");
    }

    rc = tapi_cfg_pci_get_spdk_config_filename(vnd_oid, "Test1",
                                               FALSE, &filename3);
    if (rc != TE_RC(TE_CS, TE_ENOENT))
        TEST_VERDICT("Non-existing SPDK config file was retrieved", rc);
    CHECK_RC(tapi_cfg_pci_get_spdk_config_filename(vnd_oid, "Test1",
                                                   TRUE, &filename3));
    if (strcmp(filename, filename3) == 0)
    {
        ERROR("Two filenames of different SPDK configs are the same '%s'",
              filename3);
        free(filename);
        free(filename2);
        TEST_VERDICT("Config filenames are the same");
    }

    CHECK_RC(tapi_file_read_ta(ta, filename, &contents1));
    CHECK_RC(tapi_file_read_ta(ta, filename3, &contents2));
    if (strstr(contents1, "\"Test\"") == NULL ||
        strstr(contents2, "\"Test1\"") == NULL)
    {
        TEST_VERDICT("No config name in JSON file");
    }

    free(contents1);
    free(contents2);
    free(filename);
    free(filename2);
    free(filename3);
}

static void
check_pci_class(const char *oid)
{
    unsigned int class_id;
    unsigned int subclass_id;
    unsigned int intf_id;

    CHECK_RC(tapi_cfg_pci_get_class(oid, &class_id, &subclass_id, &intf_id));

    /*
     * We assume that all reserved PCI functions are either network devices
     * or NVMe controllers. That may change in the future.
     */

    if (class_id == TE_PCI_CLASS_NETWORK_CONTROLLER)
    {
        if (subclass_id != TE_PCI_SUBCLASS_ETHERNET_CONTROLLER)
        {
            TEST_VERDICT("Unexpected subclass %s (%04x)",
                         te_pci_subclass_id2str(subclass_id), subclass_id);
        }

        if (intf_id !=
            te_pci_progintf_default(TE_PCI_SUBCLASS_ETHERNET_CONTROLLER))
        {
            TEST_VERDICT("Unexpected interface %s (%06x)",
                         te_pci_progintf_id2str(intf_id), intf_id);
        }
    }
    else if (class_id == TE_PCI_CLASS_MASS_STORAGE_CONTROLLER)
    {
        if (subclass_id != TE_PCI_SUBCLASS_NON_VOLATILE_MEMORY_CONTROLLER)
            TEST_VERDICT("Unexpected subclass %s (%04x)",
                         te_pci_subclass_id2str(subclass_id), subclass_id);

        if (intf_id != TE_PCI_PROG_INTERFACE_NVM_CONTROLLER_NVME)
        {
            TEST_VERDICT("Unexpected interface %s (%06x)",
                         te_pci_progintf_id2str(intf_id), intf_id);
        }
    }
    else
    {
        TEST_VERDICT("Unexpected class %s (%02x)",
                     te_pci_class_id2str(class_id),
                     class_id);
    }

    RING("Reported class for %s is %s (%02x)", oid,
         te_pci_class_id2str(class_id), class_id);
    RING("Reported subclass for %s is %s (%04x)", oid,
         te_pci_subclass_id2str(subclass_id), subclass_id);
    RING("Reported interface for %s is %s (%06x)", oid,
         te_pci_progintf_id2str(intf_id), intf_id);
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    te_vec vnd_oids = TE_VEC_INIT(char *);
    te_vec pci_oids = TE_VEC_INIT(char *);
    char **oid;
    unsigned int i;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Get list of PCI devices");
    CHECK_RC(cfg_find_pattern_iter_fmt(for_each_pci_instance, &vnd_oids,
                                       "/agent:%s/hardware:/pci:"
                                       "/vendor:*/device:*/instance:*",
                                       pco_iut->ta));
    if (te_vec_size(&vnd_oids) == 0)
        TEST_SKIP("No PCI devices grabbed");

    TEST_STEP("Check the resolving of PCI instance OIDs");
    TE_VEC_FOREACH(&vnd_oids, oid)
    {
        char *pci_oid = NULL;
        CHECK_RC(tapi_cfg_pci_resolve_device_oid(&pci_oid, "%s", *oid));
        CHECK_NOT_NULL(pci_oid);
        CHECK_RC(TE_VEC_APPEND(&pci_oids, pci_oid));
    }

    TEST_STEP("Check that resolving of PCI OIDs is idempotent");
    TE_VEC_FOREACH(&pci_oids, oid)
    {
        char *pci_oid = NULL;
        CHECK_RC(tapi_cfg_pci_resolve_device_oid(&pci_oid, "%s", *oid));
        CHECK_NOT_NULL(pci_oid);
        if (strcmp(pci_oid, *oid) != 0)
        {
            TEST_VERDICT("PCI OID '%s' is not resolved to itself, "
                         "got '%s' instead", pci_oid, *oid);
        }
        free(pci_oid);
    }

    TEST_STEP("Retrieve the device class by PCI instance OID");
    TE_VEC_FOREACH(&vnd_oids, oid)
    {
        check_pci_class(*oid);
    }
    TEST_STEP("Retrieve the device class by PCI OID");
    TE_VEC_FOREACH(&pci_oids, oid)
    {
        check_pci_class(*oid);
    }

    TEST_STEP("Check SPDK config generation");
    for (i = 0; i < te_vec_size(&pci_oids); i++)
    {
        char *pci_oid = TE_VEC_GET(char *, &pci_oids, i);
        char *vnd_oid = TE_VEC_GET(char *, &vnd_oids, i);

        check_spdk_config(pco_iut->ta, pci_oid, vnd_oid);
    }

    TEST_SUCCESS;

cleanup:
    te_vec_deep_free(&vnd_oids);
    te_vec_deep_free(&pci_oids);

    TEST_END;
}
