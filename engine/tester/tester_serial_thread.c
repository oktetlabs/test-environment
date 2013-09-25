/** @file
 * @brief Serial console parser events handler thread of the Tester
 *
 * Implementation of the Tester thread to handle events of the serial
 * consoles.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "logger_api.h"
#include "te_errno.h"
#include "conf_api.h"
#include "te_queue.h"
#include "te_sigmap.h"

#include <pthread.h>

/* Configurator subtrees */
#define SERIAL_FMT_LOC      "/local:/tester:"
#define SERIAL_FMT_HLR      "/local:/tester:/event:%s/handler:"

/* Default handlers location */
#define TESTER_SERIAL_LOC   "handlers"

/* Default period to poll of events status */
#define TESTER_SERIAL_PERIOD    100
/* Max path length to external handler */
#define TESTER_SERIAL_MAX_PATH  256

/* 
 * Default timeout of waiting between attempts to appeal to configurator in
 * case if it busy by a local sequence, in microseconds
 */
#define SERIAL_WAIT_LOCAL_SEQ_TIMEOUT 10000

/*
 * Attemtions limit to avoid infinity loop
 */
#define SERIAL_WAIT_LOCAL_SEQ_LIMIT 1000

/** Allowable results of the Tester serial events handlers */
typedef enum {
    SERIAL_EVENT_CONTINUE, /**< Continue handlers execution */
    SERIAL_EVENT_STOP_H,   /**< Stop handlers execution */
    SERIAL_EVENT_STOP_B,   /**< Stop both handlers and test execution */
    SERIAL_EVENT_STOP_ALL, /**< Stop handlers execution, kill the test and
                                stop tests sequence execution */
} serial_event_result;

#define TE_SERIAL_MALLOC(ptr, size)       \
    if ((ptr = malloc(size)) == NULL)   \
        assert(0);

/** Process identifier with mutex */
typedef struct tester_serial_pid_t {
    pid_t           id;     /**< The protocol identifier */
    pthread_mutex_t mutex;  /**< Provide access to the structure */
} tester_serial_pid_t;

/* Process identifier of the current test */
static tester_serial_pid_t pid;

/* The serial thread of Tester */
static pthread_t serial_thread;

/* Flag to stop test sequence performance */
static te_bool stop_test_sequence = FALSE;

/* Flag to finalize and stop the thread */
static te_bool stop_thread = TRUE;

/** Struct to configure a Tester handler */
typedef struct tester_serial_handler_t {
    char            *name;      /**< Name of the event handler */
    char            *path;      /**< Path to executable file */
    cfg_handle       handle;    /**< Confapi handle of the handler */
    int              priority;  /**< The handler priority */
    te_bool          internal;  /**< Internal flag */
    int              signal;    /**< Signal to perform internal handler */

    SLIST_ENTRY(tester_serial_handler_t)  ent_hand_l; /**< Connector */
} tester_serial_handler_t;

/* Type of the tester_serial_handler_t list head */
SLIST_HEAD(serial_hand_h_t, tester_serial_handler_t);
typedef struct serial_hand_h_t serial_hand_h_t;

/** Counter to avoid infinite loops in configurator waiting */
static int serial_wait_local_counter = 0;

/**
 * Try perform request to configurator and wait if it busy by local sequence
 */
#define SERIAL_WAIT_LOCAL_SEQ(_func) \
do { \
    serial_wait_local_counter = 0; \
    while ((rc = (_func)) == TE_RC(TE_CS, TE_EACCES) && \
           serial_wait_local_counter++ < SERIAL_WAIT_LOCAL_SEQ_LIMIT) \
        usleep(SERIAL_WAIT_LOCAL_SEQ_TIMEOUT); \
} while (0)

/**
 * Get sequence of the Tester event handlers from Configurator
 * 
 * @param event_name    The event name
 * @param hh            Pointer to head of the handlers list (OUT)
 * 
 * @return Status code
 * @retval 0            Success
 */
