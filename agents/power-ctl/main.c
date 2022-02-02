/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Test Agent running on the Linux and used to control the Power
 * Distribution Units.
 *
 * Copyright (C) 2003-2021 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 * 
 * $Id$
 */

#define TE_LGR_USER     "Main"

#include "te_config.h"
#include "config.h"

#include <pthread.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_sockaddr.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "ta_common.h"
#include "power_ctl_internal.h"


const char *ta_name = "(power-ctl)";

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;

const char *te_lockdir = "/tmp";

#define SHELL_REBOOT_PARAM "cold_reboot_cmd"

static te_bool is_shell_reboot_type;

/* Send answer to the TEN */
#define SEND_ANSWER(_fmt...) \
    do {                                                                  \
        int _rc;                                                          \
        if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,    \
                             _fmt) >= (buflen - answer_plen))             \
        {                                                                 \
            VERB("Answer is truncated\n");                                \
        }                                                                 \
        RCF_CH_LOCK;                                                      \
        _rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);       \
        RCF_CH_UNLOCK;                                                    \
        return _rc;                                                       \
    } while (FALSE)

/* See description in rcf_ch_api.h */
int
rcf_ch_init()
{
    return 0;
}

/* See description in rcf_ch_api.h */
void
rcf_ch_lock()
{
    int rc = pthread_mutex_lock(&ta_lock);

    if (rc != 0)
        fprintf(stderr, "%s(): pthread_mutex_lock() failed - rc=%d, errno=%d",
                __FUNCTION__, rc, errno);
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock()
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
        fprintf(stderr,
                "%s(): pthread_mutex_trylock() failed - rc=%d, errno=%d",
                __FUNCTION__, rc, errno);
    }

    rc = pthread_mutex_unlock(&ta_lock);
    if (rc != 0)
        fprintf(stderr,
                "%s(): pthread_mutex_unlock() failed - rc=%d, errno=%d",
                __FUNCTION__, rc, errno);
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
            const char *rtn, te_bool is_argv, int argc, void **params)
{
    te_errno rc;

    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(rtn);
    UNUSED(is_argv);
    UNUSED(argc);
    UNUSED(params);

    if (strcmp(rtn, "cold_reboot") == 0)
    {
        if (is_argv && argc == 1)
        {
            if (is_shell_reboot_type)
            {
                rc = ta_shell_cold_reboot(params[0]);
            }
            else
            {
#ifdef HAVE_SNMP_PDU_TYPE
                rc = ta_snmp_cold_reboot(params[0]);
#else
                ERROR("Cold reboot via SNMP is not supported");
                rc = TE_EFAIL;
#endif
            }
        }
        else
        {
            rc = TE_EINVAL;
        }
    }

    if (rc != 0)
    {
        ERROR("Failed to make a cold reboot");
        rc = TE_RC(TE_RCF_PCH, rc);
    }

    SEND_ANSWER("%d %d", rc, RCF_FUNC);
    return 0;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_start_process(pid_t *pid,
                     int priority, const char *rtn, te_bool is_argv,
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
                    int priority, const char *rtn, te_bool is_argv,
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
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    /* Standard handler is OK */
    return -1;
}

/* See description in rcf_ch_api.h */
uint32_t
thread_self()
{
    return (uint32_t)pthread_self();
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

/* See description in rcf_ch_api.h */
void *
thread_mutex_create(void)
{
    static pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t *mutex = (pthread_mutex_t *)calloc(1, sizeof(*mutex));

    if (mutex != NULL)
        *mutex = init;

    return (void *)mutex;
}

/* See description in rcf_ch_api.h */
void
thread_mutex_destroy(void *mutex)
{
    free(mutex);
}

/* See description in rcf_ch_api.h */
void
thread_mutex_lock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to lock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_lock(mutex);
}

/* See description in rcf_ch_api.h */
void
thread_mutex_unlock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to unlock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_unlock(mutex);
}

/* See the description in lib/rcfpch/rcf_ch_api.h */
int
rcf_ch_conf_init()
{
    /*
     * We do not export/support any configuration object,
     * so we do not register any subtree with rcf_pch_add_node().
     */
    return 0;
}

/**
 * Get Test Agent name.
 *
 * @return name pointer
 */
const char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void
rcf_ch_conf_fini()
{
}


static int
init_cold_reboot(char *param)
{
    char *ch;
    int rc = 0;

    ch = strstr(param, "=");
    if (ch == NULL)
        return -1;

    if (strncmp(param, SHELL_REBOOT_PARAM, ch - param - 1) == 0)
    {
        is_shell_reboot_type = TRUE;
        rc = ta_shell_init_cold_reboot(param);
    }
    else
    {
#ifdef HAVE_SNMP_PDU_TYPE
        is_shell_reboot_type = FALSE;
        rc = ta_snmp_init_cold_reboot(param);
#else
        fprintf(stderr, "Cold reboot via SNMP is not supported");
        rc = -1;
#endif
    }

    return rc;
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
    int         rc;
    char        buf[16];
    char       *param;

    fprintf(stderr, "Starting power agent\n");
    if (argc < 4)
    {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    ta_name = argv[1];

    if ((rc = ta_log_init(ta_name)) != 0)
    {
        fprintf(stderr, "ta_log_init() failed: error=%d\n", rc);
        return rc;
    }

    if ((param = strrchr(argv[argc - 1], ':')) != NULL)
        param++;
    else
        param = argv[argc - 1];

    if ((rc = init_cold_reboot(param)) != 0)
    {
        fprintf(stderr, "Failed to initialize the cold reboot\n");
        return rc;
    }

    snprintf(buf, sizeof(buf), "PID %lu", (unsigned long)getpid());
    rc = rcf_pch_run(argv[2], buf);
    if (rc != 0)
    {
        fprintf(stderr, "Failed to rcf_pch_run(), rc=%d", rc);
        return rc;
    }

    return 0;
}
