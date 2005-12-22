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

#include "rgt_common.h"

#include <tcl.h>

#include "filter.h"

static Tcl_Interp *tcl_interp = NULL;
static te_bool     initialized = FALSE;

/** Maximum length of TCL command */
#define MAX_CMD_LEN 256

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
 * Runs specified Tcl command and returns filtering result
 *
 * @param cmd        TCL command to run
 * @param func_name  Function name (for debugging purpose)
 *
 * @return Filtering result
 */
static enum node_fltr_mode
run_tcl_cmd(const char *cmd, const char *func_name)
{
    if (initialized && tcl_interp == NULL)
        return NFMODE_INCLUDE;

    if (Tcl_Eval(tcl_interp, cmd) == TCL_OK)
    {
        /* There is such procedure in the Tcl environment */
        if (strcmp(Tcl_GetStringResult(tcl_interp), "pass") == 0)
        {
            return NFMODE_INCLUDE;
        }
        else if (strcmp(Tcl_GetStringResult(tcl_interp), "fail") == 0)
        {
            return NFMODE_EXCLUDE;
        }
        else if (strcmp(Tcl_GetStringResult(tcl_interp), "default") == 0)
        {
            return NFMODE_DEFAULT;
        }
        else
        {
            FMT_TRACE("Tcl filter file: "
                      "function %s has returned \"%s\", "
                      "but it is allowed to return only strings "
                      "\"pass\", \"fail\" or \"default\".",
                      func_name,
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
        THROW_EXCEPTION;
    }

    assert(0);

    return NFMODE_EXCLUDE;
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
    char cmd[MAX_CMD_LEN];

    snprintf(cmd, sizeof(cmd), "rgt_msg_filter {%s} {%s} {%s} %d",
             level, entity, user, *timestamp);

    return run_tcl_cmd(cmd, "rgt_msg_filter");
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
    char cmd[MAX_CMD_LEN];
        
    snprintf(cmd, sizeof(cmd), "rgt_branch_filter %s", path);

    return run_tcl_cmd(cmd, "rgt_branch_filter");
}

/**
 * Validates if the particular node (TEST, SESSION or PACKAGE) passes 
 * through duration filter.
 *
 * @param node_type  Typo of the node ("TEST", "SESSION" or "PACKAGE")
 * @param start_ts   Start timestamp
 * @param end_ts     End timestamp
 *
 * @return Returns filtering mode for the node.
 *
 * @retval NFMODE_INCLUDE   the node is passed through the filter.
 * @retval NFMODE_EXCLUDE   the node is rejected by the filter.
 */
enum node_fltr_mode
rgt_filter_check_duration(const char *node_type,
                          uint32_t *start_ts, uint32_t *end_ts)
{
    enum node_fltr_mode fmode;
    uint32_t            duration[2];
    char                cmd[MAX_CMD_LEN];


    TIMESTAMP_SUB(duration, end_ts, start_ts);

    snprintf(cmd, sizeof(cmd), "rgt_duration_filter %s %d",
             node_type, duration[0]);

    fmode = run_tcl_cmd(cmd, "rgt_duration_filter");
    
    return (fmode == NFMODE_DEFAULT) ? NFMODE_INCLUDE : fmode;
}
