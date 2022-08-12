/** @file
 * @brief Creating RCF listener before running TA
 *
 * Auxuliary program creating RCF listener before TA
 * is run. It can be used when TA is run in a different
 * network namespace to which RCF cannot connect. Listener
 * created before moving to the namespace can still
 * accept connections via interfaces not belonging
 * to that namespace.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

#include "te_config.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "logger_file.h"
#include "te_errno.h"
#include "te_string.h"
#include "comm_agent.h"

int
main(int argc, char *argv[])
{
    int                s;
    int                rc;
    te_errno           te_rc;
    char               buf[100] = "";
    long int           port = -1;
    char              *endptr = NULL;

    char       *new_args[4];
    int         i;
    te_string   cmd_str = TE_STRING_INIT_STATIC(4096);

    te_log_init("ta_rcf_listener", te_log_message_file);

    if (argc < 3)
    {
        ERROR("Usage: ta_rcf_listener port cmd [args]");
        exit(EXIT_FAILURE);
    }

    port = strtol(argv[1], &endptr, 10);
    if (port < 0 || port > 0xffff || *endptr != '\0')
    {
        ERROR("Failed to convert '%s' to correct port value",
              argv[1]);
        exit(EXIT_FAILURE);
    }

    te_rc = rcf_comm_agent_create_listener(port, &s);
    if (te_rc != 0)
    {
        ERROR("Failed to create listener, rc = %r", te_rc);
        exit(EXIT_FAILURE);
    }

    rc = snprintf(buf, sizeof(buf), "%d", s);
    if (rc < 0 || rc >= (int)sizeof(buf))
    {
        ERROR("snprintf() returned unexpected result "
              "when writing socket FD to the buffer");
        exit(EXIT_FAILURE);
    }

    rc = setenv("TE_TA_RCF_LISTENER", buf, 1);
    if (rc < 0)
    {
        ERROR("setenv() failed, errno=%d ('%s')",
              errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    new_args[0] = "/bin/bash";
    new_args[1] = "-c";
    for (i = 2; i < argc; i++)
    {
        te_rc = te_string_append(&cmd_str, "%s ", argv[i]);
        if (te_rc != 0)
        {
            ERROR("te_string_append() failed when constructing "
                  "command string, rc = %r", te_rc);
            exit(EXIT_FAILURE);
        }
    }
    new_args[2] = cmd_str.ptr;
    new_args[3] = NULL;

    rc = execv("/bin/bash", new_args);
    if (rc < 0)
    {
        ERROR("execv() failed, errno=%d ('%s')",
              errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}
