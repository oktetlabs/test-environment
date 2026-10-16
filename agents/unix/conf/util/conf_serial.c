/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent serial console support.
 *
 * Implementation of unix TA serial console configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf Serial Console"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_alloc.h"
#include "te_serial.h"
#include "te_kernel_log.h"

#include "logger_api.h"
#include "te_raw_log.h"
#include "te_sockaddr.h"
#include "unix_internal.h"
#include "te_string.h"
#include "te_vector.h"
#include "rcf_pch_tree.h"

/** Head of the consoles list */
static SLIST_HEAD(, serial_console_t) consoles;

/**
 * Search for the console by name.
 *
 * @param name        console name.
 *
 * @return The console unit or @c NULL.
 */
static serial_console_t *
console_get_by_name(const char *name)
{
    serial_console_t *console;

    if (name == NULL)
        return NULL;

    SLIST_FOREACH(console, &consoles, next)
    {
        if (strcmp(console->inst_name, name) == 0)
            return console;
    }
    return NULL;
}

/**
 * Find the console a request is addressed to.
 *
 * @param ctx       Request context
 *
 * @return The console unit or @c NULL.
 */
static serial_console_t *
console_find(const ta_conf_ctx *ctx)
{
    return console_get_by_name(ta_conf_ctx_inst(ctx, "console"));
}

/**
 * Add the console object
 *
 * @param ctx       Request context
 * @param cname     The serial console name
 *
 * @return Status code
 */
static te_errno
console_add(const ta_conf_ctx *ctx, const char *cname)
{
    const char       *inst_name = ta_conf_ctx_inst(ctx, "console");
    serial_console_t *console;

    if (strlen(inst_name) > TE_SERIAL_MAX_NAME)
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    if (console_get_by_name(inst_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    console = TE_ALLOC_UNINITIALIZED(sizeof(serial_console_t));
    console->port                           = TE_SERIAL_PORT;
    te_strlcpy(console->inst_name, inst_name, sizeof(console->inst_name));
    te_strlcpy(console->name, cname, sizeof(console->name));
    te_strlcpy(console->user, TE_SERIAL_USER, sizeof(console->user));
    memset(&console->address, 0, sizeof(console->address));
    SIN(&console->address)->sin_family = AF_INET;
    SIN(&console->address)->sin_port = 0;
    SLIST_INSERT_HEAD(&consoles, console, next);

    return 0;
}

/**
 * Delete the console object.
 *
 * @param ctx       Request context
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_del(const ta_conf_ctx *ctx)
{
    serial_console_t *console;

    console = console_find(ctx);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&consoles, console, serial_console_t, next);
    free(console);

    return 0;
}

/**
 * Set a serial console name for the console.
 *
 * @param ctx       Request context
 * @param cname     Serial console name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_set(const ta_conf_ctx *ctx, const char *cname)
{
    serial_console_t *console;

    console = console_find(ctx);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(console->name, cname, sizeof(console->name));

    return 0;
}

/**
 * Get a serial console name of the console.
 *
 * @param ctx       Request context
 * @param cname     Location for the serial console name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_get(const ta_conf_ctx *ctx, te_string *cname)
{
    serial_console_t *console;

    console = console_find(ctx);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(cname, "%s", console->name);

    return 0;
}

/**
 * Get instance list of the consoles.
 *
 * @param ctx       Request context (unused)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_list(const ta_conf_ctx *ctx, te_vec *names)
{
    serial_console_t   *console;

    UNUSED(ctx);

    SLIST_FOREACH(console, &consoles, next)
    {
        char *name = TE_STRDUP(console->inst_name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Get the conserver port of the console.
 *
 * @param ctx       Request context
 * @param val       Location for the port
 *
 * @return Status code
 */
static te_errno
console_port_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_console_t *console = console_find(ctx);

    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = console->port;
    return 0;
}

/**
 * Set the conserver port of the console.
 *
 * @param ctx       Request context
 * @param val       Port
 *
 * @return Status code
 */
static te_errno
console_port_set(const ta_conf_ctx *ctx, int32_t val)
{
    serial_console_t *console = console_find(ctx);

    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    console->port = val;
    return 0;
}

/**
 * Get the conserver user of the console.
 *
 * @param ctx       Request context
 * @param val       Location for the user name
 *
 * @return Status code
 */
static te_errno
console_user_get(const ta_conf_ctx *ctx, te_string *val)
{
    serial_console_t *console = console_find(ctx);

    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", console->user);
    return 0;
}

/**
 * Set the conserver user of the console.
 *
 * @param ctx       Request context
 * @param val       User name
 *
 * @return Status code
 */
static te_errno
console_user_set(const ta_conf_ctx *ctx, const char *val)
{
    serial_console_t *console = console_find(ctx);

    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(console->user, TE_SERIAL_MAX_NAME, "%s", val);
    return 0;
}

/**
 * Get the conserver address of the console.
 *
 * @param ctx       Request context
 * @param val       Location for the address
 *
 * @return Status code
 */
static te_errno
console_address_get(const ta_conf_ctx *ctx, struct sockaddr_storage *val)
{
    serial_console_t *console = console_find(ctx);

    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = console->address;
    return 0;
}

/**
 * Set the conserver address of the console.
 *
 * @param ctx       Request context
 * @param val       IPv4 or IPv6 address
 *
 * @return Status code
 */
static te_errno
console_address_set(const ta_conf_ctx *ctx, const struct sockaddr *val)
{
    serial_console_t *console = console_find(ctx);

    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (val->sa_family != AF_INET && val->sa_family != AF_INET6)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    memcpy(&console->address, val, te_sockaddr_get_size(val));
    return 0;
}

static const ta_conf_node *const node_console =
    TA_CONF_COLL_STR_RW("console", console_get, console_set,
                        console_add, console_del, console_list,
        TA_CONF_RW_INT32("port", console_port_get, console_port_set),
        TA_CONF_RW_STR("user", console_user_get, console_user_set),
        TA_CONF_RW_ADDRESS("address", console_address_get,
                           console_address_set));

/** Initialize the module and add the subtree to agent tree */
te_errno
ta_unix_serial_console_init(void)
{
    SLIST_INIT(&consoles);

    return ta_conf_register("/agent", node_console);
}

/** Clean up the existing consoles */
te_errno
ta_unix_serial_console_cleanup(void)
{
    serial_console_t     *console;
    serial_console_t     *tmp;

    SLIST_FOREACH_SAFE(console, &consoles, next, tmp)
    {
        SLIST_REMOVE(&consoles, console, serial_console_t, next);
        free(console);
    }

    return 0;
}
