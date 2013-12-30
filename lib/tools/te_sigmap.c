/** @file
 * @brief Mapping of unix signal name->number and number->name
 *
 * Implmentation of the mapping functions.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
    return 0;
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
