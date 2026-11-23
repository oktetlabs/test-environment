/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Check the output settings of a process
 *
 * Copyright (C) 2026 OKTET Labs Ltd. All rights reserved.
 */

/** @page process_output Log process output at a chosen level or to a file
 *
 * @objective Check that /agent/process/output sets the level the agent
 *            logs each stream of a process at, or turns the logging off,
 *            and that it writes a stream to a file on the agent.
 *
 * By default the agent logs stdout at RING and stderr at WARN and
 * writes no file. A file is emptied when the process is started by
 * setting its status, and appended to when autorestart restarts it.
 * A file that cannot be opened, or is not a regular file, fails the
 * start. The settings may be changed while the process runs and reach
 * it at its next start.
 *
 * @par Scenario:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "process_output"

#include "te_config.h"

#include "tapi_test.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "te_sleep.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_process.h"
#include "tapi_file.h"

/** Name of the process entry */
#define PS_NAME         "process_output"

/** Line the process prints to stdout */
#define OUT_MARKER      "process_output stdout marker"

/** Line the process prints to stderr */
#define ERR_MARKER      "process_output stderr marker"

/** Content written to a file before a start that must empty it */
#define STALE_CONTENTS  "stale\n"

/** Path that cannot be opened */
#define BAD_PATH        "/nonexistent-dir/x"

/** Directory, which cannot be opened for writing */
#define DIR_PATH        "/"

/** Path that can be opened for writing but is not a regular file */
#define DEV_PATH        "/dev/null"

/** Time the process is given to exit */
#define EXIT_TIMEOUT_MS     5000

/** Time the output is given to reach a file */
#define FILE_TIMEOUT_MS     5000

/** Interval between reads of a file while waiting for it */
#define FILE_POLL_MS        100

static const char *ta = "Agt_A";

/** Count the occurrences of a marker in a file contents */
static unsigned int
count_markers(const char *contents, const char *marker)
{
    unsigned int count = 0;
    const char *p;

    for (p = strstr(contents, marker); p != NULL; p = strstr(p + 1, marker))
        count++;

    return count;
}

/**
 * Wait until a file on the agent holds @p marker at least @p count
 * times, then return its contents. On timeout, the last contents read
 * are returned.
 */
static char *
wait_for_markers(const char *path, const char *marker, unsigned int count)
{
    unsigned int waited;
    char *contents;

    for (waited = 0; ; waited += FILE_POLL_MS)
    {
        CHECK_RC(tapi_file_read_ta(ta, path, &contents));
        if (count_markers(contents, marker) >= count ||
            waited >= FILE_TIMEOUT_MS)
            return contents;

        free(contents);
        te_motivated_msleep(FILE_POLL_MS, "Wait for the output to reach "
                            "the file");
    }
}

/**
 * Check that the file of a stream holds exactly the line one run of the
 * process printed to the stream, that is @p marker and a newline.
 */
static void
check_one_run(const char *stream, const char *path, const char *marker)
{
    char *contents = wait_for_markers(path, marker, 1);
    size_t len = strlen(marker);

    if (strncmp(contents, marker, len) != 0 ||
        strcmp(contents + len, "\n") != 0)
    {
        TEST_VERDICT("The %s file holds '%s' instead of exactly "
                     "the line the process printed", stream, contents);
    }

    free(contents);
}

static void
check_string_node(const char *stream, const char *leaf, const char *expected)
{
    char *value = NULL;

    CHECK_RC(cfg_get_instance_string_fmt(&value,
                                         "/agent:%s/process:%s/output:%s/%s:",
                                         ta, PS_NAME, stream, leaf));
    if (strcmp(value, expected) != 0)
    {
        TEST_VERDICT("output:%s/%s reads '%s' instead of '%s'",
                     stream, leaf, value, expected);
    }

    free(value);
}

static te_errno
set_string_node(const char *stream, const char *leaf, const char *value)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, value),
                                "/agent:%s/process:%s/output:%s/%s:",
                                ta, PS_NAME, stream, leaf);
}

static void
run_once(void)
{
    CHECK_RC(tapi_cfg_ps_start(ta, PS_NAME));
    CHECK_RC(tapi_cfg_ps_wait(ta, PS_NAME, EXIT_TIMEOUT_MS, NULL));
}

