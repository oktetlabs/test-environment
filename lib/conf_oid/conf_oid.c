/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * OID conversion routines
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Configurator OID API"

#include "te_config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "conf_oid.h"


/* See description in conf_oid.h */
cfg_oid *
cfg_allocate_oid(int length, bool inst)
{
    int size;
    cfg_oid * oid;

    if (length < 1)
    {
        return NULL;
    }

    size = (inst) ? sizeof(cfg_inst_subid) : sizeof(cfg_object_subid);
    oid = TE_ALLOC(sizeof(cfg_oid));

    oid->len = length;
    oid->inst = inst;
    oid->ids = TE_ALLOC(length * size);
    return oid;
}

/* See description in conf_oid.h */
cfg_oid *
cfg_convert_oid_str(const char *str)
{
    cfg_oid *oid;
    char     oid_buf[CFG_OID_MAX] = { 0, };
    char     str_buf[CFG_OID_MAX];
    char    *token;
    bool inst;
    int      depth = 1;
    int      size;

    if (str == NULL)
    {
        ERROR("%s: 'str' parameter can't be NULL", __FUNCTION__);
        return NULL;
    }
    if (*str != '/')
    {
        ERROR("%s: OID should be started with '/' symbol, not with '%c'",
              __FUNCTION__, *str);
        return NULL;
    }
    if (strlen(str) >= CFG_OID_MAX)
    {
        ERROR("%s: OID %s is too long, maximum allowed length is %d",
              __FUNCTION__, str, CFG_OID_MAX);
        return NULL;
    }

    inst = strchr(str, ':') != NULL;

    if (strcmp(str, "/") == 0 || strcmp(str, "/:") == 0)
        goto create;

    strcpy(str_buf, str + 1);

    for (token = strtok(str_buf, "/");
         token != NULL;
         token = strtok(NULL, "/"))
    {
        if (depth >= CFG_OID_LEN_MAX)
        {
            ERROR("%s(): '%s' has too many elements, consider increasing "
                  "CFG_OID_LEN_MAX", __FUNCTION__, str);
            return NULL;
        }

        if (inst)
        {
            char *name = strchr(token, ':');

            if (name == NULL)
            {
                ERROR("%s: Cannot find instance name in %s",
                      __FUNCTION__, token);
                return NULL;
            }
            if ((*name++ = '\0', strlen(name) >= CFG_INST_NAME_MAX))
            {
                ERROR("%s: Instance name '%s' is too long, "
                      "maximum allowed length of a single instance "
                      "name is %d", __FUNCTION__, name, CFG_INST_NAME_MAX);
                return NULL;
            }
            if (strlen(token) >= CFG_SUBID_MAX)
            {
                ERROR("%s: Sub ID name '%s' is too long, "
                      "maximum allowed length of an Sub ID name is %d",
                      __FUNCTION__, token, CFG_SUBID_MAX);
                return NULL;
            }
            strcpy(oid_buf + sizeof(cfg_inst_subid) * depth, token);
            strcpy(oid_buf + sizeof(cfg_inst_subid) * depth +
                   CFG_SUBID_MAX, name);
        }
        else
        {
            if (strlen(token) >= CFG_SUBID_MAX)
                return NULL;
            strcpy(oid_buf + sizeof(cfg_object_subid) * depth, token);
        }
        depth++;
    }

    create:
    if ((oid = cfg_allocate_oid(depth, inst)) == NULL)
        return NULL;

    size = inst ? sizeof(cfg_inst_subid) : sizeof(cfg_object_subid);

    memcpy(oid->ids, oid_buf, depth * size);

    return oid;
}

/* See description in conf_oid.h */
char *
cfg_convert_oid(const cfg_oid *oid)
{
    char *str;
    int   i;
    int   len = 0;

    if (oid == NULL)
        return NULL;

    str = TE_ALLOC(CFG_OID_MAX);

    if (oid->len == 1)
    {
        sprintf(str, "%s", oid->inst ? "/:" : "/");
        return str;
    }

    for (i = 1; i < oid->len; i++)
    {
        if (oid->inst)
        {
            len += snprintf(str + len, CFG_OID_MAX - len,
                            "/%s:%s",
                            ((cfg_inst_subid *)(oid->ids))[i].subid,
                            ((cfg_inst_subid *)(oid->ids))[i].name);
        }
        else
        {
            len += snprintf(str + len, CFG_OID_MAX - len,
                            "/%s",
                            ((cfg_object_subid *)(oid->ids))[i].subid);
        }

        if (len == CFG_OID_MAX)
        {
            ERROR("%s: resulting OID is too long", __FUNCTION__);
            free(str);
            return NULL;
        }
    }

    return str;
}

/* See description in conf_oid.h */
void
cfg_free_oid(cfg_oid *oid)
{
    if (oid == NULL)
        return;

    if (oid->ids != NULL)
        free(oid->ids);
    free(oid);
    return;
}


/* See description in conf_api.h */
int
cfg_oid_cmp(const cfg_oid *o1, const cfg_oid *o2)
{
    unsigned int i;

#ifdef HAVE_ASSERT_H
    assert(o1 != NULL);
    assert(o2 != NULL);
#endif

    if (o1->len != o2->len || o1->inst != o2->inst)
    {
        return 1;
    }
    if (o1->inst)
    {
        cfg_inst_subid *iids1 = (cfg_inst_subid *)(o1->ids);
        cfg_inst_subid *iids2 = (cfg_inst_subid *)(o2->ids);

        for (i = 0; i < o1->len; ++i)
            if ((strcmp(iids1[i].subid, iids2[i].subid) != 0) ||
                (strcmp(iids1[i].name,  iids2[i].name)  != 0))
            {
                return 1;
            }
    }
    else
    {
        cfg_object_subid   *oids1 = (cfg_object_subid *)(o1->ids);
        cfg_object_subid   *oids2 = (cfg_object_subid *)(o2->ids);

        for (i = 0; i < o1->len; ++i)
        {
            if (strcmp(oids1[i].subid, oids2[i].subid) != 0)
                return 1;
        }
    }

    return 0;
}

