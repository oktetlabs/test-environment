/** @file
 * @brief Switch Control Proxy Test Agent
 *
 * Implementation.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

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
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "poe_lib.h"

#define TE_LGR_USER      "Main"
#include "logger_ta.h"

extern void *rcf_ch_symbol_addr_auto(const char *name, te_bool is_func);
extern char *rcf_ch_symbol_name_auto(const void *addr);


DEFINE_LGR_ENTITY("(switch-ctl)");

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
        rcf_ch_lock();                                                  \
        rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);      \
        rcf_ch_unlock();                                                \
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

    SEND_ANSWER("%d", (ret == 0) ? 0 : EIO);
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

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_vwrite(struct rcf_comm_connection *handle,
              char *cbuf, size_t buflen, size_t answer_plen,
              rcf_var_type_t type, const char *var, ...)
{
    UNUSED(type);
    UNUSED(var);

    SEND_ANSWER("%d", EOPNOTSUPP);
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

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_csap_create(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen,
                   const char *stack, const char *params)
{
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("CSAP create: stack <%s> params <%s>",
                        stack, params);

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_csap_destroy(struct rcf_comm_connection *handle,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    csap_handle_t csap)
{
    VERB("CSAP destroy: handle %d", csap);

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_csap_param(struct rcf_comm_connection *handle,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap, const char *param)
{
    VERB("CSAP param: handle %d param <%s>",
                        csap, param);

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_start(struct rcf_comm_connection *handle,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen,
                    csap_handle_t csap, te_bool postponed)
{
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND start: handle %d %s", csap,
                        postponed ? "postponed" : "");

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_stop(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap)
{
    VERB("TRSEND stop handle %d", csap);

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_wait(struct rcf_comm_connection *handle,
                          char *cbuf, size_t buflen, size_t answer_plen,
                                            csap_handle_t csap)
{
    ERROR("recv_wait handle, csap %d", csap);
    SEND_ANSWER("%d", EOPNOTSUPP);
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_start(struct rcf_comm_connection *handle,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen, csap_handle_t csap,
                    unsigned int num, te_bool results, unsigned int timeout)
{
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRRECV start: handle %d num %u timeout %u %s",
                        csap, num, timeout, results ? "results" : "");

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_stop(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap)
{
    VERB("TRRECV stop handle %d", csap);

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_get(struct rcf_comm_connection *handle,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap)
{
    VERB("TRRECV get handle %d", csap);
    
    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_recv(struct rcf_comm_connection *handle,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen, csap_handle_t csap,
                   te_bool results, unsigned int timeout)
{
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND recv: handle %d timeout %d %s",
                        csap, timeout, results ? "results" : "");

    SEND_ANSWER("%d", EOPNOTSUPP);
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
rcf_ch_start_task(struct rcf_comm_connection *handle,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  int priority, const char *rtn, te_bool is_argv,
                  int argc, uint32_t *params)
{
    UNUSED(priority);
    UNUSED(rtn);
    UNUSED(is_argv);
    UNUSED(argc);
    UNUSED(params);

    SEND_ANSWER("%d", EOPNOTSUPP);
}


/* See description in rcf_ch_api.h */
int
rcf_ch_kill_task(struct rcf_comm_connection *handle,
                 char *cbuf, size_t buflen, size_t answer_plen,
                 unsigned int pid)
{
    UNUSED(pid);

    SEND_ANSWER("%d", EOPNOTSUPP);
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

    if ((rc = log_init()) != 0)
    {
        fprintf(stderr, "log_init() failed: error=%d\n", rc);
        return rc;
    }

    te_lgr_entity = ta_name = argv[1];

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

    rc = log_shutdown();
    if (rc != 0)
    {
        fprintf(stderr, "log_shutdown() failed: error=%d\n", rc);
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
 * @retval EIO  - failed to read ARL table
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
        return TE_RC(TE_TA_SWITCH_CTL, EIO);
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
