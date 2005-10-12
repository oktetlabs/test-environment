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
 *
 * $Id$
 */

#define TE_LGR_USER "iSCSI Target Conf"

#ifdef WITH_ISCSI
#include <stddef.h>
#include "conf_daemons.h"
#include "chap.h"
#include "target_negotiate.h"

extern struct iscsi_global *devdata;
extern int iscsi_server_init();

#define CHAP_SET_SECRET(value, x) \
    (CHAP_SetSecret(value, x) == 1)
#define CHAP_SET_NAME(value, n) \
    (CHAP_SetName(value, n) == 1)
#define CHAP_GET_NAME(n) \
    CHAP_GetName(n)
#define CHAP_GET_SECRET(n) \
    CHAP_GetSecret(n)
#define CHAP_SET_CHALLENGE_LENGTH(len, lx) \
    (CHAP_SetChallengeLength(len, lx) == 1)
#define CHAP_GET_CHALLENGE_LENGTH(n) \
    CHAP_GetChallengeLength(n)
#define CHAP_SET_ENCODING_FMT(fmt, lx, px) \
    ((CHAP_SetNumberFormat(fmt, lx) == 1) && (CHAP_SetNumberFormat(fmt, px) == 1))


#define DEVDATA_GET_CHECK                               \
    if (devdata == NULL)                                \
    {                                                   \
        RING("devdata is NULL in %s", __FUNCTION__);    \
        *value = '\0';                                  \
        return 0;                                       \
    }

#define DEVDATA_SET_CHECK                               \
    if (devdata == NULL)                                \
    {                                                   \
        RING("devdata is NULL in %s", __FUNCTION__);    \
        return 0;                                       \
    }

static int
iscsi_target_pn_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    char *tmp;
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_GET_CHECK;
    tmp = CHAP_GET_NAME(devdata->auth_parameter.chap_peer_ctx);
    if (tmp == NULL)
        strcpy(value, "Peer name");
    else
    {
        strcpy(value, tmp);
        free(tmp);
    }
    return 0;
}

static int
iscsi_target_pn_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;

    if (!CHAP_SET_NAME(value, 
                        devdata->auth_parameter.chap_peer_ctx))
    {    
        ERROR("%s, %d: Cannot set name",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }   
    return 0;
}

static int
iscsi_target_px_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    char *tmp;
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_GET_CHECK;
    tmp = CHAP_GET_SECRET(devdata->auth_parameter.chap_peer_ctx);
    if (tmp == NULL)
        strcpy(value, "Peer secret");
    else
    {
        strcpy(value, tmp);
        free(tmp);
    }

    return 0;
}

static int
iscsi_target_px_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;
 
    if (!CHAP_SET_SECRET(value, 
                         devdata->auth_parameter.chap_peer_ctx))
    {
        ERROR("%s, %d: Cannot set secret",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }    
    return 0;
}

static int
iscsi_target_t_get(unsigned int gid, const char *oid,
                   char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);

    DEVDATA_GET_CHECK;

    *value = (devdata->auth_parameter.auth_flags & USE_TARGET_CONFIRMATION ? '1' : '0');
    value[1] = '\0';
    return 0;
}

static int
iscsi_target_t_set(unsigned int gid, const char *oid,
                   char *value, const char *instance, ...)
{
    int tgt_cfmt = strtol(value, NULL, 0);
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;
 
    if (!(tgt_cfmt == 0 || tgt_cfmt == 1))
    {
        ERROR("%s, %d: Bad cfmt parameter provideded",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_EBADF);
    }
    
    if (tgt_cfmt == 1)
        devdata->auth_parameter.auth_flags |= USE_TARGET_CONFIRMATION;
    else
        devdata->auth_parameter.auth_flags &= ~USE_TARGET_CONFIRMATION;
    return 0;
}

static int
iscsi_target_b_get(unsigned int gid, const char *oid,
                   char *value, const char *instance, ...)
{   
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_GET_CHECK;

    *value = (devdata->auth_parameter.auth_flags & USE_BASE64 ? '1' : '0');
    value[1] = '\0';

    return 0;
}

static int
iscsi_target_b_set(unsigned int gid, const char *oid,
                   char *value, const char *instance, ...)
{
    int fmt = strtol(value, NULL, 0);
    int base_fmt;
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;
    
    if (!(fmt == 0 || fmt == 1))
    {
        ERROR("%s, %d: Bad format parameter provided",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_EBADF);
    }
    if (fmt == 1)
    {    
        devdata->auth_parameter.auth_flags |= USE_BASE64;
        base_fmt = BASE64_FORMAT;
    }    
    else
    {
        devdata->auth_parameter.auth_flags &= ~USE_BASE64;
        base_fmt = HEX_FORMAT;
    }
    if (!CHAP_SET_ENCODING_FMT(base_fmt, 
                               devdata->auth_parameter.chap_local_ctx,
                               devdata->auth_parameter.chap_peer_ctx))
    {
        ERROR("%s, %d: Cannot set encoding format",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
        
    }
    return 0;
}

static int
iscsi_target_cl_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    int length;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    DEVDATA_GET_CHECK;

    length = CHAP_GET_CHALLENGE_LENGTH(devdata->auth_parameter.chap_local_ctx);
    sprintf(value, "%d", length);

    return 0;
}