/* See description in conf_oid.h */
bool
cfg_oid_match(const cfg_oid *inst_oid, const cfg_oid *obj_oid,
              bool match_prefix)
{
    unsigned int i;
    const cfg_object_subid *obj_subids;
    const cfg_inst_subid *inst_subids;

    assert(inst_oid->inst);
    assert(!obj_oid->inst);

    obj_subids = obj_oid->ids;
    inst_subids = inst_oid->ids;

    if (inst_oid->len < obj_oid->len)
        return false;

    if (!match_prefix && (obj_oid->len != inst_oid->len))
        return false;

    for (i = 0; i < obj_oid->len; i++)
    {
        if (strcmp(inst_subids[i].subid, obj_subids[i].subid) != 0)
            return false;
    }

    return true;
}

/* See description in conf_oid.h */
te_errno
cfg_oid_dispatch(const cfg_oid_rule rules[], const char *inst_oid, void *ctx)
{
    unsigned int i;
    cfg_oid *parsed_inst_oid = cfg_convert_oid_str(inst_oid);

    if (parsed_inst_oid == NULL)
    {
        ERROR("Cannot parse '%s'", inst_oid);
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }
    if (!parsed_inst_oid->inst)
    {
        ERROR("'%s' is not an instance OID", inst_oid);
        cfg_free_oid(parsed_inst_oid);
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }

    for (i = 0; rules[i].object_oid != NULL; i++)
    {
        if (cfg_oid_match(parsed_inst_oid, rules[i].object_oid,
                          rules[i].match_prefix))
        {
            te_errno rc = rules[i].action(inst_oid, parsed_inst_oid, ctx);

            cfg_free_oid(parsed_inst_oid);
            return rc;
        }
    }

    ERROR("No matching rule found for '%s'", inst_oid);
    cfg_free_oid(parsed_inst_oid);

    return TE_RC(TE_CONF_API, TE_ESRCH);
}

/* See description in conf_oid.h */
cfg_oid *
cfg_oid_common_root(const cfg_oid *oid1, const cfg_oid *oid2)
{
    unsigned    common;
    unsigned    oidlen = (oid1->len > oid2->len ? oid2->len : oid1->len);
    cfg_oid    *result;

#define GET_SUBID(oid, index)  \
    (oid->inst ? ((cfg_inst_subid *)oid->ids)[index].subid : \
     ((cfg_object_subid *)oid->ids)[index].subid)

    for (common = 0; common < oidlen; common++)
    {
        if (strcmp(GET_SUBID(oid1, common), GET_SUBID(oid2, common)) != 0)
            break;
    }

    if (oid1->inst || oid2->inst)
    {
        unsigned i;
        cfg_inst_subid *subids;

        result = cfg_allocate_oid(oid1->len, true);
        subids = result->ids;
        for (i = 0; i < common; i++)
        {
            strcpy(subids[i].subid, GET_SUBID(oid1, i));
            strcpy(subids[i].name,
                   CFG_OID_GET_INST_NAME((oid1->inst ? oid1 : oid2), i));
        }
        for (; i < oid1->len; i++)
        {
            strcpy(subids[i].subid, GET_SUBID(oid1, i));
            strcpy(subids[i].name, "*");
        }
    }
    else
    {
        unsigned i;
        cfg_object_subid *subids;

        result = cfg_allocate_oid(oid1->len, true);
        subids = oid1->ids;
        for (i = 0; i < common; i++)
        {
            strcpy(subids[i].subid, GET_SUBID(oid1, i));
        }
    }
    return result;
#undef GET_SUBID
}

/* See the description in conf_oid.h */
char *
cfg_oid_get_inst_name(const cfg_oid *oid, int idx)
{
    const char *inst_name;
    char *copy;

    if (!oid->inst)
    {
        ERROR("%s(): oid is not an instance", __func__);
        return NULL;
    }

    if (idx >= (int)oid->len)
    {
        ERROR("%s(): too big instance %d is requested from OID with %u length",
              __func__, idx, oid->len);
        return NULL;
    }

    if (idx < 0 && -idx > (int)oid->len)
    {
        ERROR("%s(): too small instance %d is requested from OID with %u length",
              __func__, idx, oid->len);
        return NULL;
    }

    inst_name = idx >= 0 ? CFG_OID_GET_INST_NAME(oid, idx) :
                           CFG_OID_GET_INST_NAME(oid, (int)oid->len + idx);

    copy = strdup(inst_name);
    if (copy == NULL)
        ERROR("strdup(%s) failed", inst_name);

    return copy;
}

/* See the description in conf_oid.h */
char *
cfg_oid_str_get_inst_name(const char *oid_str, int idx)
{
    cfg_oid *oid;
    char *result;

    oid = cfg_convert_oid_str(oid_str);
    if (oid == NULL)
        return NULL;

    result = cfg_oid_get_inst_name(oid, idx);

    cfg_free_oid(oid);

    return result;
}
