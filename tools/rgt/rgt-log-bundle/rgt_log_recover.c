/** @file
 * @brief Test Environment: recovering raw log from its fragments.
 *
 * This program recovers original raw log from fragments produced by
 * rgt-log-split.
 *
 * Copyright (C) 2016-2022 OKTET Labs. All rights reserved.
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <popt.h>

#include <inttypes.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_file.h"
#include "rgt_log_bundle_common.h"

#include "te_string.h"

/** Where to find raw log fragments and recover_list file */
static char *split_log_path = NULL;
/** Where to store recovered raw log */
static char *output_path = NULL;

/**
 * Parse command line.
 *
 * @param argc    Number of arguments
 * @param argv    Array of command line arguments
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon = NULL;
    int          rc;

    RGT_ERROR_INIT;

    /* Option Table */
    struct poptOption optionsTable[] = {
        { "split-log", 's', POPT_ARG_STRING, NULL, 's',
          "Path to split raw log.", NULL },

        { "output", 'o', POPT_ARG_STRING, NULL, 'o',
          "Output file.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    CHECK_NOT_NULL(optCon = poptGetContext(NULL, argc,
                                           (const char **)argv,
                                           optionsTable, 0));

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 's')
            split_log_path = poptGetOptArg(optCon);
        else if (rc == 'o')
            output_path = poptGetOptArg(optCon);
    }

    if (split_log_path == NULL || output_path == NULL)
    {
        ERROR("Specify all the required parameters");
        RGT_ERROR_JUMP;
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s",
              poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        RGT_ERROR_JUMP;
    }

    if (poptPeekArg(optCon) != NULL)
    {
        ERROR("Too many parameters were specified");
        RGT_ERROR_JUMP;
    }

    RGT_ERROR_SECTION;

    if (optCon != NULL)
        poptFreeContext(optCon);

    return RGT_ERROR_VAL;
}

int
main(int argc, char **argv)
{
    FILE       *f_recover = NULL;
    FILE       *f_result = NULL;
    FILE       *f_frag = NULL;
    char        s[DEF_STR_LEN];

    char      frag_name[DEF_STR_LEN];
    uint64_t  raw_offset;
    uint64_t  raw_length;
    uint64_t  frag_offset;

    RGT_ERROR_INIT;

    te_log_init("RGT LOG RECOVER", te_log_message_file);

    CHECK_RC(process_cmd_line_opts(argc, argv));

    /**
     * In recover_list file we list of raw log blocks is stored,
     * where for each block it is specified in which log fragment file
     * it can be found, what is its offset and length, and at which offset
     * it should appear in recovered raw log. Restoring original raw log
     * from this data is straightforward.
     */
    CHECK_FOPEN_FMT(f_recover, "r", "%s/recover_list", split_log_path);

    CHECK_FOPEN(f_result, output_path, "w");

    while (!feof(f_recover))
    {
        if (fgets(s, sizeof(s), f_recover) == NULL)
            break;

        if (sscanf(s, "%" PRIu64 " %" PRIu64 " %s %" PRIu64,
                   &raw_offset, &raw_length,
                   frag_name, &frag_offset) <= 0)
            break;

        CHECK_FOPEN_FMT(f_frag, "r", "%s/%s", split_log_path, frag_name);
        CHECK_RC(file2file(f_result, f_frag, raw_offset, frag_offset,
                           raw_length));
        CHECK_FCLOSE(f_frag);
    }

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_result);
    CHECK_FCLOSE(f_recover);
    free(split_log_path);
    free(output_path);

    if (RGT_ERROR)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
