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

/** Logger user name to be used here */
#define TE_LGR_USER     "TermOut"

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
#ifdef HAVE_COLOR
#include <curses.h>
#include <term.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#include "internal.h"


/** Log, if the result is not zero. */
#define CHECK_RC_ZERO(_x) \
    do {                            \
        int _rc = (_x);             \
                                    \
        if (_rc != 0)               \
        {                           \
            ERROR(#_x " failed");   \
        }                           \
    } while (0)

/** "PASSED" verdict for non-color terminal or non-terminal */
#define TESTER_PASSED_VERDICT_NOCOLOR "passed"

#ifdef HAVE_COLOR
/** Special value for term when it can't be initialised. */
#define TESTER_TERM_UNKNOWN             ((char *)(-1))

/** Type of terminal used. To be initialized once. */
static char *term = NULL;

/** Passed verdict */
static const char *tester_verdict_passed = "PASSED";
#else
/** Passed verdict */
static const char *tester_verdict_passed = 
    TESTER_PASSED_VERDICT_NOCOLOR;
#endif

/** Number of columns on terminal. 80 for hosts without curses. */
static int cols = 80;


#if HAVE_PTHREAD_H

/** Lock to mutual exclude access to terminal in multi-thread Tester */
static pthread_mutex_t win_lock = PTHREAD_MUTEX_INITIALIZER;

/** ID of the previous test */
static test_id prev_id = -1;
/** Length of the line output for previous ID */
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
colored_verdict(int color, const char *text)
{
#ifdef HAVE_COLOR
    if (term == TESTER_TERM_UNKNOWN)
    {
        printf("%s\n", text);
        fflush(stdout);
    }
    else
    {
        putp(tparm(tigetstr("setaf"), color));
        printf("%s", text);
        putp(tparm(tigetstr("sgr0")));
        printf("\n");
    }
#else
    UNUSED(color);
    printf("%s\n", text);
    fflush(stdout);
#endif
}


/* See description in internal.h */
void
tester_out_start(run_item_type type, const char *name,
                 test_id parent, test_id self, unsigned int flags)
{
    char ids[20] = "";
    char msg[256];

    if ((~flags & TESTER_VERBOSE) ||
        ((~flags & TESTER_VVERB) && (type == RUN_ITEM_SESSION)))
        return;

    if (flags & TESTER_VVERB)
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
    int         color = 0; /* To avoid varnings for non-color build */
    const char *verdict;

    if ((~flags & TESTER_VERBOSE) ||
        ((~flags & TESTER_VVERB) && (type == RUN_ITEM_SESSION)))
        return;

    fflush(stdout);
#ifdef HAVE_COLOR
    /* It's done only once */
    if (term == NULL)
    {
        int rc = 0;

        term = getenv("TERM");
        if ((term == NULL) || (strlen(term) == 0) || 
            !isatty(STDOUT_FILENO) || 
            setupterm(term, STDOUT_FILENO, &rc) != OK)
        {
            char *c = getenv("COLUMNS");

            term = TESTER_TERM_UNKNOWN;
            tester_verdict_passed = TESTER_PASSED_VERDICT_NOCOLOR;
            if (c != NULL)
                cols = atoi(c);
            if (rc != 0) /* Warn only if setupterm failed. */
                fprintf(stderr, "Failed to initialize terminal %s", term);
        } else
            cols = columns;
    }
#else
    {
        char *c = getenv("COLUMNS");

        if (c != NULL)
            cols = atoi(c);
    }
#endif

    if (flags & TESTER_VVERB)
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
        int n_spaces = cols - 10 - prev_len;

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
    if (TE_RC_GET_ERROR(result) == TE_ETESTPASS)
    {
        verdict = tester_verdict_passed;
#ifdef HAVE_COLOR
        color = COLOR_GREEN;
#endif
    }
    else
    {
        switch (result)
        {
            case TE_ETESTKILL:
                verdict = "KILLED";
#ifdef HAVE_COLOR
                color = COLOR_MAGENTA;
#endif
                break;

            case TE_ETESTCORE:
                verdict = "CORED";
#ifdef HAVE_COLOR
                color = COLOR_MAGENTA;
#endif
                break;

            case TE_ETESTSKIP:
                verdict = "SKIPPED";
#ifdef HAVE_COLOR
                color = A_STANDOUT;
#endif
                break;

            case TE_ETESTFAKE:
                verdict = "FAKED";
#ifdef HAVE_COLOR
                color = COLOR_CYAN;
#endif
                break;

            case TE_ETESTEMPTY:
                verdict = "EMPTY";
#ifdef HAVE_COLOR
                color = COLOR_CYAN;
#endif
                break;

            default:
            {
                verdict = "FAILED";
#ifdef HAVE_COLOR
                switch (result)
                {
                    case TE_ETESTFAIL:
                        color = COLOR_RED;
                        break;

                    case TE_ETESTCONF:
                        color = COLOR_YELLOW;
                        break;

                    case TE_ETESTPROLOG:
                        color = COLOR_YELLOW;
                        break;

                    case TE_ETESTEPILOG:
                        color = COLOR_YELLOW;
                        break;

                    case TE_ETESTUNEXP:
                        color = COLOR_BLUE;
                        break;

                    default:
                        color = COLOR_BLUE;
                        break;
                }
#endif
                break;
            }
        }
    }

    /* Now lock the terminal (inside Tester application) and make output */
#if HAVE_PTHREAD_H
    CHECK_RC_ZERO(pthread_mutex_lock(&win_lock));
#endif

    msg = (self == prev_id) ? msg_in : msg_out;
    if (write(STDOUT_FILENO, msg, strlen(msg)) != (ssize_t)strlen(msg))
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
