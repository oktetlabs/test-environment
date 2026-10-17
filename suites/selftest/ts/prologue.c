/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal prologue
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page ts_prologue Prologue
 *
 * @objective Demo of minimal prologue of test suite
 *
 * Prologue is test that will be run before the package is run.
 * It is typically used to configure agents before running tests in a package.
 *
 * @note @p TEST_STEP() is required. This need to generate
 * documetation of test steps.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "prologue"

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
#include "te_string.h"
#include "tapi_cfg_memory.h"
#include "tapi_env.h"
#include "tapi_test.h"
#include "tapi_tags.h"
#include "tapi_sh_env.h"
#include "tapi_rpc_misc.h"
#include "conf_api.h"
#include "tapi_cfg_dcbx.h"
#include "tapi_cfg_ets.h"
#include "tapi_cfg_pfc.h"

/**
 * Add TRC tags telling which DCB tests cannot run:
 * - @c no_dcbx, @c no_ets, @c no_pfc: the IUT interface (node A of net1)
 *   does not provide DCBX, ETS or PFC data (see tapi_cfg_dcbx_is_present()
 *   and others);
 * - @c no_dcb_tool: the dcb tool is not found on the IUT.
 *
 * The tests check the same conditions themselves and skip.
 *
 * @param iut_rpcs  RPC server on the IUT.
 *
 * @return Status code.
 */
static te_errno
add_dcb_tags(rcf_rpc_server *iut_rpcs)
{
    static const struct {
        const char *name;
        const char *tag;
        te_errno (*is_present)(const char *, const char *, bool *);
    } features[] = {
        { "DCBX", "no_dcbx", tapi_cfg_dcbx_is_present },
        { "ETS", "no_ets", tapi_cfg_ets_is_present },
        { "PFC", "no_pfc", tapi_cfg_pfc_is_present },
    };
    char *if_oid = NULL;
    cfg_oid *oid = NULL;
    unsigned int i;
    bool present;
    te_errno rc;

    rc = cfg_get_instance_string_fmt(&if_oid, "/net:net1/node:A");
    if (rc != 0)
    {
        ERROR("Failed to get the IUT interface: %r", rc);
        return rc;
    }

    oid = cfg_convert_oid_str(if_oid);
    if (oid == NULL || !oid->inst || oid->len != 3 ||
        strcmp(cfg_oid_inst_subid(oid, 2), "interface") != 0)
    {
        ERROR("IUT node '%s' is not an agent interface", if_oid);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    for (i = 0; i < TE_ARRAY_LEN(features); i++)
    {
        rc = features[i].is_present(CFG_OID_GET_INST_NAME(oid, 1),
                                    CFG_OID_GET_INST_NAME(oid, 2),
                                    &present);
        if (rc != 0)
            goto out;

        if (!present)
        {
            RING("IUT interface '%s' does not provide %s", if_oid,
                 features[i].name);
            rc = tapi_tags_add_tag(features[i].tag, "");
            if (rc != 0)
                goto out;
        }
    }

    if (rpc_te_file_check_executable(iut_rpcs, "dcb") != 0)
    {
        RING("dcb tool is not found on the IUT");
        rc = tapi_tags_add_tag("no_dcb_tool", "");
    }

out:
    cfg_free_oid(oid);
    free(if_oid);
    return rc;
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *iut_rpcs = NULL;
    uint64_t memory;
    te_string str = TE_STRING_INIT;

    const char *memtier_path = NULL;
    const char *trex_path = NULL;

    TEST_START;
    TEST_GET_PCO(iut_rpcs);

    TEST_STEP("Start prologue");

    TEST_STEP("Expand PATH variable for all agents.");
    CHECK_RC(tapi_expand_path_all_ta(NULL));

    TEST_STEP("Add TRC tag");
    CHECK_RC(tapi_tags_add_tag("tag_set_by_prologue", NULL));

    TEST_STEP("Add verdict which will be expected only if "
              "the added tag has effect");
    RING_VERDICT("Test verdict with added tag");

    /*
     * Check whether some interfaces in networks specifications are defined
     * as PCI devices. Find out names of such interfaces, grab them and
     * replace PCI device references with interface name references in
     * networks specifications.
     *
     * This makes it possible to work with such interfaces in a usual way,
     * for example add IP addresses on them.
     */
    CHECK_RC(tapi_cfg_net_nodes_update_pci_fn_to_interface(
                                          NET_NODE_TYPE_INVALID));

    /* Total memory in MB */
    CHECK_RC(tapi_cfg_get_memory(iut_rpcs->ta, 0, &memory));
    memory /= (1024 * 1024); /* Bytes to Megabytes */
    te_string_append(&str, "%u", memory);
    CHECK_RC(tapi_tags_add_tag("total_memory_mb", str.ptr));

    memtier_path = getenv("TE_IUT_MEMTIER_PATH");
    if (te_str_is_null_or_empty(memtier_path))
    {
        WARN("No path to memtier_benchmark was provided");
        CHECK_RC(tapi_tags_add_tag("no_memtier", ""));
    }
    trex_path = getenv("TE_IUT_TREX_EXEC_PATH");
    if (te_str_is_null_or_empty(trex_path))
    {
        WARN("Path to TRex exec is not specified in environment");
        CHECK_RC(tapi_tags_add_tag("no_trex", ""));
    }

    TEST_STEP("Add TRC tags for DCB tests which cannot run");
    CHECK_RC(add_dcb_tags(iut_rpcs));

    /* Print /local: subtree to see which TRC tags have been added */
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/local:%s", ""));

    TEST_SUCCESS;

cleanup:
    te_string_free(&str);
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
