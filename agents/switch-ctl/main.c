/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Switch Control Proxy Test Agent
 *
 * Implementation.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Main"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <unistd.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "poe_lib.h"


extern void *rcf_ch_symbol_addr_auto(const char *name, te_bool is_func);
extern char *rcf_ch_symbol_name_auto(const void *addr);


char *ta_name = "(switch-ctl)";


#ifdef HAVE_PTHREAD_H
static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


/* Send answer to the TEN */
#define SEND_ANSWER(_fmt...) \
    do {                                                                \
        int rc;                                                         \
                                                                        \
        if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,  \
                             _fmt) >= (buflen - answer_plen))           \
        {                                                               \
            VERB("Answer is truncated");                 \
        }                                                               \
        RCF_CH_LOCK;                                                    \
        rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);      \
        RCF_CH_UNLOCK;                                                  \
        return rc;                                                      \
    } while (FALSE)


/* See description in rcf_ch_api.h */
int
rcf_ch_init()
{
    ENTRY("main");
    EXIT("main");
    return 0;
}


/* See description in rcf_ch_api.h */
void
rcf_ch_lock()
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&ta_lock);
#endif
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock()
{
#ifdef HAVE_PTHREAD_H
    if (pthread_mutex_trylock(&ta_lock) == 0)
    {
        WARN("rcf_ch_unlock() without rcf_ch_lock()!\n"
             "It may happen in the case of asynchronous cancellation.");
    }
    pthread_mutex_unlock(&ta_lock);
#endif
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

    ENTRY("main");
    EXIT("main");

    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_reboot(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              const uint8_t *ba, size_t cmdlen, const char *params)
{
    int  ret;
    char errstr[POE_LIB_MAX_STRING];

    UNUSED(ba);
    UNUSED(cmdlen);

    ENTRY("main");

    if ((params != NULL) && (strcmp(params, "defaults") == 0))
    {
        ret = poe_system_restore_default(errstr);
    }
    else
    {
        ret = poe_system_reboot(errstr);
    }

    if (ret != 0)
    {
        ERROR("Failed %d: %s", ret, errstr);
    }

    SEND_ANSWER("%d", (ret == 0) ? 0 : TE_EIO);
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

    ENTRY("main", "Configure: op %d OID '%s' val '%s'", op,
                         (oid == NULL) ? "" : oid,
                         (val == NULL) ? "" : val);
    EXIT("main");

    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_vread(struct rcf_comm_connection *handle,
             char *cbuf, size_t buflen, size_t answer_plen,
             rcf_var_type_t type, const char *var)
{
    UNUSED(type);
    UNUSED(var);

    SEND_ANSWER("%d", TE_EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_vwrite(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              rcf_var_type_t type, const char *var, ...)
{
    UNUSED(type);
    UNUSED(var);

    SEND_ANSWER("%d", TE_EOPNOTSUPP);
}

/* See description in rcf_ch_api.h */
void *
rcf_ch_symbol_addr(const char *name, te_bool is_func)
{
    return rcf_ch_symbol_addr_auto(name, is_func);
}

/* See description in rcf_ch_api.h */
char *
rcf_ch_symbol_name(const void *addr)
{
    return rcf_ch_symbol_name_auto(addr);
}

/* See description in rcf_ch_api.h */
int
rcf_ch_file(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const uint8_t *ba, size_t cmdlen,
            rcf_op_t op, const char *filename)
{
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(op);
    UNUSED(filename);

    SEND_ANSWER("%d", TE_EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_call(struct rcf_comm_connection *handle,
            char *cbuf, size_t buflen, size_t answer_plen,
            const char *rtn, te_bool is_argv, int argc, uint32_t *params)
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
                     int priority, const char *rtn, te_bool is_argv,
                     int argc, uint32_t *params)
{
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
                    int argc, uint32_t *params)
{
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


/**
 * Entry point of the Test Agent.
 *
 * Usage:
 *     taswitch-ctl <ta_name> <communication library configuration string>
 */
int
main(int argc, char **argv)
{
    int rc, retval = 0;

    char buf[16];

    if (argc != 5)
    {
        fprintf(stderr, "Usage: taswitch-ctl <ta_name> "
                        "<communication library configuration string> "
                        "<PoE remote IPv4 address> <PoE remote port>\n");
        return -1;
    }

    ta_name = argv[1];

    if ((rc = ta_log_init(ta_name)) != 0)
    {
        fprintf(stderr, "ta_log_init() failed: error=%d\n", rc);
        return rc;
    }

    VERB("Started");

    rc = poe_lib_init_verb(argv[3], argv[4]);
    if (rc != 0)
    {
        ERROR("Failed to initialize PoE library %d", rc);
    }
    VERB("PoE library successfully initialized");

    sprintf(buf, "PID %u", getpid());

    rc = rcf_pch_run(argv[2], buf);
    if (rc != 0)
    {
        fprintf(stderr, "rcf_pch_run() failed: error=%d\n", rc);
        if (retval == 0)
            retval = rc;
    }

    poe_lib_shutdown();

    rc = ta_log_shutdown();
    if (rc != 0)
    {
        fprintf(stderr, "ta_log_shutdown() failed: error=%d\n", rc);
        if (retval == 0)
            retval = rc;
    }

    return retval;
}


/**
 * Reset DUT ARL table. All entries are deleted.
 *
 * @note ARL table may be not empty at the end of operation, but
 *       the main purpose is to remove huge number of entries.
 *
 * @return Status code.
 * @retval 0    - success
 * @retval TE_EIO  - failed to read ARL table
 */
int
arl_table_reset(void)
{
    int         rc;
    poe_arl    *arl_table = NULL;
    int         num, i;

    rc = poe_arl_read_table(&arl_table, &num, NULL);
    if (rc != 0)
    {
        ERROR("main", "poe_arl_read_table() failed %d", rc);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    for (i = 0; i < num; ++i)
    {
        rc = poe_arl_delete(arl_table + i, NULL);
        if (rc != 0)
        {
            /* It's not critical */
            VERB("main", "poe_arl_delete() failed %d", rc);
        }
    }
    free(arl_table);

    return 0;
}
