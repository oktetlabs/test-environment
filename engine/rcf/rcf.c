/** @file
 * @brief Test Environment
 *
 * RCF main process
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for RCF
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <dlfcn.h>

#include "te_stdint.h"
#include "te_printf.h"
#include "te_errno.h"
#include "te_defs.h"
#include "ipc_server.h"
#include "rcf_methods.h"
#include "rcf_api.h"
#include "rcf_internal.h"

#define RCF_NEED_TYPES      1
#define RCF_NEED_TYPE_LEN   1
#include "te_proto.h"

#include "logger_api.h"
#include "logger_ten.h"

#define TE_EXPAND_XML 1
#include "te_expand.h"


#define RCF_SELECT_TIMEOUT      1   /**< Default select timeout
                                         in seconds */
#define RCF_CMD_TIMEOUT         100  /**< Default timeout (in seconds) for
                                          command processing on the TA */
#define RCF_CMD_TIMEOUT_HUGE    10000  
                                    /**< Huge timeout for command
                                         processing on the TA */
#define RCF_REBOOT_TIMEOUT      60  /**< TA reboot timeout in seconds */
#define RCF_SHUTDOWN_TIMEOUT    5   /**< TA shutdown timeout in seconds */

#define RCF_COLD_REBOOT_MIN_TIMEOUT 60  /**< Initial timeout for
                                             cold reboot, in seconds */
#define RCF_COLD_REBOOT_MAX_TIMEOUT 240 /**< Maximal timeout for
                                             cold reboot, in seconds */
#define RCF_COLD_REBOOT_INTERVAL    10  /**< Interval between agent
                                             restarting attempts
                                             after cold reboot,
                                             in seconds */


/** Special session identifiers */
enum {
    RCF_SID_GET_LOG = 1,    /**< Session used for Log gathering */
    RCF_SID_TACHECK,        /**< Session used for TA check */

    /** Unused SID, must be the last in the enum */
    RCF_SID_UNUSED,
};



/*
 * TA reboot and RCF shutdown algorithms.
 *
 * TA reboot:
 *     send a reboot command to TA with first free SID;
 *     reboot_num++; ta.reboot_timestamp = time(NULL);
 *     wait until time() - ta.reboot_timestamp > RCF_REBOOT_TIMESTAMP or
 *     response from TA is received;
 *     (if other reboot requests from user are received
 *     reply EINPROGRESS).
 *     reboot_num--; ta.reboot_timestamp = 0;
 *
 *     If the agent is not proxy or timeout occurred:
 *         ta.finish();
 *         ta.start() (if fails, goto shutdown)
 *         ta.connect() (if fails, goto shutdown)
 *         synchronize time;
 *     response to user reboot request;
 *     response to all sent, waiting and pending requests (TE_ETAREBOOTED).
 *
 * reboot_num variable is necessary to avoid TA list scanning every time
 * when select() is returned (list scanning is performed only if
 * reboot_num > 0).
 *
 * RCF shutdown:
 *     send a shutdown command to TA with first free SID to all Test Aents;
 *     shutdown_num = ta_num;
 *     wait until time(NULL) - ta.reboot_timestamp > RCF_SHUTDOWN_TIMESTAMP
 *     or response from all TA is received (set flag TA_DOWN and decrement
 *     shutdown_num every time when response is received);
 *     for all TA with TA_DOWN flag clear ta.finish();
 *     response to all sent, waiting and pending requests (EIO);
 *     response to user shutdown request.
 */


/** One request from the user */
typedef struct usrreq {
    struct usrreq            *next;
    struct usrreq            *prev;
    rcf_msg                  *message;
    struct ipc_server_client *user;
    uint32_t                  timeout;  /**< Timeout in seconds */
    time_t                    sent;
} usrreq;

/** A description for a task/thread to be executed at TA startup */
typedef struct ta_initial_task {
    rcf_execute_mode        mode;          /**< Task execution mode */
    char                   *entry;         /**< Procedure entry point */
    int                     argc;          /**< Number of arguments */
    char                  **argv;          /**< Arguments as strings */
    struct ta_initial_task *next;          /**< Link to the next task */
} ta_initial_task;

/** Structure for one Test Agent */
typedef struct ta {
    struct ta          *next;               /**< Link to the next TA */
    rcf_talib_handle    handle;             /**< Test Agent handle returted
                                                 by start() method */
    char               *name;               /**< Test Agent name */
    char               *type;               /**< Test Agent type */
    te_bool             enable_synch_time;  /**< Enable synchronize time */
    char               *conf;               /**< Configuration string */
    usrreq              sent;               /**< User requests sent
                                                 to the TA */
    usrreq              waiting;            /**< User requests waiting
                                                 for unblocking of 
                                                 TA connection */ 
    usrreq              pending;            /**< User requests pending
                                                 until answer on previous
                                                 request with the same SID
                                                 is received */
    unsigned int        flags;              /**< Test Agent flags */
    time_t              reboot_timestamp;   /**< Time of reboot command
                                                 sending (in seconds) */
    time_t              restart_timestamp;
    int                 sid;                /**< Free session identifier 
                                                 (starts from 2) */
    te_bool             conn_locked;        /**< Connection is locked until
                                                 the response from TA 
                                                 is received */      
    int                 lock_sid;           /**< SID of the command
                                                 locked the connection */
    void               *dlhandle;           /**< Dynamic library handle */
    ta_initial_task    *initial_tasks;      /**< Startup tasks */
    char               *cold_reboot_ta;     /**< Cold reboot TA name */
    const char         *cold_reboot_param;  /**< Cold reboot params */

    te_bool            dynamic;             /**< Dynamic creation flag */

    /** @name Methods */
    rcf_talib_start     start;              /**< Start TA */
    rcf_talib_close     close;              /**< Close TA */
    rcf_talib_finish    finish;             /**< Stop TA */
    rcf_talib_connect   connect;            /**< Connect to TA */
    rcf_talib_transmit  transmit;           /**< Transmit to TA */
    rcf_talib_is_ready  is_ready;           /**< Is data from TA pending */
    rcf_talib_receive   receive;            /**< Receive from TA */
    /*@}*/
} ta;


DEFINE_LGR_ENTITY("RCF");


/**
 * TA check initiator data.
 */
typedef struct ta_check {
    usrreq         *req;    /**< User request */
    unsigned int    active; /**< Number of active checks */
} ta_check;

static ta_check ta_checker;


#define RCF_FOREGROUND  0x01    /**< Flag to run RCF in foreground */
static unsigned int flags = 0;  /**< Global flags */

static const char *cfg_file;    /**< Configuration file name */
static ta *agents = NULL;       /**< List of Test Agents */
static int ta_num = 0;          /**< Number of Test Agents */
static int reboot_num = 0;      /**< Number of TA which should be 
                                     for rebooted */
static int shutdown_num = 0;    /**< Number of TA which should be 
                                     for shut down */

static struct ipc_server *server = NULL;    /**< IPC Server handle */

static char cmd[RCF_MAX_LEN];   /**< Test Protocol command location */
static char names[RCF_MAX_LEN - sizeof(rcf_msg)];   /**< TA names */
static int  names_len = 0;      /**< Length of TA name list */

/* Backup select parameters */
static struct timeval tv0;
static fd_set set0;

/** Name of directory for temporary files */
static char *tmp_dir;


/* Forward declarations */
static usrreq * alloc_usrreq(void);
static int write_str(char *s, size_t len);
static int send_cmd(ta *agent, usrreq *req);
static void rcf_ta_check_done(usrreq *req);
static te_errno cold_reboot(ta *agent);
static void send_all_pending_commands(ta *agent);

/*
 * Release memory allocated for Test Agents structures.
 */
static void
free_ta_list()
{
    ta *agent, *next;

    ta_initial_task *task, *nexttask;
    

    for (agent = agents; agent != NULL; agent = next)
    {
        next = agent->next;

        if (agent->dlhandle != NULL && dlclose(agent->dlhandle) != 0)
            ERROR("dlclose() failed");

        free(agent->name);
        free(agent->type);
        free(agent->conf);

        for (task = agent->initial_tasks; task != NULL; task = nexttask)
        {
            nexttask = task->next;
            for (; task->argc; task->argc--)
            {
                free(task->argv[task->argc - 1]);
            }
            free(task->entry);
            free(task->argv);
            free(task);
        }

        free(agent);
    }
    agents = NULL;
}

/**
 * Obtain TA structure address by Test Agent name.
 *
 * @param name          Test Agent name
 *
 * @return TA structure pointer or NULL
 */
static ta *
find_ta_by_name(char *name)
{
    ta *tmp;

    for (tmp = agents; tmp != NULL; tmp = tmp->next)
        if (strcmp(name, tmp->name) == 0)
            return tmp;

    return NULL;
}

/**
 * Check if a message with the same SID is already sent.
 *
 * @param req           request list anchor (ta->sent or ta->failed)
 * @param sid           session identifier of the received user request
 */
static usrreq *
find_user_request(usrreq *req, int sid)
{
    usrreq *tmp;

    for (tmp = req->next; tmp != req; tmp = tmp->next)
    {
        if (tmp->message->sid == sid)
            return tmp;
    }

    return NULL;
}

/**
 * Load shared library to control the Test Agent and resolve method
 * routines.
 *
 * @param agent         Test Agent structure
 * @param libname       name of the shared library (without "lib" prefix)
 *
 * @return 0 (success) or -1 (failure)
 */
