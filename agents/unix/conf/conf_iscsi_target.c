/** @file
 * @brief Unix Test Agent
 *
 * iSCSI Target Configuring
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Renata Sayakhova <Renata.Sayakhova@oktetlabs.ru>
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef WITH_ISCSI
#include <stddef.h>
#include "conf_daemons.h"
#include <sys/wait.h>
#include "iscsi_target_api.h"


static te_errno
copy_value(char *buf, int size, void *value)
{
    memcpy(value, buf, size);
    return 0;
}

static te_errno
boolean_value(char *buf, int size, void *value)
{
    UNUSED(size);

    if (strcmp(buf, "true") == 0)
        *(char *)value = '1';
    else if (strcmp(buf, "false") == 0)
        *(char *)value = '0';
    else
        return TE_RC(TE_TA_UNIX, TE_EPROTO);
    ((char *)value)[1] = '\0';
    return 0;
}

static const char *
map_oid_to_seccmd(const char *oid)
{
    static char *mappings[][2] =
        {{"pn:", "peername"},
         {"px:", "peersecret"},
         {"ln:", "localname"},
         {"lx:", "localsecret"},
         {"t:", "mutualauth"},
         {"b:", "base64"},
         {"cl:", "length"},
         {NULL, NULL}
        };
    int i;
    
    oid = strrchr(oid, '/');
    if (oid == NULL)
    {
        ERROR("OID is malformed");
        return "";
    }

    oid++;
    for (i = 0; mappings[i][0] != NULL; i++)
    {
        if (strcmp(mappings[i][0], oid) == 0)
        {
            return mappings[i][1];
        }
    }
    ERROR("Unknown OID: %s", oid);
    return "";
}


static te_errno
iscsi_target_security_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }

    return iscsi_target_send_msg(copy_value, value, 
                                 "getsecurity", 
                                 map_oid_to_seccmd(oid));
}

/** Set CHAP peer name */
static te_errno
iscsi_target_security_set(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        return (*value == '\0' ? 0 : TE_RC(TE_TA_UNIX, TE_ECONNREFUSED));
    }

    return iscsi_target_send_msg(NULL, NULL, 
                                 "security", 
                                 "%s %s",
                                 map_oid_to_seccmd(oid), value);
}

static te_errno
iscsi_target_security_flag_get(unsigned int gid, const char *oid,
                   char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }

    return iscsi_target_send_msg(boolean_value, value, 
                                 "getsecurity", 
                                 map_oid_to_seccmd(oid));
}

/** Set CHAP peer name */
static te_errno
iscsi_target_security_flag_set(unsigned int gid, const char *oid,
                          char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        return (*value == '\0' || *value == '0' ? 
                0 : TE_RC(TE_TA_UNIX, TE_ECONNREFUSED));
    }

    return iscsi_target_send_msg(NULL, NULL, 
                                 "security", 
                                 "%s %s",
                                 map_oid_to_seccmd(oid), 
                                 (*value == '\0' || *value == '0') ? "false" : "true");
}


static te_errno
iscsi_target_chap_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        return (*value == '\0' ? 0 : TE_RC(TE_TA_UNIX, TE_ECONNREFUSED));
    }

    return iscsi_target_send_msg(NULL, NULL, 
                                 "set", 
                                 "AuthMethod=%s",
                                 (*value == '\0' ? "None" : value));
    return 0;
}   

/** Set auth method */
static te_errno
iscsi_target_chap_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }

    return iscsi_target_send_msg(copy_value, value, 
                                 "get", 
                                 "AuthMethod");

    return 0;
}

/** Maps OIDs to iSCSI parameter names algorithmically.
 * The algoritm is as follows:
 *  -# The OID is truncated to the rightmost object name
 *  -# A list of special cases is looked up and the corresponding
 *  name is used, if an OID is found in the list.
 *  -# Otherwise, all underscores are removed and the following
 *  letter is capitalized. Also capitalized are the first letter 
 *  and any letter following a digit.
 */   
