/** @file
 * @brief Test Environment
 *
 * Test rebootability of TA
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Elena A. Vengerova <Mamadou.Ngom@oktetlabs.ru>
 * 
 * $Id$
 */
 
#include "config.h"

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

int
main()
{
    char ta_list[40];
    char *agentA;
    char *agentB;
    const char *delim = "\0";
    int retval;
    size_t list_len;
    
    te_log_init("tareboot", ten_log_message);

    list_len = sizeof(ta_list);
    retval = rcf_get_ta_list(ta_list, &list_len);
    if (retval != 0)
    {
        ERROR("Cannot get TA list.");
        exit(1);
    }
    
    if (list_len == 0 || list_len <= strlen(ta_list) + 1)
    {
        ERROR("Cannot get TA names");
        exit(1);
    }
    agentA = ta_list;
    agentB = ta_list + strlen(agentA) + 1;
    
    retval = rcf_ta_reboot(agentA, NULL, NULL);
    if (retval != 0)
    {
        if (TE_EPERM == TE_RC_GET_ERROR(retval))
            ERROR("/*****TA %s is not rebootable.********/", agentA);
        else
            ERROR("/****rcf_ta_reboot failed. errno=%r ****/",
                   TE_RC_GET_ERROR(retval)); 
    }
    else
        ERROR("/********TA %s rebooted *******/", agentA);
    
    retval = rcf_ta_reboot(agentB, NULL, NULL);

    if (retval != 0)
    {
        if (TE_EPERM == TE_RC_GET_ERROR(retval))
            ERROR("/*****TA %s is not rebootable.********/", agentB);
        else
            ERROR("rcf_ta_reboot failed. errno=%r",
                   TE_RC_GET_ERROR(retval)); 
    }
    else
        ERROR("/********TA %s rebooted *******/", agentB);    
    return 0;
}
