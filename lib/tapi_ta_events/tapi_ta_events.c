/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API for TA events
 *
 * Copyright (C) 2024-2024 ARK NETWORKS LLC. All rights reserved.
 */

#define TE_LGR_USER "TAPI TA events"

#include "te_config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_alloc.h"
#include "te_vector.h"

#include "conf_api.h"
#include "logger_api.h"
#include "rcf_api.h"
#include "tapi_ta_events.h"

/** Test context to handle specified subset of TA events */
typedef struct ta_events_handler {
    bool              used;     /**< Flag to identify used handlers */
    char             *ta;       /**< TA name to catch TA events */
    char             *events;   /**< Comma separated list of TA event names */
    tapi_ta_events_cb callback; /**< Callback to handle TA events */
} ta_events_handler;

/** Array to describe full set of TA events to handle */
static te_vec ta_events_handlers = TE_VEC_INIT(ta_events_handler);

static bool
ta_events_handler_match(ta_events_handler *handler, const char *event)
{
    int   len = strlen(event);
    char *ptr;

    assert(len > 0);

    for (ptr = handler->events; ptr != NULL; ptr++)
    {
        char *base = ptr;

        ptr = strstr(base, event);
        if ((ptr == base || ptr[-1] == ',') &&
            (ptr[len] == '\0' || ptr[len] == ','))
            return true;
    }

    return false;
}

/**
 * Get unused entry from @ref ta_events_handlers array
 *
 * @retval NULL    If array doesn't have unused entries
 * @retval other   Unused entry from array
 */
static ta_events_handler *
ta_events_get_unused_handler(void)
{
    ta_events_handler *handler;

    TE_VEC_FOREACH (&ta_events_handlers, handler)
    {
        if (!handler->used)
            return handler;
    }

    return NULL;
}

/* See description in tapi_ta_events.h */
te_errno
tapi_ta_events_subscribe(const char *ta, const char *events,
                         tapi_ta_events_cb      callback,
                         tapi_ta_events_handle *handle)
{
    unsigned int       pid = (unsigned int)getpid();
    unsigned int       tid = (unsigned int)pthread_self();
    te_errno           rc;
    ta_events_handler *new_entry = ta_events_get_unused_handler();

    if (new_entry == NULL)
    {
        size_t size = te_vec_size(&ta_events_handlers);

        CHECK_NZ_RETURN(TE_VEC_APPEND(&ta_events_handlers,
                                      (ta_events_handler){.used = false}));
        new_entry = te_vec_get(&ta_events_handlers, size);
    }

    *new_entry = (ta_events_handler){
        .used = true,
        .ta = TE_STRDUP(ta),
        .events = TE_STRDUP(events),
        .callback = callback,
    };

    *handle = new_entry - (ta_events_handler *)ta_events_handlers.data.ptr;

    rc = rcf_ta_events_subscribe(pid, tid);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_fmt(NULL, CVT_STRING, events,
                              "/agent:%s/ta_events:%u_%u_%u", ta, pid, tid,
                              *handle);
    if (rc != 0)
        return rc;

    return 0;
}

/* See description in tapi_ta_events.h */
te_errno
tapi_ta_events_unsubscribe(tapi_ta_events_handle handle)
{
    unsigned int       pid = (unsigned int)getpid();
    unsigned int       tid = (unsigned int)pthread_self();
    te_errno           rc;
    ta_events_handler *entry = NULL;

    if (handle < te_vec_size(&ta_events_handlers))
        entry = te_vec_get(&ta_events_handlers, handle);

    if (entry == NULL || !entry->used)
    {
        ERROR("Failed to unsubscribe %s TA events handle (%u)",
              entry == NULL ? "unknown" : "disabled", handle);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    rc = cfg_del_instance_fmt(false, "/agent:%s/ta_events:%u_%u_%u",
                              entry->ta, pid, tid, handle);
    if (rc != 0)
        return rc;

    rc = rcf_ta_events_unsubscribe(pid, tid);
    if (rc != 0)
        return rc;

    free(entry->ta);
    free(entry->events);

    *entry = (ta_events_handler){
        .used = false,
    };

    return 0;
}

/**
 * Call suitable TA events handlers
 *
 * @param ta          TA event source name (TA name)
 * @param msg_value   TA event name and value
 *
 * @return Status code
 *
 * @retval EINVAL    if there is only TA event name (without TA event value);
 * @retval ECANCELED if one of handlers returns @c false
 */
te_errno
tapi_ta_events_process_event(char *ta, char *msg_value)
{
    char              *value;
    char              *event = strtok_r(msg_value, " ", &value);
    ta_events_handler *handler;

    if (event == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    TE_VEC_FOREACH (&ta_events_handlers, handler)
    {
        if (handler->used && ta_events_handler_match(handler, event))
        {
            if (!handler->callback(ta, event, value))
                return TE_RC(TE_RCF_API, TE_ECANCELED);
        }
    }

    return 0;
}
