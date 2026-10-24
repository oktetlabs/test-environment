/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Test Environment
 *
 * Check the egress QoS map of VLAN interfaces in Configurator
 */

/** @page vlan_egress_qos Check the VLAN egress QoS map
 *
 * @objective Check that a mapping from a socket priority to a PCP can be
 *            added, read, changed and deleted on a VLAN interface, and
 *            that a backup restore removes it.
 *
 * The test works on a VLAN over one end of a veth pair it creates, so it
 * needs no interface of the rig.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "cs/vlan_egress_qos"

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

#include "tapi_cfg_base.h"
#include "tapi_test.h"
#include "tapi_env.h"

/** Name of the test interface: one end of a veth pair */
#define EGRESS_QOS_TEST_IF "vethqos0"

/** Name of the other end of the veth pair */
#define EGRESS_QOS_TEST_PEER "vethqos1"

/** VLAN ID of the test VLAN */
#define EGRESS_QOS_TEST_VID 10

/** Socket priority mapped by the test */
#define EGRESS_QOS_TEST_PRIO 3

/** Socket priority mapped inside the backup */
#define EGRESS_QOS_TEST_PRIO_BACKUP 4

/** First PCP */
#define EGRESS_QOS_TEST_PCP 5

/** Second PCP */
#define EGRESS_QOS_TEST_PCP_NEW 6

/** PCP of the mapping inside the backup */
#define EGRESS_QOS_TEST_PCP_BACKUP 2

/**
 * Check that a priority maps to a PCP.
 *
 * @param ta        Test Agent.
 * @param prio      Socket priority.
 * @param pcp       PCP expected.
 * @param what      What is checked, for verdicts.
 */
static void
egress_qos_check(const char *ta, uint32_t prio, uint8_t pcp,
                 const char *what)
{
    uint8_t got = 0;

    CHECK_RC(tapi_cfg_base_if_vlan_egress_qos_get(ta, EGRESS_QOS_TEST_IF,
                                                  EGRESS_QOS_TEST_VID,
                                                  prio, &got));
    if (got != pcp)
        TEST_VERDICT("%s: PCP %u read back instead of %u", what, got, pcp);
}

/**
 * Check that a priority has no mapping.
 *
 * @param ta        Test Agent.
 * @param prio      Socket priority.
 * @param what      What is checked, for verdicts.
 */
