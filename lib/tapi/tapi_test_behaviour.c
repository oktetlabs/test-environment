/** @file
 * @brief Test behaviour switches API
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TAPI Test Behaviour"


#include "tapi_test.h"
#include "tapi_cfg.h"

#define BEHV_FIELD

/* See description in tapi_test_behaviour.h */
test_behaviour test_behaviour_storage;

void
test_behaviour_get(test_behaviour *behaviour)
{
    char *s;
    te_errno rc;

    memset(behaviour, 0, sizeof(*behaviour));

#define GET_BEHV(name_)                                                  \
    do {                                                                 \
        rc = cfg_get_instance_fmt(NULL, &s,                              \
                                  "/local:/test:/behaviour:%s", #name_); \
        if (rc == 0)                                                     \
        {                                                                \
            CHECK_RC(te_strtol_bool(s, &(behaviour-> name_)));           \
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

    GET_BEHV(wait_on_fail);
    GET_BEHV(wait_on_cleanup);
    GET_BEHV(log_stack);
    GET_BEHV(log_test_fail_state);

#undef GET_BEHV
}
