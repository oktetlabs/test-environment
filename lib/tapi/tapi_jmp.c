/** @file
 * @brief Test API to use setjmp/longjmp.
 *
 * Thread safe stack of jump buffers and API to deal with it.
 *
 * Copyright (C) 2005 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Jumps"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "logger_api.h"