static const char *
map_oid_to_param(const char *oid)
{
    static char param_name[32];
    te_bool upper_case = TRUE;
    char *p = param_name;
    static char *special_mappings[][2] =
        {{"data_pdu_in_order:", "DataPDUInOrder"},
         {"if_marker:", "IFMarker"},
         {"of_marker:", "OFMarker"},
         {"if_mark_int:", "IFMarkInt"},
         {"of_mark_int:", "OFMarkInt"},
         {NULL, NULL}
        };
    int i;
    
    oid = strrchr(oid, '/');
    if (oid == NULL)
    {
        ERROR("OID is malformed");
        return "";
    }

    oid++;
    for (i = 0; special_mappings[i][0] != NULL; i++)
    {
        if (strcmp(special_mappings[i][0], oid) == 0)
        {
            return special_mappings[i][1];
        }
    }

    for (; *oid != ':' && *oid != '\0'; oid++)
    {
        if (upper_case)
        {
            *p++ = toupper(*oid);
            upper_case = FALSE;
        }
        else if (*oid != '_')
        {
            *p++ = *oid;
        }
        if (*oid == '_' || isdigit(*oid))
            upper_case = TRUE;
    }
    *p = '\0';
    return param_name;
}

/** Get an operational parameter */
static te_errno
iscsi_target_oper_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    const char *param = map_oid_to_param(oid);
    UNUSED(gid);
    UNUSED(instance);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }

    return iscsi_target_send_msg(copy_value, value, 
                                 "get", 
                                 param);
}


/** Set an operational parameter */
static te_errno
iscsi_target_oper_set(unsigned int gid, const char *oid,
                      const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    if (*value == '\0')
    {
        if (!iscsi_server_check())
            return 0;
        return iscsi_target_send_msg(NULL, NULL,
                                     "restore",
                                     map_oid_to_param(oid));
    }
    else
    {
        if (!iscsi_server_check())
            return TE_RC(TE_TA_UNIX, TE_ECONNREFUSED);

        return iscsi_target_send_msg(NULL, NULL, 
                                     "set",
                                     "%s=%s", 
                                     map_oid_to_param(oid), value);
    }
}

#define TARGET_BLOCK_SIZE 512

static int is_backstore_mounted;
static char backstore_mountpoint[128];

/** Mount a backing store as a loopback filesystem */
static int
iscsi_target_backstore_mount(void)
{
    int status;
    char cmd[128];

    if (is_backstore_mounted++ > 0)
        return 0;

    RING("Mounting iSCSI target backing store as a loop device");
    status = iscsi_target_send_msg(NULL, NULL,
                                   "sync", "0 0");
    if (status != 0)
        return status;
    
    if (mkdir(backstore_mountpoint, S_IREAD | S_IWRITE | S_IEXEC) != 0 && errno != EEXIST)
    {
        status = errno;
        ERROR("Cannot create mountpoint for backing store: %s",
              strerror(status));
        return TE_OS_RC(TE_TA_UNIX, status);
    }
    snprintf(cmd,  sizeof(cmd), "/bin/mount -o loop,sync /tmp/te_backing_store.%lu %s",
             (unsigned long)getpid(), 
             backstore_mountpoint);
    status = ta_system(cmd);
    if (status < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        ERROR("Cannot mount backing store");
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    return 0;
}

/** Unmount a backing store */
static void
iscsi_target_backstore_unmount(void)
{
    int status;
    char cmd[128];

    if (is_backstore_mounted == 0)
        return;
    if (--is_backstore_mounted != 0)
        return;

    snprintf(cmd, sizeof(cmd), "/bin/umount %s", backstore_mountpoint);
    status = ta_system(cmd);
    if (status < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        WARN("Cannot unount backing store");
    }
    if (rmdir(backstore_mountpoint) != 0)
    {
        WARN("Cannot delete backing store mountpoint: %s",
             strerror(errno));
    }
}

typedef struct device_param {
    te_bool  is_mmap;
    unsigned long size;    
} device_param;

/** Get the size of a target backing store */
static te_errno
parse_device_param(char *buf, int size, void *result)
{
    device_param *p = result;
    char b[32];

    UNUSED(size);

    if (sscanf(buf, "%s %lu", b, &p->size) != 2)
        return TE_RC(TE_TA_UNIX, TE_EPROTO);
    p->is_mmap = (strcmp(b, "true") == 0);
    return 0;
}

static te_errno
iscsi_target_backstore_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    device_param params;
    int      rc;

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }


    rc = iscsi_target_send_msg(parse_device_param, &params,
                               "getparam", "0 0");
    if (rc != 0)
        return rc;
    if (!params.is_mmap)
        *value = '\0';
    else if (params.size > 1024 * 1024 && (params.size % (1024 * 1024) == 0))
    {
        sprintf(value, "%lum", (unsigned long)params.size / (1024 * 1024));
    }
    else if (params.size > 1024 && (params.size % 1024) == 0)
    {
        sprintf(value, "%luk", (unsigned long)params.size / 1024);
    }
    else
    {
        sprintf(value, "%lu", (unsigned long)params.size);
    }
    return 0;
}

