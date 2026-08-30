/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 ARK NETWORKS LLC. All rights reserved. */
/** @file
 * @brief TAPI TA events test
 *
 * Check TAPI TA events
 */

/** @page tapi-ta-events-sample Trivial test to check TAPI API for TA events
 *
 * @objective Check tapi_ta_events to subscrube/unsubscribe TA event handlers
 *            without triggering TA events.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "sample"

#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_ta_events.h"

#include "te_string.h"

/**
 * Callback to handle TA events
 *
 * @note Should not be called in this test
 */
static bool
ta_event_cb(const char *ta, char *name, char *value)
{
    TEST_FAIL("Unexpected TA event: TA: '%s', event: '%s':'%s'", ta, name,
              value);

    return true;
}

/**
 * Generate comma separated list of TA event names
 *
 * @param buf    Buffer to generate list
 * @param size   Size of @p buf
 */
static void
gen_ta_event_names(char *buf, size_t size)
{
    unsigned    i, index;
    unsigned    mask = 0;
    te_string   result = TE_STRING_EXT_BUF_INIT(buf, size);
    const char *names[] = {
        "event1", "event2", "event3", "foo", "bar",
    };

    for (i = 0; i < 3; i++)
    {
        index = rand_range(0, TE_ARRAY_LEN(names) - 1);
        if ((mask & (1 << index)) != 0)
            continue;
        mask |= (1 << index);

        te_string_append(&result, "%s,", names[index]);
    }

    te_string_cut(&result, 1);
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *rpcs;

    TEST_START;

    TEST_STEP("Create RPC servers");
    TEST_GET_RPCS("Agt_A", "rpcs", rpcs);

    TEST_STEP("One TA event handler");
    {
        tapi_ta_events_handle handle;

        TEST_SUBSTEP("Subscribe TA events");
        tapi_ta_events_subscribe(rpcs->ta, "event1,event2", ta_event_cb,
                                 &handle);

        TEST_SUBSTEP("Unsubscribe TA events");
        tapi_ta_events_unsubscribe(handle);
    }

    TEST_STEP("Multiple TA event handlers");
    {
        unsigned              i, index;
        bool                  active[32] = {false};
        tapi_ta_events_handle handles[32];
        char                  names[128];

        for (i = 0; i < 100; i++)
        {
            index = rand_range(0, TE_ARRAY_LEN(handles) - 1);
            if (!active[index])
            {
                gen_ta_event_names(names, sizeof(names));

                TEST_SUBSTEP("Subscribe TA events for handle[%d]", index);
                tapi_ta_events_subscribe(rpcs->ta, names, ta_event_cb,
                                         &handles[index]);
            }
            else
            {
                TEST_SUBSTEP("Unsubscribe TA events for handle[%d] (%u)",
                             index, handles[index]);
                tapi_ta_events_unsubscribe(handles[index]);
            }

            active[index] = !active[index];
        }

        TEST_SUBSTEP("Unsubscribe remaining TA events handles");
        for (index = 0; index < TE_ARRAY_LEN(handles); index++)
        {
            if (active[index])
            {
                TEST_SUBSTEP("Unsubscribe TA events for handle[%d] (%u)",
                             index, handles[index]);
                tapi_ta_events_unsubscribe(handles[index]);
            }
        }
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