static int
iscsi_target_cl_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    int len = strtol(value, NULL, 0);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;
    
    if (len == 0)
    {
        RING("Attempted to set challenge length to 0, ignored");
        return 0;
    }

    if (!CHAP_SET_CHALLENGE_LENGTH(len, 
                                   devdata->auth_parameter.
                                       chap_local_ctx))
    {
        ERROR("%s, %d: Cannot set challenge length",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }    
    return 0;
}

static int
iscsi_target_ln_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    char *tmp;
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);

    DEVDATA_GET_CHECK;
    tmp = CHAP_GET_NAME(devdata->auth_parameter.chap_local_ctx);
    if (tmp == NULL)
        strcpy(value, "Local name");
    else
    {
        strcpy(value, tmp);
        free(tmp);
    }
    return 0;
}

static int
iscsi_target_ln_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;

    if (!CHAP_SET_NAME((char *)value, 
                        devdata->auth_parameter.chap_local_ctx))
    {    
        ERROR("%s, %d: Cannot set name",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }   
    return 0;
}

static int
iscsi_target_lx_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    char *tmp;
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);

    DEVDATA_GET_CHECK;
    tmp = CHAP_GET_SECRET(devdata->auth_parameter.chap_local_ctx);
    if (tmp == NULL)
        strcpy(value, "Local secret");
    else
    {
        strcpy(value, tmp);
        free(tmp);
    }
    return 0;

}


static int
iscsi_target_lx_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;

    if (!CHAP_SET_SECRET(value, 
                         devdata->auth_parameter.chap_local_ctx))
    {
        ERROR("%s, %d: Cannot set secret",
              __FUNCTION__,
              __LINE__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }    
    return 0;
}

static int
iscsi_target_chap_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int chap_use = strtol(value, NULL, 0);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    DEVDATA_SET_CHECK;

    if (!(chap_use == 0 || chap_use == 1))
    {
        ERROR("%s, %d: Bad chap_use parameter provideded %d",
              __FUNCTION__,
              __LINE__, chap_use);
        return TE_RC(TE_TA_UNIX, TE_EBADF);
    }
    iscsi_configure_param_value(KEY_TO_BE_NEGOTIATED,
                                "AuthMethod",
                                chap_use ? "CHAP,None" : "None",
                                *devdata->param_tbl);
    return 0;
}   

static int
iscsi_target_chap_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "0");
    return 0;
}

static int
iscsi_target_oper_get(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    return 0;
}

static const char *
map_oid_to_param(const char *oid)
{
    static char param_name[32];
    te_bool upper_case = TRUE;
    char *p = param_name;
    
    for (; *oid != '\0'; oid++)
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

static int
iscsi_target_oper_set(unsigned int gid, const char *oid,
                      const char *value, const char *instance, ...)
{
    UNUSED(gid);
     UNUSED(instance);
    iscsi_configure_param_value(KEY_TO_BE_NEGOTIATED,
                                map_oid_to_param(oid),
                                value,
                                *devdata->param_tbl);
    return 0;
}


static int
iscsi_target_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(instance);
    iscsi_convert_param_to_str(value,
                               map_oid_to_param(oid),
                               *devdata->param_tbl);
    return 0;
}

RCF_PCH_CFG_NODE_RW(node_iscsi_target_oper_session_type, "session_type",
                    NULL, NULL,
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

RCF_PCH_CFG_NODE_RO(node_iscsi_target_oper, "oper", 
                    &node_iscsi_target_oper_header_digest, 
                    NULL, NULL);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_pn, "pn",
                    NULL, NULL,
                    iscsi_target_pn_get,
                    iscsi_target_pn_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_px, "px",
                    NULL, &node_iscsi_target_pn,
                    iscsi_target_px_get,
                    iscsi_target_px_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_t, "t",
                    &node_iscsi_target_px, NULL,
                    iscsi_target_t_get,
                    iscsi_target_t_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_b, "b",
                    NULL, &node_iscsi_target_t,
                    iscsi_target_b_get,
                    iscsi_target_b_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_cl, "cl",
                    NULL, &node_iscsi_target_b,
                    iscsi_target_cl_get,
                    iscsi_target_cl_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_ln, "ln",
                    NULL, &node_iscsi_target_cl,
                    iscsi_target_ln_get,
                    iscsi_target_ln_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_lx, "lx",
                    NULL, &node_iscsi_target_ln,
                    iscsi_target_lx_get,
                    iscsi_target_lx_set);

RCF_PCH_CFG_NODE_RW(node_iscsi_target_chap, "chap", 
                    &node_iscsi_target_lx, &node_iscsi_target_oper, 
                    iscsi_target_chap_get,
                    iscsi_target_chap_set);

RCF_PCH_CFG_NODE_RO(node_ds_iscsi_target, "iscsi_target", 
                    &node_iscsi_target_chap,
                    NULL, iscsi_target_get);

int
ta_unix_iscsi_target_init(rcf_pch_cfg_object **last)
{
    int rc;
        
    if ((rc = iscsi_server_init()) != 0)
    {
        ERROR("%s, %d: Cannot init iscsi server");
        return rc;
    }
    DS_REGISTER(iscsi_target);
    
    return 0;
}

#endif /* ! WITH_ISCSI */