static te_errno
iscsi_target_backstore_set(unsigned int gid, const char *oid,
                           const char *value, const char *instance, ...)
{
    char         fname[64];
    char         cmd[64];

    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);


    if (!iscsi_server_check())
    {
        return 0;
    }


    while (is_backstore_mounted > 0)
        iscsi_target_backstore_unmount();
    sprintf(fname, "/tmp/te_backing_store.%lu", (unsigned long)getpid());
    if (*value == '\0')
    {
        if (remove(fname) != 0)
        {
            WARN("Cannot remove backing store: %s", strerror(errno));
        }
        return iscsi_target_send_msg(NULL, NULL,
                                     "sync", "0 0");
    }
    else
    {
        int          rc;
        int          fd;
        char         *end;
        unsigned long size = strtoul(value, &end, 10);
        const char   zero[1] = {0};
        
        switch(*end)
        {
            case 'k':
            case 'K':
                size *= 1024;
                break;
            case 'm':
            case 'M':
                size *= 1024 * 1024;
                break;
            case '\0': /* do nothing */
                break;
            default:
                ERROR("Invalid size specifier '%s'", value);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        if ((size % TARGET_BLOCK_SIZE) != 0)
        {
            ERROR("The size %lu is not a multiply of SCSI block size",
                  size);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        fd = open(fname, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
        if (fd < 0)
        {
            rc = errno;
            ERROR("Cannot create backing store: %s", strerror(rc));
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
        if (lseek(fd, size - 1, SEEK_SET) == (off_t)-1)
        {
            rc = errno;
            ERROR("Cannot seek to %lu: %s", (unsigned long)size,
                  strerror(rc));
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
        if (write(fd, zero, 1) < 1)
        {
            rc = errno;
            ERROR("Cannot create a backing store of size %lu: %s", size,
                  strerror(rc));
            remove(fname);
            close(fd);
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
        close(fd);
        sprintf(cmd, "/sbin/mke2fs -F -q %s", fname);
        rc = ta_system(cmd);
        if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
        {
            ERROR("Cannot create a file system on backing store");
            remove(fname);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        rc = iscsi_target_send_msg(NULL, NULL,
                                   "mmap",
                                   "0 0 %s", fname);
        if (rc != 0)
        {
            remove(fname);
            return rc;
        }
        return 0;
    }
}


/** Set a backing store mount point */
static te_errno
iscsi_tgt_backstore_mp_set(unsigned int gid, const char *oid,
                           const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (!iscsi_server_check())
    {
        return 0;
    }

    if (strcmp(value, backstore_mountpoint) != 0)
    {
        while (is_backstore_mounted > 0)
            iscsi_target_backstore_unmount();
    }

    strcpy(backstore_mountpoint, value);
    return *value == '\0' ? 0 : iscsi_target_backstore_mount();
}

/** Get a backing store mount point */
static te_errno
iscsi_tgt_backstore_mp_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    strcpy(value, backstore_mountpoint);
    return 0;
}

/** Get a target verbosity level */
static te_errno
iscsi_tgt_verbose_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }


    return iscsi_target_send_msg(copy_value, value,
                                 "getverbosity", 
                                 NULL, 0);

}

/** Set a target verbosity level */
static te_errno
iscsi_tgt_verbose_set(unsigned int gid, const char *oid,
                      const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (!iscsi_server_check())
    {
        /* obviously, there's no harm in doing this,
         *  but it prevents history restore failing
         *  in some cases
         */
        return 0;
    }
    
    return iscsi_target_send_msg(NULL, NULL,
                                 "verbosity", 
                                 value);

}

/** A stub for a target topmost object */
static te_errno
iscsi_target_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    *value = '\0';
    return 0;
}


static int max_cmd_sn_delta;

/** Get a default MaxCmdSn - ExpCmdSn value */
static te_errno
iscsi_tgt_max_cmd_sn_delta_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    sprintf(value, "%d", max_cmd_sn_delta);
    return 0;
}

/** Get a default MaxCmdSn - ExpCmdSn value */
static te_errno
iscsi_tgt_max_cmd_sn_delta_set(unsigned int gid, const char *oid,
                               const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    max_cmd_sn_delta = strtol(value, NULL, 10);
    if (!iscsi_server_check())
    {
        return (max_cmd_sn_delta == 0 ? 0 : TE_RC(TE_TA_UNIX, TE_ECONNREFUSED));
    }
    
    return iscsi_target_send_msg(NULL, NULL,
                                 "tweak",
                                 "-1 max_cmd_sn_delta %d",
                                 max_cmd_sn_delta);
}

static te_errno
iscsi_tgt_phase_collapse_get(unsigned int gid, const char *oid,
                               char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (!iscsi_server_check())
    {
        *value = '\0';
        return 0;
    }


    return iscsi_target_send_msg(boolean_value, value,
                                 "collapse", 
                                 "keep");
}

static te_errno
iscsi_tgt_phase_collapse_set(unsigned int gid, const char *oid,
                             const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    UNUSED(oid);

    if (!iscsi_server_check())
    {
        return (*value == '\0' || *value == '0' ? 
                0 : TE_RC(TE_TA_UNIX, TE_ECONNREFUSED));
    }

    return iscsi_target_send_msg(NULL, NULL,
                                 "collapse", 
                                 (*value == '\0' || *value == '0') ? "false" : "true");

}


RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_phase_collapse, "phase_collapse",
                    NULL, NULL,
                    iscsi_tgt_phase_collapse_get, 
                    iscsi_tgt_phase_collapse_set);


RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_max_cmd_sn_delta, "max_cmd_sn_delta",
                    NULL, &node_iscsi_target_oper_phase_collapse,
                    iscsi_tgt_max_cmd_sn_delta_get, 
                    iscsi_tgt_max_cmd_sn_delta_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_if_mark_int, "if_mark_int",
                    NULL, &node_iscsi_target_oper_max_cmd_sn_delta,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_of_mark_int, "of_mark_int",
                    NULL, &node_iscsi_target_oper_if_mark_int,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_if_marker, "if_marker",
                    NULL, &node_iscsi_target_oper_of_mark_int,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_of_marker, "of_marker",
                    NULL, &node_iscsi_target_oper_if_marker,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_session_type, "session_type",
                    NULL, &node_iscsi_target_oper_of_marker,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_error_recovery_level, "error_recovery_level",
                    NULL, &node_iscsi_target_oper_session_type,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_data_sequence_in_order, "data_sequence_in_order",
                    NULL, &node_iscsi_target_oper_error_recovery_level,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_data_pdu_in_order, "data_pdu_in_order",
                    NULL, &node_iscsi_target_oper_data_sequence_in_order,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_max_outstanding_r2t, "max_outstanding_r2t",
                    NULL, &node_iscsi_target_oper_data_pdu_in_order,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_default_time2retain, "default_time2retain",
                    NULL, &node_iscsi_target_oper_max_outstanding_r2t,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_default_time2wait, "default_time2wait",
                    NULL, &node_iscsi_target_oper_default_time2retain,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_first_burst_length, "first_burst_length",
                    NULL, &node_iscsi_target_oper_default_time2wait,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_max_burst_length, 
                    "max_burst_length",
                    NULL, &node_iscsi_target_oper_first_burst_length,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_max_recv_data_segment_length, 
                    "max_recv_data_segment_length",
                    NULL, &node_iscsi_target_oper_max_burst_length,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_immediate_data, 
                    "immediate_data",
                    NULL, &node_iscsi_target_oper_max_recv_data_segment_length,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_initial_r2t, 
                    "initial_r2t",
                    NULL, &node_iscsi_target_oper_immediate_data,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_target_address, 
                    "target_address",
                    NULL, &node_iscsi_target_oper_initial_r2t,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_initiator_alias, 
                    "initiator_alias",
                    NULL, &node_iscsi_target_oper_target_address,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_target_alias, 
                    "target_alias",
                    NULL, &node_iscsi_target_oper_initiator_alias,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_initiator_name, 
                    "initiator_name",
                    NULL, &node_iscsi_target_oper_target_alias,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_target_name, 
                    "target_name",
                    NULL, &node_iscsi_target_oper_initiator_name,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_send_targets, 
                    "send_targets",
                    NULL, &node_iscsi_target_oper_target_name,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_max_connections, 
                    "max_connections",
                    NULL, &node_iscsi_target_oper_send_targets,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_data_digest, 
                    "data_digest",
                    NULL, &node_iscsi_target_oper_max_connections,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_header_digest, 
                    "header_digest",
                    NULL, &node_iscsi_target_oper_data_digest,
                    iscsi_target_oper_get, iscsi_target_oper_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_tgt_verbose, "verbose", 
                    NULL, NULL, 
                    iscsi_tgt_verbose_get,
                    iscsi_tgt_verbose_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_tgt_backstore_mp, "backing_store_mp", 
                    NULL, &node_iscsi_tgt_verbose, 
                    iscsi_tgt_backstore_mp_get,
                    iscsi_tgt_backstore_mp_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_backing_store, "backing_store", 
                    NULL, &node_iscsi_tgt_backstore_mp, 
                    iscsi_target_backstore_get,
                    iscsi_target_backstore_set);

