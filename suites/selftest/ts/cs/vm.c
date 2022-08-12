/** @file
 * @brief Test Environment
 *
 * Check virtual machines support in Configurator
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

/** @page vm Create virtual machine
 *
 * @objective Check that virtual machine may be created and test agent
 *            started on it.
 *
 * @par Scenario:
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "vm"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "te_string.h"
#include "tapi_cfg_vm.h"

int
main(int argc, char *argv[])
{
    const char     *ta = "Agt_A";
    char           *vm_name = "testvm";
    const char     *ta_vm = "Agt_VM";
    const char     *key = "/srv/virtual/testvm.id_rsa";
    int             ssh_port;
    int             rcf_port;
    char           *confstr;
    char           *vendor_str;
    char           *device_str;
    char           *instance_str;
    unsigned long   device;
    unsigned long   vendor;
    unsigned long   instance;

    TEST_START;

    TEST_STEP("Add a virtual machine");
    CHECK_RC(tapi_cfg_vm_add(ta, vm_name, "/local:/vm:testvm", FALSE));

    vendor_str = getenv("TE_VM_PCI_PT_VENDOR");
    if (vendor_str != NULL)
    {
        TEST_STEP("Pass PCI");
        device_str = getenv("TE_VM_PCI_PT_DEVICE");
        instance_str = getenv("TE_VM_PCI_PT_INSTANCE");
        if (device_str == NULL || instance_str == NULL)
            TEST_FAIL("PCI address is not specified");

        if ((te_strtoul(vendor_str, 16, &vendor) != 0) || vendor > UINT16_MAX ||
            (te_strtoul(device_str, 16, &device) != 0) || device > UINT16_MAX ||
            (te_strtoul(instance_str, 0, &instance) != 0))
        {
            TEST_FAIL("PCI address is incorrect %s:%s %s",
                      vendor_str, device_str, instance_str);
        }

        CHECK_RC(tapi_cfg_vm_pass_pci(ta, vm_name, "vm_pci",
                                      vendor, device, instance));
    }

    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s/vm:%s", ta, vm_name));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/vm:%s", ta, vm_name));

    TEST_STEP("Start the virtual machine");
    CHECK_RC(tapi_cfg_vm_start(ta, vm_name));

    TEST_STEP("Start a test agent on the virtual machine");
    CHECK_RC(cfg_get_instance_int_fmt(&ssh_port,
                                      "/agent:%s/vm:%s/ssh_port:/host:",
                                      ta, vm_name));
    CHECK_RC(cfg_get_instance_int_fmt(&rcf_port, "/agent:%s/vm:%s/rcf_port:",
                                      ta, vm_name));

    SLEEP(2);

    confstr = te_string_fmt("host=%s:port=%d:user=root:key=%s:ssh_port=%u:",
                            "127.0.0.1", rcf_port, key, ssh_port);

    CHECK_RC(rcf_add_ta(ta_vm, "linux", "rcfunix", confstr,
                 RCF_TA_REBOOTABLE | RCF_TA_NO_SYNC_TIME | RCF_TA_NO_HKEY_CHK));

    TEST_STEP("Sync the VM test agent configuration tree");
    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s", ta_vm));

    TEST_STEP("Log the VM test agent configuration tree");
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s", ta_vm));

    TEST_STEP("Stop the VM test agent");
    CHECK_RC(rcf_del_ta(ta_vm));

    TEST_STEP("Sync the VM test agent configuration tree after the TA removal");
    CHECK_RC(rc = cfg_synchronize_fmt(TRUE, "/agent:%s", ta_vm));

    TEST_STEP("Delete the virtual machine");
    CHECK_RC(tapi_cfg_vm_del(ta, vm_name));

    TEST_SUCCESS;

cleanup:
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
