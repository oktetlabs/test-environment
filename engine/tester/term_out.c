/** @file
 * @brief Tester Subsystem
 *
 * Output to terminal routines
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define LGR_USER    "TermOut"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#include "internal.h"


/** Default terminal used by Tester, if TERM is unset or empty */
#define TESTER_TERM_DEF             "dumb"

/** Default number of columns on terminal */
#define TESTER_TERM_COLUMNS_DEF     80

#define CHECK_RC_ZERO(_x) \
    do {                            \
        int _rc = (_x);             \
                                    \
        if (_rc != 0)               \
        {                           \
            ERROR(#_x " failed");   \
        }                           \
    } while (0)


/** Colors */
typedef enum {
    TESTER_COLOR_BLACK      = 0,
    TESTER_COLOR_DARK_RED   = 1,
    TESTER_COLOR_LIGHT_RED  = 3,
    TESTER_COLOR_GREEN      = 2,
    TESTER_COLOR_DARK_BLUE  = 4,
    TESTER_COLOR_LIGHT_BLUE = 5,
    TESTER_COLOR_GRAY       = 6,
    TESTER_COLOR_WHITE      = 8,
} tester_color;


static int columns = 0;

#if HAVE_PTHREAD_H

static pthread_mutex_t win_lock = PTHREAD_MUTEX_INITIALIZER;

static test_id prev_id = -1;
static size_t  prev_len = 0;

#endif



/**
 * Return string by run item type.
 */
static const char *
run_item_type_to_string(run_item_type type)
{
    switch (type)
    {
        case RUN_ITEM_SCRIPT:
            return "test";
        case RUN_ITEM_PACKAGE:
            return "package";
        case RUN_ITEM_SESSION:
            return "session";
        default:
            return "(UNKNOWN)";
    }
}

/**
 * Colored output.
 */
static void
colored_verdict(tester_color color, const char *text)
{
    char buf[32];

    sprintf(buf, "tput setaf %d", color);
    system(buf);
    printf("%s", text);
    system("tput sgr0");
    printf("\n");
    fflush(stdout);
}

/* See description in internal.h */
void
tester_out_start(run_item_type type, const char *name,
                 test_id parent, test_id self, unsigned int flags)
{
    char ids[20] = "";
    char msg[256];

    if ((~flags & TESTER_CTX_VERBOSE) ||
        ((~flags & TESTER_CTX_VVERB) && (type == RUN_ITEM_SESSION)))
        return;

    if (flags & TESTER_CTX_VVERB)
    {
        if (snprintf(ids, sizeof(ids), " %d:%d", parent, self) >=
                (int)sizeof(ids))
        {
            ERROR("Too short buffer for pair of test IDs");
            /* Continue */
        }
    }

    if (snprintf(msg, sizeof(msg), "Starting%s %s %s",
                 ids, run_item_type_to_string(type), name) >=
            (int)sizeof(msg))
    {
        ERROR("Too short buffer for output message");
        /* Continue */
    }

#if HAVE_PTHREAD_H
    CHECK_RC_ZERO(pthread_mutex_lock(&win_lock));
#endif

    if (prev_id != -1)
    {
        printf("\n");
        fflush(stdout);
    }

    prev_id = self;
    prev_len = strlen(msg);

    if (write(1, msg, strlen(msg)) != (ssize_t)prev_len)
    {
        ERROR("Output to 'stdout' failed");
    }
    fflush(stdout);
    
#if HAVE_PTHREAD_H
    CHECK_RC_ZERO(pthread_mutex_unlock(&win_lock));
#endif
}

/* See description in internal.h */
void
tester_out_done(run_item_type type, const char *name,
               test_id parent, test_id self, unsigned int flags,
               int result)
{
    char        ids[20] = "";
    char        msg_out[256];
    char        msg_in[256];
    char       *msg;
    int         color;
    const char *verdict;

    if ((~flags & TESTER_CTX_VERBOSE) ||
        ((~flags & TESTER_CTX_VVERB) && (type == RUN_ITEM_SESSION)))
        return;

    /* It's done only once */
    if (columns == 0)
    {
        char       *term = getenv("TERM");
        const char *cols = getenv("COLUMNS");
        char       *end;
        
        if ((term == NULL) || (strlen(term) == 0))
            setenv("TERM", TESTER_TERM_DEF, TRUE);
        if (cols != NULL)
            columns = strtol(cols, &end, 10);
        if ((end == cols) || (columns == 0))
            columns = TESTER_TERM_COLUMNS_DEF;
    }

    if (flags & TESTER_CTX_VVERB)
    {
        if (snprintf(ids, sizeof(ids), " %d:%d", parent, self) >=
                (int)sizeof(ids))
        {
            ERROR("Too short buffer for pair of test IDs");
            /* Continue */
        }
    }

    /* Prepare message to be output in another line */
    if (snprintf(msg_out, sizeof(msg_out), "Done%s %s %s ",
                 ids, run_item_type_to_string(type), name) >=
            (int)sizeof(msg_out))
    {
        ERROR("Too short buffer for output message");
        /* Continue */
    }
    /* We have a chance to output in the same line, prepare offset string */
    if (self == prev_id)
    {
        int n_spaces = columns - 10 - prev_len;

        if (n_spaces < 1)
            n_spaces = 1;
        if ((size_t)n_spaces >= sizeof(msg_in))
        {
            ERROR("Too short buffer for output message");
            n_spaces = sizeof(msg_in) - 1;
        }
        memset(msg_in, ' ', n_spaces);
        msg_in[n_spaces] = '\0';
    }

    /* Map result to color and verdict */
    if (result == ETESTPASS)
    {
        verdict = "PASSED";
        color = TESTER_COLOR_GREEN;
    }
    else
    {
        switch (result)
        {
            case ETESTKILL:
                verdict = "KILLED";
                color = TESTER_COLOR_LIGHT_BLUE;
                break;

            case ETESTCORE:
                verdict = "CORED";
                color = TESTER_COLOR_LIGHT_BLUE;
                break;

            case ETESTSKIP:
                verdict = "SKIPPED";
                color = TESTER_COLOR_WHITE;
                break;

            case ETESTFAKE:
                verdict = "FAKED";
                color = TESTER_COLOR_GRAY;
                break;

            default:
            {
                verdict = "FAILED";
                switch (result)
                {
                    case ETESTFAIL:
                        color = TESTER_COLOR_DARK_RED;
                        break;

                    case ETESTCONF:
                        color = TESTER_COLOR_LIGHT_RED;
                        break;

                    case ETESTPROLOG:
                        color = TESTER_COLOR_LIGHT_RED;
                        break;

                    case ETESTEPILOG:
                        color = TESTER_COLOR_LIGHT_RED;
                        break;

                    case ETESTUNEXP:
                        color = TESTER_COLOR_DARK_BLUE;
                        break;

                    default:
                        color = TESTER_COLOR_DARK_BLUE;
                        break;
                }
                break;
            }
        }
    }

    /* Now lock the terminal (inside Tester application) and make output */
#if HAVE_PTHREAD_H
    CHECK_RC_ZERO(pthread_mutex_lock(&win_lock));
#endif

    msg = (self == prev_id) ? msg_in : msg_out;
    if (write(1, msg, strlen(msg)) != (ssize_t)strlen(msg))
    {
        ERROR("Write to 'stdout' failed");
    }
    colored_verdict(color, verdict);
    fflush(stdout);
    
    prev_id = -1;
    prev_len = 0;

#if HAVE_PTHREAD_H
    CHECK_RC_ZERO(pthread_mutex_unlock(&win_lock));
#endif
}
