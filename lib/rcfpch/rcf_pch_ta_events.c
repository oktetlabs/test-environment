/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Agent events functions.
 *
 * TA events configuration tree support.
 *
 *
 * Copyright (C) 2024-2024 ARK NETWORKS LLC. All rights reserved.
 */

#define TE_LGR_USER     "TA Events"

#include "te_config.h"
#include "config.h"

#include "te_alloc.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_str.h"

#include "logger_api.h"
#include "rcf_pch.h"

/** TA events to filter */
typedef struct ta_events_param {
    SLIST_ENTRY(ta_events_param) links; /**< List links */

    char *name;  /** Unique TA events ID (RCF client + TA events handle) */
    char *value; /** Comma separated list of interesting TA event name  */
} ta_events_param;

/** Head of list TA events to filters */
typedef SLIST_HEAD(ta_events_param_list,
                   ta_events_param) ta_events_param_list;

/** Full set of TA events to filters */
static ta_events_param_list ta_events_params =
    SLIST_HEAD_INITIALIZER(ta_events_params);

/** Internal buffer to generate list of TA events */
static char buf[4096];

/**
 * Check that given TA events @p param is match for TA @p event
 *
 * @param param   TA events parameters
 * @param event   Required name of TA event
 *
 * @return @c true if TA events parameter is suitable for this @p event
 */
static bool
ta_events_param_match(ta_events_param *param, const char* event)
{
    int   len = strlen(event);
    char *ptr;

    assert(len > 0);

    for (ptr = param->value; ptr != NULL; ptr++)
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
 * Find TA events parameters by unique @ref name
 *
 * @param name       Required TA events parameters name
 *
 * @retval @c NULL   If there is no such TA events parameters
 * @retval other     Pointer to required TA events parameters
 */
static ta_events_param *
ta_events_find(const char *name)
{
    ta_events_param *param;

    SLIST_FOREACH (param, &ta_events_params, links)
    {
        if (strcmp(name, param->name) == 0)
            return param;
    }

    return NULL;
}

/**
 * Get the value of TA events parameter
 *
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  Location for the value
 * @param name   Unique TA events ID
 *
 * @return Status code.
 */
static te_errno
ta_events_get(unsigned int gid, const char *oid, char *value,
              const char *name)
{
    ta_events_param *param;

    param = ta_events_find(name);
    if (param == NULL)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, TE_ENOENT);

        ERROR("Failed to find TA events name (%s) to get value; errno %r",
              name, rc);
        return rc;
    }

    te_strlcpy(value, param->value, RCF_MAX_VAL);
    return 0;
}

/**
 * Add the new TA events parameter
 *
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param value  Comma separated list of interesting TA events
 * @param name   Unique TA events ID
 *
 * @return Status code.
 */
static te_errno
ta_events_add(unsigned int gid, const char *oid, const char *value,
              const char *name)
{
    ta_events_param *cur;
    ta_events_param *last;
    ta_events_param *new_param = TE_ALLOC(sizeof(*new_param));

    INFO("Adding TA events '%s' with value '%s'", name, value);

    if (value[0] == '\0')
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, TE_EINVAL);

        ERROR("No TA events value to add; errno %r", rc);
        return rc;
    }

    *new_param = (ta_events_param){
        .name = strdup(name),
        .value = strdup(value),
    };

    if (SLIST_EMPTY(&ta_events_params))
    {
        SLIST_INSERT_HEAD(&ta_events_params, new_param, links);
        return 0;
    }

    SLIST_FOREACH (cur, &ta_events_params, links)
    {
        if (strcmp(cur->name, name) == 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, TE_EEXIST);

            ERROR("Such TA events already exists; errno %r", rc);
            return rc;
        }
        last = cur;
    }

    assert(last != NULL);
    SLIST_INSERT_AFTER(last, new_param, links);
    return 0;
}

/**
 * Remove the existing TA events parameter
 *
 * @param gid    Group identifier (unused).
 * @param oid    Full object instance identifier (unused).
 * @param name   Unique TA events ID
 *
 * @return Status code.
 */
static te_errno
ta_events_del(unsigned int gid, const char *oid, const char *name)
{
    ta_events_param *param;

    INFO("Freeing TA events '%s'", name);

    param = ta_events_find(name);
    if (param == NULL)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, TE_ENOENT);

        ERROR("Failed to find TA events to remove; errno %r", rc);
        return rc;
    }

    SLIST_REMOVE(&ta_events_params, param, ta_events_param, links);
    return 0;
}

/**
 * Get instance list for object TA events.
 *
 * @param gid           Group identifier
 * @param oid           Full identifier of the father instance
 * @param sub_id        ID of the object to be listed
 * @param list          Location for the list pointer
 *
 * @return              Status code
 * @retval 0            Success
 * @retval TE_ENOMEM    Cannot allocate memory
 */
static te_errno
ta_events_list(unsigned int gid, const char *oid, const char *sub_id,
               char **list)
{
    te_errno   rc;
    te_string  str = TE_STRING_BUF_INIT(buf);
    ta_events_param *param;

    buf[0] = '\0';

    SLIST_FOREACH (param, &ta_events_params, links)
    {
        rc = te_string_append(&str, "%s ", param->name);
        if (rc != 0)
            return rc;
    }

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

RCF_PCH_CFG_NODE_RW_COLLECTION(node_ta_events, "ta_events", NULL, NULL,
                               ta_events_get, NULL, ta_events_add,
                               ta_events_del, ta_events_list, NULL);

/* See description in rcf_pch_ta_events.h */
te_errno
rcf_pch_ta_events_conf_init(void)
{
    return rcf_pch_add_node("/agent", &node_ta_events);
}

/* See description in rcf_pch_ta_events.h */
int
rcf_pch_ta_events_collect_rcf_clients(const char *event, te_string *rcf_clients)
{
    int              cnt = 0;
    ta_events_param *param;

    SLIST_FOREACH (param, &ta_events_params, links)
    {
        if (ta_events_param_match(param, event))
        {
            char *last_delim = strrchr(param->name, '_');

            if (cnt++ != 0)
                te_string_append(rcf_clients, ",");
            te_string_append_buf(rcf_clients, param->name,
                                 last_delim - param->name);
        }
    }

    return cnt;
}
