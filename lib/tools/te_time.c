/** @file
 * @brief API to operate the time
 *
 * Functions to operate the date and time.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#include "te_config.h"
#include "te_defs.h"
#if HAVE_TIME_H
#include <time.h>
#endif
#include "te_time.h"


/* See description in te_time.h */
char *
te_time_current_date2str(void)
{
    char buf[2 + 1 + 2 + 1 + 4 + 1];    /* depends on format, see below */
    time_t rawtime;
    struct tm  *tm;

    time(&rawtime);
    tm = localtime(&rawtime);
    if (tm == NULL)
        return NULL;
    if (strftime(buf, sizeof(buf), "%d/%m/%Y", tm) == 0)
        return NULL;

    return strdup(buf);
}
