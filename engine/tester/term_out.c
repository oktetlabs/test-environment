/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Output to terminal routines
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
    int         attrs;
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

/**
 * Colored verdict (testing status) in different situations.
 *
 * See colored_verdict(); note that currently only A_BOLD text attribute
 * is supported there.
 */
static const colored_verdict_data colored_verdicts[TESTER_TEST_STATUS_MAX]
                                                  [COLORED_VERDICT_MAX] = {

    { { COLOR_CYAN,      0,         "INCOMPLETE",   "INCOMPLETE" },
      { COLOR_CYAN,      0,         "INCOMPLETE?",  "INCOMPLETE?" },
      { COLOR_CYAN,      0,         "INCOMPLETE",   "INCOMPLETE" },
      { COLOR_CYAN,      0,         "INCOMPLETE",   "INCOMPLETE" },
      { COLOR_CYAN,      0,         "INCOMPLETE",   "INCOMPLETE" } },

    { { COLOR_CYAN,      0,         "EMPTY",        "empty" },
      { COLOR_CYAN,      0,         "EMPTY?",       "empty?" },
      { COLOR_CYAN,      0,         "EMPTY",        "empty" },
      { COLOR_CYAN,      0,         "EMPTY",        "empty" },
      { COLOR_CYAN,      0,         "EMPTY",        "empty" } },

    { { A_STANDOUT,      0,         "SKIPPED",      "SKIPPED" },
      { A_STANDOUT,      0,         "SKIPPED?",     "SKIPPED?" },
      { A_STANDOUT,      0,         "SKIPPED",      "SKIPPED" },
      { A_STANDOUT,      0,         "skip",         "skip" },
      { A_STANDOUT,      0,         "skip",         "skip" } },

    { { COLOR_CYAN,      0,         "FAKED",        "faked" },
      { COLOR_CYAN,      0,         "FAKED?",       "faked?" },
      { COLOR_CYAN,      0,         "FAKED",        "faked" },
      { COLOR_CYAN,      0,         "FAKED",        "faked" },
      { COLOR_CYAN,      0,         "FAKED",        "faked" } },

    { { COLOR_GREEN,     0,         "PASSED",       "PASSED" },
      { COLOR_RED,       0,         "PASSED?",      "PASSED?" },
      { COLOR_RED,       0,         "PASSED",       "PASSED" },
      { COLOR_GREEN,     0,         "pass",         "pass" },
      { COLOR_GREEN,     0,         "pass",         "pass" } },

    { { COLOR_RED,       0,         "FAILED",       "FAILED" },
      { COLOR_RED,       0,         "FAILED?",      "FAILED?" },
      { COLOR_RED,       0,         "FAILED",       "FAILED" },
      { COLOR_GREEN,     0,         "fail",         "fail" },
      { COLOR_GREEN,     0,         "fail",         "fail" } },

    { { COLOR_YELLOW,    0,         "NOT FOUND",    "NOT FOUND" },
      { COLOR_YELLOW,    0,         "NOT FOUND?",   "NOT FOUND?" },
      { COLOR_YELLOW,    0,         "NOT FOUND",    "NOT FOUND" },
      { COLOR_YELLOW,    0,         "NOT FOUND",    "NOT FOUND" },
      { COLOR_YELLOW,    0,         "NOT FOUND",    "NOT FOUND" } },

    /* Unexpected configuration changes */
    { { COLOR_YELLOW,    0,         "DIRTY",        "DIRTY" },
      { COLOR_YELLOW,    0,         "DIRTY?",       "DIRTY?" },
      { COLOR_YELLOW,    0,         "DIRTY",        "DIRTY" },
      { COLOR_YELLOW,    0,         "DIRTY",        "DIRTY" },
      { COLOR_YELLOW,    0,         "DIRTY",        "DIRTY" } },

    { { COLOR_MAGENTA,   0,         "KILLED",       "KILLED" },
      { COLOR_MAGENTA,   0,         "KILLED?",      "KILLED?" },
      { COLOR_MAGENTA,   0,         "KILLED",       "KILLED" },
      { COLOR_MAGENTA,   0,         "KILLED",       "KILLED" },
      { COLOR_MAGENTA,   0,         "KILLED",       "KILLED" } },

    { { COLOR_MAGENTA,   0,         "CORED",        "CORED" },
      { COLOR_MAGENTA,   0,         "CORED?",       "CORED?" },
      { COLOR_MAGENTA,   0,         "CORED",        "CORED" },
      { COLOR_MAGENTA,   0,         "CORED",        "CORED" },
      { COLOR_MAGENTA,   0,         "CORED",        "CORED" } },

    /* Prologue failed */
    { { COLOR_YELLOW,    0,         "FAILED",       "FAILED" },
      { COLOR_YELLOW,    0,         "FAILED?",      "FAILED?" },
      { COLOR_YELLOW,    0,         "FAILED",       "FAILED" },
      { COLOR_YELLOW,    0,         "FAILED",       "FAILED" },
      { COLOR_YELLOW,    0,         "FAILED",       "FAILED" } },

    /* Epilogue failed */
    { { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED?",      "FAILED?" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" } },

    /* Keep-alive validation handler failed */
    { { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED?",      "FAILED?" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" } },

    /* Exception handler failed */
    { { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED?",      "FAILED?" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" },
      { COLOR_BLUE,      0,         "FAILED",       "FAILED" } },

    { { COLOR_BLUE,      0,         "STOPPED",       "STOPPED" },
      { COLOR_BLUE,      0,         "STOPPED?",      "STOPPED?" },
      { COLOR_BLUE,      0,         "STOPPED",       "STOPPED" },
      { COLOR_BLUE,      0,         "STOPPED",       "STOPPED" },
      { COLOR_BLUE,      0,         "STOPPED",       "STOPPED" } },

    { { COLOR_RED,       A_BOLD,    "ERROR",        "ERROR" },
      { COLOR_RED,       A_BOLD,    "ERROR?",       "ERROR?" },
      { COLOR_RED,       A_BOLD,    "ERROR",        "ERROR" },
      { COLOR_RED,       A_BOLD,    "ERROR",        "ERROR" },
      { COLOR_RED,       A_BOLD,    "ERROR",        "ERROR" } }
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
        if (what->attrs & A_BOLD)
            putp(tparm(tigetstr("bold")));

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
tester_term_out_start(tester_flags flags, run_item_type type,
                      const char *name, unsigned int tin,
                      test_id parent, test_id self)
{
    char ids[20] = "";
    char tin_str[16] = "";
    char msg[256];
    int actual_msg_len;

    if ((~flags & TESTER_VERBOSE) ||
        ((~flags & TESTER_VVERB) && (type == RUN_ITEM_SESSION)))
        return;

    if (flags & TESTER_OUT_TIN && tin != TE_TIN_INVALID)
    {
        if (snprintf(tin_str, sizeof(tin_str), " [%u]", tin) >=
                (int)sizeof(tin_str))
        {
            ERROR("Too short buffer for TIN");
            /* Continue */
        }
    }

    if (flags & TESTER_VVERB)
    {
        if (snprintf(ids, sizeof(ids), " %d:%d", parent, self) >=
                (int)sizeof(ids))
        {
            ERROR("Too short buffer for pair of test IDs");
            /* Continue */
        }
    }

    if (((actual_msg_len =
          snprintf(msg, sizeof(msg), "Starting%s %s %s%s",
                   ids, run_item_type_to_string(type), name, tin_str))) >=
            (int)sizeof(msg))
    {
        ERROR("%s: Too short buffer for output message: msg_len=%d "
              "and available len %d",
              __FUNCTION__, actual_msg_len, (int)sizeof(msg));
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
tester_term_out_done(tester_flags flags,
                     run_item_type type, const char *name,
                     unsigned int tin, test_id parent, test_id self,
                     tester_test_status status, trc_verdict trcv)
{
    char                    ids[20] = "";
    char                    tin_str[16] = "";
    char                    msg_out[512];
    char                    msg_in[512];
    char                   *msg;
    int                     actual_msg_len;
    colored_verdict_type    cvt;

    assert(status < TESTER_TEST_STATUS_MAX);

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

    if (flags & TESTER_OUT_TIN && tin != TE_TIN_INVALID)
    {
        if (snprintf(tin_str, sizeof(tin_str), " [%u]", tin) >=
                (int)sizeof(tin_str))
        {
            ERROR("Too short buffer for TIN");
            /* Continue */
        }
    }

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
    if ((actual_msg_len =
         snprintf(msg_out, sizeof(msg_out), "Done%s %s %s%s ",
                  ids, run_item_type_to_string(type), name, tin_str)) >=
            (int)sizeof(msg_out))
    {
        ERROR("%s: Too short buffer for output message: msg_len=%d "
              "and available len %d",
              __FUNCTION__, actual_msg_len, (int)sizeof(msg));
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
            ERROR("Too short buffer for output message: "
                  "n_spaces = %d, msg_in=%u", n_spaces,
                  (unsigned)sizeof(msg_in));
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
                cvt = (flags & TESTER_OUT_EXP) ?
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
