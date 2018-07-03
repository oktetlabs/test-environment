/** @file
 * @brief API to operate the time
 *
 * Functions to operate the date and time.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
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
#include "logger_api.h"


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

/* See description in te_time.h */
te_errno
te_gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if (gettimeofday(tv, tz) < 0)
    {
        te_errno err = te_rc_os2te(errno);

        ERROR("gettimeofday() failed with errno %r", err);
        return err;
    }

    return 0;
}