RCF_PCH_CFG_NODE_RO(node_iscsi_target_oper, "oper", 
                    &node_iscsi_target_oper_header_digest, 
                    &node_iscsi_target_backing_store, NULL);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_pn, "pn",
                    NULL, NULL,
                    iscsi_target_security_get,
                    iscsi_target_security_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_px, "px",
                    NULL, &node_iscsi_target_pn,
                    iscsi_target_security_get,
                    iscsi_target_security_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_t, "t",
                    &node_iscsi_target_px, NULL,
                    iscsi_target_security_flag_get,
                    iscsi_target_security_flag_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_b, "b",
                    NULL, &node_iscsi_target_t,
                    iscsi_target_security_flag_get,
                    iscsi_target_security_flag_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_cl, "cl",
                    NULL, &node_iscsi_target_b,
                    iscsi_target_security_get,
                    iscsi_target_security_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_ln, "ln",
                    NULL, &node_iscsi_target_cl,
                    iscsi_target_security_get,
                    iscsi_target_security_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_lx, "lx",
                    NULL, &node_iscsi_target_ln,
                    iscsi_target_security_get,
                    iscsi_target_security_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_chap, "chap", 
                    &node_iscsi_target_lx, &node_iscsi_target_oper, 
                    iscsi_target_chap_get,
                    iscsi_target_chap_set);

RCF_PCH_CFG_NODE_RO(node_ds_iscsi_target, "iscsi_target", 
                    &node_iscsi_target_chap,
                    NULL, iscsi_target_get);

te_errno
ta_unix_iscsi_target_init()
{
    return rcf_pch_add_node("/agent", &node_ds_iscsi_target);
}

#endif /* ! WITH_ISCSI */
