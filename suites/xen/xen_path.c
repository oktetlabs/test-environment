/** @file
 * @brief XEN Test Suite
 *
 * XEN Test Suite
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * 
 * $Id: xen_path.c $
 */


#define TE_TEST_NAME "xen/xen_path"

#include "xen_suite.h"
#include "xen.h"

inline static void
test_core(rcf_rpc_server *pco, char const *xen_path, te_errno rc)
{
    te_errno returned_rc = tapi_cfg_xen_set_path(pco->ta, xen_path);

    if (rc != returned_rc)
    {
        TEST_FAIL("XEN path set to '%s' on %s has failed: "
                  "expected rc is %r while the returned one is %r",
                  xen_path, pco->ta, rc, returned_rc);
    }
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_aux = NULL;

    te_errno const failure_rc = TE_RC(TE_TA_UNIX, TE_ENOENT);

    char const *xen_path    = NULL;
    te_bool     should_fail = FALSE;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_aux);

    TEST_GET_STRING_PARAM(xen_path);
    TEST_GET_BOOL_PARAM(should_fail);

   /* Check that XEN path can be set */ 
    test_core(pco_iut, xen_path, should_fail ? failure_rc : 0);
    test_core(pco_aux, xen_path, should_fail ? failure_rc : 0);

    /* Check that XEN path can be reset */
    test_core(pco_iut, "", 0);
    test_core(pco_aux, "", 0);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