static void
egress_qos_check_none(const char *ta, uint32_t prio, const char *what)
{
    uint8_t got = 0;
    te_errno rc;

    rc = tapi_cfg_base_if_vlan_egress_qos_get(ta, EGRESS_QOS_TEST_IF,
                                              EGRESS_QOS_TEST_VID, prio,
                                              &got);
    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        TEST_VERDICT("%s: the mapping read back as %r instead of TE_ENOENT",
                     what, rc);
    }
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    cfg_handle *handles = NULL;
    char *vlan_ifname = NULL;
    char *backup = NULL;
    cfg_handle handle;
    uint32_t listed_prio = 0;
    unsigned int n = 0;
    bool if_created = false;
    bool vlan_created = false;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create a veth pair and a VLAN over its first end.");
    if (cfg_find_fmt(&handle, "/agent:%s/veth:%s", pco_iut->ta,
                     EGRESS_QOS_TEST_IF) == 0)
    {
        if_created = true;
        TEST_VERDICT("The test interface exists before the test");
    }

    rc = tapi_cfg_base_if_add_veth(pco_iut->ta, EGRESS_QOS_TEST_IF,
                                   EGRESS_QOS_TEST_PEER);
    if (rc != 0 &&
        cfg_find_fmt(&handle, "/agent:%s/veth:%s", pco_iut->ta,
                     EGRESS_QOS_TEST_IF) != 0)
    {
        RING("The veth pair cannot be created: %r", rc);
        TEST_SKIP("Cannot create a veth pair");
    }
    if_created = true;
    CHECK_RC(rc);

    CHECK_RC(tapi_cfg_base_if_add_vlan(pco_iut->ta, EGRESS_QOS_TEST_IF,
                                       EGRESS_QOS_TEST_VID, &vlan_ifname));
    vlan_created = true;
    RING("VLAN interface %s", vlan_ifname);

    TEST_STEP("Check that the VLAN has no mapping.");
    CHECK_RC(cfg_find_pattern_fmt(&n, &handles,
                                  "/agent:%s/interface:%s/vlans:%d/"
                                  "egress_qos:*", pco_iut->ta,
                                  EGRESS_QOS_TEST_IF, EGRESS_QOS_TEST_VID));
    if (n != 0)
        TEST_VERDICT("A new VLAN lists %u mappings", n);
    free(handles);
    handles = NULL;

    TEST_STEP("Map a priority to a PCP and check that it reads back and "
              "is listed.");
    CHECK_RC(tapi_cfg_base_if_vlan_egress_qos_set(pco_iut->ta,
                                                  EGRESS_QOS_TEST_IF,
                                                  EGRESS_QOS_TEST_VID,
                                                  EGRESS_QOS_TEST_PRIO,
                                                  EGRESS_QOS_TEST_PCP));
    egress_qos_check(pco_iut->ta, EGRESS_QOS_TEST_PRIO, EGRESS_QOS_TEST_PCP,
                     "After adding the mapping");
    CHECK_RC(cfg_find_pattern_fmt(&n, &handles,
                                  "/agent:%s/interface:%s/vlans:%d/"
                                  "egress_qos:*", pco_iut->ta,
                                  EGRESS_QOS_TEST_IF, EGRESS_QOS_TEST_VID));
    if (n != 1)
        TEST_VERDICT("%u mappings are listed instead of 1", n);
    CHECK_RC(cfg_get_inst_name_type(handles[0], CVT_UINT32,
                                    CFG_IVP(&listed_prio)));
    if (listed_prio != EGRESS_QOS_TEST_PRIO)
    {
        TEST_VERDICT("The mapping is listed as priority %u instead of %u",
                     listed_prio, EGRESS_QOS_TEST_PRIO);
    }

    TEST_STEP("Change the PCP and check that it reads back.");
    CHECK_RC(tapi_cfg_base_if_vlan_egress_qos_set(pco_iut->ta,
                                                  EGRESS_QOS_TEST_IF,
                                                  EGRESS_QOS_TEST_VID,
                                                  EGRESS_QOS_TEST_PRIO,
                                                  EGRESS_QOS_TEST_PCP_NEW));
    egress_qos_check(pco_iut->ta, EGRESS_QOS_TEST_PRIO,
                     EGRESS_QOS_TEST_PCP_NEW, "After changing the mapping");

    TEST_STEP("Check that the node refuses PCP 0 and the mapping is kept.");
    /* Not through the TAPI: it refuses PCP 0 before the agent sees it */
    rc = cfg_set_instance_fmt(CFG_VAL(UINT8, 0),
                              "/agent:%s/interface:%s/vlans:%d/egress_qos:%d",
                              pco_iut->ta, EGRESS_QOS_TEST_IF,
                              EGRESS_QOS_TEST_VID, EGRESS_QOS_TEST_PRIO);
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
        TEST_VERDICT("Setting PCP 0 returned %r instead of TE_EINVAL", rc);
    egress_qos_check(pco_iut->ta, EGRESS_QOS_TEST_PRIO,
                     EGRESS_QOS_TEST_PCP_NEW, "After the refused PCP 0");

    TEST_STEP("Delete the mapping and check that it is gone.");
    CHECK_RC(tapi_cfg_base_if_vlan_egress_qos_del(pco_iut->ta,
                                                  EGRESS_QOS_TEST_IF,
                                                  EGRESS_QOS_TEST_VID,
                                                  EGRESS_QOS_TEST_PRIO));
    egress_qos_check_none(pco_iut->ta, EGRESS_QOS_TEST_PRIO,
                          "After deleting the mapping");

    TEST_STEP("Add a mapping after a backup and check that restoring the "
              "backup removes it.");
    CHECK_RC(cfg_create_backup(&backup));
    CHECK_RC(tapi_cfg_base_if_vlan_egress_qos_set(pco_iut->ta,
                                                  EGRESS_QOS_TEST_IF,
                                                  EGRESS_QOS_TEST_VID,
                                                  EGRESS_QOS_TEST_PRIO_BACKUP,
                                                  EGRESS_QOS_TEST_PCP_BACKUP));
    egress_qos_check(pco_iut->ta, EGRESS_QOS_TEST_PRIO_BACKUP,
                     EGRESS_QOS_TEST_PCP_BACKUP, "Inside the backup");
    CHECK_RC(cfg_restore_backup(backup));
    egress_qos_check_none(pco_iut->ta, EGRESS_QOS_TEST_PRIO_BACKUP,
                          "After restoring the backup");
    CHECK_RC(cfg_release_backup(&backup));

    TEST_SUCCESS;

cleanup:
    free(handles);
    if (backup != NULL)
        CLEANUP_CHECK_RC(cfg_release_backup(&backup));
    free(backup);
    free(vlan_ifname);
    if (vlan_created)
    {
        CLEANUP_CHECK_RC(tapi_cfg_base_if_del_vlan(pco_iut->ta,
                                                   EGRESS_QOS_TEST_IF,
                                                   EGRESS_QOS_TEST_VID));
    }
    if (if_created)
    {
        CLEANUP_CHECK_RC(tapi_cfg_base_if_del_veth(pco_iut->ta,
                                                   EGRESS_QOS_TEST_IF));
    }
    TEST_END;
}
