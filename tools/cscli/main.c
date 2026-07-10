/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 Advanced Micro Devices, Inc. */
/** @file
 * @brief Configurator CLI utility main file
 *
 * Configurator CLI utility main file.
 */

#include <inttypes.h>
#include <stdio.h>

#include "conf_api.h"
#include "conf_types.h"
#include "te_sockaddr.h"
#include "te_string.h"

static void
usage(FILE *out)
{
    fprintf(out, "Usage: cscli <command>\n");
    fprintf(out, "\n");
    fprintf(out, "Available commands:\n");
    fprintf(out, "\n");
    fprintf(out, "  get <oid>           Get value from Configurator by OID\n");
}

int main(int argc, char *argv[])
{
    te_errno rc = 0;

    if (argc < 2)
    {
        usage(stderr);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "help") == 0 && argc == 2)
    {
        usage(stdout);
    }
    else if (strcmp(argv[1], "get") == 0 && argc == 3)
    {
        char *value;

        rc = cfg_get_instance_strval_fmt(NULL, &value, "%s", argv[2]);
        if (rc != 0)
        {
            fprintf(stderr, "Configurator get failed: %s-%s\n",
                    te_rc_mod2str(rc), te_rc_err2str(rc));
        }
        else
        {
            puts(value);
            free(value);
        }
    }
    else
    {
        usage(stderr);
        return EXIT_FAILURE;
    }

    if (rc != 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
