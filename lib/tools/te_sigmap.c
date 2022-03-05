/** @file
 * @brief Mapping of unix signal name->number and number->name
 *
 * Implmentation of the mapping functions.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Log Sigmap"

#include "te_config.h"
#include "te_defs.h"

#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#include "logger_api.h"


static const struct {
    char *name;
    int   signo;
} te_signals[] = {{"SIGHUP",   SIGHUP},
                  {"SIGINT",   SIGINT},
                  {"SIGQUIT",  SIGQUIT},
                  {"SIGILL",   SIGILL},
                  {"SIGTRAP",  SIGTRAP},
                  {"SIGABRT",  SIGABRT},
                  {"SIGBUS",   SIGBUS},
                  {"SIGFPE",   SIGFPE},
                  {"SIGKILL",  SIGKILL},
                  {"SIGUSR1",  SIGUSR1},
                  {"SIGSEGV",  SIGSEGV},
                  {"SIGUSR2",  SIGUSR2},
                  {"SIGPIPE",  SIGPIPE},
                  {"SIGALRM",  SIGALRM},
                  {"SIGTERM",  SIGTERM},
#ifdef SIGSTKFLT
                  {"SIGSTKFLT",  SIGSTKFLT},
#endif
};

/* See description in the te_sigmap.h */
int
map_name_to_signo(const char *name)
{
    unsigned i;

    for (i = 0; i < sizeof(te_signals) / sizeof(*te_signals); i++)
    {
        if (!strcmp(te_signals[i].name, name))
            return te_signals[i].signo;
    }
    ERROR("%s() unsupported signal name '%s'", __func__, name);
    return -1;
}

/* See description in the te_sigmap.h */
char *
map_signo_to_name(int signo)
{
    unsigned i;

    for (i = 0; i < sizeof(te_signals) / sizeof(*te_signals); i++)
    {
        if (te_signals[i].signo == signo)
            return strdup(te_signals[i].name);
    }
    return NULL;
}
