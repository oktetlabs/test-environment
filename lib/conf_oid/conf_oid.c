/** @file
 * @brief Configurator
 *
 * OID conversion routines
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "conf_oid.h"

#define TE_LGR_USER     "Configurator OID API"
#include "logger_api.h"

/**
 * Allocate memory for object identifier or object instance identifier.
 *
 * @param length        number of identifier elements
 * @param inst          if TRUE - object instance identifier should be
 *                      allocated
 *
 * @return newly allocated structure pointer or NULL
 */
cfg_oid *
cfg_allocate_oid(int length, te_bool inst)
{
    int size;
    cfg_oid * oid;

    if (length < 1)
    {
        return NULL;
    }

    size = (inst) ? sizeof(cfg_inst_subid) : sizeof(cfg_object_subid);
    oid = (cfg_oid *)calloc(1, sizeof(cfg_oid));
    if (oid == NULL)
        return NULL;

    oid->len = length;
    oid->inst = inst;
    oid->ids = (void *)calloc(length, size);
    if (oid->ids == NULL)
    {
        free(oid);
        return NULL;
    }
    return oid;
}

/**
 * Convert object identifier or object instance identifier in
 * string representation to cfg_oid structure. We hope that OID string
 * representation and we do not check overflow and etc.
 *
 * @param str OID in string representation.
 *
 * @return newly allocated structure pointer or NULL
 */
cfg_oid *
cfg_convert_oid_str(const char *str)
{
    cfg_oid *oid;
    char     oid_buf[CFG_OID_MAX] = {0,};
    char     str_buf[CFG_OID_MAX];
    char    *token;
    te_bool  inst;
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

/**
 * Convert object identifier or object instance identifier in
 * structure representation to string (memory is allocated by the
 * routine using malloc()).
 *
 *  @param oid   OID in structure representation
 *
 * @return newly allocated string or NULL
 */
char *
cfg_convert_oid(const cfg_oid *oid)
{
    char *str = (char *)malloc(CFG_OID_MAX);
    char *tmp = str;
    int   i;

    if (str == NULL)
        return NULL;

    if (oid->len == 1)
    {
        sprintf(str, "%s", oid->inst ? "/:" : "/");
        return str;
    }

    for (i = 1; i < oid->len; tmp += strlen(tmp), i++)
        if (oid->inst)
        {
            sprintf(tmp, "/%s:%s", ((cfg_inst_subid *)(oid->ids))[i].subid,
                    ((cfg_inst_subid *)(oid->ids))[i].name);
        }
        else
            sprintf(tmp, "/%s", ((cfg_object_subid *)(oid->ids))[i].subid);

    return str;
}

/**
 * Free memory allocated for OID stucture.
 *
 * @param oid   oid structure
 */
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
            if ((strcmp(iids1->subid, iids2->subid) != 0) ||
                (strcmp(iids1->name,  iids2->name)  != 0))
            {
                return 1;
            }
    }
    else
    {
        cfg_object_subid   *oids1 = (cfg_object_subid *)(o1->ids);
        cfg_object_subid   *oids2 = (cfg_object_subid *)(o2->ids);

        for (i = 0; i < o1->len; ++i)
            if (strcmp(oids1->subid, oids2->subid) != 0)
                return 1;
    }

    return 0;
}
