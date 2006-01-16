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
#include <semaphore.h>

#include <te_defs.h>
#include <logger_api.h>
#include <logger_defs.h>

#include <pthread.h>
#include "iscsi_custom.h"

#define ISCSI_CUSTOM_MAX_PARAM 12

struct iscsi_custom_data
{
    int id;
    struct iscsi_custom_data *next, *prev;
    int params[ISCSI_CUSTOM_MAX_PARAM];
    te_bool changed[ISCSI_CUSTOM_MAX_PARAM];
    sem_t on_change;
};

static pthread_mutex_t custom_mutex = PTHREAD_MUTEX_INITIALIZER;

static iscsi_custom_data default_block = { id: ISCSI_CUSTOM_DEFAULT };
static iscsi_custom_data *custom_data_list = &default_block;



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
    memcpy(block->params, default_block.params, sizeof(block->params));
    sem_init(&block->on_change, 0, 0);
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

typedef struct iscsi_custom_descr
{
    char    *name;
    te_bool  need_post;
    char   **enumeration;
} iscsi_custom_descr;


static char *async_messages[] = {"scsi_async_event",
                                 "logout_request",
                                 "drop_connection",
                                 "drop_all_connections",
                                 "renegotiate",
                                 NULL};

static char *boolean_values[] = {"no", "yes", NULL};

static iscsi_custom_descr param_descr[ISCSI_CUSTOM_MAX_PARAM + 1] = 
{
    {"reject", FALSE, NULL},
    {"CHAP_I", FALSE, NULL},
    {"send_async", TRUE, async_messages},
    {"async_logout_timeout", FALSE, NULL},
    {"async_drop_time2wait", FALSE, NULL},
    {"async_drop_time2retain", FALSE, NULL},
    {"async_vcode", FALSE, NULL},
    {"async_text_timeout", FALSE, NULL},
    {"disable_t_bit", FALSE, boolean_values},
    {"split_pdu_at", FALSE, NULL},
    {"max_cmd_sn_delta", FALSE, NULL},
    {"send_nopin", TRUE, boolean_values},
    {NULL, FALSE, NULL},
};
    

static int
find_custom_param (const char *name, te_bool *need_post)
{
    iscsi_custom_descr *iter;

    for (iter = param_descr; iter->name != NULL; iter++)
    {
        if (strcmp(iter->name, name) == 0)
        {
            if (need_post != NULL)
                *need_post = iter->need_post;
            return iter - param_descr;
        }
    }
    ERROR("Unknown iSCSI customization parameter: '%s'", name);
    return -1;
}


static int
translate_custom_value(int idx, const char *value)
{
    char  *endptr;
    int    intval;
    char **iter;
    
    intval = strtol(value, &endptr, 0);
    if (*endptr == '\0')
        return intval;
    if (param_descr[idx].enumeration == NULL)
    {
        ERROR("Non-integer value '%s' for parameter '%s'", 
              value, param_descr[idx].name);
        return 0;
    }
    for (iter = param_descr[idx].enumeration; *iter != NULL; iter++)
    {
        if (strcmp(*iter, value) == 0)
        {
            return iter - param_descr[idx].enumeration;
        }
    }
    ERROR("Unrecognized value '%s' for parameter '%s'", 
          value, param_descr[idx].name);
    return 0;    
}

int
iscsi_set_custom_value(int id, const char *param, const char *value)
{
    iscsi_custom_data *block;
    te_bool            need_post;
    int                param_no = find_custom_param(param, &need_post);
    int                intvalue;
    te_bool            has_set = FALSE;
        
    if (param_no < 0)
    {
        ERROR("Parameter %s not found", param);
        return -1;
    }
        
    intvalue = translate_custom_value(param_no, value);
    RING("Setting a custom value %s to %s (%d) for %d", param, value,
         intvalue, id);
    pthread_mutex_lock(&custom_mutex);
    for (block = custom_data_list; block != NULL; block = block->next)
    {
        if (id < 0 || block->id == id)
        {
            block->params[param_no] = intvalue;
            block->changed[param_no] = TRUE;
            if (need_post && block->id != ISCSI_CUSTOM_DEFAULT)
            {
                RING("Awakening the manager");
                sem_post(&block->on_change);
            }
            has_set = TRUE;
            if (id >= 0)
                break;
        }
    }
    pthread_mutex_unlock(&custom_mutex);
    return !has_set;
}

int
iscsi_get_custom_value(iscsi_custom_data *block,
                       const char *param)
{
    int value;
    int param_no = find_custom_param(param, NULL);

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
    int param_no = find_custom_param(param, NULL);

    if (param_no < 0)
    {
        return FALSE;
    }

    pthread_mutex_lock(&custom_mutex);
    flag = block->changed[param_no];
    pthread_mutex_unlock(&custom_mutex);
    return flag;
}

int
iscsi_custom_wait_change(iscsi_custom_data *block)
{
    int i;
    int count;

    VERB("Waiting for a custom change");
    sem_wait(&block->on_change);
    VERB("semaphore fired");
    pthread_mutex_lock(&custom_mutex);
    for (i = 0, count = 0; i < ISCSI_CUSTOM_MAX_PARAM; i++)
    {
        if(block->changed[i])
            count++;
    }
    pthread_mutex_unlock(&custom_mutex);
    VERB("Has %d changed values", count);
    return count;
}
