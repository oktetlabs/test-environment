/** @file
 * @brief Testing Results Comparator: common
 *
 * Routines to work with TRC tags sets.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_queue.h"
#include "logger_api.h"

#include "trc_tags.h"
#include "te_trc.h"


/**
 * Parse string with TRC tags and add them into the list.
 *
 * @param tags_str      String with tags
 * @param tags          List to add tags
 *
 * @return Status code.
 */
te_errno
trc_tags_str_to_list(tqh_strings *tags, char *tags_str)
{
    char *tag, *space;
    int   rc = 0;

    for (tag = tags_str + strspn(tags_str, " ");
         rc == 0 && tag != NULL && *tag != '\0';
         tag = (space == NULL) ? NULL : (space + strspn(space, " ")))
    {
        space = strchr(tag, ' ');
        if (space != NULL)
            *space++ = '\0';
        rc = trc_add_tag(tags, tag);
    }
    return rc;
}
