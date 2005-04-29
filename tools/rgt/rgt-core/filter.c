/** @file
 * @brief Test Environment: Interface for filtering of log messages.
 *
 * The module is responsible for making descisions about message filtering.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tcl.h>

#include "rgt_common.h"
#include "filter.h"

static Tcl_Interp *tcl_interp = NULL;
static te_bool     initialized = FALSE;

/* See the description in filter.h */
int
rgt_filter_init(const char *fltr_fname)
{
    if (initialized)
    {
        TRACE("rgt_filter library has already been initialized");
        return -1;
    }
    
    if (fltr_fname == NULL)
    {
        initialized = TRUE;
        return 0;
    }

    /* Create "pure" Tcl interpreter */
    if ((tcl_interp = Tcl_CreateInterp()) == NULL)
    {
        TRACE("Cannot create TCL interpreter");
        return -1;
    }

    /* Load TCL filters */
    if (Tcl_EvalFile(tcl_interp, (char *)fltr_fname) != TCL_OK)
    {
        FMT_TRACE("%s", Tcl_GetStringResult(tcl_interp));
        Tcl_DeleteInterp(tcl_interp);
        return -1;
    }
    
    initialized = TRUE;
    return 0;
}

/**
 * Destroys filter module.
 *
 * @return  Nothing.
 */
void
rgt_filter_destroy()
{
    if (!initialized)
    {
        TRACE("rgt_filter library has not been initialized");
        return;
    }

    if (tcl_interp == NULL)
        return;

    Tcl_DeleteInterp(tcl_interp);
    tcl_interp = NULL;
}

/**
 * Validates if log message with a particular tuple (level, entity name, 
 * user name and timestamp) passes through user defined filter.
 *
 * @param level         Log level
 * @param entity        Entity name
 * @param user          User name
 * @param timestamp     Timestamp
 *
 * @return Returns filtering mode for the tuple.
 *         It never returns NFMODE_DEFAULT value.
 *
 * @retval NFMODE_INCLUDE   the tuple is passed through the filter.
 * @retval NFMODE_EXCLUDE   the tuple is rejected by the filter.
 */
enum node_fltr_mode
rgt_filter_check_message(const char *level,
                         const char *entity, const char *user,
                         const uint32_t *timestamp)
{
    enum node_fltr_mode  fmode = NFMODE_DEFAULT;
    char                *cmd;

    if (initialized && tcl_interp == NULL)
        return NFMODE_INCLUDE;

#define RGT_MSG_FILTER_FUNC "rgt_msg_filter {%s} {%s} {%s} %d"

    if ((cmd = malloc(strlen(RGT_MSG_FILTER_FUNC) + strlen(level) + 
                      strlen(entity) + strlen(user) + 20)) == NULL)
    {
        TRACE("No memory available");
        THROW_EXCEPTION;
    }

    sprintf(cmd, RGT_MSG_FILTER_FUNC,
            level, entity, user, *timestamp);

#undef RGT_MSG_FILTER_FUNC

    if (Tcl_Eval(tcl_interp, cmd) == TCL_OK)
    {
        free(cmd);

        /* There is such procedure in the Tcl environment */
        if (strcmp(Tcl_GetStringResult(tcl_interp), "pass") == 0)
        {
            fmode = NFMODE_INCLUDE;
        }
        else if (strcmp(Tcl_GetStringResult(tcl_interp), "fail") == 0)
        {
            fmode = NFMODE_EXCLUDE;
        }
        else
        {
            FMT_TRACE("Tcl filter file: "
                      "function rgt_msg_filter has returned \"%s\", "
                      "but it is allowed to return only strings "
                      "\"pass\" or \"fail\".",
                      Tcl_GetStringResult(tcl_interp));
            THROW_EXCEPTION;
        }
    }
    else
    {
        /* 
         * Something awful is occured: Tcl filter file doesn't contain
         * standard routine "rgt_msg_filter" that is used for filtering.
         */
        FMT_TRACE("Tcl filter file: %s.", Tcl_GetStringResult(tcl_interp));
        free(cmd);
        THROW_EXCEPTION;
    }

    return fmode;
}

/**
 * Verifies if the whole branch of execution flow should be excluded or 
 * included from the log report.
 *
 * @param   path  Path (name) of the branch to be checked.
 *                Path is formed from names of packages and/or test
 *                of the execution flow separated by '/'. For example
 *                path "/a/b/c/d" means that execution flow is
 *                pkg "a" -> pkg "b" -> pkg "c" -> [test | pkg] "d"
 *
 * @return  Returns filtering mode for the branch.
 */
enum node_fltr_mode
rgt_filter_check_branch(const char *path)
{
    enum node_fltr_mode fmode = NFMODE_DEFAULT;
        
    char *cmd_name = "rgt_branch_filter ";
    char *cmd;

    if (initialized && tcl_interp == NULL)
        return NFMODE_INCLUDE;
        
    if ((cmd = malloc(strlen(cmd_name) + strlen(path) + 1)) == NULL)
    {
        TRACE("No memory available");
        THROW_EXCEPTION;
    }

    memcpy(cmd, cmd_name, strlen(cmd_name));
    memcpy(cmd + strlen(cmd_name), path, strlen(path) + 1);

    if (Tcl_Eval(tcl_interp, cmd) == TCL_OK)
    {
        free(cmd);

        /* There is such procedure in the Tcl environment */
        if (strcmp(Tcl_GetStringResult(tcl_interp), "pass") == 0)
        {
            fmode = NFMODE_INCLUDE;
        }
        else if (strcmp(Tcl_GetStringResult(tcl_interp), "fail") == 0)
        {
            fmode = NFMODE_EXCLUDE;
        }
        else if (strcmp(Tcl_GetStringResult(tcl_interp), "default") == 0)
        {
            fmode = NFMODE_DEFAULT;
        }
        else
        {
            FMT_TRACE("Tcl filter file: "
                      "function rgt_branch_filter has returned \"%s\", "
                      "but it is allowed to return only strings "
                      "\"pass\", \"fail\" or \"default\".",
                      Tcl_GetStringResult(tcl_interp));
            THROW_EXCEPTION;
        }
    }
    else
    {
        /* 
         * Something awful is occured: Tcl filter file doesn't contain
         * standard routine "rgt_branch_filter" that is used for filtering.
         */
        FMT_TRACE("Tcl filter file: %s.", Tcl_GetStringResult(tcl_interp));
        free(cmd);
        THROW_EXCEPTION;
    }

    return fmode;
}
