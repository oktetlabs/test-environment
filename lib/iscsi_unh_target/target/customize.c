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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <te_defs.h>
#include <logger_api.h>
#include <logger_defs.h>

#include "mutex.h"
#include "my_memory.h"
#include "iscsi_custom.h"
#include "te_iscsi.h"

#define ISCSI_CUSTOM_MAX_PARAM 21

#define ISCSI_CUSTOM_MAGIC 0xeba1eba1

struct iscsi_custom_data
{
    int params[ISCSI_CUSTOM_MAX_PARAM];
    te_bool changed[ISCSI_CUSTOM_MAX_PARAM];
    pid_t pid;
    ipc_mutex_t mutex;
};

static iscsi_custom_data default_block = { pid: (pid_t)-1 };

SHARED iscsi_custom_data *
iscsi_alloc_custom(void)
{
    SHARED iscsi_custom_data *block = shalloc(sizeof(*block));

    if (block == NULL)
    {
        ERROR("%s: Not enough memory", __FUNCTION__);
        return NULL;
    }
    shmemset(block, 0, sizeof(*block));
    shmemcpy(block->params, default_block.params, sizeof(block->params));
    block->pid = (pid_t)-1;
    block->mutex = ipc_mutex_alloc();
    if (block->mutex < 0)
    {
        ERROR("%s(): Cannot alloc custom data mutex", __FUNCTION__);
        shfree(block);
        return NULL;
    }
    VERB("Allocated custom block %p", block);
    return block;
}

void
iscsi_bind_custom(SHARED iscsi_custom_data *block, pid_t pid)
{
    block->pid = pid;
}

void
iscsi_free_custom(SHARED iscsi_custom_data *block)
{
    VERB("Freeing custom block: %p", block);
    
    if (block == NULL)
        return;

    ipc_mutex_free(block->mutex);
    shfree(block);
}

typedef struct iscsi_custom_descr
{
    char    *name;
    te_bool  need_post;
    char   **enumeration;
} iscsi_custom_descr;

#define CUSTOM_ENUM_EXPLICIT(name, value) "=" name, (char *)value

static char *async_messages[] = {"scsi_async_event",
                                 "logout_request",
                                 "drop_connection",
                                 "drop_all_connections",
                                 "renegotiate",
                                 NULL};

static char *boolean_values[] = {"no", "yes", NULL};

static char *reject_reasons[] = {
    "none",
    "reserved1",
    "data_digest_error",
    "data_snack",
    "protocol_error",
    "command_not_supported",
    "too_many_immediate_commands",
    "task_in_progress",
    "invalid_snack",
    "invalid_pdu_field",
    "out_of_resources",
    "negotiation_reset",
    "waiting_for_logout",
    NULL,
};

/* The following are meant to match SCSI sense keys (see SPC-3, p.41) */
static char *senses[] = {
    "none", 
    "recovered_error",
    "not_ready",
    "medium_error",
    "hardware_error",
    "illegal_request",
    "unit_attention",
    "data_protect",
    "blank_check",
    "vendor_specific",
    "copy_aborted",
    "aborted_command",
    CUSTOM_ENUM_EXPLICIT("-", 0),
    NULL
};


static char *statuses[] = {
    CUSTOM_ENUM_EXPLICIT("good", SAM_STAT_GOOD),
    CUSTOM_ENUM_EXPLICIT("check_condition", 
                         SAM_STAT_CHECK_CONDITION),
    CUSTOM_ENUM_EXPLICIT("busy", SAM_STAT_BUSY),
    CUSTOM_ENUM_EXPLICIT("reservation_conflict", 
                         SAM_STAT_RESERVATION_CONFLICT),
    NULL
};

static char *asc_values[] = {
    CUSTOM_ENUM_EXPLICIT("protocol_service_crc_error", 0x4705),
    CUSTOM_ENUM_EXPLICIT("unexpected_unsolicited_data", 0x0c0c),
    CUSTOM_ENUM_EXPLICIT("not_enough_unsolicited_data", 0x0c0d),
    CUSTOM_ENUM_EXPLICIT("-", 0),
    NULL
};

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
    {"xfer_len", FALSE, NULL},
    {"zero_dsl_interval", FALSE, NULL},
    {"reject_reason", FALSE, reject_reasons},
    {"nopin_after", FALSE, NULL},
    {"nopin_count", FALSE, NULL},
    {"max_send_length", TRUE, NULL},
    {"force_status", TRUE, statuses},
    {"sense", FALSE, senses},
    {"asc_value", FALSE, asc_values},
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
        if (**iter == '=')
        {
            if (strcmp(*iter + 1, value) == 0)
                return (int)iter[1];
            iter++;
        }
        else if (strcmp(*iter, value) == 0)
        {
            return iter - param_descr[idx].enumeration;
        }
    }
    ERROR("Unrecognized value '%s' for parameter '%s'", 
          value, param_descr[idx].name);
    return 0;    
}

te_errno
iscsi_set_custom_value(SHARED iscsi_custom_data *block, const char *param, const char *value)
{
    te_bool            need_post;
    int                param_no = find_custom_param(param, &need_post);
    int                intvalue;
        
    if (param_no < 0)
    {
        ERROR("Parameter %s not found", param);
        return TE_RC(TE_ISCSI_TARGET, TE_ENOENT);
    }
        
    if (block == NULL)
        block = &default_block;

    intvalue = translate_custom_value(param_no, value);
    RING("Setting a custom value %s to %s (%d)", param, value,
         intvalue);

    ipc_mutex_lock(block->mutex);
    block->params[param_no] = intvalue;
    block->changed[param_no] = TRUE;
    ipc_mutex_unlock(block->mutex);
    if (need_post && block->pid != (pid_t)-1)
    {
        RING("Awakening the manager");
        if (kill(block->pid, SIGUSR1) != 0)
        {
            ERROR("The target process is dead");
            return TE_OS_RC(TE_ISCSI_TARGET, TE_ESRCH);
        }
    }
    return 0;
}

int
iscsi_get_custom_value(SHARED iscsi_custom_data *block,
                       const char *param)
{
    int value;
    int param_no = find_custom_param(param, NULL);

    if (param_no < 0)
    {
        return 0;
    }

    ipc_mutex_lock(block->mutex);
    value = block->params[param_no];
    block->changed[param_no] = FALSE;
    ipc_mutex_unlock(block->mutex);
    return value;
}

te_bool
iscsi_is_changed_custom_value(SHARED iscsi_custom_data *block,
                              const char *param)
{
    int param_no = find_custom_param(param, NULL);

    if (param_no < 0)
    {
        return FALSE;
    }

    return block->changed[param_no];
}

static sig_atomic_t custom_change_counter;

void
iscsi_custom_change_sighandler(int signo)
{
    UNUSED(signo);
    custom_change_counter++;
}

te_bool
iscsi_custom_pending_changes(void)
{
    if (custom_change_counter == 0)
        return FALSE;
    custom_change_counter--;
    return TRUE;
}
