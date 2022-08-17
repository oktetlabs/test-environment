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
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"



static char vcm_address[20];

static char vcmconn_path[500];

/* TODO: explore correct -cp option! */
/* fixme!!!! */
static char java_command_base[] = "/usr/bin/java -cp <my_path> com.tilgin.vcm.connector.client.VoodTerminalServicesTestClient";

/**
 * Get route value.
 *
 * @param  gid          Group identifier (unused)
 * @param  oid          Object identifier (unused)
 * @param  value        Place for route value (gateway address
 *                      or zero if it is a direct route)
 * @param route_name    Name of the route
 *
 * @return              Status code
 */
static te_errno
vcm_swversion_get(unsigned int gid, const char *oid,
          char *value, const char *box_name)
{
    UNUSED(oid);
    UNUSED(gid);
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

    strcpy(value, "aa");
    return 0;
}

static te_errno
vcm_swversion_set(unsigned int gid, const char *oid,
                  char *value, const char *name)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(name);
    char box_name[100];
    char *p;
    char out_buffer[1000];
    char err_buffer[1000];
    char java_command[2000];

    int out_fd = -1, err_fd = -1;
    int status;

    pid_t java_cmd_pid;

    p = strstr(oid, "box:");
    p += 4;
    te_strlcpy(box_name, p, sizeof(box_name));

    p = strchr(box_name, '/');
    *p = 0;

    RING("%s: called for oid <%s> , box_name <%s>, value <%s>",
        __FUNCTION__, oid, box_name, value);

    snprintf(java_command, sizeof(java_command),
             "%s --setSoftwareRevision %s %s %s",
             java_command_base, vcm_address, box_name, value);

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
    RING("%s: status of java comand: %d",  __FUNCTION__, status);
    return 0;
}


static te_errno
vcm_parameter_get(unsigned int gid, const char *oid,
          char *value, const char *name)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(name);

    strcpy(value, "");
    return 0;
}


static te_errno
vcm_parameter_set(unsigned int gid, const char *oid,
          char *value, const char *name)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(name);
    char box_name[100];
    char *p;
    char out_buffer[1000];
    char err_buffer[1000];
    char java_command[2000];

    int out_fd = -1, err_fd = -1;
    int status;

    pid_t java_cmd_pid;

    p = strstr(oid, "box:");
    p += 4;
    te_strlcpy(box_name, p, sizeof(box_name));

    p = strchr(box_name, '/');
    *p = 0;

    RING("%s: called for oid <%s> , box_name <%s>, value <%s>",
        __FUNCTION__, oid, box_name, value);

    snprintf(java_command, sizeof(java_command),
             "%s --setSoftwareRevision %s %s %s",
             java_command_base, vcm_address, box_name, value);

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
    RING("%s: status of java comand: %d",  __FUNCTION__, status);
    return 0;
}

/**
 * Get route value.
 *
 * @param  gid          Group identifier (unused)
 * @param  oid          Object identifier (unused)
 * @param  value        Place for route value (gateway address
 *                      or zero if it is a direct route)
 * @param route_name    Name of the route
 *
 * @return              Status code
 */
static te_errno
vcm_get(unsigned int gid, const char *oid,
          char *value, const char *vcm_name)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(vcm_name);

    strcpy(value, vcm_address);

    return 0;
}

/**
 * Set VCM  value (that is, IP address to connect)
 *
 * @param       gid        Group identifier (unused)
 * @param       oid        Object identifier
 * @param       value      New value for the route
 * @param       route_name Name of the route
 *
 * @return      Status code.
 */
static te_errno
vcm_set(unsigned int gid, const char *oid, const char *value,
          const char *vcm_name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(vcm_name);

    te_strlcpy(vcm_address, value, sizeof(vcm_address));

    return 0;
}


static te_errno
vcmconn_path_get(unsigned int gid, const char *oid,
          char *value, const char *vcm_name)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(vcm_name);

    strcpy(value, vcmconn_path);

    return 0;
}

static te_errno
vcmconn_path_set(unsigned int gid, const char *oid,
          char *value, const char *vcm_name)
{
    UNUSED(oid);
    UNUSED(gid);
    UNUSED(vcm_name);

    te_strlcpy(vcmconn_path, value, sizeof(vcmconn_path));

    return 0;
}

/**
 * Determine list of VCM boxes.
 *
 * @param gid     group identifier (unused)
 * @param oid     full parent object instence identifier (unused)
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    location for the list pointer
 * @param name    interface name
 *
 * @return error code
 */
static te_errno
vcm_box_list(unsigned int gid, const char *oid,
             const char *sub_id, char **list,
             const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(name);

    /* TODO get from VCM */
    *list = strdup("V303L622R1A0-0001742121 V403L5155B10-0001553123 V601L622R1A0-1000000001");

    return 0;
}

RCF_PCH_CFG_NODE_RW(node_vcm_parameter, "parameter", NULL, NULL,
                    vcm_parameter_get, vcm_parameter_set);

RCF_PCH_CFG_NODE_RW(node_vcm_swversion, "swversion",
                    NULL, &node_vcm_parameter,
                    vcm_swversion_get, vcm_swversion_set);

RCF_PCH_CFG_NODE_RW(node_vcmconn_path, "vcmconn_path", NULL, NULL,
                    vcmconn_path_get, vcmconn_path_set);


RCF_PCH_CFG_NODE_COLLECTION(node_vcm_box, "box",
                            &node_vcm_swversion, &node_vcmconn_path,
                            NULL, NULL,
                            vcm_box_list, NULL);

RCF_PCH_CFG_NODE_RW(node_vcm, "vcm",
                    &node_vcm_box, NULL,
                    vcm_get, vcm_set);

/**
 * Initializes ta_unix_conf_vcm support.
 *
 * @return Status code (see te_errno.h)
 */
te_errno
ta_unix_conf_vcm_init()
{
    return rcf_pch_add_node("/agent", &node_vcm);
}