static te_errno
tester_serial_get_handlers(const char *event_name, serial_hand_h_t *hh)
{
    unsigned                    n_handles;
    cfg_handle                 *handles     = NULL;
    int                         i;
    cfg_val_type                type;
    tester_serial_handler_t    *h;
    tester_serial_handler_t    *first;
    tester_serial_handler_t    *current;
    int                         rc;
    char                       *signame;

    SLIST_INIT(hh);

/**
 * Macro to check received status codes and cleanup the handler
 * 
 * @param _rc   Status code
 * @param _fmt  Error message
 */
#define SERIAL_CHECK_RC(_rc, _fmt...) \
if (_rc != 0) \
{ \
    ERROR(_fmt); \
    if (h->name != NULL) \
        free(h->name); \
    free(h); \
    continue; \
}

    SERIAL_WAIT_LOCAL_SEQ(cfg_find_pattern_fmt(&n_handles, &handles,
        SERIAL_FMT_HLR "*/priority:", event_name));
    if (rc != 0)
        return rc;

    for (i = 0; (unsigned)i < n_handles; i++)
    {
        TE_SERIAL_MALLOC(h, sizeof(tester_serial_handler_t));
        h->name = NULL;
        h->path = NULL;

        SERIAL_WAIT_LOCAL_SEQ(cfg_get_father(handles[i], &h->handle));
        SERIAL_CHECK_RC(rc, "Couldn't get the handler instance handle");

        type = CVT_INTEGER;
        SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance(handles[i], &type,
                                               &h->priority));
        SERIAL_CHECK_RC(rc, "Couldn't get the handler instance priority");

        SERIAL_WAIT_LOCAL_SEQ(cfg_get_inst_name(h->handle, &h->name));
        SERIAL_CHECK_RC(rc, "Couldn't get the handler instance name");

        if (h->name == NULL)
            SERIAL_CHECK_RC(TE_ENOENT, "The handler name is NULL");

        type = CVT_INTEGER;
        SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance_fmt(&type, &h->internal, 
            SERIAL_FMT_HLR "%s/internal:", event_name, h->name));
        SERIAL_CHECK_RC(rc, "Failed to get the handler type");

        if (h->internal == FALSE)
        {
            type = CVT_STRING;
            SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance(h->handle, &type,
                                                   &h->path));
            SERIAL_CHECK_RC(rc, "Failed to get the handler %s path inst",
                            h->name);
            if (h->path == NULL)
                SERIAL_CHECK_RC(rc, "The handler %s path is NULL", h->name);
        }
        else
        {
            signame = NULL;
            type = CVT_STRING;
            SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance_fmt(&type, &signame,
                SERIAL_FMT_HLR "%s/signal:", event_name, h->name));
            SERIAL_CHECK_RC(rc, "Failed to get the handler signal");
            if (signame == NULL)
                SERIAL_CHECK_RC(TE_RC(TE_TESTER, TE_EINVAL),
                                "Failed to get the handler signal");

            h->signal = map_name_to_signo(signame);
            free(signame);
        }

        /* Insert element to ordered list */
        current = NULL;
        SLIST_FOREACH(first, hh, ent_hand_l)
        {
            if (h->priority > first->priority)
                break;
            current = first;
        }
        if (current != NULL)
            SLIST_INSERT_AFTER(current, h, ent_hand_l);
        else
            SLIST_INSERT_HEAD(hh, h, ent_hand_l);
    }

    if (handles != NULL)
    {
        free(handles);
        handles = NULL;
    }
    return 0;
#undef SERIAL_CHECK_RC
}

/**
 * Call the external handler of the event
 * 
 * @param path  Path to the handler execute
 * 
 * @return Status code
 * @retval 0    Continue handlers execution
 * @retval 1    Stop handlers execution
 * @retval 2    Stop both handlers and test execution
 * @retval 3    Stop handlers execution, kill the test and stop tests
 *              sequence execution.
 * @retval -1   Error
 */
static int
tester_serial_call_handler(const char *path)
{
    char            full_path[TESTER_SERIAL_MAX_PATH];
    char           *loc = NULL;
    cfg_val_type    type;
    int             res;
    int             rc;

    if (*path != '/' && *path != '~')
    {
        type = CVT_STRING;
        SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance_fmt(&type, &loc,
            SERIAL_FMT_LOC "/location:"));
        if (rc != 0 || loc == NULL)
        {
            ERROR("Failed to get path to the handlers directory");
            return -1;
        }
        if (strlen(loc) > 0)
            res = snprintf(full_path, TESTER_SERIAL_MAX_PATH, "%s/%s", loc,
                           path);
        else
            res = snprintf(full_path, TESTER_SERIAL_MAX_PATH, "%s/%s",
                           TESTER_SERIAL_LOC, path);

        free(loc);
    }
    else
        res = snprintf(full_path, TESTER_SERIAL_MAX_PATH, "%s", path);

    if (res >= TESTER_SERIAL_MAX_PATH)
    {
        ERROR("Too long path to handler directory");
        return -1;
    }

    res = system(full_path);
    if (res == -1)
    {
        ERROR("Couldn't perform system(%s)", full_path);
        return -1;
    }

    return WEXITSTATUS(res);
}

