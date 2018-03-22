/** @file
 * @brief Test Environment
 *
 * Builder C API library
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "te_errno.h"
#include "te_builder_ts.h"

/* Maxmimum length of the shell command */
#define MAX_SH_CMD      2048

static char cmd[MAX_SH_CMD];

/**
 * This function is called by Tester subsystem to build dynamically
 * a Test Suite. Test Suite is installed to 
 * ${TE_INSTALL_SUITE}/bin/<suite>.
 *
 * Test Suite may be linked with TE libraries. If ${TE_INSTALL} or
 * ${TE_BUILD} are not empty necessary compiler and linker flags are
 * passed to Test Suite configure in variables TE_CFLAGS and TE_LDFLAGS.
 *
 * @param suite         Unique suite name.
 * @param sources       Source location of the Test Suite (Builder will
 *                      look for configure script in this directory.
 *                      If the path is started from "/" it is considered as
 *                      absolute.  Otherwise it is considered as relative
 *                      from ${TE_BASE}.
 *
 * @return Status code.
 */
te_errno
builder_build_test_suite(const char *suite, const char *sources)
{
    te_errno            rc;
#if HAVE_SIGNAL_H
    struct sigaction    act;
    struct sigaction    oldact;
#endif

    if (suite == NULL || *suite == 0 || sources == NULL || *sources == 0)
        return TE_EINVAL;

#if HAVE_SIGNAL_H
    memset(&oldact, 0, sizeof(oldact));
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;

    if (sigaction(SIGINT, &act, &oldact) != 0)
        return te_rc_os2te(errno);
#endif
    
    sprintf(cmd, "te_build_suite %s \"%s\" "
                 ">builder.log.%s.1 2>builder.log.%s.2",
            suite, sources, suite, suite);
    rc = system(cmd) == 0 ? 0 : TE_ESHCMD;

#if HAVE_SIGNAL_H
    if (sigaction(SIGINT, &oldact, NULL) != 0)
        return te_rc_os2te(errno);
#endif

    return rc;
}