static int
resolve_ta_methods(ta *agent, char *libname)
{
    void *handle;
    char name[RCF_MAX_NAME];

    sprintf(name, "lib%s.so", libname);
    if ((handle = dlopen(name, RTLD_LAZY)) == NULL)
    {
        ERROR("FATAL ERROR: Cannot load shared library %s errno %s", 
              libname, dlerror());
        return -1;
    }

#define RESOLVE_SYMBOL(sym) \
    do {                                                        \
        sprintf(name, "%s_"#sym, libname);                      \
        if ((agent->sym = dlsym(handle, name)) == NULL)         \
        {                                                       \
            ERROR("FATAL ERROR: Cannot resolve symbol '%s' "    \
                  "in the shared library", name);               \
            if (dlclose(handle) != 0)                           \
                ERROR("dlclose() failed");                      \
            return -1;                                          \
        }                                                       \
    } while (0)

    RESOLVE_SYMBOL(start);
    RESOLVE_SYMBOL(close);
    RESOLVE_SYMBOL(finish);
    RESOLVE_SYMBOL(connect);
    RESOLVE_SYMBOL(transmit);
    RESOLVE_SYMBOL(is_ready);
    RESOLVE_SYMBOL(receive);

#undef RESOLVE_SYMBOL

    agent->dlhandle = handle;

    return 0;
}

/**
 * Parse configuration file and initializes list of Test Agents.
 *
 * @param filename      full name of the configuration file
 *
 * @return 0 (success) or -1 (failure)
 */
static int
parse_config(const char *filename)
{
    char *name_ptr = names;
    ta   *agent;
    ta_initial_task *ta_task = NULL;

    xmlDocPtr  doc;
    xmlNodePtr cur;
    xmlNodePtr task;
    xmlNodePtr arg;

    if ((doc = xmlParseFile(filename)) == NULL)
    {
        ERROR("error occured during parsing configuration file %s\n",
                  filename);
        return -1;
    }

    if ((cur = xmlDocGetRootElement(doc)) == NULL)
    {
        ERROR("Empty configuration file");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return -1;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *)"rcf") != 0)
        goto bad_format;

    for (cur = cur->xmlChildrenNode; cur != NULL; cur = cur->next)
    {
        char *attr;

        if (xmlStrcmp(cur->name , (const xmlChar *)"ta") != 0)
            continue;

        attr = xmlGetProp_exp(cur, (const xmlChar *)"disabled");
        if (attr != NULL)
        {
            if (strcmp(attr, "yes") == 0)
            {
                free(attr);
                continue;
            }
            free(attr);
        }

        if ((agent = (ta *)calloc(1, sizeof(ta))) == NULL)
            return -1;

        agent->next = agents;
        agents = agent;
        agent->flags = TA_DEAD;

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"name")) != NULL)
        {
            agent->name = attr;
        }
        else
            goto bad_format;

        strcpy(name_ptr, agent->name);
        if (names_len + strlen(agent->name) + 1 > sizeof(names))
        {
            ERROR("FATAL ERROR: Too many Test Agents - "
                  "increase memory constants");
            goto error;
        }
        name_ptr += strlen(agent->name) + 1;
        names_len += strlen(agent->name) + 1;

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"type")) != NULL)
        {
            agent->type = attr;
        }
        else
            goto bad_format;


        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"rcflib")) != NULL)
        {
            if (resolve_ta_methods(agent, (char *)attr) != 0)
            {
                free(attr);
                goto error;
            }
            free(attr);
        }
        else
            goto bad_format;

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"synch_time")) 
            != NULL)
        {
            agent->enable_synch_time = (strcmp(attr, "yes") == 0);
            free(attr);
        }
        else
            agent->enable_synch_time = FALSE;

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"confstr")) 
            != NULL)
        {
            agent->conf = attr;
        }
        else
            agent->conf = strdup("");

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"rebootable")) 
            != NULL)
        {
            if (strcmp(attr, "yes") == 0)
                agent->flags |= TA_REBOOTABLE;
           free(attr);
        }

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"cold_reboot"))
            != NULL)
        {
            char *param;

            if ((param = strchr(attr, ':')) != 0)
            {
                agent->cold_reboot_ta = attr;
                *param = '\0';
                agent->cold_reboot_param = param + 1;
            }
            else
                free(attr);
        }

        if ((attr = xmlGetProp_exp(cur, (const xmlChar *)"fake")) != NULL)
        {
            if (strcmp(attr, "yes") == 0)
            {
                /* TA is already running under gdb */
                agent->flags |= TA_FAKE;
            }
            free(attr);
        }

        for (task = cur->xmlChildrenNode; task != NULL; task = task->next)
        {
            char *condition;

            if (xmlStrcmp(task->name, (const xmlChar *)"thread") != 0 &&
                xmlStrcmp(task->name, (const xmlChar *)"task") != 0 &&
                xmlStrcmp(task->name, (const xmlChar *)"function") != 0)
                continue;

            condition = xmlGetProp_exp(task, (const xmlChar *)"when");
            if (condition != NULL)
            {
                if (*condition == '\0')
                {
                    free(condition);
                    continue;
                }
                free(condition);
            }

            ta_task = malloc(sizeof(*ta_task));
            if (ta_task == NULL)
            {
                ERROR("malloc failed: %d", errno);
                goto error;
            }
            ta_task->mode = 
                (xmlStrcmp(task->name , (const xmlChar *)"thread") == 0 ?
                 RCF_THREAD : 
                 (xmlStrcmp(task->name , (const xmlChar *)"function") == 0 ?
                  RCF_FUNC : RCF_PROCESS));
            
            if ((attr = xmlGetProp_exp(task, (const xmlChar *)"name")) 
                != NULL)
            {
                ta_task->entry = attr;
            }
            else
            {
                INFO("No name attribute in <task>/<thread>");
                goto bad_format;
            }

            ta_task->argc = 0;
            ta_task->argv = malloc(sizeof(*ta_task->argv) * RCF_MAX_PARAMS);
            for (arg = task->xmlChildrenNode; arg != NULL; arg = arg->next)
            {
                if (xmlStrcmp(arg->name, (const xmlChar *)"arg") != 0)
                    continue;
                if ((attr = xmlGetProp_exp(arg, (const xmlChar *)"value")) 
                    != NULL)
                {
                    ta_task->argv[ta_task->argc++] = attr;
                }
                else
                {
                    ERROR("No value attribute in <arg>");
                    goto bad_format;
                }
            }
            ta_task->next = agent->initial_tasks;
            agent->initial_tasks = ta_task;
        }

        agent->sent.prev = agent->sent.next = &(agent->sent);
        agent->pending.prev = agent->pending.next = &(agent->pending);
        agent->waiting.prev = agent->waiting.next = &(agent->waiting);

        agent->sid = RCF_SID_UNUSED;

        ta_num++;
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;

bad_format:
    ERROR("Wrong configuration file format");

error:
    xmlFreeDoc(doc);
    xmlCleanupParser();
    free_ta_list();

    return -1;
}


/**
 * Wait for a response from TA
 *
 * @param agent    Test Agent to interact
 *
 * @return 0 (success) or -1 (failure)
 */
static int
consume_answer(ta *agent)
{
    struct timeval tv;
    fd_set         set;
    time_t         t0, t;

    t = t0 = time(NULL);
    while (t - t0 < RCF_SHUTDOWN_TIMEOUT)
    {
        char *ba;

        set = set0;
        tv = tv0;
        select(FD_SETSIZE, &set, NULL, NULL, &tv);
        if ((agent->is_ready)(agent->handle))
        {
            size_t len = sizeof(cmd);

            if ((agent->receive)(agent->handle, cmd, &len, &ba) != 0)
            {
                ERROR("Failed to receive answer from TA %s", agent->name);
                return -1;
            }
            return 0;
        }
        t = time(NULL);
    }

    ERROR("Failed to receive answer from TA %s - timed out", agent->name);
    return -1;

}


/**
 * Send time synchronization command to the Test Agent and wait an answer
 *
 * @param agent         Test Agent structure
 *
 * @return 0 (success) or -1 (failure)
 */
static int
synchronize_time(ta *agent)
{
    struct timeval tv;
    int            rc;

    gettimeofday(&tv, NULL);

    sprintf(cmd, "%s time string %u:%u", TE_PROTO_VWRITE,
                  (unsigned)tv.tv_sec, (unsigned)tv.tv_usec);
    if ((rc = (agent->transmit)(agent->handle, cmd, strlen(cmd) + 1)) != 0)
    {
        ERROR("Failed to transmit command to TA '%s' error=%r", 
              agent->name, rc);
        return -1;
    }

    rc = consume_answer(agent);

    if (rc == 0 && strcmp(cmd, "0") != 0)
    {
        WARN("Time synchronization failed for TA %s: "
             "log may be inconsistent", agent->name);
    }
    else
    {
        struct timeval tv2;
        
        gettimeofday(&tv2, NULL);
        if (tv2.tv_sec - tv.tv_sec > 1)
            WARN("Possible time drift is larger than 1s");
        else
        {
            INFO("Possible time drift: %u us", 
                 (tv2.tv_usec + (tv2.tv_sec == tv.tv_sec ? 0 : 1000000) 
                  - tv.tv_usec) / 2);
        }
        
    }
    return rc;
}

/**
 * Respond to user request and remove the request from the list.
 *
 * @param req           request with already filled out parameters
 */
static void
answer_user_request(usrreq *req)
{
    if (req->message->error != 0)
        req->message->data_len = 0;
        
    if (req->user != NULL)
    {
        int rc;

        INFO("Send %sreply for %u:%d:'%s' to user '%s'",
             (req->message->flags & INTERMEDIATE_ANSWER) ?
                 "intermediate " : "",
             (unsigned)req->message->seqno, req->message->sid,
             rcf_op_to_string(req->message->opcode),
             ipc_server_client_name(req->user));

        rc = ipc_send_answer(server, req->user, (char *)req->message,
                             sizeof(rcf_msg) + req->message->data_len);
        if (rc != 0)
        {
            ERROR("Cannot send an answer to user: error=%r", rc);
            RING("Failed msg has: opcode %d; TA %s; SID %d; file %s;", 
                 req->message->opcode, 
                 req->message->ta, 
                 req->message->sid, 
                 req->message->file);
        }
    }
    else if (req->message->sid == RCF_SID_TACHECK)
    {
        if (ta_checker.req != NULL)
            rcf_ta_check_done(req);
        else
            ERROR("Unexpected answer with TA checker SID=%d",
                  req->message->sid);
    }
    if (!(req->message->flags & INTERMEDIATE_ANSWER))
    {
        free(req->message);
        if (req->prev != NULL)
            (req->prev)->next = req->next;
        if (req->next != NULL)
            (req->next)->prev = req->prev;
        free(req);
    }
    else
    {
        /* Intermediate flag is valid for one reply only */
        req->message->flags &= ~INTERMEDIATE_ANSWER;
    }
}

/**
 * Respond to all user requests in the specified list with specified error.
 *
 * @param req           anchor of request list (&ta->sent or &ta->pending)
 * @param error         error to be filled in
 */
static void
answer_all_requests(usrreq *req, int error)
{
    usrreq *tmp, *next;

    for (tmp = req->next ; tmp != req ; tmp = next)
    {
        next = tmp->next;
        tmp->message->error = TE_RC(TE_RCF, error);
        answer_user_request(tmp);
    }
}


