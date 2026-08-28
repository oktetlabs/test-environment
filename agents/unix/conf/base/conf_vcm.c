/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * VCM configuring support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf VCM"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include<string.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_str.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_pch_tree.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "te_alloc.h"
#include "te_vector.h"



static char vcm_address[20];

static char vcmconn_path[500];

/* TODO: explore correct -cp option! */
/* fixme!!!! */
static char java_command_base[] = "/usr/bin/java -cp <my_path> com.tilgin.vcm.connector.client.VoodTerminalServicesTestClient";

/**
 * Get software version of a VCM box.
 *
 * @param ctx           Request context
 * @param val           Location for the value
 *
 * @return              Status code
 */
static te_errno
vcm_swversion_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *box_name = ta_conf_ctx_inst(ctx, "box");
    char buffer[1000];
    char java_command[2000];

    int out_fd = -1, err_fd = -1;
    int status;

    pid_t java_cmd_pid;

    snprintf(java_command, sizeof(java_command),
             "%s --getBoxDetails %s %s",
             java_command_base, vcm_address, box_name);

    java_cmd_pid = te_shell_cmd(java_command, -1, NULL, &out_fd, &err_fd);
    if (java_cmd_pid < 0)
    {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    read(err_fd, buffer, sizeof(buffer));
    buffer[999] = 0;
    RING("%s: java command stderr: <%s>", __FUNCTION__, buffer);

    ta_waitpid(java_cmd_pid, &status, 0);
    RING("%s: status of java command: %d", __FUNCTION__, status);

    te_string_append(val, "aa");
    return 0;
}

/**
 * Set software version of a VCM box.
 *
 * @param ctx           Request context
 * @param val           New value
 *
 * @return              Status code
 */
static te_errno
vcm_swversion_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *box_name = ta_conf_ctx_inst(ctx, "box");
    const char *oid = ta_conf_ctx_oid(ctx);
    char out_buffer[1000];
    char err_buffer[1000];
    char java_command[2000];

    int out_fd = -1, err_fd = -1;
    int status;

    pid_t java_cmd_pid;

    RING("%s: called for oid <%s> , box_name <%s>, value <%s>",
        __FUNCTION__, oid, box_name, val);

    snprintf(java_command, sizeof(java_command),
             "%s --setSoftwareRevision %s %s %s",
             java_command_base, vcm_address, box_name, val);

    RING("%s: prepared java command: <%s>",  __FUNCTION__, java_command);
    java_cmd_pid = te_shell_cmd(java_command, -1, NULL, &out_fd, &err_fd);
    if (java_cmd_pid < 0)
    {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    err_buffer[0] = 0;
    out_buffer[0] = 0;

    read(err_fd, err_buffer, sizeof(err_buffer));
    err_buffer[999] = 0;
    read(out_fd, out_buffer, sizeof(out_buffer));
    out_buffer[999] = 0;

    RING("%s: java command stdout: <%s>; stderr: <%s>",
         __FUNCTION__, out_buffer, err_buffer);

    ta_waitpid(java_cmd_pid, &status, 0);
    RING("%s: status of java command: %d",  __FUNCTION__, status);
    return 0;
}


/**
 * Get a VCM box parameter.
 *
 * @param ctx           Request context
 * @param val           Location for the value
 *
 * @return              Status code
 */
static te_errno
vcm_parameter_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    UNUSED(val);

    return 0;
}


/**
 * Set a VCM box parameter.
 *
 * @param ctx           Request context
 * @param val           New value
 *
 * @return              Status code
 */
static te_errno
vcm_parameter_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *box_name = ta_conf_ctx_inst(ctx, "box");
    const char *oid = ta_conf_ctx_oid(ctx);
    char out_buffer[1000];
    char err_buffer[1000];
    char java_command[2000];

    int out_fd = -1, err_fd = -1;
    int status;

    pid_t java_cmd_pid;

    RING("%s: called for oid <%s> , box_name <%s>, value <%s>",
        __FUNCTION__, oid, box_name, val);

    snprintf(java_command, sizeof(java_command),
             "%s --setSoftwareRevision %s %s %s",
             java_command_base, vcm_address, box_name, val);

    RING("%s: prepared java comand: <%s>",  __FUNCTION__, java_command);
    java_cmd_pid = te_shell_cmd(java_command, -1, NULL, &out_fd, &err_fd);
    if (java_cmd_pid < 0)
    {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    err_buffer[0] = 0;
    out_buffer[0] = 0;

    read(err_fd, err_buffer, sizeof(err_buffer));
    err_buffer[999] = 0;
    read(out_fd, out_buffer, sizeof(out_buffer));
    out_buffer[999] = 0;

    RING("%s: java command stdout: <%s>; stderr: <%s>",
         __FUNCTION__, out_buffer, err_buffer);

    ta_waitpid(java_cmd_pid, &status, 0);
    RING("%s: status of java command: %d",  __FUNCTION__, status);
    return 0;
}

/**
 * Get VCM address (that is, IP address to connect to).
 *
 * @param ctx           Request context
 * @param val           Location for the value
 *
 * @return              Status code
 */
static te_errno
vcm_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    te_string_append(val, "%s", vcm_address);

    return 0;
}

/**
 * Set VCM address (that is, IP address to connect to).
 *
 * @param ctx           Request context
 * @param val           New value
 *
 * @return              Status code.
 */
static te_errno
vcm_set(const ta_conf_ctx *ctx, const char *val)
{
    UNUSED(ctx);

    te_strlcpy(vcm_address, val, sizeof(vcm_address));

    return 0;
}


static te_errno
vcmconn_path_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    te_string_append(val, "%s", vcmconn_path);

    return 0;
}

static te_errno
vcmconn_path_set(const ta_conf_ctx *ctx, const char *val)
{
    UNUSED(ctx);

    te_strlcpy(vcmconn_path, val, sizeof(vcmconn_path));

    return 0;
}

/**
 * Determine list of VCM boxes.
 *
 * @param ctx     Request context
 * @param names   Vector of heap-allocated names to append to
 *
 * @return error code
 */
static te_errno
vcm_box_list(const ta_conf_ctx *ctx, te_vec *names)
{
    /* TODO get from VCM */
    static const char boxes[] =
        "V303L622R1A0-0001742121 V403L5155B10-0001553123 "
        "V601L622R1A0-1000000001";
    char *copy = TE_STRDUP(boxes);
    char *saveptr;
    char *tok;

    UNUSED(ctx);

    for (tok = strtok_r(copy, " ", &saveptr); tok != NULL;
         tok = strtok_r(NULL, " ", &saveptr))
    {
        char *name = TE_STRDUP(tok);

        TE_VEC_APPEND(names, name);
    }
    free(copy);

    return 0;
}

static const ta_conf_node *const node_vcm =
    TA_CONF_RW_STR("vcm", vcm_get, vcm_set,
        TA_CONF_LIST("box", vcm_box_list,
            TA_CONF_RW_STR("swversion", vcm_swversion_get,
                           vcm_swversion_set),
            TA_CONF_RW_STR("parameter", vcm_parameter_get,
                           vcm_parameter_set)),
        TA_CONF_RW_STR("vcmconn_path", vcmconn_path_get,
                       vcmconn_path_set));

/**
 * Initializes ta_unix_conf_vcm support.
 *
 * @return Status code (see te_errno.h)
 */
te_errno
ta_unix_conf_vcm_init()
{
    return ta_conf_register("/agent", node_vcm);
}
