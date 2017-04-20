/** @file
 * @brief API to operate the time
 *
 * Functions to operate the date and time.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