/**
 * Check that the process does not start with its stdout file set to
 * @p path, that the start fails with @p expected and that the process
 * is not running then.
 */
static void
check_bad_file(const char *path, unsigned int expected)
{
    bool is_running;
    te_errno rc;

    CHECK_RC(set_string_node("stdout", "file", path));
    rc = tapi_cfg_ps_start(ta, PS_NAME);
    if (rc == 0)
        TEST_VERDICT("The process started with stdout file '%s'", path);
    if (TE_RC_GET_ERROR(rc) != expected)
    {
        TEST_VERDICT("The start with stdout file '%s' failed with %r "
                     "instead of %r", path, rc, expected);
    }
    RING("The start with stdout file '%s' failed as expected: %r",
         path, rc);

    CHECK_RC(tapi_cfg_ps_get_status(ta, PS_NAME, &is_running));
    if (is_running)
        TEST_VERDICT("The process runs although its start failed");
}

/** Remove a file on the agent; a file that does not exist is fine */
static te_errno
remove_ta_file(const char *path)
{
    te_errno rc = rcf_ta_del_file(ta, 0, path);

    return TE_RC_GET_ERROR(rc) == TE_ENOENT ? 0 : rc;
}

int
main(int argc, char *argv[])
{
    te_string name = TE_STRING_INIT;
    char *out_path = NULL;
    char *out2_path = NULL;
    char *err_path = NULL;
    cfg_handle *streams = NULL;
    unsigned int n_streams = 0;
    bool ps_added = false;
    bool is_running;
    char *contents = NULL;

    TEST_START;

    tapi_file_make_name(&name);
    out_path = tapi_file_resolve_ta_pathname(NULL, ta,
                                             TAPI_CFG_BASE_TA_DIR_TMP,
                                             name.ptr);
    CHECK_NOT_NULL(out_path);
    te_string_append(&name, "_2");
    out2_path = tapi_file_resolve_ta_pathname(NULL, ta,
                                              TAPI_CFG_BASE_TA_DIR_TMP,
                                              name.ptr);
    CHECK_NOT_NULL(out2_path);
    te_string_reset(&name);
    tapi_file_make_name(&name);
    err_path = tapi_file_resolve_ta_pathname(NULL, ta,
                                             TAPI_CFG_BASE_TA_DIR_TMP,
                                             name.ptr);
    CHECK_NOT_NULL(err_path);

    TEST_STEP("Add a process printing a line to stdout and another one "
              "to stderr, check that the defaults read back as RING, WARN "
              "and no files, then run it once: the agent logs both lines");
    CHECK_RC(tapi_cfg_ps_add(ta, PS_NAME, "/bin/sh", false));
    ps_added = true;
    CHECK_RC(tapi_cfg_ps_add_arg(ta, PS_NAME, 1, "-c"));
    CHECK_RC(tapi_cfg_ps_add_arg(ta, PS_NAME, 2,
                                 "echo " OUT_MARKER "; echo " ERR_MARKER
                                 " >&2"));

    CHECK_RC(cfg_find_pattern_fmt(&n_streams, &streams,
                                  "/agent:%s/process:%s/output:*",
                                  ta, PS_NAME));
    if (n_streams != 2)
        TEST_VERDICT("The process has %u output streams instead of 2",
                     n_streams);

    check_string_node("stdout", "log_level", "RING");
    check_string_node("stderr", "log_level", "WARN");
    check_string_node("stdout", "file", "");
    check_string_node("stderr", "file", "");

    run_once();

    TEST_STEP("Write stdout to a file in the agent tmp dir and do not log "
              "it, run the process: the file holds the line it printed");
    CHECK_RC(tapi_file_create_ta(ta, out_path, "%s", STALE_CONTENTS));
    CHECK_RC(set_string_node("stdout", "file", out_path));
    CHECK_RC(set_string_node("stdout", "log_level", "none"));
    check_string_node("stdout", "file", out_path);
    check_string_node("stdout", "log_level", "none");

    run_once();
    check_one_run("stdout", out_path, OUT_MARKER);

    TEST_STEP("Run the process again: the file is emptied, not appended to");
    /*
     * The stale line is what would be left if the start appended: the
     * marker already in the file cannot tell, since it is the expected
     * content.
     */
    CHECK_RC(tapi_file_create_ta(ta, out_path, "%s", STALE_CONTENTS));
    run_once();
    check_one_run("stdout", out_path, OUT_MARKER);

    TEST_STEP("Write stderr to a file of its own, run the process: each "
              "file holds only the line printed to its stream");
    CHECK_RC(set_string_node("stderr", "file", err_path));
    check_string_node("stderr", "file", err_path);

    run_once();
    check_one_run("stderr", err_path, ERR_MARKER);
    check_one_run("stdout", out_path, OUT_MARKER);

    /* The later steps check stdout only */
    CHECK_RC(set_string_node("stderr", "file", ""));

    TEST_STEP("Set a file that cannot be opened, then a directory, then "
              "a device: each start fails and the process is not running; "
              "restore the file");
    check_bad_file(BAD_PATH, TE_ENOENT);
    check_bad_file(DIR_PATH, TE_EISDIR);
    check_bad_file(DEV_PATH, TE_EINVAL);
    CHECK_RC(set_string_node("stdout", "file", out_path));

    TEST_STEP("Set an unknown log level: TE_EINVAL, the value is kept");
    rc = set_string_node("stdout", "log_level", "bogus");
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
        TEST_VERDICT("Setting log level 'bogus' returned %r", rc);
    /* Level names are case-sensitive */
    rc = set_string_node("stdout", "log_level", "ring");
    if (TE_RC_GET_ERROR(rc) != TE_EINVAL)
        TEST_VERDICT("Setting log level 'ring' returned %r", rc);
    check_string_node("stdout", "log_level", "none");

    TEST_STEP("Start the process with autorestart: the start empties the "
              "file, the restarts append to it");
    CHECK_RC(tapi_file_create_ta(ta, out_path, "%s", STALE_CONTENTS));
    CHECK_RC(tapi_cfg_ps_set_autorestart(ta, PS_NAME, 1));
    CHECK_RC(tapi_cfg_ps_start(ta, PS_NAME));

    contents = wait_for_markers(out_path, OUT_MARKER, 2);
    RING("The stdout file holds:\n%s", contents);
    if (count_markers(contents, OUT_MARKER) < 2)
        TEST_VERDICT("The file does not grow across autorestarts");
    if (strstr(contents, STALE_CONTENTS) != NULL)
        TEST_VERDICT("The start with autorestart did not empty the file");
    free(contents);
    contents = NULL;

    TEST_STEP("Change the file while the process is running: the set is "
              "accepted and the next restart writes to the new file");
    CHECK_RC(tapi_cfg_ps_get_status(ta, PS_NAME, &is_running));
    if (!is_running)
        TEST_FAIL("The autorestarted process is reported as stopped");
    /* Created here so that it can be read before the agent opens it */
    CHECK_RC(tapi_file_create_ta(ta, out2_path, "%s", ""));
    CHECK_RC(set_string_node("stdout", "file", out2_path));

    contents = wait_for_markers(out2_path, OUT_MARKER, 1);
    if (count_markers(contents, OUT_MARKER) < 1)
        TEST_VERDICT("A restart after the file change did not write to "
                     "the new file");
    free(contents);
    contents = NULL;

    CHECK_RC(tapi_cfg_ps_stop(ta, PS_NAME));

    TEST_STEP("Delete the process: its output files stay");
    CHECK_RC(tapi_cfg_ps_del(ta, PS_NAME));
    ps_added = false;

    CHECK_RC(tapi_file_read_ta(ta, out_path, &contents));
    if (count_markers(contents, OUT_MARKER) < 2)
        TEST_VERDICT("The stdout file lost its contents with the process");

    TEST_SUCCESS;

cleanup:
    free(contents);
    free(streams);

    if (ps_added)
        CLEANUP_CHECK_RC(tapi_cfg_ps_del(ta, PS_NAME));
    if (out_path != NULL)
        CLEANUP_CHECK_RC(remove_ta_file(out_path));
    if (out2_path != NULL)
        CLEANUP_CHECK_RC(remove_ta_file(out2_path));
    if (err_path != NULL)
        CLEANUP_CHECK_RC(remove_ta_file(err_path));

    free(out_path);
    free(out2_path);
    free(err_path);
    te_string_free(&name);

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
