/** @file
 * @brief Unix Test Agent serial console support.
 *
 * Implementation of unix TA serial console configuring support.
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Svetlana Menshikova <Svetlana.Menshikova@oktetlabs.ru>
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf Serial Console"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_serial.h"
#include "te_kernel_log.h"

#include "logger_api.h"
#include "te_raw_log.h"
#include "te_sockaddr.h"
#include "unix_internal.h"
#include "te_string.h"

/** Head of the consoles list */
SLIST_HEAD(, serial_console_t) consoles;

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
 * Add the console object
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param cname     The serial console name
 * @param inst_name The console name
 *
 * @return Status code
 */
static te_errno
console_add(unsigned int gid, const char *oid, const char *cname,
            const char *inst_name)
{
    serial_console_t *console;

    UNUSED(gid);
    UNUSED(oid);

    TE_SERIAL_MALLOC(console, sizeof(serial_console_t));
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
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param inst_name    The console name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_del(unsigned int gid, const char *oid, const char *inst_name)
{
    serial_console_t *console;

    UNUSED(gid);
    UNUSED(oid);

    console = console_get_by_name(inst_name);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_REMOVE(&consoles, console, serial_console_t, next);
    free(console);

    return 0;
}

/**
 * Set a serial console name for the console.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param cname        Serial console name.
 * @param inst_name    The console name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_set(unsigned int gid, const char *oid, const char *cname,
            const char *inst_name)
{
    serial_console_t *console;

    UNUSED(gid);
    UNUSED(oid);

    console = console_get_by_name(inst_name);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(console->name, cname, sizeof(console->name));

    return 0;
}

/**
 * Get a serial console name of the console.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param cname        Serial console name.
 * @param inst_name    The console name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_get(unsigned int gid, const char *oid, char *cname,
            const char *inst_name)
{
    serial_console_t *console;

    UNUSED(gid);
    UNUSED(oid);

    console = console_get_by_name(inst_name);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(cname, console->name, RCF_MAX_VAL);

    return 0;
}

/**
 * Get instance list of the consoles.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full parent object instance identifier (unused).
 * @param sub_id    ID of the object to be listed (unused).
 * @param list      Location for the list pointer.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_list(unsigned int gid, const char *oid, const char *sub_id,
             char **list)
{
    te_errno            rc;
    serial_console_t   *console;
    te_string           str = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    SLIST_FOREACH(console, &consoles, next)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s",
                              console->inst_name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    *list = str.ptr;

    return 0;
}

/**
 * Common function to set variable values.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier.
 * @param value        New value.
 * @param inst_name    The console name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_common_set(unsigned int gid, const char *oid, const char *value,
                   const char *inst_name)
{
    te_errno          rc = 0;
    serial_console_t *console;

    UNUSED(gid);

    console = console_get_by_name(inst_name);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (value == NULL)
    {
        ERROR("A variable value is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/port:") != NULL)
    {
        console->port = atoi(value);
    }
    else if (strstr(oid, "/user:") != NULL)
    {
        snprintf(console->user, TE_SERIAL_MAX_NAME, "%s", value);
    }
    else if (strstr(oid, "/address:") != NULL)
    {
        rc = te_sockaddr_str2h(value, SA(&console->address));
    }
    else
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return rc;
}

/**
 * Common function to get variable values.
 *
 * @param[in]  gid          Group identifier (unused).
 * @param[in]  oid          Full object instance identifier.
 * @param[out] value        New value.
 * @param[in]  inst_name    The console name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_common_get(unsigned int gid, const char *oid, char *value,
                   const char *inst_name)
{
    serial_console_t *console;

    UNUSED(gid);

    console = console_get_by_name(inst_name);
    if (console == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (value == NULL)
    {
       ERROR("A buffer to get a variable value is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/port:") != NULL)
    {
        snprintf(value, RCF_MAX_VAL, "%d", console->port);
    }
    else if (strstr(oid, "/user:") != NULL)
    {
        snprintf(value, RCF_MAX_VAL, "%s", console->user);
    }
    else if (strstr(oid, "/address:") != NULL)
    {
        snprintf(value, RCF_MAX_VAL, "%s",
                 te_sockaddr_get_ipstr(SA(&console->address)));
    }
    else
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return 0;
}

RCF_PCH_CFG_NODE_RW(console_address, "address", NULL, NULL,
                    console_common_get, console_common_set);

RCF_PCH_CFG_NODE_RW(console_user, "user", NULL, &console_address,
                    console_common_get, console_common_set);

RCF_PCH_CFG_NODE_RW(console_port, "port", NULL, &console_user,
                    console_common_get, console_common_set);

static rcf_pch_cfg_object node_console_inst =
    { "console", 0, &console_port, NULL,
      (rcf_ch_cfg_get)console_get, (rcf_ch_cfg_set)console_set,
      (rcf_ch_cfg_add)console_add, (rcf_ch_cfg_del)console_del,
      (rcf_ch_cfg_list)console_list, NULL, NULL, NULL };

/** Initialize the module and add the subtree to agent tree */
te_errno
ta_unix_serial_console_init(void)
{
    SLIST_INIT(&consoles);

    return rcf_pch_add_node("/agent", &node_console_inst);
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
