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
#include "rpc_xdr.h"

#define RCF_NEED_TYPES      1
#define RCF_NEED_TYPE_LEN   1
#include "te_proto.h"

#include "logger_api.h"
#include "logger_ten.h"

#include "te_expand.h"


#define RCF_SELECT_TIMEOUT      1   /**< Default select timeout
                                         in seconds */
#define RCF_CMD_TIMEOUT         100  /**< Default timeout (in seconds) for
                                         command processing on the TA */
#define RCF_CMD_TIMEOUT_HUGE    10000  
                                    /**< Default timeout for command
                                         processing on the TA */
#define RCF_REBOOT_TIMEOUT      60  /**< TA reboot timeout in seconds */
#define RCF_SHUTDOWN_TIMEOUT    5   /**< TA shutdown timeout in seconds */


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
 *     response to all sent and pending requests (TE_ETAREBOOTED).
 *
 * reboot_num variable is necessary to avoid TA list scanning every time
 * when select() is returned (list scanning is performed only if
 * reboot_num > 0).
 *
 * RCF shutdown:
 *     send a shutdown command to TA with first free SID to all Test Aents;
 *     shutdown_num = ta_num;
 *     wait until time(NULL) - ta.reboot_timestamp > RCF_SHUTDOWN_TIMESTAMP
 *     or response from all TA is received (set flag TA_DOWN and decriment
 *     shutdown_num every time when response is received);
 *     for all TA with TA_DOWN flag clear ta.finish();
 *     response to all sent and pending requests (EIO);
 *     response to user shutdown request.
 */


/** One request from the user */
typedef struct usrreq {
    struct usrreq            *next;
    struct usrreq            *prev;
    rcf_msg                  *message;
    struct ipc_server_client *user;
    uint32_t                  timeout;
    time_t                    sent;
} usrreq;

