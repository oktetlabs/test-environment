/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test behaviour switches API
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI Test Behaviour"


#include "tapi_test.h"
#include "tapi_cfg.h"

#define BEHV_FIELD

/* See description in tapi_test_behaviour.h */
test_behaviour test_behaviour_storage;

/* See description in tapi_test_behaviour.h */
te_bool fd_not_closed_verdict = FALSE;

/**
 * Wrapper function to call te_strtoui().
 * It helps to get boolean and unsigned integer values from the GET_BEHV macro.
 *
 * @param str   String to convert
 * @param value Location for the resulting value
 *
 * @return Status code
 */
static te_errno
test_behaviour_strtoul(const char *str, unsigned int *value)
{
    return te_strtoui(str, 0, value);
}

void
test_behaviour_get(test_behaviour *behaviour)
{
    char *s;
    te_errno rc;

    memset(behaviour, 0, sizeof(*behaviour));

#define GET_BEHV(name_, func_)                                           \
    do {                                                                 \
        rc = cfg_get_instance_fmt(NULL, &s,                              \
                                  "/local:/test:/behaviour:%s", #name_); \
        if (rc == 0)                                                     \
        {                                                                \
            CHECK_RC(func_(s, &(behaviour-> name_)));                    \
            free(s);                                                     \
        }                                                                \
        else if (TE_RC_GET_ERROR(rc) == TE_ENOENT)                       \
        {                                                                \
            INFO("'%s' switch is not present in the /local subtree",     \
                 #name_);                                                \
        }                                                                \
        else                                                             \
        {                                                                \
            TEST_FAIL("Failed to get '%s' behaviour specifier: %r",      \
                      #name_, rc);                                       \
        }                                                                \
    } while (0)                                                          \

    GET_BEHV(wait_on_fail, te_strtol_bool);
    GET_BEHV(wait_on_cleanup, te_strtol_bool);
    GET_BEHV(log_stack, te_strtol_bool);
    GET_BEHV(log_test_fail_state, te_strtol_bool);
    GET_BEHV(log_all_rpc, te_strtol_bool);
    GET_BEHV(cleanup_fd_leak_check, te_strtol_bool);
    GET_BEHV(cleanup_fd_close_enforce_libc, te_strtol_bool);
    GET_BEHV(prologue_sleep, test_behaviour_strtoul);
    GET_BEHV(fail_verdict, te_strtol_bool);
    GET_BEHV(rpc_fail_verdict, te_strtol_bool);
    GET_BEHV(use_chk_funcs, te_strtol_bool);
    GET_BEHV(iface_toggle_delay_ms, test_behaviour_strtoul);

#undef GET_BEHV
}
