/** @file
 * @brief Test Environment
 *
 * Program which may be called to shut down RCF.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "rcf_internal.h"
#include "ipc_client.h"


int
main(void)
{
    rcf_msg msg;
    char   *name = "rcfshutdown_client";
    size_t  anslen = sizeof(msg);
    int     rc;
    int     result = EXIT_SUCCESS;

    struct ipc_client *handle = NULL;


    if ((rc = ipc_init_client(name, &handle)) != 0)
    {
        fprintf(stderr, "ipc_init_client() for '%s' failed: %X\n",
                name, rc);
        return EXIT_FAILURE;
    }

    memset((char *)&msg, 0, sizeof(rcf_msg));
    msg.opcode = RCFOP_SHUTDOWN;

    rc = ipc_send_message_with_answer(handle, RCF_SERVER, (char *)&msg,
                                      sizeof(rcf_msg), (char *)&msg,
                                      &anslen);
    if (rc != 0)
    {
        fprintf(stderr, "RCF shut down failed\n");
        result = EXIT_FAILURE;
    }

    rc = ipc_close_client(handle);
    if (rc != 0)
    {
        fprintf(stderr, "ipc_close_client() failed");
        result = EXIT_FAILURE;
    }

    return result;
}
