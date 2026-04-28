/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 ARK NETWORKS LLC. All rights reserved. */
/** @file
 * @brief TAPI TA events test
 *
 * Check TAPI TA events
 */

/** @page tapi-ta-events-sample Test to check ability to handle TA events
 *
 * @objective Check tapi_ta_events to trigger and handle TA event.
 *
 * @param ta_wait      TA to run test RPCs
 * @param ta_trigger   TA to trigger TA events
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "trigger"

#include <pthread.h>

#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_ta_events.h"

/** Total number of TA events handled in this test */
static int total_ev_cnt = 0;

/** Current maximum number of expected TA events handled in this test */
static int expect_ev_cnt = 0;

/** The name of good TA event (handling of this event should not stop RPC) */
#define GOOD_EVENT "good"

/** The name of good TA event (handling of this event must stop RPC call) */
#define BAD_EVENT "bad"

/** Number of seconds between test steps */
#define STEP_TIMEOUT_S 1

/** Total number of TA events processed by the end of this test */
#define TOTAL_EVENTS 2

/** Callback to handle TA events */
static bool
ta_event_cb(const char *ta, char *name, char *value)
{
    RING("TA event: TA: '%s', event: '%s':'%s'", ta, name, value);

    total_ev_cnt++;
    if (total_ev_cnt > expect_ev_cnt)
    {
        TEST_FAIL("Unexpected TA event was handled (total=%d > expected=%d)",
                  total_ev_cnt, expect_ev_cnt);
    }

    return strcmp(name, BAD_EVENT) != 0;
}

/** Describes the thread wait routine arguments. */
typedef struct wt_ctx {
    rcf_rpc_server *rpcs; /**< RPC server */
    const char     *ta;   /**< TA to subscribe TA events */
} wt_ctx;

/** Thread with target RPC to interrupt by TA events */
static void *
wait_thread(void *arg)
{
    wt_ctx               *ctx = (wt_ctx *)arg;
    te_errno              rc;
    tapi_ta_events_handle handle;
    rcf_rpc_server       *rpcs = ctx->rpcs;
    tapi_job_factory_t   *factory = NULL;
    tapi_job_t           *job = NULL;
    int                   timeout_s = STEP_TIMEOUT_S * 2;
    char                  total_timeout_s[32];

    TAPI_ON_JMP(goto cleanup);

    TEST_STEP("Subscribe TA events");
    tapi_ta_events_subscribe(ctx->ta, GOOD_EVENT "," BAD_EVENT, ta_event_cb,
                             &handle);

    TE_SPRINTF(total_timeout_s, "%d",
               timeout_s * TOTAL_EVENTS + STEP_TIMEOUT_S);
    TEST_STEP("Start job to sleep for %s seconds", total_timeout_s);
    CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
    CHECK_RC(tapi_job_create(factory, NULL, "/usr/bin/sleep",
                             (const char *[]){"sleep", total_timeout_s, NULL},
                             NULL, &job));
    CHECK_RC(tapi_job_start(job));

    expect_ev_cnt += 1;
    TEST_STEP("Wait for %d seconds with good TA event "
              "(RPC must not stop with ECANCELED)",
              timeout_s);
    rc = tapi_job_wait(job, TE_SEC2MS(timeout_s), NULL);

    if (TE_RC_GET_ERROR(rc) != TE_EINPROGRESS)
        TEST_FAIL("Unexpected error: %r", rc);

    expect_ev_cnt += 1;
    TEST_STEP("Wait for %d seconds with good TA event "
              "(RPC should stop with ECANCELED)",
              timeout_s);
    RPC_AWAIT_ERROR(rpcs);
    rc = tapi_job_wait(job, TE_SEC2MS(timeout_s), NULL);

    if (rc != TE_RC(TE_TAPI, TE_ECORRUPTED))
        TEST_FAIL("Failed to cancel tapi_job_wait: %r != TAPI-ECORRUPTED");

    if (RPC_ERRNO(rpcs) != TE_RC(TE_RCF_API, TE_ECANCELED))
        TEST_FAIL("Failed to cancel RPC operation: %r != RCF_API-ECANCELED");

    TEST_STEP("Destroy the job instance");
    CHECK_RC(tapi_job_destroy(job, -1));
    tapi_job_factory_destroy(factory);

    TEST_STEP("Unsubscribe TA events");
    tapi_ta_events_unsubscribe(handle);

    TEST_STEP("Restart RPC server to drop unhandler replies");
    CHECK_RC(rcf_rpc_server_restart(rpcs));

cleanup:
    TAPI_JMP_POP;

    return NULL;
}

/** Thread with trigger TA events */
static void *
trigger_thread(void *arg)
{
    rcf_rpc_server *rpcs = (rcf_rpc_server *)arg;
    const char     *value = "payload";

    TEST_STEP("Waiting for the first tapi_job_wait call");
    te_motivated_sleep(STEP_TIMEOUT_S,
                       "Waiting for the middle of tapi_job_wait");

    TEST_STEP("Trigger TA %s event with value: '%s'", GOOD_EVENT, value);
    rcf_ta_events_trigger_event(rpcs->ta, GOOD_EVENT, value);

    TEST_STEP("Waiting for the second tapi_job_wait call");
    te_motivated_sleep(STEP_TIMEOUT_S * 2,
                       "Waiting for the middle of tapi_job_wait");

    TEST_STEP("Trigger TA %s event with value: '%s'", BAD_EVENT, value);
    rcf_ta_events_trigger_event(rpcs->ta, BAD_EVENT, value);

    return NULL;
}

int
main(int argc, char **argv)
{
    int err;

    const char     *ta_wait;
    rcf_rpc_server *rpcs1;
    pthread_t       thread1;
    wt_ctx          ctx;

    const char     *ta_trigger;
    rcf_rpc_server *rpcs2;
    pthread_t       thread2;

    TEST_START;

    TEST_GET_STRING_PARAM(ta_wait);
    TEST_GET_STRING_PARAM(ta_trigger);

    TEST_STEP("Create RPC servers");
    TEST_GET_RPCS(ta_wait, "rpcs1", rpcs1);
    TEST_GET_RPCS(ta_trigger, "rpcs2", rpcs2);

    TEST_STEP("Create a thread to wait TA events on agent %s", ta_wait);
    ctx = (wt_ctx){
        .rpcs = rpcs1,
        .ta = ta_trigger,
    };
    err = pthread_create(&thread1, NULL, wait_thread, (void *)&ctx);
    if (err != 0)
    {
        rc = te_rc_os2te(err);
        TEST_FAIL("Failed to create wait thread: %r", rc);
    }

    TEST_STEP("Create a thread to trigger TA event on agent %s", ta_trigger);
    err = pthread_create(&thread2, NULL, trigger_thread, (void *)rpcs2);
    if (err != 0)
    {
        rc = te_rc_os2te(err);
        TEST_FAIL("Failed to create trigger thread: %r", rc);
    }

    TEST_STEP("Waiting for all threads to complete");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    TEST_STEP("Check total number of TA events (%d)", TOTAL_EVENTS);
    if (total_ev_cnt != TOTAL_EVENTS)
    {
        TEST_FAIL("Unexpected total number of events (%d) handler in test",
                  total_ev_cnt);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