/**
 * Run all registered startup task
 *
 * @param agent   Test Agent to interact
 *
 * @return 0 (success) or -1 (failure)
 */
static int
startup_tasks(ta *agent)
{
    ta_initial_task *task;
    int              i;
    char            *args;
    int rc;
    
    for (task = agent->initial_tasks; task; task = task->next)
    {
        sprintf(cmd,  "SID 0 " TE_PROTO_EXECUTE " %s %s",
                task->mode == RCF_FUNC ? TE_PROTO_FUNC :
                task->mode == RCF_THREAD ? TE_PROTO_THREAD 
                : TE_PROTO_PROCESS, task->entry);
        args = cmd + strlen(cmd);
        if (task->argc > 0)
            strcat(cmd, " argv ");
        for (i = 0; i < task->argc; i++)
        {
            write_str(task->argv[i], strlen(task->argv[i]));
            strcat(cmd, " ");
        }
        RING("Running startup task(%s) on TA '%s': entry-point='%s' "
             "args=%s",
             (task->mode == RCF_FUNC) ? "function" :
             (task->mode == RCF_THREAD) ? "thread" : "process",
             agent->name, task->entry, args);
        VERB("Running startup task %s", cmd);
        (agent->transmit)(agent->handle, cmd, strlen(cmd) + 1);
        rc = consume_answer(agent);
        
        if (rc != 0 || strncmp(cmd, "SID 0 0", 7) != 0)
        {
            WARN("Startup task '%s' failed on %s", task->entry, 
                 agent->name);
            return -1;
        }
        VERB("Startup task %s succeeded", cmd);
    }
    return 0;
}

/**
 * Mark test agent as recoverable dead.
 *
 * @param agent     Test Agent
 */
static void
set_ta_dead(ta *agent)
{
    if (~agent->flags & TA_DEAD)
    {
        int rc;

        ERROR("TA '%s' is dead", agent->name);
        answer_all_requests(&(agent->sent), TE_ETADEAD);
        answer_all_requests(&(agent->waiting), TE_ETADEAD);
        rc = (agent->close)(agent->handle, &set0);
        if (rc != 0)
            ERROR("Failed to close connection with TA '%s': rc=%r",
                  agent->name, rc);
        agent->flags |= TA_DEAD;
        agent->conn_locked = FALSE;
    }
}

/**
 * Mark test agent as unrecoverable dead.
 *
 * @param agent     Test Agent
 */
static void
set_ta_unrecoverable(ta *agent)
{
    if (~agent->flags & TA_UNRECOVER)
    {
        ERROR("TA '%s' is unrecoverable dead", agent->name);
        answer_all_requests(&(agent->sent), TE_ETADEAD);
        answer_all_requests(&(agent->pending), TE_ETADEAD);
        answer_all_requests(&(agent->waiting), TE_ETADEAD);
        if (agent->handle != NULL)
        {
            int rc;

            if (~agent->flags & TA_DEAD)
            {
                rc = (agent->close)(agent->handle, &set0);
                if (rc != 0)
                    ERROR("Failed to close connection with TA '%s': "
                          "rc=%r", agent->name, rc);
            }
            rc = (agent->finish)(agent->handle, NULL);
            if (rc != 0)
                ERROR("Failed to finish TA '%s': rc=%r",
                      agent->name, rc);
            agent->handle = NULL;
        }
        agent->flags |= (TA_DEAD | TA_UNRECOVER);
    }
}

/**
 * Initialize Test Agent or recovery it after reboot.
 * Test Agent is marked as "unrecoverable dead" in the case of failure.
 *
 * @param agent         Test Agent structure
 *
 * @return Status code
 */
static int
init_agent(ta *agent)
{
    int rc;
    te_bool is_reboot = ((agent->flags & TA_REBOOTING) != 0);

    INFO("Start TA '%s' type=%s confstr='%s'",
         agent->name, agent->type, agent->conf);
    agent->restart_timestamp = time(NULL);
    if (agent->flags & TA_FAKE)
        RING("TA '%s' has been already started");

    /* Initially mark TA as dead - no valid connection */
    agent->flags |= TA_DEAD;

    if ((rc = (agent->start)(agent->name, agent->type,
                             agent->conf, &(agent->handle),
                             &(agent->flags))) != 0)
    {
        if (!is_reboot)
        {
            ERROR("Cannot (re-)initialize TA '%s' error=%r",
                  agent->name, rc);
            set_ta_unrecoverable(agent);
        }
        return rc;
    }
    INFO("TA '%s' started, trying to connect", agent->name);
    if ((rc = (agent->connect)(agent->handle, &set0, &tv0)) != 0)
    {
        ERROR("Cannot connect to TA '%s' error=%r", agent->name, rc);
        set_ta_unrecoverable(agent);
        return rc;
    }
    agent->flags &= ~(TA_DEAD | TA_REBOOTING);
    INFO("Connected with TA '%s'", agent->name);

    rc = (agent->enable_synch_time ? synchronize_time(agent) : 0);
    if (rc == 0)
    {
        rc = startup_tasks(agent);
    }

    if (rc != 0)
    {
        set_ta_unrecoverable(agent);
    }
    else
    {
        answer_all_requests(&(agent->sent), TE_ETAREBOOTED);
        if (is_reboot)
            send_all_pending_commands(agent);
        else
        {
            answer_all_requests(&(agent->pending), TE_ETAREBOOTED);
            answer_all_requests(&(agent->waiting), TE_ETAREBOOTED);
        }
        agent->conn_locked = FALSE;
    }

    return rc;
}

/**
 * Force reboot of the Test Agent via RCF library method.
 * Test Agent is marked as "unrecoverable dead" in the case of failure.
 *
 * @param agent         Test Agent structure
 * @param req           user reboot request
 *
 * @return Status code
 */
static int
force_reboot(ta *agent, usrreq *req)
{
    int rc;

    agent->reboot_timestamp = 0;

    if (req != NULL)
    {
        reboot_num--;
    }

    if (~agent->flags & TA_DEAD)
    {
        rc = (agent->close)(agent->handle, &set0);
        if (rc != 0)
            ERROR("Failed to close connection with TA '%s': rc=%r",
                  agent->name, rc);
        agent->flags |= TA_DEAD;
    }

    rc = (agent->finish)(agent->handle,
                         (req == NULL) ? NULL :
                         (req->message->data_len > 0) ?
                            req->message->data : NULL);
    if (rc != 0)
    {
        ERROR("Cannot reboot TA %s", agent->name);
        agent->handle = NULL;
        set_ta_unrecoverable(agent);
        return rc;
    }
    agent->handle = NULL;

    if (req != NULL)
        answer_user_request(req);

    return init_agent(agent);
}

/**
 * Check if a reboot timer expires for any of Test Agents and perform
 * appropriate actions (see algorithm above).
 */
static void
check_reboot()
{
    ta *agent;
    time_t t = time(NULL);

    for (agent = agents; agent != NULL; agent = agent->next)
    {
        if (agent->reboot_timestamp > 0)
        {
            uint32_t reboot_time = t - agent->reboot_timestamp;

            if ((agent->flags & TA_REBOOTING) != 0)
            {
                /* Hardware reboot */
                if (reboot_time > RCF_COLD_REBOOT_MIN_TIMEOUT &&
                    t - agent->restart_timestamp >=
                                            RCF_COLD_REBOOT_INTERVAL)
                {
                    int rc;

                    WARN("Trying to re-start '%s' at %d s after "
                         "cold reboot", agent->name, reboot_time);
                    if ((rc = init_agent(agent)) != 0)
                    {
                        if (reboot_time <= RCF_COLD_REBOOT_MAX_TIMEOUT)
                        {
                            WARN("Restart of '%s' at %d s failed, waiting",
                                 agent->name, reboot_time);
                            agent->flags &= (~TA_UNRECOVER);
                            return;
                        }
                        else
                        {
                            ERROR("Failed to restart TA via cold reboot "
                                  "in %d seconds",
                                  RCF_COLD_REBOOT_MAX_TIMEOUT);
                            agent->flags |= TA_UNRECOVER;
                            agent->flags &= (~TA_REBOOTING);
                        }
                    }
                    agent->reboot_timestamp = 0;
                }
            }
            else if (reboot_time > RCF_REBOOT_TIMEOUT)
            {
                /* Software reboot */
                usrreq *req;
                for (req = agent->sent.next;
                     req != &(agent->sent) &&
                         req->message->opcode != RCFOP_REBOOT;
                     req = req->next);
                force_reboot(agent, req);
            }
        }
    }
}

/**
 * Sent cold_reboot() call for the agent specified if there exists
 * corresponding power control agent
 *
 * @param agent  TA to reboot
 * @return Status code.
 */
static te_errno
cold_reboot(ta *agent)
{
    ta     *power_ta;
    char   *tmp;
    usrreq *req;
    size_t  param_len;

    if (agent->cold_reboot_ta == NULL)
    {
        RING("There is no cold rebooting support for '%s'", agent->name);
        return TE_ENOSYS;
    }

    RING("Cold rebooting TA '%s' using '%s', '%s'", agent->name,
         agent->cold_reboot_ta, agent->cold_reboot_param);

    if ((power_ta = find_ta_by_name(agent->cold_reboot_ta)) == NULL)
    {
        ERROR("Unexisting TA '%s' is specified for cold_reboot of '%s'",
              agent->cold_reboot_ta, agent->name);
        return TE_ENOENT;
    }

    if ((power_ta->flags & TA_DEAD) != 0)
    {
        ERROR("Power agent '%s' for TA '%s' is dead!",
              power_ta->name, agent->name);
        return TE_ETADEAD;
    }

    if ((req = alloc_usrreq()) == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return TE_ENOMEM;
    }

    param_len = strlen(agent->cold_reboot_param) + 1;
    if ((tmp = realloc(req->message, sizeof(rcf_msg) + param_len)) == NULL)
    {
        ERROR("%s(): failed to re-allocate memory", __FUNCTION__);
        free(req->message);
        free(req);
        return TE_ENOMEM;
    }
    req->message = (rcf_msg *)tmp;

    req->user = NULL;
    req->timeout = RCF_CMD_TIMEOUT;
    strcpy(req->message->ta, power_ta->name);
    req->message->sid = ++(power_ta->sid);
    req->message->opcode = RCFOP_EXECUTE;
    req->message->intparm = RCF_FUNC;
    strcpy(req->message->id, "cold_reboot");
    req->message->num = 1;
    req->message->flags |= PARAMETERS_ARGV;
    strcpy(req->message->data, agent->cold_reboot_param);
    req->message->data_len = param_len;

    send_cmd(power_ta, req);
    return 0;
}


