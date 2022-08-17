/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Program which may be called to shut down RCF.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "te_str.h"
#include "logger_ten.h"
#include "rcf_api.h"
#include "rcf_internal.h"
#include "ipc_client.h"


int
main(void)
{
    rcf_msg msg;
    char    name[64];
    size_t  anslen = sizeof(msg);
    int     rc;
    int     result = EXIT_SUCCESS;

    struct ipc_client *handle = NULL;

    te_log_init("RCF ShutDown", ten_log_message);

    TE_SPRINTF(name, "rcf_shut_%d", getpid());

    if ((rc = ipc_init_client(name, RCF_IPC, &handle)) != 0)
    {
        fprintf(stderr, "ipc_init_client() for '%s' failed: %d\n",
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
