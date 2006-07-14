/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Test Agent running on the Linux and used to control the Power
 * Distribution Units via SNMP.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
#include "ta_snmp.h"

DEFINE_LGR_ENTITY("(power-ctl)");

const char *ta_name = "(power-ctl)";

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;

/* IP address of the unit */
static struct sockaddr unit_netaddr;

/* Number of outlets in the unit */
static long int unit_size = 0;

const char *te_lockdir = "/tmp";

RCF_PCH_CFG_NODE_AGENT(node_agent, NULL);

/* APC: Default name of SNMP community with read-write access */
const char *apc_rw_community = "private";

/* APC: Commands for controlling outlets */
typedef enum {
    OUTLET_IMMEDIATE_ON     = 1,
    OUTLET_IMMEDIATE_OFF    = 2,
    OUTLET_IMMEDIATE_REBOOT = 3,
} outlet_cmd_t;

/**
 * Open SNMP session to the power unit with current default settings.
 *
 * @return Pointer to new SNMP session (should be closed 
 *         with ta_snmp_close_session()), or NULL in the case of error.
 */
ta_snmp_session *
power_snmp_open()
{
    ta_snmp_session *ss;

    ss = ta_snmp_open_session(&unit_netaddr, SNMP_VERSION_1, apc_rw_community);
    if (ss == NULL)
    {
        ERROR("Failed to open SNMP session for %s",
              te_sockaddr_get_ipstr(&unit_netaddr));
    }
    return ss;
}

/**
 * Get the size of outlet array of the unit
 *
 * @param size  Number of outlets (OUT)
 *
 * @return Status code.
 */
te_errno
power_get_size(long int *size)
{
    /* APC PowerMIB sPDUOutletControlTableSize */
    static ta_snmp_oid oid_sPDUOutletControlTableSize[] =
                    { 1, 3, 6, 1, 4, 1, 318, 1, 1, 4, 4, 1, 0 };

    ta_snmp_session *ss;
    te_errno         rc;

    ss = power_snmp_open();
    if (ss == NULL)
        return TE_EFAIL;

    rc = ta_snmp_get_int(ss, oid_sPDUOutletControlTableSize,
                         TE_ARRAY_LEN(oid_sPDUOutletControlTableSize),
                         size);
    ta_snmp_close_session(ss);
    return rc;
}

/**
 * Find number of outlet by its human-readable name
 *
 * @param name  Name of outlet
 * @param num   Number of outlet (OUT)
 *
 * @return Status code.
 */
te_errno
power_find_outlet(const char *name, long int *num)
{
    /* APC PowerMIB sPDUOutletName */
    static ta_snmp_oid oid_sPDUOutletName[] =
                    { 1, 3, 6, 1, 4, 1, 318, 1, 1, 4, 5, 2, 1, 3, 0 };

    long int         i;
    te_errno         rc, retval = TE_ENOENT;
    ta_snmp_session *ss;

    ss = power_snmp_open();
    if (ss == NULL)
        return TE_EFAIL;

    for (i = 1; i <= unit_size; i++)
    {
        char    buf[128];
        size_t  buf_len;

        oid_sPDUOutletName[TE_ARRAY_LEN(oid_sPDUOutletName) - 1] = i;
        buf_len = sizeof(buf);
        if ((rc = ta_snmp_get_string(ss, oid_sPDUOutletName,
                                     TE_ARRAY_LEN(oid_sPDUOutletName),
                                     buf, &buf_len)) != 0)
            continue;

        if (strcmp(buf, name) == 0)
        {
            *num = i;
            retval = 0;
            break;
        }
    }
    ta_snmp_close_session(ss);
    return retval;
}

/**
 * Perform command on specific outlet
 *
 * @param outlet_num  Number of outlet to control
 * @param command     Command to perform
 *
 * @return Status code.
 */