/**
 * Handling of a serial console event
 * 
 * @param event_name    Name of the Tester event
 * 
 * @return Status code
 * @retval 0            Success
 */
static te_errno
tester_handle_serial_event(const char *event_name)
{
    serial_hand_h_t             hh;        /* Head of handlers list */
    tester_serial_handler_t    *h;         /* Current handler */
    te_errno                    rc;
    int                         res;
    te_bool                     h_exec;    /* While value is TRUE handlers
                                              executes */
    te_bool                     fail = FALSE;

    if ((rc = tester_serial_get_handlers(event_name, &hh)) != 0)
    {
        ERROR("Couldn't get handlers list of event %s: %r", event_name, rc);
        return rc;
    }

    h_exec = TRUE;
    while(!SLIST_EMPTY(&hh))
    {
        h = SLIST_FIRST(&hh);
        if (h_exec == TRUE)
        {
            if (h->internal == TRUE)
            {
                pthread_mutex_lock(&pid.mutex);
                if (pid.id > 0)
                {
                    if (kill(pid.id, h->signal) < 0)
                    {
                        if (errno == ESRCH)
                        {
                            fail = TRUE;
                            VERB("kill(%d, %d) failed: %s", pid.id,
                                 h->signal, strerror(errno));
                        }
                        else
                            ERROR("kill(%d, %d) failed: %s", pid.id,
                                  h->signal, strerror(errno));
                    }
                }
                pthread_mutex_unlock(&pid.mutex);
            }
            else
            {
                INFO("Call external handler %s", h->path);
                res = tester_serial_call_handler(h->path);
                switch (res)
                {
                    case SERIAL_EVENT_CONTINUE:
                        break;

                    case SERIAL_EVENT_STOP_H:
                        h_exec = FALSE;
                        break;

                    case SERIAL_EVENT_STOP_B:
                        h_exec = FALSE;
                        /* Stop the test */
                        pthread_mutex_lock(&pid.mutex);
                        if (pid.id > 0)
                        {
                            if (kill(pid.id, SIGTERM) < 0)
                            {
                                if (errno == ESRCH)
                                {
                                    fail = TRUE;
                                    VERB("kill(%d, %d) failed: %s", pid.id,
                                         h->signal, strerror(errno));
                                }
                                else
                                    ERROR("kill(%d, %d) failed: %s", pid.id,
                                          h->signal, strerror(errno));
                            }
                            else
                                WARN("Test has been stopped by the serial"
                                     " console handler %s", h->name);
                        }
                        pthread_mutex_unlock(&pid.mutex);
                        break;

                    case SERIAL_EVENT_STOP_ALL:
                        h_exec = FALSE;
                        /* Stop the test */
                        pthread_mutex_lock(&pid.mutex);
                        if (pid.id > 0)
                        {
                            if (kill(pid.id, SIGTERM) < 0)
                            {
                                if (errno == ESRCH)
                                {
                                    fail = TRUE;
                                    VERB("kill(%d, %d) failed: %s", pid.id,
                                         h->signal, strerror(errno));
                                }
                                else
                                    ERROR("kill(%d, %d) failed: %s", pid.id,
                                          h->signal, strerror(errno));
                            }
                        }
                        pthread_mutex_unlock(&pid.mutex);

                        /* Stop the sequence of tests */
                        stop_test_sequence = TRUE;
                        WARN("Test and test sequence were stopped by"
                             "result of the serial console handler %s",
                             h->name);
                        break;

                    default:
                        ERROR("Wrong handler (%s) execution result %d",
                              h->path, res);
                } /* switch */
            } /* else */
        } /* if (h_exec == TRUE) */

        /* Cleanup and remove element */
        SLIST_REMOVE_HEAD(&hh, ent_hand_l);
        if (h->name != NULL)
            free(h->name);
        if (h->path != NULL)
            free(h->path);
        free(h);
    }

    if (fail)
        return TE_EFAIL;

    return 0;
}

/**
 * Entry point to the Tester thread to handle events of the serial consoles
 */
