/** @file
 * @brief Test Environment: 
 *
 * UNH iSCSI target port
 * Extra target customization functions
 * 
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#include <te_config.h>

#define TE_LGR_USER "iSCSI Target"

#include <stdlib.h>
#include <string.h>

#include <te_defs.h>
#include <logger_api.h>
#include <logger_defs.h>

#include <pthread.h>
#include "iscsi_custom.h"

#define ISCSI_CUSTOM_MAX_PARAM 2

struct iscsi_custom_data
{
    int id;
    struct iscsi_custom_data *next, *prev;
    int params[ISCSI_CUSTOM_MAX_PARAM];
    te_bool changed[ISCSI_CUSTOM_MAX_PARAM];
};

static pthread_mutex_t custom_mutex = PTHREAD_MUTEX_INITIALIZER;
static iscsi_custom_data *custom_data_list;

iscsi_custom_data *
iscsi_register_custom(int id)
{
    iscsi_custom_data *block = malloc(sizeof(*block));

    if (block == NULL)
    {
        ERROR("%s: Not enough memory", __FUNCTION__);
        return NULL;
    }
    memset(block, 0, sizeof(*block));
    block->id = id;
    pthread_mutex_lock(&custom_mutex);
    block->next = custom_data_list;
    custom_data_list = block;
    pthread_mutex_unlock(&custom_mutex);
    return block;
}

void
iscsi_deregister_custom(iscsi_custom_data *block)
{
    if (block == NULL)
        return;

    pthread_mutex_lock(&custom_mutex);
    if (block->next != NULL)
        block->next->prev = block->prev;
    if (block->prev != NULL)
        block->prev->next = block->next;
    else
        custom_data_list = block->next;
    pthread_mutex_unlock(&custom_mutex);
    free(block);
}

static int
find_custom_param (const char *name)
{
    static char *name_list[ISCSI_CUSTOM_MAX_PARAM + 1] = 
        {
            "reject",
            "CHAP_I",
            NULL
        };
    char **iter;

    for (iter = name_list; *iter != NULL; iter++)
    {
        if (strcmp(*iter, name) == 0)
            return iter - name_list;
    }
    ERROR("Unknown iSCSI customization parameter: '%s'", name);
    return -1;
}

int
iscsi_set_custom_value(int id, const char *param, const char *value)
{
    iscsi_custom_data *block;
    int param_no = find_custom_param(param);
    int intvalue = strtol(value, NULL, 10);

    if (param_no < 0)
    {
        return -1;
    }

    pthread_mutex_lock(&custom_mutex);
    for (block = custom_data_list; block != NULL; block = block->next)
    {
        if (id < 0 || block->id == id)
        {
            block->params[param_no] = intvalue;
            block->changed[param_no] = TRUE;
            if (id >= 0)
                break;
        }
    }
    pthread_mutex_unlock(&custom_mutex);
    return (block == NULL);
}

int
iscsi_get_custom_value(iscsi_custom_data *block,
                       const char *param)
{
    int value;
    int param_no = find_custom_param(param);

    if (param_no < 0)
    {
        return 0;
    }

    pthread_mutex_lock(&custom_mutex);
    value = block->params[param_no];
    block->changed[param_no] = FALSE;
    pthread_mutex_unlock(&custom_mutex);
    return value;
}

te_bool
iscsi_is_changed_custom_value(iscsi_custom_data *block,
                              const char *param)
{
    te_bool flag;
    int param_no = find_custom_param(param);

    if (param_no < 0)
    {
        return FALSE;
    }

    pthread_mutex_lock(&custom_mutex);
    flag = block->changed[param_no];
    pthread_mutex_unlock(&custom_mutex);
    return flag;
}
