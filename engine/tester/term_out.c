/** @file
 * @brief Tester Subsystem
 *
 * Output to terminal routines
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

/** Logger user name to be used here */
#define TE_LGR_USER     "TermOut"

#include "te_config.h"
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
#include "te_errno.h"
#include "logger_api.h"

#include "tester_flags.h"
#include "tester_result.h"
#include "tester_term.h"


/** Log, if the status is not zero. */
#define CHECK_RC_ZERO(_x) \
    do {                            \
        int _rc = (_x);             \
                                    \
        if (_rc != 0)               \
        {                           \
            ERROR(#_x " failed");   \
        }                           \
    } while (0)


#ifdef HAVE_COLOR

/** Special value for term when it can't be initialised. */
#define TESTER_TERM_UNKNOWN             ((char *)(-1))

/** Type of terminal used. To be initialized once. */
static char *term = NULL;

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
 * How to output colored verdict on terminal?
 */
typedef struct colored_verdict_data {
    int         color;
    const char *text;
    const char *no_color_text;
} colored_verdict_data;

/**
 * Types of situations when verdict is generated.
 */
typedef enum colored_verdict_type {
    COLORED_VERDICT_NO_TRC = 0,
    COLORED_VERDICT_TRC_UNKNOWN,
    COLORED_VERDICT_TRC_UNEXP,
    COLORED_VERDICT_TRC_EXP,
    COLORED_VERDICT_TRC_EXP_VERB,
    COLORED_VERDICT_MAX
} colored_verdict_type;

/** Colored verdict when TRC is not used? */
static const colored_verdict_data colored_verdicts[TESTER_TEST_STATUS_MAX]
                                                  [COLORED_VERDICT_MAX] = {

    { { COLOR_CYAN,             "INCOMPLETE",   "INCOMPLETE" },
      { COLOR_CYAN,             "INCOMPLETE?",  "INCOMPLETE?" },
      { COLOR_CYAN,             "INCOMPLETE",   "INCOMPLETE" },
      { COLOR_CYAN,             "INCOMPLETE",   "INCOMPLETE" },
      { COLOR_CYAN,             "INCOMPLETE",   "INCOMPLETE" } },

    { { COLOR_CYAN,             "EMPTY",        "empty" },
      { COLOR_CYAN,             "EMPTY?",       "empty?" },
      { COLOR_CYAN,             "EMPTY",        "empty" },
      { COLOR_CYAN,             "EMPTY",        "empty" },
      { COLOR_CYAN,             "EMPTY",        "empty" } },

    { { A_STANDOUT,             "SKIPPED",      "SKIPPED" },
      { A_STANDOUT,             "SKIPPED?",     "SKIPPED?" },
      { A_STANDOUT,             "SKIPPED",      "SKIPPED" },
      { A_STANDOUT,             "OK",           "OK" },
      { A_STANDOUT,             "OK skipped",   "OK skipped" } },

    { { COLOR_CYAN,             "FAKED",        "faked" },
      { COLOR_CYAN,             "FAKED?",       "faked?" },
      { COLOR_CYAN,             "FAKED",        "faked" },
      { COLOR_CYAN,             "FAKED",        "faked" },
      { COLOR_CYAN,             "FAKED",        "faked" } },

    { { COLOR_GREEN,            "PASSED",       "passed" },
      { COLOR_RED,              "PASSED?",      "passed?" },
      { COLOR_RED,              "PASSED",       "passed" },
      { COLOR_GREEN,            "OK",           "OK" },
      { COLOR_GREEN,            "OK passed",    "OK passed" } },

    { { COLOR_RED,              "FAILED",       "FAILED" },
      { COLOR_RED,              "FAILED?",      "FAILED?" },
      { COLOR_RED,              "FAILED",       "FAILED" },
      { COLOR_GREEN,            "OK",           "OK" },
      { COLOR_GREEN,            "OK failed",    "OK failed" } },

    { { COLOR_YELLOW,           "NOT FOUND",    "NOT FOUND" },
      { COLOR_YELLOW,           "NOT FOUND?",   "NOT FOUND?" },
      { COLOR_YELLOW,           "NOT FOUND",    "NOT FOUND" },
      { COLOR_YELLOW,           "NOT FOUND",    "NOT FOUND" },
      { COLOR_YELLOW,           "NOT FOUND",    "NOT FOUND" } },

    /* Unexpected configuration changes */
    { { COLOR_YELLOW,           "DIRTY",        "DIRTY" },
      { COLOR_YELLOW,           "DIRTY?",       "DIRTY?" },
      { COLOR_YELLOW,           "DIRTY",        "DIRTY" },
      { COLOR_YELLOW,           "DIRTY",        "DIRTY" },
      { COLOR_YELLOW,           "DIRTY",        "DIRTY" } },

    { { COLOR_MAGENTA,          "KILLED",       "KILLED" },
      { COLOR_MAGENTA,          "KILLED?",      "KILLED?" },
      { COLOR_MAGENTA,          "KILLED",       "KILLED" },
      { COLOR_MAGENTA,          "KILLED",       "KILLED" },
      { COLOR_MAGENTA,          "KILLED",       "KILLED" } },

    { { COLOR_MAGENTA,          "CORED",        "CORED" },
      { COLOR_MAGENTA,          "CORED?",       "CORED?" },
      { COLOR_MAGENTA,          "CORED",        "CORED" },
      { COLOR_MAGENTA,          "CORED",        "CORED" },
      { COLOR_MAGENTA,          "CORED",        "CORED" } },

    /* Prologue failed */
    { { COLOR_YELLOW,           "FAILED",       "FAILED" },
      { COLOR_YELLOW,           "FAILED?",      "FAILED?" },
      { COLOR_YELLOW,           "FAILED",       "FAILED" },
      { COLOR_YELLOW,           "FAILED",       "FAILED" },
      { COLOR_YELLOW,           "FAILED",       "FAILED" } },

    /* Epilogue failed */
    { { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED?",      "FAILED?" },
      { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED",       "FAILED" } },

    /* Keep-alive validation handler failed */
    { { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED?",      "FAILED?" },
      { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED",       "FAILED" } },

    /* Exception handler failed */
    { { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED?",      "FAILED?" },
      { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED",       "FAILED" },
      { COLOR_BLUE,             "FAILED",       "FAILED" } },

    { { COLOR_BLUE,             "STOPPED",       "STOPPED" },
      { COLOR_BLUE,             "STOPPED?",      "STOPPED?" },
      { COLOR_BLUE,             "STOPPED",       "STOPPED" },
      { COLOR_BLUE,             "STOPPED",       "STOPPED" },
      { COLOR_BLUE,             "STOPPED",       "STOPPED" } },

    { { COLOR_RED | A_BLINK,    "ERROR",        "ERROR" },
      { COLOR_RED | A_BLINK,    "ERROR?",       "ERROR?" },
      { COLOR_RED | A_BLINK,    "ERROR",        "ERROR" },
      { COLOR_RED | A_BLINK,    "ERROR",        "ERROR" },
      { COLOR_RED | A_BLINK,    "ERROR",        "ERROR" } }
};

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
colored_verdict(const colored_verdict_data *what)
{
#ifdef HAVE_COLOR
    if (term == TESTER_TERM_UNKNOWN)
    {
        fputs(what->no_color_text, stdout);
        fputc('\n', stdout);
    }
    else
    {
        putp(tparm(tigetstr("setaf"), what->color));
        fputs(what->text, stdout);
        putp(tparm(tigetstr("sgr0")));
        fputc('\n', stdout);
    }
#else
    UNUSED(color);
    fputs(what->no_color_text, stdout);
    fputc('\n', stdout);
#endif
}


/* See description in tester_term.h */
void
tester_term_out_start(unsigned int flags, run_item_type type,
                      const char *name, test_id parent, test_id self)
{
    char ids[20] = "";
    char msg[256];

    if ((~flags & TESTER_VERBOSE) ||
        ((~flags & TESTER_VVVVERB) && (type == RUN_ITEM_SESSION)))
        return;

    if (flags & TESTER_VVVVERB)
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

/* See description in tester_term.h */
void
tester_term_out_done(unsigned int flags,
                     run_item_type type, const char *name,
                     test_id parent, test_id self,
                     tester_test_status status, trc_verdict trcv)
{
    char                    ids[20] = "";
    char                    msg_out[256];
    char                    msg_in[256];
    char                   *msg;
    colored_verdict_type    cvt;

    assert(status < TESTER_TEST_STATUS_MAX);

    if ((~flags & TESTER_VERBOSE) ||
        ((~flags & TESTER_VVVVERB) && (type == RUN_ITEM_SESSION)))
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
            if (c != NULL)
                cols = atoi(c);
            if (rc != 0) /* Warn only if setupterm failed. */
                fprintf(stderr, "Failed to initialize terminal %s\n", term);
        } 
        else
            cols = columns;
    }
#else
    {
        char *c = getenv("COLUMNS");

        if (c != NULL)
            cols = atoi(c);
    }
#endif

    if (flags & TESTER_VVVVERB)
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
        int n_spaces = cols - 11 - prev_len;

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

    if (flags & TESTER_NO_TRC)
    {
        cvt = COLORED_VERDICT_NO_TRC;
    }
    else
    {
        switch (trcv)
        {
            case TRC_VERDICT_UNKNOWN:
                cvt = COLORED_VERDICT_TRC_UNKNOWN;
                break;

            case TRC_VERDICT_UNEXPECTED:
                cvt = COLORED_VERDICT_TRC_UNEXP;
                break;

            case TRC_VERDICT_EXPECTED:
                cvt = (flags & TESTER_VVERB) ?
                           COLORED_VERDICT_TRC_EXP_VERB :
                           COLORED_VERDICT_TRC_EXP;
                break;

            default:
                assert(FALSE);
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
    colored_verdict(&colored_verdicts[status][cvt]);
    fflush(stdout);
    
    prev_id = -1;
    prev_len = 0;

#if HAVE_PTHREAD_H
    CHECK_RC_ZERO(pthread_mutex_unlock(&win_lock));
#endif
}

/* See description in tester_term.h */
int
tester_term_cleanup(void)
{
    if (cur_term != NULL)
        return (del_curterm(cur_term) == OK) ? 0 : -1;
    return 0;
}