static int
tester_serial_thread(void)
{
    cfg_val_type    type;
    te_errno        rc;
    int             period;
    unsigned        n_handles;
    cfg_handle     *handles         = NULL;
    cfg_handle      status_handle;
    char           *event_name      = NULL;
    char           *ag_event        = NULL;
    int             i;
    int             status;

    type = CVT_INTEGER;
    SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance_fmt(&type, &period,
                                               "/local:/tester:/period:"));
    if (rc != 0)
    {
        ERROR("Failed to get the parser period");
        return rc;
    }
    if (period <= 0)
        period = TESTER_SERIAL_PERIOD;
    period *= 1000;

    while (stop_thread == FALSE)
    {
        SERIAL_WAIT_LOCAL_SEQ(cfg_find_pattern_fmt(&n_handles, &handles,
            "/agent:*/parser:*/event:*"));

        if (rc != 0)
            return rc;
        for (i = 0; (unsigned)i < n_handles; i++)
        {
            SERIAL_WAIT_LOCAL_SEQ(cfg_get_oid_str(handles[i], &ag_event));
            if (rc != 0 || ag_event == NULL)
            {
                ERROR("Couldn't get event oid");
                continue;
            }

            SERIAL_WAIT_LOCAL_SEQ(cfg_find_fmt(&status_handle, "%s/status:",
                                               ag_event));
            if (rc != 0)
            {
                ERROR("Couldn't get event status handle of %s", ag_event);
                free(ag_event);
                continue;
            }
            free(ag_event);

            SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance(status_handle,
                                                   &type, &status));
            if (rc != 0)
            {
                ERROR("Couldn't get event status");
                continue;
            }

            if (status != 0)
            {
                event_name = NULL;
                type = CVT_STRING;
                SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance(handles[i], &type,
                                                       &event_name));
                if (rc != 0 || event_name == NULL)
                {
                    ERROR("Couldn't get the event name");
                    continue;
                }

                rc = tester_handle_serial_event(event_name);
                if (rc != 0 && rc != TE_EFAIL)
                {
                    ERROR("Couldn't handle the event %s", event_name);
                    free(event_name);
                    continue;
                }
                else if (rc == TE_EFAIL)
                {
                    free(event_name);
                    continue;
                }

                type = CVT_INTEGER;
                SERIAL_WAIT_LOCAL_SEQ(cfg_set_instance(status_handle, type,
                                                       FALSE));
                if (rc != 0)
                {
                    ERROR("Couldn't change event %s status", event_name);
                    free(event_name);
                    continue;
                }

                free(event_name);
            }
        }

        if (handles != NULL)
        {
            free(handles);
            handles = NULL;
        }

        if (stop_thread != FALSE)
            break;
        usleep(period);
    }

    return 0;
}

/* See description in the tester_seria_thread.h */
te_errno
tester_set_serial_pid(pid_t i_pid)
{
    int rc;

    rc = pthread_mutex_lock(&pid.mutex);
    if (rc != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    pid.id = i_pid;

    rc = pthread_mutex_unlock(&pid.mutex);
    if (rc != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);
    return 0;
}

/* See description in the tester_seria_thread.h */
te_errno
tester_release_serial_pid(void)
{
    int rc;

    rc = pthread_mutex_lock(&pid.mutex);
    if (rc != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    pid.id = -1;

    rc = pthread_mutex_unlock(&pid.mutex);
    if (rc != 0)
        return TE_OS_RC(TE_TA_UNIX, errno);
    return 0;
}

/* See description in the tester_seria_thread.h */
te_bool
tester_check_serial_stop(void)
{
    return stop_test_sequence;
}

/* See description in the tester_seria_thread.h */
te_errno
tester_start_serial_thread(void)
{
    int             rc;
    cfg_val_type    type;
    int             enable;

    /* Check support of the serial parsing framework */
    type = CVT_INTEGER;
    SERIAL_WAIT_LOCAL_SEQ(cfg_get_instance_fmt(&type, &enable,
                                               "/local:/tester:/enable:"));
    if ((rc != 0 && TE_RC(TE_CS, TE_ENOENT)) || enable == FALSE)
        return 0;

    stop_thread = FALSE;

    pid.id = -1;
    rc = pthread_mutex_init(&pid.mutex, NULL);
    if (rc != 0)
        return TE_OS_RC(TE_TESTER, rc);

    rc = pthread_create(&serial_thread, NULL, (void *)&tester_serial_thread,
                        NULL);
    if (rc != 0)
        rc = TE_OS_RC(TE_TESTER, rc);
    return rc;
}

/* See description in the tester_seria_thread.h */
te_errno
tester_stop_serial_thread(void)
{
    int rc;

    if (stop_thread == FALSE)
    {
        stop_thread = TRUE;
        rc = pthread_join(serial_thread, NULL);
        if (rc != 0)
        {
            ERROR("pthread_join failed %d, %s", rc, strerror(rc));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
    }

    return 0;
}