te_errno
power_set_outlet(long int outlet_num, outlet_cmd_t command)
{
    /* APC PowerMIB rPDUOutletControlOutletCommand */
    static oid oid_rPDUOutletControlOutletCommand[] =
                    { 1, 3, 6, 1, 4, 1, 318, 1, 1, 12, 3, 3, 1, 1, 4, 0 };
    ta_snmp_session *ss;
    int              rc;

    if (outlet_num > unit_size || outlet_num == 0)
        return TE_ENOENT;

    ss = power_snmp_open();
    if (ss == NULL)
        return TE_EFAIL;

    oid_rPDUOutletControlOutletCommand[
        TE_ARRAY_LEN(oid_rPDUOutletControlOutletCommand) - 1] = outlet_num;

    if ((rc = ta_snmp_set(ss, oid_rPDUOutletControlOutletCommand,
                          TE_ARRAY_LEN(oid_rPDUOutletControlOutletCommand),
                          TA_SNMP_INTEGER,
                          (uint8_t *)&command, sizeof(command))) != 0)
    {
        ERROR("%s(): failed to perform power outlet command", __FUNCTION__);
    }

    ta_snmp_close_session(ss);
    return rc;
}

/**
 * Perform rebooting an outlet
 *
 * @param id  Decimal number or human-readable name of the outlet
 *
 * @return Status code.
 */
te_errno
power_reboot_outlet(const char *id)
{
    int       rc;
    long int  outlet_num;
    char     *endptr;

    WARN("Rebooting host at outlet '%s'", id);
    outlet_num = strtol(id, &endptr, 10);
    if (*endptr != '\0')
    {
        WARN("Outlet is referenced by name '%s', looking up", id);
        rc = power_find_outlet(id, &outlet_num);
        if (rc != 0)
        {
            ERROR("Failed to find outlet named '%s'", id);
            return rc;
        }
        WARN("Found outlet number %u named '%s'", outlet_num, id);
    }

    return power_set_outlet(outlet_num, OUTLET_IMMEDIATE_REBOOT);
}

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
void *
rcf_ch_symbol_addr(const char *name, te_bool is_func)
{
    UNUSED(name);
    UNUSED(is_func);

    return NULL;
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
            rc = power_reboot_outlet(params[0]);
        else
            rc = TE_EINVAL;
        SEND_ANSWER("%d %d", rc, RCF_FUNC);
        return 0;
    }
    return -1;
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


/**
 * Get root of the tree of supported objects.
 *
 * @return root pointer
 */
rcf_pch_cfg_object *
rcf_ch_conf_root(void)
{
    return &node_agent;
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
rcf_ch_conf_release()
{
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
    const char *unit_netaddr_str;

    fprintf(stderr, "Starting power agent");
    if (argc < 4)
    {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    if ((rc = ta_log_init()) != 0)
        return rc;

    ta_name = argv[1];

    if ((unit_netaddr_str = strrchr(argv[argc - 1], ':')) != NULL)
        unit_netaddr_str++;
    else
        unit_netaddr_str = argv[argc - 1];

    if ((te_sockaddr_netaddr_from_string(unit_netaddr_str, &unit_netaddr) != 0))
    {
        fprintf(stderr, "Failed to start for '%s': invalid unit address",
                unit_netaddr_str);
        return -1;
    }
    WARN("started at '%s'\n", unit_netaddr_str);

    ta_snmp_init();

    if ((rc = power_get_size(&unit_size)) != 0)
    {
        ERROR("Failed to detect the number of outlets of unit, rc=%r", rc);
        return -1;
    }
    RING("Found APC Power Unit at %s with %d outlets",
         unit_netaddr_str, unit_size);

    snprintf(buf, sizeof(buf), "PID %lu", (unsigned long)getpid());
    rc = rcf_pch_run(argv[2], buf);
    if (rc != 0)
    {
        ERROR("Failed to rcf_pch_run(), rc=%r", rc);
        return -1;
    }

    return 0;
}
