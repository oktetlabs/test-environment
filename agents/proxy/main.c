/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proxy Test Agent
 *
 * Test Agent implementation.
 *
 * Copyright (C) 2024 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Main"

#include "te_config.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "logger_file.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "ta_common.h"
#include "agentlib.h"
#include "proxy_internal.h"

const char *ta_name = "(proxy)";

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;

const char *te_lockdir = "/tmp";


/* See description in rcf_ch_api.h */
int
rcf_ch_init(void)
{
    return 0;
}

/* See description in rcf_ch_api.h */
void
rcf_ch_lock(void)
{
    int rc = pthread_mutex_lock(&ta_lock);

    if (rc != 0)
    {
        LOG_PRINT("%s(): pthread_mutex_lock() failed: rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock(void)
{
    int rc;

    rc = pthread_mutex_trylock(&ta_lock);
    if (rc == 0)
    {
        WARN("rcf_ch_unlock() without rcf_ch_lock()!\n"
             "It may happen in the case of asynchronous cancellation.");
    }
    else if (rc != EBUSY)
    {
        LOG_PRINT("%s(): pthread_mutex_trylock() failed: rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }

    rc = pthread_mutex_unlock(&ta_lock);
    if (rc != 0)
    {
        LOG_PRINT("%s(): pthread_mutex_unlock() failed: rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }
}

/* See description in rcf_ch_api.h */
int
rcf_ch_reboot(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              const uint8_t *ba, size_t cmdlen, const char *params)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(params);

    return -1;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_configure(struct rcf_comm_connection *handle,
                 char *cbuf, size_t buflen, size_t answer_plen,
                 const uint8_t *ba, size_t cmdlen,
                 rcf_ch_cfg_op_t op, const char *oid, const char *val)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(op);
    UNUSED(oid);
    UNUSED(val);

    VERB("Configure: op %d OID <%s> val <%s>\n", op,
         oid == NULL ? "" : oid, val == NULL ? "" : val);

    /* Standard handler is OK */
    return -1;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_vread(struct rcf_comm_connection *handle,
             char *cbuf, size_t buflen, size_t answer_plen,
             rcf_var_type_t type, const char *var)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(type);
    UNUSED(var);
    return -1;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_vwrite(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              rcf_var_type_t type, const char *var, ...)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(type);
    UNUSED(var);

    /* Standard handler is OK */
    return -1;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_file(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const uint8_t *ba, size_t cmdlen,
            rcf_op_t op, const char *filename)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(op);
    UNUSED(filename);

    return -1;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_call(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const char *rtn, bool is_argv, int argc, void **params)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(rtn);
    UNUSED(is_argv);
    UNUSED(argc);
    UNUSED(params);

    /* Standard handler is OK */
    return -1;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_start_process(pid_t *pid,
                     int priority, const char *rtn, bool is_argv,
                     int argc, void **params)
{
    UNUSED(pid);
    UNUSED(priority);
    UNUSED(rtn);
    UNUSED(is_argv);
    UNUSED(argc);
    UNUSED(params);

    return TE_EOPNOTSUPP;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_start_thread(int *tid,
                    int priority, const char *rtn, bool is_argv,
                    int argc, void **params)
{
    UNUSED(tid);
    UNUSED(priority);
    UNUSED(rtn);
    UNUSED(is_argv);
    UNUSED(argc);
    UNUSED(params);

    return TE_EOPNOTSUPP;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_process(unsigned int pid)
{
    UNUSED(pid);

    return TE_EOPNOTSUPP;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_free_proc_data(unsigned int pid)
{
    UNUSED(pid);

    return TE_EOPNOTSUPP;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_kill_thread(unsigned int tid)
{
    UNUSED(tid);

    return TE_EOPNOTSUPP;
}

/* See description in rcf_ch_api.h */
int
rcf_ch_shutdown(struct rcf_comm_connection *handle,
                char *cbuf, size_t buflen, size_t answer_plen)
{
    te_errno rc;

    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    rc = ta_log_shutdown();
    if (rc != 0)
        LOG_PRINT("%s(): ta_log_shutdown() failed: rc=%d", __FUNCTION__, rc);

    /* Standard handler is OK */
    return -1;
}

/* See description in rcf_rpc_defs.h */
int
rcf_rpc_server_init(void)
{
    return 0;
}

/* See description in rcf_rpc_defs.h */
int
rcf_rpc_server_finalize(void)
{
    return 0;
}

/* See description in ta_common.h */
uint32_t
thread_self(void)
{
    return (unsigned int)pthread_self();
}

/* See description in ta_common.h */
void *
thread_mutex_create(void)
{
    static pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex = (pthread_mutex_t *)calloc(1, sizeof(*mutex));

    if (mutex != NULL)
        *mutex = init;

    return mutex;
}

/* See description in ta_common.h */
void
thread_mutex_destroy(void *mutex)
{
    free(mutex);
}

/* See description in ta_common.h */
void
thread_mutex_lock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to lock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_lock(mutex);
}

/* See description in ta_common.h */
void
thread_mutex_unlock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to unlock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_unlock(mutex);
}

/**
 * Entry point of the Test Agent.
 *
 * Usage:
 *     ta <ta_name> <communication library configuration string>
 */
int
main(int argc, char **argv)
{
    te_errno rc;
    int ret;
    char buf[16];

    te_log_init(ta_name, te_log_message_file);

    LOG_PRINT("%s", "Starting Proxy agent");

    if (argc < 4)
    {
        LOG_PRINT("%s", "Invalid number of arguments");
        return EXIT_FAILURE;
    }

    ta_name = argv[1];

    rc = ta_process_mgmt_init();
    if (rc != 0)
    {
        LOG_PRINT("ta_process_mgmt_init() failed: rc=%d", rc);
        return EXIT_FAILURE;
    }

    if ((rc = ta_log_init(ta_name)) != 0)
    {
        LOG_PRINT("ta_log_init() failed: rc=%d", rc);
        return EXIT_FAILURE;
    }

    TE_SPRINTF(buf, "PID %u", (unsigned int)getpid());

    ret = rcf_pch_run(argv[2], buf);
    if (ret != 0)
    {
        LOG_PRINT("rcf_pch_run() failed: rc=%d", ret);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