/** A description for a task/thread to be executed at TA startup */
typedef struct ta_initial_task {
    enum rcf_start_modes    mode;          /**< Task execution mode */
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
    usrreq              pending;            /**< Pending user requests */
    unsigned int        flags;              /**< Test Agent flags */
    int                 reboot_timestamp;   /**< Time of reboot command
                                                 sending (in seconds) */
    int                 sid;                /**< Free session identifier 
                                                 (starts from 2) */

    void               *dlhandle;           /**< Dynamic library handle */
    ta_initial_task    *initial_tasks;      /**< Startup tasks */

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

static te_bool rcf_wait_shutdown = FALSE;   /**< Should RCF go to wait
                                                 of shut down state */

static struct ipc_server *server = NULL;    /**< IPC Server handle */

static char cmd[RCF_MAX_LEN];   /**< Test Protocol command location */
static char names[RCF_MAX_LEN - sizeof(rcf_msg)];   /**< TA names */
static int  names_len = 0;      /**< Length of TA name list */

/* Backup select parameters */
static struct timeval tv0;
static fd_set set0;

/** Name of directory for temporary files */
static char *tmp_dir;


/* Forward decraration */
static int send_cmd(ta *agent, usrreq *req);
static void rcf_ta_check_done(usrreq *req);

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

        if ((agent = (ta *)calloc(1, sizeof(ta))) == NULL)
            return -1;

        agent->next = agents;
        agents = agent;
        agent->flags = 0;

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
                agent->flags = TA_REBOOTABLE;
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
            if (xmlStrcmp(task->name, (const xmlChar *)"thread") != 0 &&
                xmlStrcmp(task->name, (const xmlChar *)"task") != 0 &&
                xmlStrcmp(task->name, (const xmlChar *)"function") != 0)
                continue;
            ta_task = malloc(sizeof(*ta_task));
            if (ta_task == NULL)
            {
                ERROR("malloc failed: %d", errno);
                goto error;
            }
            ta_task->mode = 
                (xmlStrcmp(task->name , (const xmlChar *)"thread") == 0 ?
                 RCF_START_THREAD : 
                 (xmlStrcmp(task->name , (const xmlChar *)"function") == 0 ?
                  RCF_START_FUNC : RCF_START_FORK));
            
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
        ERROR("Failed to transmit command to TA '%s' errno %d", 
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
    if (req->user != NULL)
    {
        int rc;

        INFO("Send reply for %u:%d:'%s' to user '%s'",
             (unsigned)req->message->seqno, req->message->sid,
             rcf_op_to_string(req->message->opcode),
             ipc_server_client_name(req->user));

        rc = ipc_send_answer(server, req->user, (char *)req->message,
                             sizeof(rcf_msg) + req->message->data_len);
        if (rc != 0)
        {
            ERROR("Cannot send an answer to user: errno %d", rc);
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


void write_str(char *s, int len);


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
        strcpy(cmd, "SID 0 " TE_PROTO_EXECUTE " ");
        strcat(cmd, (task->mode == RCF_START_FUNC) ? "function " :
               (task->mode == RCF_START_THREAD) ? "thread " : "fork ");
        strcat(cmd, task->entry);
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
             (task->mode == RCF_START_FUNC) ? "function" :
             (task->mode == RCF_START_THREAD) ? "thread" : "fork",
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
        rc = (agent->close)(agent->handle, &set0);
        if (rc != 0)
            ERROR("Failed to close connection with TA '%s': rc=0x%x",
                  agent->name, rc);
        agent->flags |= TA_DEAD;
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
        if (agent->handle != NULL)
        {
            int rc;

            if (~agent->flags & TA_DEAD)
            {
                rc = (agent->close)(agent->handle, &set0);
                if (rc != 0)
                    ERROR("Failed to close connection with TA '%s': "
                          "rc=0x%x", agent->name, rc);
            }
            rc = (agent->finish)(agent->handle, NULL);
            if (rc != 0)
                ERROR("Failed to finish TA '%s': rc=0x%x",
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

    INFO("Start TA '%s' type=%s confstr='%s'",
         agent->name, agent->type, agent->conf);
    if (agent->flags & TA_FAKE)
        RING("TA '%s' has been already started");

    /* Initially mark TA as dead - no valid connection */
    agent->flags |= TA_DEAD;

    if ((rc = (agent->start)(agent->name, agent->type,
                             agent->conf, &(agent->handle),
                             &(agent->flags))) != 0)
    {
        ERROR("Cannot (re-)initialize TA '%s' error %d", agent->name, rc);
        set_ta_unrecoverable(agent);
        return rc;
    }
    INFO("TA '%s' started, trying to connect", agent->name);
    if ((rc = (agent->connect)(agent->handle, &set0, &tv0)) != 0)
    {
        ERROR("Cannot connect to TA '%s' error %d", agent->name, rc);
        set_ta_unrecoverable(agent);
        return rc;
    }
    agent->flags &= ~TA_DEAD;
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
        answer_all_requests(&(agent->pending), TE_ETAREBOOTED);
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
            ERROR("Failed to close connection with TA '%s': rc=0x%x",
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
        if (agent->reboot_timestamp > 0 &&
            t - agent->reboot_timestamp > RCF_REBOOT_TIMEOUT)
        {
            usrreq *req;
            for (req = agent->sent.next;
                 req != &(agent->sent) &&
                     req->message->opcode != RCFOP_REBOOT;
                 req = req->next);
            force_reboot(agent, req);
        }
    }
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
        ERROR("cannot open file %s for writing - skipping\n",
                  msg->file);
    }

    if (file >= 0 && write(file, ba, write_len) < 0)
    {
        ERROR("cannot write to file %s errno %d - skipping\n",
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
        if (rc != 0 && rc != TE_EPENDING)
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
            ERROR("cannot write to file %s errno %d - skipping\n",
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
    if (send_cmd(agent, req) == 0)
        QEL_INSERT(&(agent->sent), req);
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
            if (send_cmd(agent, req) == 0)
                QEL_INSERT(&(agent->sent), req);
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

#define READ_INT(n) \
    do {                                              \
        char *tmp;                                    \
        n = strtol(ptr, &tmp, 10);                    \
        if (ptr == tmp || (*tmp != ' ' && *tmp != 0)) \
        {                                             \
            ERROR("BAD PROTO: %s, %d",     \
                      __FILE__, __LINE__);            \
            goto bad_protocol;                        \
        }                                             \
        ptr = tmp;                                    \
        while (*ptr == ' ')                           \
            ptr++;                                    \
    } while (0)

    rc = (agent->receive)(agent->handle, cmd, &len, &ba);

    if (rc == TE_ESMALLBUF)
    {
        ERROR("Too big answer from TA '%s' - increase memory constants", 
              agent->name);
        set_ta_dead(agent);
        return;
    }

    if (rc != 0 && rc != TE_EPENDING)
    {
        ERROR("Receiving answer from TA '%s' failed error %d",
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
        send_pending_command(agent, sid);
        return;
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
        msg->flags = INTERMEDIATE_ANSWER;

        /*
         * File name for attachment couldn't be presented by user
         * therefore it allways is generated by save_attachment and
         * should be cleared after answer to user.
         */
        msg->file[0] = '\0';
        save_attachment(agent, msg, len, ba);
        answer_user_request(req);
        return;
    }

    READ_INT(error);

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

            case RCFOP_EXECUTE:
                if(msg->handle == RCF_START_FUNC)
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

    VERB("Command \"%s\" is transmitted to TA '%s'", cmd, agent->name);

    len = strlen(cmd) + 1;
    while (TRUE)
    {
        if ((rc = (agent->transmit)(agent->handle, data, len)) != 0)
        {
            req->message->error = TE_RC(TE_RCF, rc);
            ERROR("Failed to transmit command to TA '%s' errno %d", 
                  agent->name, rc);

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
            ERROR("Read from file '%s' failed errno %d", 
                  req->message->file, errno);
            close(file);
            answer_user_request(req);
            return -1;
        }
        
    }

    if (file != -1)
        close(file);
        
    req->sent = time(NULL);

    return 0;
}

/**
 * Write string to the command buffer (inserting '\' before ") and quotes.
 *
 * @param s     string to be filled in
 * @param len   number of symbols to be copied
 */
void
write_str(char *s, int len)
{
    char *ptr = cmd + strlen(cmd);
    int i = 0;

    *ptr++ = ' ';
    *ptr++ = '\"';

    while (*s != 0 && i < len)
    {
        if (*s == '\"' || *s == '\\')
        {
            *ptr++ = '\\';
        }
        *ptr++ = *s++;
        i++;
    }
    *ptr++ = '\"';
    *ptr = 0;
}

/**
 * Print type and value to the Test Protocol command.
 *
 * @param cmd           command pointer
 * @param type          type (see rcf_api.h)
 * @param value         value pointer
 */
static void
print_value(char *cmd, unsigned char type, void *value)
{
    switch (type)
    {
        case RCF_INT8:
            sprintf(cmd, "%" TE_PRINTF_8 "d", *(int8_t *)value);
            break;

        case RCF_INT16:
            sprintf(cmd, "%" TE_PRINTF_16 "d", *(int16_t *)value);
            break;

        case RCF_INT32:
            sprintf(cmd, "%" TE_PRINTF_32 "d", *(int32_t *)value);
            break;

        case RCF_INT64:
            sprintf(cmd, "%" TE_PRINTF_64 "d", *(int64_t *)value);
            break;

        case RCF_UINT8:
            sprintf(cmd, "%" TE_PRINTF_8 "u", *(uint8_t *)value);
            break;

        case RCF_UINT16:
            sprintf(cmd, "%" TE_PRINTF_16 "u", *(uint16_t *)value);
            break;

        case RCF_UINT32:
            sprintf(cmd, "%" TE_PRINTF_32 "u", *(uint32_t *)value);
            break;

        case RCF_UINT64:
            sprintf(cmd, "%" TE_PRINTF_64 "u", *(uint64_t *)value);
            break;

        case RCF_STRING:
            write_str((char *)value, strlen((char *)value));
            break;
    }
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

    sprintf(cmd, "SID %d ", msg->sid);
    switch (msg->opcode)
    {
        case RCFOP_REBOOT:
            strcat(cmd, TE_PROTO_REBOOT);
            if (msg->data_len > 0)
                write_str(msg->data, msg->data_len);
            req->timeout = RCF_REBOOT_TIMEOUT;
            break;

        case RCFOP_CONFGET:
            strcat(cmd, TE_PROTO_CONFGET);
            strcat(cmd, " ");
            strncat(cmd, msg->id, RCF_MAX_ID);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFDEL:
            strcat(cmd, TE_PROTO_CONFDEL);
            strcat(cmd, " ");
            strncat(cmd, msg->id, RCF_MAX_ID);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFADD:
            strcat(cmd, TE_PROTO_CONFADD);
            strcat(cmd, " ");
            strncat(cmd, msg->id, RCF_MAX_ID);
            write_str(msg->value, RCF_MAX_VAL);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFSET:
            strcat(cmd, TE_PROTO_CONFSET);
            strcat(cmd, " ");
            strncat(cmd, msg->id, RCF_MAX_ID);
            write_str(msg->value, RCF_MAX_VAL);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFGRP_START:
            strcat(cmd, TE_PROTO_CONFGRP_START);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CONFGRP_END:
            strcat(cmd, TE_PROTO_CONFGRP_END);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_GET_LOG:
            if (msg->sid != RCF_SID_GET_LOG)
            {
                msg->error = TE_RC(TE_RCF, TE_EINVAL);
                answer_user_request(req);
                return -1;
            }
            strcat(cmd, TE_PROTO_GET_LOG);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_VREAD:
            strcat(cmd, TE_PROTO_VREAD);
            sprintf(cmd + strlen(cmd), " %s %s", msg->id,
                    rcf_types[msg->intparm]);
            if (req->timeout == 0)
                req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_VWRITE:
            strcat(cmd, TE_PROTO_VWRITE);
            sprintf(cmd + strlen(cmd), " %s %s ", msg->id,
                    rcf_types[msg->intparm]);
            if (msg->intparm == RCF_STRING)
                write_str(msg->value, RCF_MAX_VAL);
            else
                sprintf(cmd + strlen(cmd), "%s", msg->value);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_FPUT:
        case RCFOP_FGET:
        case RCFOP_FDEL:
            strcat(cmd, msg->opcode == RCFOP_FPUT ? TE_PROTO_FPUT :
                        msg->opcode == RCFOP_FDEL ? TE_PROTO_FDEL :
                                                    TE_PROTO_FGET);
            strcat(cmd, " ");
            strncat(cmd, msg->data, RCF_MAX_PATH);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_CSAP_CREATE:
            strcat(cmd, TE_PROTO_CSAP_CREATE);
            strcat(cmd, " ");
            strncat(cmd, msg->id, RCF_MAX_ID);
            if (msg->data_len > 0)
                write_str(msg->data, msg->data_len);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CSAP_DESTROY:
            strcat(cmd, TE_PROTO_CSAP_DESTROY);
            sprintf(cmd + strlen(cmd), " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_CSAP_PARAM:
            strcat(cmd, TE_PROTO_CSAP_PARAM);
            sprintf(cmd + strlen(cmd), " %u %s", msg->handle, msg->id);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        case RCFOP_TRSEND_START:
            strcat(cmd, TE_PROTO_TRSEND_START);
            sprintf(cmd + strlen(cmd), " %u %s", msg->handle,
                    (msg->intparm & TR_POSTPONED) ? "postponed" : "");
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRSEND_STOP:
            strcat(cmd, TE_PROTO_TRSEND_STOP);
            sprintf(cmd + strlen(cmd), " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRRECV_START:
            strcat(cmd, TE_PROTO_TRRECV_START);
            sprintf(cmd + strlen(cmd), " %u %u %u%s", msg->handle,
                    msg->num, msg->timeout,
                    (msg->intparm & TR_RESULTS) ? " results" : "");
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRRECV_WAIT:
            strcat(cmd, TE_PROTO_TRRECV_WAIT);
            sprintf(cmd + strlen(cmd), " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRRECV_STOP:
        case RCFOP_TRRECV_GET:
            strcat(cmd, msg->opcode == RCFOP_TRRECV_STOP ?
                            TE_PROTO_TRRECV_STOP : TE_PROTO_TRRECV_GET);
            sprintf(cmd + strlen(cmd), " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_TRSEND_RECV:
            strcat(cmd, TE_PROTO_TRSEND_RECV);
            sprintf(cmd + strlen(cmd), " %u %u%s",
                    msg->handle, msg->timeout,
                    (msg->intparm & TR_RESULTS) ? " results" : "");
            msg->num = 0;
            req->timeout = RCF_CMD_TIMEOUT_HUGE;
            break;

        case RCFOP_EXECUTE:
            strcat(cmd, TE_PROTO_EXECUTE);
            strcat(cmd, " ");
            switch ((enum rcf_start_modes)msg->handle)
            {
                case RCF_START_FUNC:
                    strcat(cmd, "function ");
                    break;
                case RCF_START_THREAD:
                    strcat(cmd, "thread ");
                    break;                  
                case RCF_START_FORK:
                    strcat(cmd, "fork ");
                    break;
            }
            strncat(cmd, msg->id, RCF_MAX_NAME);
            strcat(cmd, " ");
            if (msg->intparm >= 0)
                sprintf(cmd + strlen(cmd), " %d", msg->intparm);

            if (msg->num > 0)
            {
                char *ptr = msg->data;
                int i;

                if (msg->flags & PARAMETERS_ARGV)
                {
                    strcat(cmd, " argv ");
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

                        sprintf(cmd + strlen(cmd), " %s ", rcf_types[type]);

                        print_value(cmd + strlen(cmd), type, ptr);
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
            char *s = cmd + strlen(cmd);
            
            s += sprintf(s, "%s %s %d ", 
                         TE_PROTO_RPC, msg->id, msg->timeout);
            
            if (msg->intparm < RCF_MAX_VAL && 
                strcmp_start("<?xml", msg->file) == 0)
            {
                write_str(msg->file, strlen(msg->file));
            }
            else
            {
                sprintf(s, "attach %u", (unsigned int)msg->intparm);
                msg->flags |= BINARY_ATTACHMENT;
            }
            req->timeout = msg->timeout / 1000 + 5;
            break;
        }

        case RCFOP_KILL:
            strcat(cmd, TE_PROTO_KILL);
            sprintf(cmd + strlen(cmd), " %u", msg->handle);
            req->timeout = RCF_CMD_TIMEOUT;
            break;

        default:
            ERROR("Unhandled case value %d", msg->opcode);
    }

    return transmit_cmd(agent, req);
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
    if (ta_checker.req != NULL && ta_checker.active == 0)
    {
        te_bool     rebooted = FALSE;
        te_bool     remain_dead = FALSE;
        ta         *agent;

        for (agent = agents; agent != NULL; agent = agent->next)
        {
            if (agent->flags & TA_UNRECOVER)
            {
                remain_dead = TRUE;
                continue;
            }

            if (agent->flags & TA_DEAD)
            {
                ERROR("Reboot TA '%s'", agent->name);
                rebooted = TRUE;
                if (force_reboot(agent, NULL) != 0)
                    remain_dead = TRUE;
            }
        }

        ta_checker.req->message->error =
            remain_dead ? TE_ETADEAD : rebooted ? TE_ETAREBOOTED : 0;

        answer_user_request(ta_checker.req);
        ta_checker.req = NULL;
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
    for (agent = agents; agent != NULL; agent = agent->next)
    {
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
        if ((rc = send_cmd(agent, req)) == 0)
            QEL_INSERT(&(agent->sent), req);
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
    
    if (agent->flags & TA_DEAD)
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
            QEL_INSERT(&(agent->sent), req);
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
        find_user_request(&(agent->sent), msg->sid) != NULL)
    {
        VERB("Pending user request for TA %s:%d", agent->name, msg->sid);
        QEL_INSERT(&(agent->pending), req);
    }
    else
    {
        if ((rc = send_cmd(agent, req)) == 0)
            QEL_INSERT(&(agent->sent), req);
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

/** Wait for shutdown message in the case of initialization error */
static void
wait_shutdown()
{
    RING("Wait shutdown command");
    while (TRUE)
    {
        usrreq *req;
        size_t  len;
        int     rc;

        if ((req = alloc_usrreq()) == NULL)
            return;

        len = RCF_MAX_LEN;
        if ((rc = ipc_receive_message(server, (char *)req->message,
                                      &len, &(req->user))) != 0)
        {
            ERROR("Failed to receive user request: errno %d", rc);
        }
        else
        {
            VERB("Request from user is received");
            if (req->message->opcode == RCFOP_SHUTDOWN)
            {
                answer_user_request(req);
                return;
            }
            else
            {
                WARN("Reject request from user - RCF is shutdowning");
                req->message->error = TE_ENORCF;
                answer_user_request(req);
            }
        }
    }
}


/**
 * SIGPIPE handler.
 *
 * @param sig       Signal number
 */
static void
sigpipe_handler(int sig)
{
    UNUSED(sig);
    rcf_wait_shutdown = TRUE;
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

    /* Register SIGPIPE handler, by default SIGPIPE kills the process */
    signal(SIGPIPE, sigpipe_handler);

    ipc_init();
    if (ipc_register_server(RCF_SERVER, &server) != 0)
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
                goto error;

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
                ERROR("Failed to receive user request: errno=0x%X", rc);
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

        now = time(NULL);
        for (agent = agents; agent != NULL; agent = agent->next)
        {
            usrreq *next;
            
            if ((agent->is_ready)(agent->handle))
                process_reply(agent);
                
            for (req = agent->sent.next; 
                 req != &(agent->sent); 
                 req = next)
            {
                next = req->next;
                
                if ((uint32_t)(now - req->sent) < req->timeout)
                    continue;

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
                    ERROR("Request %u:%d:'%s' sent to TA '%s' at '%s' is "
                          "timed out",
                          (unsigned)req->message->seqno, req->message->sid,
                          rcf_op_to_string(req->message->opcode),
                          agent->name, time_buf);
                }
                req->message->error = TE_RC(TE_RCF, TE_ETIMEDOUT);
                answer_user_request(req);
                set_ta_dead(agent);
                break;
            }
        }

        /* If TA check is in progress, may be all checks are done? */
        rcf_ta_check_all_done();

        if (rcf_wait_shutdown)
        {
            rc = 0;
            goto error;
        }
    }

    rcf_shutdown();

    if (req != NULL && req->message->opcode == RCFOP_SHUTDOWN)
        answer_user_request(req);

    result = EXIT_SUCCESS;

error:
    if (rc != 0)
        wait_shutdown();
        
exit:
    free_ta_list();
    ipc_close_server(server);

    if (result != 0)
        ERROR("Error exit");
    else
        RING("Exit");

    return result;
}