/**
 * Save binary attachment to the local file.
 *
 * @param agent         Test Agent structure
 * @param msg           message where filename is contained in file field
 *                      (is file name is empty file is created in
 *                       ${TE_TMP}/rcf_<taname>_<time>_<unique mark>
 *                      and its name is put to file parameter)
 * @param cmdlen        command length (including binary attachment)
 * @param ba            pointer to the first byte after end marker
 */
static void
save_attachment(ta *agent, rcf_msg *msg, size_t cmdlen, char *ba)
{
    /** Unique mark for temporary file names */
    static unsigned int unique_mark = 0;

    int file = -1;
    size_t write_len;
    int len;

    if (strlen(msg->file) == 0)
    {
        sprintf(msg->file, "%s/rcf_%s_%u_%u", tmp_dir, agent->name,
                (unsigned int)time(NULL), unique_mark++);
    }

    assert((ba - cmd) >= 0);
    assert(cmdlen >= (size_t)(ba - cmd));
    /* Above asserts guarantee that 'len' is not negative */
    len = cmdlen - (ba - cmd);
    VERB("Save attachment length=%d", len); 

    write_len = (cmdlen > sizeof(cmd)) ? (sizeof(cmd) - (ba - cmd)) :
                                         (size_t)len;

    if ((file = open(msg->file, O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
    {
        ERROR("Cannot open file %s for writing - skipping", msg->file);
    }

    if (file >= 0 && write(file, ba, write_len) < 0)
    {
        ERROR("Cannot write to file %s errno %d - skipping",
              msg->file, errno);
        close(file);
        file = -1;
    }

    len -= write_len;

    while (len > 0)
    {
        size_t maxlen = sizeof(cmd);
        int rc;

        rc = (agent->receive)(agent->handle, cmd, &maxlen, NULL);
        if (rc != 0 && rc != TE_RC(TE_COMM, TE_EPENDING))
        {
            ERROR("Failed receive rest of binary attachment TA %s - "
                  "cutting\n", agent->name);

            if (file >= 0)
                close(file);

            EXIT();

            return;
        }
        /*
         * 'len' is greater than zero, therefore cast to unsigned type
         * size_t is painless.
         */
        if (file > 0 &&
            write(file, cmd, ((size_t)len < sizeof(cmd)) ?
                                 (size_t)len : sizeof(cmd)) < 0)
        {
            ERROR("Cannot write to file %s errno %d - skipping",
                  msg->file, errno);
            close(file);
            file = -1;
        }
        len -= sizeof(cmd);
    }
    if (file > 0)
        close(file);

    msg->flags |= BINARY_ATTACHMENT;

    EXIT();
}


/**
 * Send pending command for specified SID.
 *
 * @param agent         Test Agent structure
 * @param sid           session identifier
 */
static void
send_pending_command(ta *agent, int sid)
{
    usrreq *req = find_user_request(&(agent->pending), sid);

    if (req == NULL)
    {
        VERB("There is NO pending requests for TA %s:%d",
                         agent->name, sid);
        return;
    }

    VERB("Send pending command to TA %s:%d", agent->name, sid);

    QEL_DELETE(req);
    send_cmd(agent, req);
}

/**
 * Send all pending commands to the TA, if no request with such SID
 * already sent.
 *
 * @param agent     test agent
 */
static void
send_all_pending_commands(ta *agent)
{
    usrreq *head;
    usrreq *req;
    usrreq *next;

    for (head = &(agent->pending), req = head->next;
         req != head;
         req = next)
    {
        next = req->next;
        if (find_user_request(&(agent->sent), req->message->sid) == NULL)
        {
            /* No requests with such SID sent */
            QEL_DELETE(req);
            send_cmd(agent, req);
        }
    }
}


/**
 * Read string value from the answer strippong off quotes and escape
 * sequences.
 *
 * @param ptr           answer pointer
 * @param s             location for string value
 */
static void
read_str(char **ptr, char *s)
{
    char *tmp = s,
         *p = *ptr;
    int   quotes = 0,
          cut = 0;

    if (*p == '\"')
    {
        p++;
        quotes = 1;
    }

    while (*p != 0)
    {
        if (quotes && *p == '\\' &&
            (*(p + 1) == '\\' || *(p + 1) == '\"'))
            p++;
        else if (quotes && *p == '\"')
        {
            p++;
            break;
        }
        else if (!quotes && *p == ' ')
            break;

        if (cut)
        {
            p++;
            continue;
        }

        *tmp++ = *p++;
        if (tmp - s == RCF_MAX_VAL - 1)
        {
            cut = 1;
            WARN("Too long string value is received in the answer - "
                 "cutting\n");
        }
    }

    while (*p == ' ')
        p++;

    *tmp = 0;
    *ptr = p;
}

/**
 * Receive reply from the Test Agent, send answer to user and send pending
 * message if necessary.
 *
 * @param agent         Test Agent structure
 */
static void
process_reply(ta *agent)
{
    int     rc;
    int     sid;
    int     error;
    size_t  len = sizeof(cmd);

    rcf_msg *msg;
    usrreq  *req = NULL;
    char    *ptr = cmd;
    char    *ba = NULL;
    te_bool  ack = FALSE;

#define READ_INT(n) \
    do {                                                    \
        char *tmp;                                          \
        n = strtol(ptr, &tmp, 10);                          \
        if (ptr == tmp || (*tmp != ' ' && *tmp != 0))       \
        {                                                   \
            ERROR("BAD PROTO: %s, %d", __FILE__, __LINE__); \
            goto bad_protocol;                              \
        }                                                   \
        ptr = tmp;                                          \
        while (*ptr == ' ')                                 \
            ptr++;                                          \
    } while (0)

    rc = (agent->receive)(agent->handle, cmd, &len, &ba);

    if (TE_RC_GET_ERROR(rc) == TE_ESMALLBUF)
    {
        ERROR("Too big answer from TA '%s' - increase memory constants", 
              agent->name);
        set_ta_dead(agent);
        return;
    }
    
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_EPENDING)
    {
        ERROR("Receiving answer from TA '%s' failed error=%r",
              agent->name, rc);
        set_ta_dead(agent);
        return;
    }

    VERB("Answer \"%s\" is received from TA '%s'", cmd, agent->name);
    
    if (strncmp(ptr, "SID ", strlen("SID ")) != 0)
    {
        if (strstr(ptr, "bad command") != NULL)
        {
            ERROR("TA %s received incorrect command", agent->name);
            return;
        }
        ERROR("BAD PROTO: %s, %d", __FILE__, __LINE__);
        goto bad_protocol;
    }

    ptr += strlen("SID ");
    READ_INT(sid);

    if ((req = find_user_request(&(agent->sent), sid)) == NULL)
    {
        ERROR("Can't find user request with SID %d", sid);
        goto push;
    }

    msg = req->message;
    msg->flags = 0;
    msg->data_len = 0;

    if ((msg->opcode == RCFOP_TRRECV_STOP ||
         msg->opcode == RCFOP_TRRECV_GET ||
         msg->opcode == RCFOP_TRRECV_WAIT ||
         msg->opcode == RCFOP_TRSEND_RECV) &&
         ba != NULL)
    {
        /* Set intermediate flag to keep request in the queue */
        msg->flags = INTERMEDIATE_ANSWER;

        /*
         * File name for attachment couldn't be presented by user
         * therefore it always is generated by save_attachment and
         * should be cleared after answer to user.
         */
        msg->file[0] = '\0';
        save_attachment(agent, msg, len, ba);
        answer_user_request(req);
        return;
    }

    READ_INT(error);

    if (TE_RC_GET_ERROR(error) == TE_EACK)
    {
        ack = TRUE;
        goto push;
    }

    if (msg->opcode == RCFOP_REBOOT)
    {
        if (error == 0)
        {
            INFO("Reboot of TA '%s' finished", agent->name);
            reboot_num--;
            agent->reboot_timestamp = 0;
            if (!(agent->flags & TA_PROXY))
            {
                if (init_agent(agent) != 0)
                {
                    ERROR("Initialization of the TA '%s' "
                          "after reboot failed ", agent->name);
                    return;
                }
            }
        }
        else
        {
            if (agent->flags & TA_PROXY)
            {
                req->message->error = error;
            }
            else
            {
                if (force_reboot(agent, req) != 0)
                    return;
            }
        }
        answer_user_request(req);
        agent->conn_locked = FALSE;
        send_all_pending_commands(agent);
        return;
    }

    if (error != 0)
        req->message->error = error;

    if (error == 0 ||
        /*
         * In case of TRRECV_STOP and TRRECV_WAIT we should get actual
         * number of received packets.
         */
        msg->opcode == RCFOP_TRRECV_STOP ||
        msg->opcode == RCFOP_TRRECV_WAIT)
    {
        VERB("Answer on %s command is received from TA '%s':\"%s\"",
             rcf_op_to_string(msg->opcode), agent->name, cmd);
        
        switch (msg->opcode)
        {
            case RCFOP_CONFGRP_START:
            case RCFOP_CONFGRP_END:
            case RCFOP_CONFSET:
            case RCFOP_CONFADD:
            case RCFOP_CONFDEL:
            case RCFOP_VWRITE:
            case RCFOP_FPUT:
            case RCFOP_FDEL:
            case RCFOP_CSAP_DESTROY:
            case RCFOP_KILL:
            case RCFOP_TRPOLL_CANCEL:
                break;

            case RCFOP_CONFGET:
                if (ba != NULL)
                    save_attachment(agent, msg, len, ba);
                else
                    read_str(&ptr, msg->value);
                break;

            case RCFOP_VREAD:
            case RCFOP_CSAP_PARAM:
                read_str(&ptr, msg->value);
                break;

            case RCFOP_GET_LOG:
            case RCFOP_FGET:
                if (ba == NULL)
                    goto bad_protocol;
                save_attachment(agent, msg, len, ba);
                break;

            case RCFOP_CSAP_CREATE:
                READ_INT(msg->handle);
                break;

            case RCFOP_TRRECV_START:
            case RCFOP_TRSEND_START:
            case RCFOP_TRSEND_STOP:
            case RCFOP_TRRECV_STOP:
            case RCFOP_TRRECV_GET:
            case RCFOP_TRRECV_WAIT:
                READ_INT(msg->num);
                break;

            case RCFOP_TRSEND_RECV:
                if (strncmp(ptr, "timeout", strlen("timeout")))
                {
                    msg->num = 1;
                    break;
                }
                if (isdigit(*ptr))
                {
                    READ_INT(msg->intparm);
                }
                break;

            case RCFOP_TRPOLL:
            {
                int poll_id; 

                READ_INT(poll_id);
                if (poll_id != 0)
                {
                    if (msg->intparm == 0)
                    {
                        /* intermediate reply with poll ID */
                        msg->intparm = poll_id;
                        msg->flags |= INTERMEDIATE_ANSWER;
                    }
                    /* final reply with successful result */
                    else if (msg->intparm != poll_id)
                    {
                        ERROR("Invalid traffic poll ID in final reply: "
                              "CSAP %u, poll id %u, new %u",
                              msg->handle, msg->intparm, poll_id);
                    }
                }
                break;
            }

            case RCFOP_EXECUTE:
                if(msg->intparm == RCF_FUNC)
                    READ_INT(msg->intparm);
                else
                    READ_INT(msg->handle);
                break;

            case RCFOP_RPC:
            {
#define INSIDE_LEN \
    (sizeof(((rcf_msg *)0)->file) + sizeof(((rcf_msg *)0)->value))
                size_t n = ba ? len - (ba - cmd) : strlen(ptr);
                
                if (msg->intparm < (int)n && n > INSIDE_LEN)
                {
                    rcf_msg *new_msg = 
                        (rcf_msg *)malloc(n + sizeof(*msg) - INSIDE_LEN);
                    
                    if (new_msg == NULL)
                    {
                        msg->error = TE_RC(TE_RCF, TE_EINVAL);
                        break;
                    }
                    *new_msg = *msg;
                    free(msg);
                    msg = req->message = new_msg;
                }
                
                if (ba)
                {
                    size_t start_len = sizeof(cmd) - (ba - cmd);
                    
                    if (start_len > n)
                        start_len = n;
                        
                    msg->intparm = n;
                    memcpy(msg->file, ba, start_len);
                    if (rc != 0)
                    {
                        n -= start_len;
                        msg->error = (agent->receive)(agent->handle, 
                                         msg->file + start_len, &n, NULL);
                    } 
                }
                else
                {
                    read_str(&ptr, msg->file);
                    msg->intparm = strlen(msg->file) + 1;
                }
                msg->data_len = msg->intparm  < (int)INSIDE_LEN ? 0 
                                : msg->intparm - INSIDE_LEN;
                
                break;
#undef INSIDE_LEN                
            }

            default:
                ERROR("Unhandled case value %d", msg->opcode);
        }
    }

    answer_user_request(req);
    
    /* Push next waiting request */
push:
    if (agent->conn_locked && sid == agent->lock_sid)
    {
        agent->conn_locked = FALSE;
        req = agent->waiting.next;
            
        if (req != &agent->waiting)
        {
            QEL_DELETE(req);
            send_cmd(agent, req);
        }
    }
    
    if (!ack)
        send_pending_command(agent, sid);

    return;

bad_protocol:
    ERROR("Bad answer is received from TA '%s'", agent->name);
    if (req != NULL)
    {
        req->message->error = TE_RC(TE_RCF, TE_EIPC);
        answer_user_request(req);
    }
    set_ta_dead(agent);

#undef READ_INT
}

/**
 * Transmit the command and possibly binary attachment to the Test Agent.
 *
 * @param agent         Test Agent structure
 * @param req           user request structure
 *
 * @return 0 (success) or -1 (failure)
 */
static int
transmit_cmd(ta *agent, usrreq *req)
{
    int   rc, len;
    int   file = -1;
    char *data = cmd;

    if (req->message->flags & BINARY_ATTACHMENT && 
        req->message->opcode != RCFOP_RPC)
    {
        struct stat st;

        if ((file = open(req->message->file, O_RDONLY)) < 0)
        {
            req->message->error = TE_OS_RC(TE_RCF, errno);
            ERROR("Cannot open file '%s'", req->message->file);
            answer_user_request(req);
            return -1;
        }
        if (fstat(file, &st) != 0)
        {
            req->message->error = TE_OS_RC(TE_RCF, errno);
            ERROR("RCF", "stat() failed for file %s", req->message->file);
            answer_user_request(req);
            close(file);
            return -1;
        }

        sprintf(cmd + strlen(cmd), " attach %u", (unsigned int)st.st_size);
    }

    VERB("Transmit command \"%s\" to TA '%s'", cmd, agent->name);

    len = strlen(cmd) + 1;
    while (TRUE)
    {
        if ((rc = (agent->transmit)(agent->handle, data, len)) != 0)
        {
            req->message->error = TE_RC(TE_RCF, rc);
            ERROR("Failed to transmit command to TA '%s' errno %r", 
                  agent->name, req->message->error);

            if (req->message->opcode == RCFOP_REBOOT)
                return -1;
                
            answer_user_request(req);
            set_ta_dead(agent);

            if (file != -1)
                close(file);

            return -1;
        }
        if (req->message->opcode == RCFOP_RPC && 
            req->message->flags & BINARY_ATTACHMENT)
        {
            if (data == req->message->file)
                break;
            data = req->message->file;
            len = req->message->intparm;
            continue;
        }
            
        if (file < 0 || (len = read(file, cmd, sizeof(cmd))) == 0)
            break;

        if (len < 0)
        {
            req->message->error = TE_OS_RC(TE_RCF, errno);
            ERROR("Read from file '%s' failed error=%r", 
                  req->message->file, req->message->error);
            close(file);
            answer_user_request(req);
            return -1;
        }
        
    }

    if (file != -1)
        close(file);
        
    VERB("The command is transmitted to %s", agent->name);
    req->sent = time(NULL);
    agent->conn_locked = TRUE;
    agent->lock_sid = req->message->sid;

    return 0;
}

/**
 * Write string to the command buffer (inserting '\' before ") and quotes.
 *
 * @param s     string to be filled in
 * @param len   number of symbols to be copied
 *
 * @return Number of added symbols
 */
static int
write_str(char *s, size_t len)
{
    char   *ptr0 = cmd + strlen(cmd);
    char   *ptr = ptr0;
    size_t  i = 0;

    *ptr++ = ' ';
    *ptr++ = '\"';

    while (*s != 0 && i < len)
    {
        if (*s == '\n')
        {
            *ptr++ = '\\';
            *ptr++ = 'n';
            s++;
            i++;
            continue;
        }

        if (*s == '\"' || *s == '\\')
        {
            *ptr++ = '\\';
        }
        *ptr++ = *s++;
        i++;
    }
    *ptr++ = '\"';
    *ptr = '\0';

    return ptr - ptr0;
}

/**
 * Print type and value to the Test Protocol command.
 *
 * @param cmd           command pointer
 * @param size          maximum command size
 * @param type          type (see rcf_api.h)
 * @param value         value pointer
 *
 * @return snprintf() return value
 */
static int
print_value(char *cmd, size_t size, unsigned char type, void *value)
{
    int ret = 0;

    switch (type)
    {
        case RCF_INT8:
            ret = snprintf(cmd, size, "%" TE_PRINTF_8 "d",
                           *(int8_t *)value);
            break;

        case RCF_INT16:
            ret = snprintf(cmd, size, "%" TE_PRINTF_16 "d",
                           *(int16_t *)value);
            break;

        case RCF_INT32:
            ret = snprintf(cmd, size, "%" TE_PRINTF_32 "d",
                           *(int32_t *)value);
            break;

        case RCF_INT64:
            ret = snprintf(cmd, size, "%" TE_PRINTF_64 "d",
                           *(int64_t *)value);
            break;

        case RCF_UINT8:
            ret = snprintf(cmd, size, "%" TE_PRINTF_8 "u",
                           *(uint8_t *)value);
            break;

        case RCF_UINT16:
            ret = snprintf(cmd, size, "%" TE_PRINTF_16 "u",
                           *(uint16_t *)value);
            break;

        case RCF_UINT32:
            ret = snprintf(cmd, size, "%" TE_PRINTF_32 "u",
                           *(uint32_t *)value);
            break;

        case RCF_UINT64:
            ret = snprintf(cmd, size, "%" TE_PRINTF_64 "u",
                           *(uint64_t *)value);
            break;

        case RCF_STRING:
            ret = write_str((char *)value, strlen((char *)value));
            break;

        default:
            assert(FALSE);
    }
    return ret;
}

/**
 * Send command to the Test Agent according to user request.
 *
 * @param agent         Test Agent structure
 * @param req           user request
 *
 * @return 0 (success) or -1 (failure)
 */
static int
send_cmd(ta *agent, usrreq *req)
{
    rcf_msg *msg = req->message;
    
    unsigned int space = 0;

    if (agent->conn_locked)
    {
        if (req->message->opcode == RCFOP_REBOOT)
            return -1;
        
        INFO("Command '%s' is placed to waiting queue of TA %s", 
             rcf_op_to_string(req->message->opcode), agent->name);
        QEL_INSERT(&(agent->waiting), req);
        return 0;
    }

#define CHECK_SPACE \
    do {                                                          \
        if (space >= sizeof(cmd))                                 \
        {                                                         \
            ERROR("Too long RCF command");                        \
            msg->error = TE_RC(TE_RCF, TE_EINVAL);                \
            answer_user_request(req);                             \
            return -1;                                            \
        }                                                         \
    } while (0)

#define PUT(fmt...) \
    do {                                                          \
        space += snprintf(cmd + space, sizeof(cmd) - space, fmt); \
        CHECK_SPACE;                                              \
    } while (0)

    PUT("SID %d ", msg->sid);
    switch (msg->opcode)
    {
        case RCFOP_REBOOT:
            PUT(TE_PROTO_REBOOT);
            if (msg->data_len > 0)
                write_str(msg->data, msg->data_len);
            req->timeout = RCF_REBOOT_TIMEOUT;
            break;

        case RCFOP_CONFGET:
            PUT(TE_PROTO_CONFGET " %s", msg->id);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFDEL:
            PUT(TE_PROTO_CONFDEL " %s", msg->id);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFADD:
            PUT(TE_PROTO_CONFADD " %s", msg->id);
            write_str(msg->value, RCF_MAX_VAL);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFSET:
            PUT(TE_PROTO_CONFSET " %s", msg->id);
            write_str(msg->value, RCF_MAX_VAL);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFGRP_START:
            PUT(TE_PROTO_CONFGRP_START);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFGRP_END:
            PUT(TE_PROTO_CONFGRP_END);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_GET_LOG:
            if (msg->sid != RCF_SID_GET_LOG)
            {
                msg->error = TE_RC(TE_RCF, TE_EINVAL);
                answer_user_request(req);
                return -1;
            }
            PUT(TE_PROTO_GET_LOG);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_VREAD:
            PUT(TE_PROTO_VREAD " %s %s", msg->id, rcf_types[msg->intparm]);
            if (req->timeout == 0)
                req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_VWRITE: 
            PUT(TE_PROTO_VWRITE " %s %s ", msg->id, 
                rcf_types[msg->intparm]);
            if (msg->intparm == RCF_STRING)
                write_str(msg->value, RCF_MAX_VAL);
            else
                PUT(msg->value);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_FPUT:
        case RCFOP_FGET:
        case RCFOP_FDEL:
            PUT("%s %s", 
                msg->opcode == RCFOP_FPUT ? TE_PROTO_FPUT :
                msg->opcode == RCFOP_FDEL ? TE_PROTO_FDEL : TE_PROTO_FGET,
                msg->data);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_CSAP_CREATE:
            PUT(TE_PROTO_CSAP_CREATE " %s", msg->id);
            if (msg->data_len > 0)
                write_str(msg->data, msg->data_len);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CSAP_DESTROY:
            PUT(TE_PROTO_CSAP_DESTROY " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CSAP_PARAM:
            PUT(TE_PROTO_CSAP_PARAM " %u %s", msg->handle, msg->id);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_TRSEND_START:
            PUT(TE_PROTO_TRSEND_START " %u %s", msg->handle,
                (msg->intparm & TR_POSTPONED) ? "postponed" : "");
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRSEND_STOP:
            PUT(TE_PROTO_TRSEND_STOP " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRRECV_START:
            PUT(TE_PROTO_TRRECV_START " %u %u %u%s%s", msg->handle,
                msg->num, msg->timeout,
                (msg->intparm & TR_RESULTS) ? " results" : "",
                (msg->intparm & TR_NO_PAYLOAD) ? " no-payload" : "");
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRRECV_GET:
        case RCFOP_TRRECV_WAIT:
        case RCFOP_TRRECV_STOP:
            PUT("%s %u", 
                msg->opcode == RCFOP_TRRECV_GET ? TE_PROTO_TRRECV_GET :
                msg->opcode == RCFOP_TRRECV_WAIT ? TE_PROTO_TRRECV_WAIT :
                TE_PROTO_TRRECV_STOP,
                msg->handle);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRSEND_RECV:
            PUT(TE_PROTO_TRSEND_RECV " %u %u%s",
                msg->handle, msg->timeout,
                (msg->intparm & TR_RESULTS) ? " results" : "");
            msg->num = 0;
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRPOLL:
            PUT(TE_PROTO_TRPOLL " %u %u", msg->handle, msg->timeout);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRPOLL_CANCEL:
            PUT(TE_PROTO_TRPOLL_CANCEL " %u %u", msg->handle,
                msg->intparm);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_EXECUTE:
            PUT(TE_PROTO_EXECUTE " ");
            switch (msg->intparm)
            {
                case RCF_FUNC:    PUT(TE_PROTO_FUNC); break;
                case RCF_THREAD:  PUT(TE_PROTO_THREAD); break;
                case RCF_PROCESS: PUT(TE_PROTO_PROCESS); break;
                default: 
                    ERROR("Incorrect execute mode");
                    msg->error = TE_RC(TE_RCF, TE_EINVAL);
                    answer_user_request(req);
                    return -1;
            }
            PUT(" %s", msg->id);
            if (msg->num >= 0)
                PUT(" %d", msg->num);

            if (msg->num > 0)
            {
                char *ptr = msg->data;
                int i;

                if (msg->flags & PARAMETERS_ARGV)
                {
                    PUT(" argv ");
                    for (i = 0 ; i < msg->num; i++)
                    {
                        write_str(ptr, strlen(ptr));
                        ptr += strlen(ptr) + 1;
                    }
                }
                else
                {
                    for (i = 0; i < msg->num; i++)
                    {
                        unsigned char type = *(ptr++);

                        PUT(" %s ", rcf_types[type]);

                        space += print_value(cmd + space,
                                             sizeof(cmd) - space,
                                             type, ptr);
                        CHECK_SPACE;

                        if (type == RCF_STRING)
                            ptr += strlen(ptr) + 1;
                        else
                            ptr += rcf_type_len[type];
                    }
                }
            }
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;
            
        case RCFOP_RPC:
        {
            PUT("%s %s %u ", 
                TE_PROTO_RPC, msg->id, (unsigned)msg->timeout);
            
            if (msg->intparm < RCF_MAX_VAL && 
                strcmp_start("<?xml", msg->file) == 0)
            {
                write_str(msg->file, strlen(msg->file));
            }
            else
            {
                PUT("attach %u", (unsigned int)msg->intparm);
                msg->flags |= BINARY_ATTACHMENT;
            }
            req->timeout = TE_MS2SEC(msg->timeout) + RCF_CMD_TIMEOUT;
            break;
        }

        case RCFOP_KILL:
            PUT(TE_PROTO_KILL " ");
            switch (msg->intparm)
            {
                case RCF_THREAD:  PUT(TE_PROTO_THREAD); break;
                case RCF_PROCESS: PUT(TE_PROTO_PROCESS); break;
                default: 
                    ERROR("Incorrect execute mode");
                    msg->error = TE_RC(TE_RCF, TE_EINVAL);
                    answer_user_request(req);
                    return -1;
            }
            PUT(" %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        default:
            ERROR("Unhandled case value %d", msg->opcode);
            msg->error = TE_RC(TE_RCF, TE_EINVAL);
            answer_user_request(req);
            return -1;
    }

#undef PUT

    if (transmit_cmd(agent, req) == 0)
        QEL_INSERT(&(agent->sent), req);

    return 0;
}


/**
 * Allocate memory for user request.
 */
static usrreq *
alloc_usrreq(void)
{
    usrreq *req;
    
    if ((req = (usrreq *)calloc(1, sizeof(usrreq))) == NULL)
        return NULL;

    if ((req->message = (rcf_msg *)calloc(1, sizeof(rcf_msg))) == NULL)
    {
        free(req);
        return NULL;
    }

    return req;
}


/**
 * This function is used to finish check that all running TA are
 * still working.
 */
static void
rcf_ta_check_all_done(void)
{
    VERB("%s()", __FUNCTION__);
    if (ta_checker.req != NULL && ta_checker.active == 0)
    {
        te_bool     cold_rebooting = FALSE;
        te_bool     rebooted = FALSE;
        te_bool     remain_dead = FALSE;
        ta         *agent;

        for (agent = agents; agent != NULL; agent = agent->next)
        {
            VERB("%s(): '%s' [%c %c %c]", __FUNCTION__, agent->name,
                 (agent->flags & TA_DEAD) ? 'D' : '-',
                 (agent->flags & TA_UNRECOVER) ? 'U' : '-',
                 (agent->flags & TA_REBOOTING) ? 'R' : '-'
                );
            if (agent->flags & TA_UNRECOVER)
            {
                remain_dead = TRUE;
                continue;
            }

            if (agent->flags & TA_REBOOTING)
            {
                cold_rebooting = TRUE;
                continue;
            }

            if (agent->flags & TA_DEAD)
            {
                ERROR("TA '%s' is dead, try to reboot...", agent->name);
                rebooted = TRUE;
                if (force_reboot(agent, NULL) != 0)
                {
                    remain_dead = TRUE;
                    if ((agent->flags & TA_REBOOTABLE) && 
                        cold_reboot(agent) == 0)
                    {
                        agent->flags &= (~TA_UNRECOVER);
                        agent->flags |= TA_REBOOTING;
                        agent->reboot_timestamp = time(NULL);
                        reboot_num++;
                        cold_rebooting = TRUE;
                    }
                }
            }
        }

        if (!cold_rebooting)
        {
            ta_checker.req->message->error =
                remain_dead ? TE_ETADEAD : rebooted ? TE_ETAREBOOTED : 0;

            answer_user_request(ta_checker.req);
            ta_checker.req = NULL;
        }
    }
}

/**
 * Process reply to TA check request. Mark TA as checked on success.
 *
 * @param req       User request with reply
 */
static void
rcf_ta_check_done(usrreq *req)
{
    ta *agent;

    agent = find_ta_by_name(req->message->ta);
    if (agent == NULL)
    {
        ERROR("Failed to find TA by name '%s'", req->message->ta);
        return;
    }
    VERB("%s('%s')", __FUNCTION__, req->message->ta);

    ta_checker.active--;
    agent->flags &= ~TA_CHECKING;

    if (req->message->error == 0)
        send_all_pending_commands(agent);
}

/**
 * This function is used to initiate check that all running TA are
 * still working.
 */
static void
rcf_ta_check_start(void)
{
    ta             *agent;
    usrreq         *req;
    int             rc = 0;

    assert(ta_checker.active == 0);
    VERB("%s()", __FUNCTION__);
    for (agent = agents; agent != NULL; agent = agent->next)
    {
        VERB("%s('%s') [%c]", __FUNCTION__, agent->name,
             (agent->flags & TA_DEAD) ? 'D' : '-');
        if (agent->flags & TA_DEAD)
            continue;

        req = alloc_usrreq();
        if (req == NULL)
        {
            rc = TE_RC(TE_RCF, TE_ENOMEM);
            break;
        }

        req->user = NULL;
        req->timeout = RCF_SHUTDOWN_TIMEOUT;

        strcpy(req->message->ta, agent->name);
        req->message->sid = RCF_SID_TACHECK;
        req->message->opcode = RCFOP_VREAD;
        req->message->intparm = RCF_STRING;
        strcpy(req->message->id, "time");

        /* Prepared request is answered in any case */
        if (agent->flags & TA_CHECKING)
            ERROR("TA '%s' checking is already in progress", agent->name);
        agent->flags |= TA_CHECKING;
        ta_checker.active++;
        send_cmd(agent, req);
    }
    rcf_ta_check_all_done();
}


/**
 * Process a request from the user: send the command to the Test Agent or
 * put the request to the pending queue.
 *
 * @param req           user request structure
 */
static void
process_user_request(usrreq *req)
{
    ta *agent;
    rcf_msg *msg = req->message;
    int rc;

    /* Process non-TA commands */
    switch (msg->opcode)
    {
        case RCFOP_TALIST:
        {
            rcf_msg *new_msg;

            new_msg = (rcf_msg *)calloc(1, sizeof(rcf_msg) + names_len);
            if (new_msg == NULL)
            {
                msg->error = TE_RC(TE_RCF, TE_ENOMEM);
                answer_user_request(req);
                return;
            }
            *new_msg = *msg;
            free(msg);
            msg = req->message = new_msg;
            msg->data_len = names_len;
            memcpy(msg->data, names, names_len);
            answer_user_request(req);
            return;
        }

        case RCFOP_TACHECK:
            if (ta_checker.req == NULL)
            {
                ta_checker.req = req;
                rcf_ta_check_start();
            }
            else
            {
                msg->error = TE_RC(TE_RCF, TE_EINPROGRESS);
                answer_user_request(req);
            }
            return;

        case RCFOP_ADD_TA:
            do {
                int          rc;
                ta          *agent;
                char const  *p;
                size_t const len = strlen(msg->ta) + 1;

                rcf_msg *new_msg;

                if ((new_msg = (rcf_msg *)calloc(1, sizeof(rcf_msg) +
                                                    names_len)) == NULL)
                {
                    msg->error = TE_RC(TE_RCF, TE_ENOMEM);
                    break; /** Leave 'do/while' block */
                }

                *new_msg = *msg;
                free(msg);
                msg = req->message = new_msg;
                msg->data_len = 0;
                msg->error = 0;

                /* Check whether agent with such name exists */
                for (p = names; p < names + names_len; p += strlen(p) + 1)
                {
                    if (strcmp(p, msg->ta) == 0)
                        break; /** Leave 'for' loop */
                }

                if (p < names + names_len)
                {
                    ERROR("TA '%s' already exists", msg->ta);
                    msg->error = TE_RC(TE_RCF, TE_EEXIST);
                    break; /** Leave 'do/while' block */
                }

                if (names_len + len > sizeof(names))
                {
                    ERROR("FATAL ERROR: Too many Test Agents - "
                          "increase memory constants");
                    msg->error = TE_RC(TE_RCF, TE_ETOOMANY);
                    break; /** Leave 'do/while' block */
                }

                if ((agent = (ta *)calloc(1, sizeof(ta))) == NULL)
                {
                    msg->error = TE_RC(TE_RCF, TE_ENOMEM);
                    break; /** Leave 'do/while' block */
                }

                agent->dynamic = TRUE;
                agent->next = agents;
                agent->flags = msg->flags | TA_DEAD;

                if (resolve_ta_methods(agent, msg->file) != 0)
                {
                    free(agent);
                    msg->error = TE_RC(TE_RCF, TE_EFAIL);
                    break; /** Leave 'do/while' block */
                }

                agent->enable_synch_time = msg->intparm ? TRUE : FALSE;

                agent->sent.prev = agent->sent.next = &agent->sent;
                agent->pending.prev = agent->pending.next = &agent->pending;
                agent->waiting.prev = agent->waiting.next = &agent->waiting;

                agent->sid = RCF_SID_UNUSED;

                if ((agent->name = strdup(msg->ta)) == NULL ||
                    (agent->type = strdup(msg->id)) == NULL ||
                    (agent->conf = strdup(msg->value)) == NULL)
                {
                    /* Suppose that calloc provided all NULLs */
                    free(agent->name);
                    free(agent->type);
                    free(agent->conf);

                    free(agent);
                    msg->error = TE_RC(TE_RCF, TE_ENOMEM);
                    break; /** Leave 'do/while' block */
                }

                if ((rc = init_agent(agent)) != 0)
                {
                    /* Suppose that calloc provided all NULLs */
                    free(agent->name);
                    free(agent->type);
                    free(agent->conf);

                    free(agent);
                    msg->error = rc;
                    break; /** Leave 'do/while' block */
                }

                /* 'msg->error' here is 0 since it was previously 0'ed */

                strcpy(names + names_len, agent->name);
                names_len += len;

                agents = agent;
                ta_num++;
            } while(0);

            answer_user_request(req);
            return;

        case RCFOP_DEL_TA:
            do {
                ta         **a;
                char        *p;
                size_t const len = strlen(msg->ta) + 1;

                rcf_msg *new_msg;

                new_msg = (rcf_msg *)calloc(1, sizeof(rcf_msg) + names_len);

                if (new_msg == NULL)
                {
                    msg->error = TE_RC(TE_RCF, TE_ENOMEM);
                    answer_user_request(req);
                    return;
                }

                *new_msg = *msg;
                free(msg);
                msg = req->message = new_msg;
                msg->data_len = 0;
                msg->error = 0;

                /* Check whether agent with such name exists */
                for (p = names; p < names + names_len; p += strlen(p) + 1)
                {
                    if (strcmp(p, msg->ta) == 0)
                        break; /** Leave 'for' loop */
                }

                /* Try to find the agent that is to be removed */
                if (p >= names + names_len)
                {
                    ERROR("TA '%s' does not exist", msg->ta);
                    msg->error = TE_RC(TE_RCF, TE_ENOENT);
                    break; /** Leave 'do/while' block */
                }

                /* Find the agent by name */
                for (a = &agents; *a != NULL; a = &(*a)->next)
                {
                    if (strcmp(msg->ta, (*a)->name) == 0)
                        break; /** Leave current 'for' loop */
                }

                /* Handle the case when the agent item is not found */
                if (*a == NULL)
                {
                    ERROR("TA '%s' does not found", msg->ta);
                    msg->error = TE_RC(TE_RCF, TE_ENOENT);
                    break; /** Leave 'do/while' block */
                }

                if (!(*a)->dynamic)
                {
                    ERROR("TA '%s' is specified in RCF configuration "
                          "file and cannot be removed", msg->ta);
                    msg->error = TE_RC(TE_RCF, TE_EPERM);
                    break; /** Leave 'do/while' block */
                }

                /* Shutdown TA */
                RING("Shutting down '%s' TA", (*a)->name);

                {
                    time_t t = time(NULL);

                    ta *agt = *a;
                    ta *next = agt->next; /**< Save 'next' item */

                    if (!(agt->flags & TA_DEAD)) /** If TA is NOT DEAD */
                    {
                        sprintf(cmd, "SID %d %s",
                                ++agt->sid, TE_PROTO_SHUTDOWN);
                        (agt->transmit)(agt->handle, cmd, strlen(cmd) + 1);
                        answer_all_requests(&(agt->sent), TE_EIO);
                        answer_all_requests(&(agt->pending), TE_EIO);
                        answer_all_requests(&(agt->waiting), TE_EIO);

                        while (time(NULL) - t < RCF_SHUTDOWN_TIMEOUT)
                        {
                            struct timeval tv  = tv0;
                            fd_set         set = set0;

                            select(FD_SETSIZE, &set, NULL, NULL, &tv);

                            if ((agt->is_ready)(agt->handle))
                            {
                                char    answer[16];
                                char   *ba;
                                size_t  len = sizeof(cmd);

                                if ((agt->receive)(agt->handle, cmd,
                                                   &len, &ba) != 0)
                                {
                                    continue;
                                }

                                sprintf(answer, "SID %d 0", agt->sid);

                                if (strcmp(cmd, answer) != 0)
                                    continue;

                                INFO("Test Agent '%s' is down", agt->name);
                                agt->flags |= TA_DOWN;
                                (agt->close)(agt->handle, &set0);
                                break; /** Leave current 'while' loop */
                            }
                        }
                    }

                    if (!(agt->flags & TA_DOWN))
                        ERROR("Soft shutdown of TA '%s' failed", agt->name);

                    if (agt->handle != NULL)
                    {
                        if ((agt->finish)(agt->handle, NULL) != 0)
                        {
                            ERROR("Cannot finish TA '%s'", agt->name);
                            break; /** Leave 'do/while' block */
                        }

                        agt->handle = NULL;
                    }

                    RING("Test Agent '%s' is stopped", agt->name);

                    /* Free agent */
                    free(agt->name);
                    free(agt->type);
                    free(agt->conf);
                    free(agt);

                    /* Remove agent from linked list */
                    *a = next;
                }

                /* Remove TA name from the list of TA names */
                if (names + names_len - p - len > 0)
                    memmove(p, p + len, names + names_len - p - len);

                names_len -= len;

                /* Handle the edge case */
                if (names_len == 0)
                    *names = '\0';

                /* 'msg->error' here is 0 since it was previously 0'ed */

            } while(0);

            answer_user_request(req);
            return;

        default:
            /* The rest of commands are processed below */
            break;
    }
    
    if ((agent = find_ta_by_name(msg->ta)) == NULL)
    {
        ERROR("Request '%s' to unknown TA '%s'",
              rcf_op_to_string(msg->opcode), msg->ta);
        msg->error = TE_RC(TE_RCF, TE_EINVAL);
        answer_user_request(req);
        return;
    }
    
    if ((agent->flags & TA_DEAD) && !(agents->flags & TA_REBOOTING))
    {
        ERROR("Request '%s' to dead TA '%s'",
              rcf_op_to_string(msg->opcode), msg->ta);
        msg->error = TE_RC(TE_RCF, TE_ETADEAD);
        answer_user_request(req);
        return;
    }
    
    if (msg->opcode == RCFOP_TADEAD)
    {
        answer_user_request(req);
        set_ta_unrecoverable(agent);
        return;
    }
    
    if (req->message->sid > agent->sid)
    {
        ERROR("Request '%s' with invalid SID %d for TA '%s'",
              rcf_op_to_string(msg->opcode), req->message->sid, msg->ta);
        msg->error = TE_RC(TE_RCF, TE_EINVAL);
        answer_user_request(req);
        return;
    }

    /* Special commands */
    switch (msg->opcode)
    {
        case RCFOP_TATYPE:
            strcpy(msg->id, agent->type);
            answer_user_request(req);
            return;

        case RCFOP_SESSION:
            msg->sid = ++(agent->sid);
            answer_user_request(req);
            return;

        case RCFOP_REBOOT:
            if ((agent->flags & TA_REBOOTABLE) == 0)
            {
                msg->error = TE_RC(TE_RCF, TE_EPERM);
                answer_user_request(req);
                return;
            }
            if (agent->reboot_timestamp > 0)
            {
                msg->error = TE_RC(TE_RCF, TE_EINPROGRESS);
                answer_user_request(req);
                return;
            }
            if ((agent->flags & TA_LOCAL) && !(agent->flags & TA_PROXY))
            {
                msg->error = TE_RC(TE_RCF, TE_ETALOCAL);
                answer_user_request(req);
                return;
            }
            msg->sid = ++agent->sid;
            if (send_cmd(agent, req) != 0)
            {
                VERB("Reboot using TA type support library");
                rc = (agent->finish)(agent->handle,
                                     req->message->data_len > 0 ?
                                     req->message->data : NULL);
                if (rc != 0)
                {
                    ERROR("Cannot reboot TA '%s'", agent->name);
                    msg->error = TE_RC(TE_RCF, rc);
                    answer_user_request(req);
                    return;
                }
                agent->handle = NULL;
                answer_user_request(req);
                init_agent(agent);
                return;
            }
            reboot_num++;
            agent->reboot_timestamp = time(NULL);
            RING("Reboot of TA '%s' initiated", agent->name);
            return;

        default:
            /* Continue processing below */
            break;
    }

    /* Usual commands */
    if (shutdown_num > 0 ||
        agent->reboot_timestamp > 0 ||
        (agent->flags & TA_CHECKING) ||
        find_user_request(&(agent->sent), msg->sid) != NULL || 
        find_user_request(&(agent->waiting), msg->sid) != NULL)
    {
        VERB("Pending user request for TA %s:%d", agent->name, msg->sid);
        QEL_INSERT(&(agent->pending), req);
    }
    else
    {
        send_cmd(agent, req);
    }
}

/**
 * Shut down the RCF according to algorithm described above.
 */
static void
rcf_shutdown()
{
    ta *agent;

    struct timeval tv;
    fd_set         set;

    time_t t = time(NULL);

    RING("Shutting down");

    shutdown_num = ta_num;

    for (agent = agents; agent != NULL; agent = agent->next)
    {
        if (agent->flags & TA_DEAD)
            continue;

        sprintf(cmd, "SID %d %s", ++agent->sid, TE_PROTO_SHUTDOWN);
        (agent->transmit)(agent->handle, cmd, strlen(cmd) + 1);
        answer_all_requests(&(agent->sent), TE_EIO);
        answer_all_requests(&(agent->pending), TE_EIO);        
        answer_all_requests(&(agent->waiting), TE_EIO);        
    }

    while (shutdown_num > 0 && time(NULL) - t < RCF_SHUTDOWN_TIMEOUT)
    {
        tv = tv0;
        set = set0;
        select(FD_SETSIZE, &set, NULL, NULL, &tv);
        for (agent = agents; agent != NULL; agent = agent->next)
        {
            if (agent->flags & (TA_DOWN | TA_DEAD))
                continue;

            if ((agent->is_ready)(agent->handle))
            {
                char    answer[16];
                char   *ba;
                size_t  len = sizeof(cmd);
                
                if ((agent->receive)(agent->handle, cmd, &len, &ba) != 0)
                    continue;
                    
                sprintf(answer, "SID %d 0", agent->sid);

                if (strcmp(cmd, answer) != 0)
                    continue;

                INFO("Test Agent '%s' is down", agent->name);
                agent->flags |= TA_DOWN;
                (agent->close)(agent->handle, &set0);
                shutdown_num--;
            }
        }
    }
    for (agent = agents; agent != NULL; agent = agent->next)
    {
        if ((agent->flags & TA_DOWN) == 0)
            ERROR("Soft shutdown of TA '%s' failed", agent->name);
        if (agent->handle != NULL)
        {
            if ((agent->finish)(agent->handle, NULL) != 0)
                ERROR("Cannot finish TA '%s'", agent->name);
            agent->handle = NULL;
        }
    }
    RING("Test Agents are stopped");
}


/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if some new options are going to be added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
process_cmd_line_opts(int argc, const char **argv)
{
    poptContext  optCon;
    int          rc;
    
    /* Option Table */
    struct poptOption options_table[] = {
        { "foreground", 'f', POPT_ARG_NONE | POPT_BIT_SET, &flags,
          RCF_FOREGROUND,
          "Run in foreground (usefull for debugging).", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);
      
    poptSetOtherOptionHelp(optCon, "[OPTIONS] <cfg-file>");

    rc = poptGetNextOpt(optCon);
    if (rc != -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s", poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    cfg_file = poptGetArg(optCon);
    if (cfg_file == NULL)
    {
        ERROR("No configuration file in command-line arguments");
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }
    if (poptGetArg(optCon) != NULL)
    {
        ERROR("Unexpected arguments in command-line");
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    poptFreeContext(optCon);

    return EXIT_SUCCESS;
}

/**
 * Main routine of the RCF process. Usage: rcf <configuration file name>
 *
 * @return 0 (success) or 1 (failure)
 */
int
main(int argc, const char *argv[])
{
    usrreq *req = NULL;
    ta     *agent;
    int     result = EXIT_FAILURE;
    int     rc;

    if ((rc = process_cmd_line_opts(argc, argv)) != EXIT_SUCCESS)
    {
        ERROR("Fatal error during command line options processing");
        goto exit;
    }

    /* Ignore SIGPIPE, by default SIGPIPE kills the process */
    signal(SIGPIPE, SIG_IGN);

    ipc_init();
    if (ipc_register_server(RCF_SERVER, RCF_IPC, &server) != 0)
        goto exit;
    assert(server != NULL);

    FD_ZERO(&set0);
    tv0.tv_sec = RCF_SELECT_TIMEOUT;
    tv0.tv_usec = 0;

    INFO("Starting...\n");

    if ((tmp_dir = getenv("TE_TMP")) == NULL)
    {
        ERROR("FATAL ERROR: TE_TMP is empty");
        goto exit;
    }

    if (parse_config(cfg_file) != 0)
        goto exit;

    /* Initialize Test Agents */
    if (agents == NULL)
    {
        RING("Empty list with TAs");
    }
    for (agent = agents; agent != NULL; agent = agent->next)
    {
        if (init_agent(agent) != 0)
        {
            ERROR("FATAL ERROR: TA initialization failed");
            goto exit;
        }
    }

    /* 
     * Go to background, if foreground mode is not requested.
     * No threads should be created before become a daemon.
     */
    if ((~flags & RCF_FOREGROUND) && (daemon(TRUE, TRUE) != 0))
    {
        ERROR("daemon() failed");
        goto exit;
    }

    INFO("Initialization is finished");
    while (1)
    {
        struct timeval  tv = tv0;
        fd_set          set = set0;
        size_t          len;
        time_t          now;
        int             select_rc;
        
        req = NULL;
        rc = -1;

        (void)ipc_get_server_fds(server, &set);

        select_rc = select(FD_SETSIZE, &set, NULL, NULL, &tv);
        if (select_rc < 0)
        {
            if (errno != EINTR)
                ERROR("Unexpected failure of select(): rc=%d, errno=%d",
                      select_rc, errno);
            else
                INFO("select() has been interrupted by signal");
        }

        if (reboot_num > 0)
            check_reboot();

        if ((select_rc > 0) &&
            ipc_is_server_ready(server, &set, FD_SETSIZE))
        {
            len = sizeof(rcf_msg);
            
            if ((req = alloc_usrreq()) == NULL)
                goto exit;

            rc = ipc_receive_message(server, req->message,
                                     &len, &(req->user));

            if (TE_RC_GET_ERROR(rc) == TE_ESMALLBUF) 
            {
                size_t n = len;
                
                len += sizeof(rcf_msg);
                if ((req->message = 
                         (rcf_msg *)realloc(req->message, len)) == NULL)
                {
                    ERROR("Cannot allocate memory for user request");
                    free(req);
                    continue;
                }
                rc = ipc_receive_message(server, req->message + 1,
                                         &n, &(req->user));
            }
            
            if (rc != 0)
            {
                ERROR("Failed to receive user request: errno %r", rc);
                free(req->message);
                free(req);
                continue;
            }

            if (len != sizeof(rcf_msg) + req->message->data_len)
            {
                ERROR("Incorrect user request is received: "
                      "data_len field does not match to IPC "
                      "message size: %d != %d + %d",
                      len, req->message->data_len, sizeof(rcf_msg));
                free(req->message);
                free(req);
                continue;
            }
            
            INFO("Got request %u:%d:'%s' from user '%s'",
                 (unsigned)req->message->seqno, req->message->sid,
                 rcf_op_to_string(req->message->opcode),
                 ipc_server_client_name(req->user));

            if (req->message->opcode == RCFOP_SHUTDOWN)
            {
                INFO("Shutdown command is recieved");
                break;
            }
            process_user_request(req);
        }

        for (agent = agents; agent != NULL; agent = agent->next)
        {
            usrreq *next;
            
            if ((agent->is_ready)(agent->handle))
                process_reply(agent);
                
            now = time(NULL);
            for (req = agent->sent.next; 
                 req != &(agent->sent); 
                 req = next)
            {
                next = req->next;
                
                if ((uint32_t)(now - req->sent) < req->timeout)
                    continue;

                if (now < req->sent)
                {
                    WARN("Current time is less than request's sent time");
                    continue;
                }
               
                {
                    size_t  ret;
                    char    time_buf[9];    /* Sufficient for format
                                               string used below */

                    ret = strftime(time_buf, sizeof(time_buf), "%H:%M:%S",
                                   localtime(&req->sent));
                    if (ret == 0)
                    {
                        ERROR("%s:%d: Buffer is too small",
                              __FILE__, __LINE__);
                        time_buf[0] = '\0';
                    }

                    ERROR("Request %u:%d: opcode '%s' id '%s' "
                          "sent to TA '%s' at '%s' is timed out (%u sec)",
                          (unsigned)req->message->seqno, req->message->sid,
                          rcf_op_to_string(req->message->opcode),
                          req->message->id,
                          agent->name, time_buf, (unsigned)req->timeout);
                }
                req->message->error = TE_RC(TE_RCF, TE_ETIMEDOUT);
                answer_user_request(req);
                set_ta_dead(agent);
                break;
            }
        }

        /* If TA check is in progress, may be all checks are done? */
        rcf_ta_check_all_done();
    }

    result = EXIT_SUCCESS;

exit:
    rcf_shutdown();

    if (req != NULL && req->message->opcode == RCFOP_SHUTDOWN)
        answer_user_request(req);

    free_ta_list();
    ipc_close_server(server);

    if (result != 0)
        ERROR("Error exit");
    else
        RING("Exit");

    return result;
}
