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

#ifdef WITH_ISCSI
#include <stddef.h>
#include "conf_daemons.h"
#include "chap.h"
#include "target_negotiate.h"

extern struct iscsi_global *devdata;

#define CHAP_SET_SECRET(value, x) \
    (CHAP_SetSecret(value, x) == 1)
#define CHAP_SET_NAME(value, n) \
    (CHAP_SetName(value, n) == 1)
#define CHAP_GET_NAME(n) \
    CHAP_GetName(n)
#define CHAP_SET_CHALLENGE_LENGHT(len, lx) \
    (CHAP_SetChallengeLength(len, lx) == 1)
#define CHAP_SET_ENCODING_FMT(fmt, lx, px) \
    ((CHAP_SetNumberFormat(fmt, lx) == 1) && (CHAP_SetNumberFormat(fmt, px) == 1))


static int
iscsi_target_pn_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "Peer name");
    return 0;
}

static int
iscsi_target_pn_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    if (!CHAP_SET_NAME(value, 
                        devdata->auth_parameter.chap_peer_ctx))
    {    
        ERROR("%s, %d: Cannot set name",
              __FUNCTION__,
              __LINE__);
        return ENOMEM;
    }   
    return 0;
}

static int
iscsi_target_px_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "Peer secret");
    return 0;
}

static int
iscsi_target_px_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
 
    if (!CHAP_SET_SECRET(value, 
                         devdata->auth_parameter.chap_peer_ctx))
    {
        ERROR("%s, %d: Cannot set secret",
              __FUNCTION__,
              __LINE__);
        return ENOMEM;
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
    sprintf(value, "0");
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
 
    if (!(tgt_cfmt == 0 || tgt_cfmt == 1))
    {
        ERROR("%s, %d: Bad cfmt parameter provideded",
              __FUNCTION__,
              __LINE__);
        return EBADF;
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
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "0");
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

    assert(devdata != NULL);
    
    if (!(fmt == 0 || fmt == 1))
    {
        ERROR("%s, %d: Bad format parameter provided",
              __FUNCTION__,
              __LINE__);
        return EBADF;
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
        return EPERM;
        
    }
    return 0;
}

static int
iscsi_target_cl_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "256");
    return 0;
}

static int
iscsi_target_cl_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    assert(devdata);
    
    if (!CHAP_SET_CHALLENGE_LENGHT(strtol(value, NULL, 0), 
                                   devdata->auth_parameter.
                                       chap_local_ctx))
    {
        ERROR("%s, %d: Cannot set challenge length",
              __FUNCTION__,
              __LINE__);
        return EPERM;
    }    
    return 0;
}

static int
iscsi_target_ln_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "Local name");
    return 0;
}

static int
iscsi_target_ln_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (!CHAP_SET_NAME((char *)value, 
                        devdata->auth_parameter.chap_local_ctx))
    {    
        ERROR("%s, %d: Cannot set name",
              __FUNCTION__,
              __LINE__);
        return ENOMEM;
    }   
    return 0;
}

static int
iscsi_target_lx_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    sprintf(value, "Local secret");
    return 0;
}


static int
iscsi_target_lx_set(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (!CHAP_SET_SECRET(value, 
                         devdata->auth_parameter.chap_local_ctx))
    {
        ERROR("%s, %d: Cannot set secret",
              __FUNCTION__,
              __LINE__);
        return ENOMEM;
    }    
    return 0;
}

static int
iscsi_target_chap_set(unsigned int gid, const char *oid,
                      char *value, const char *instance, ...)
{
    int chap_use = strtol(value, NULL, 0);
    struct parameter_type *auth_p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (!(chap_use == 0 || chap_use == 1))
    {
        ERROR("%s, %d: Bad chap_use parameter provideded %d",
              __FUNCTION__,
              __LINE__, chap_use);
        return EBADF;
    }
    
    if ((auth_p = find_flag_parameter(AUTHMETHOD_FLAG, 
                                      devdata->param_tbl)) == NULL)
    {
        ERROR("%s, %d: Cannot find AuthMethod Parameter in param_table",
              __FUNCTION__,
              __LINE__);
        return EFAULT;
    }
    if (chap_use == 1)
    {    
        auth_p->value_list = "CHAP,None";
        auth_p->str_value = "CHAP";
    } 
    else
    {    
        auth_p->value_list = "NONE";
        auth_p->str_value = "None";
    }    
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
iscsi_target_get(unsigned int gid, const char *oid,
                 char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(instance);
    return 0;
}


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
                    &node_iscsi_target_lx, NULL, 
                    iscsi_target_chap_get,
                    iscsi_target_chap_set);

RCF_PCH_CFG_NODE_RO(node_ds_iscsi_target, "iscsi_target", 
                    &node_iscsi_target_chap,
                    NULL, iscsi_target_get);

int
ta_unix_iscsi_target_init(rcf_pch_cfg_object **last)
{
    DS_REGISTER(iscsi_target);
    
    return 0;
}

#endif /* ! WITH_ISCSI */
