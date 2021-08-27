/** @file
 * @brief Test Environment: implementation of merging fragments into raw log.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_file.h"
#include "te_string.h"
#include "te_str.h"
#include "te_file.h"
#include "rgt_log_bundle_common.h"

/** If @c TRUE, find log messages to be merged by TIN */
static te_bool use_tin = FALSE;
/** If @c TRUE, find log messages to be merged by test ID */
static te_bool use_test_id = FALSE;
/** TIN of log node to be merged */
static unsigned int filter_tin = (unsigned int)-1;
/** Test ID of log node to be merged */
static unsigned int filter_test_id = (unsigned int)-1;
/** Depth of log node to be merged */
static int filter_depth = 0;
/** Sequential number of log node to be merged */
static int filter_seq = 0;
/** Number of inner fragment file to be merged */
static int64_t filter_frag_num = 0;

/**
 * Open log fragment file and copy all its contents into
 * resulting file.
 *
 * @param f           File to where to copy contents.
 * @param frag_path   Path to the file with log fragment to append.
 */
static void
append_frag_to_file(FILE *f, const char *frag_path)
{
    FILE *f_frag;
    off_t frag_len;

    f_frag = fopen(frag_path, "r");
    if (f_frag == NULL)
    {
        fprintf(stderr, "Failed to open %s for reading\n",
                frag_path);
        exit(EXIT_FAILURE);
    }

    if (fseeko(f_frag, 0LLU, SEEK_END) < 0 ||
        (frag_len = ftello(f_frag)) < 0 ||
        fseeko(f_frag, 0LLU, SEEK_SET) < 0)
    {
        fprintf(stderr, "Failed to get length of %s, error='%s'\n",
                frag_path, strerror(errno));
        fclose(f_frag);
        exit(EXIT_FAILURE);
    }

    if (file2file(f, f_frag, -1, -1, frag_len) < 0)
    {
        fprintf(stderr, "Failed to append %s to result file\n",
                frag_path);
        fclose(f_frag);
        exit(EXIT_FAILURE);
    }

    if (fclose(f_frag) != 0)
    {
        fprintf(stderr, "fclose() failed for %s\n", frag_path);
        exit(EXIT_FAILURE);
    }
}

/**
 * Merge inner log fragments into "raw gist log" consisting
 * of starting and terminating fragments only.
 *
 * @param split_log_path      Where log fragments are to be found
 * @param f_raw_gist          "raw gist" log
 * @param f_frags_list        File with list of starting and terminating
 *                            fragments (in the same order they appear in
 *                            "raw gist" log)
 * @param f_result            Where to store merged raw log
 * @param f_frags_count       If not @c NULL, save number of fragments
 *                            in the target node there
 * @param get_needed_frags    If @c TRUE, do not create merged log, only
 *                            get list of all the fragment files which
 *                            should be extracted from the RAW log bundle
 *                            before this function can work.
 * @param needed_frags        String to which to append names of required
 *                            fragment files if @p get_needed_frags is
 *                            @c TRUE.
 *
 * @return Number of required fragment files if @p get_needed_frags is
 *         @c TRUE; @c 0 otherwise. On failure this function calls
 *         exit() to terminate the tool.
 */
static int
merge(const char *split_log_path,
      FILE *f_raw_gist, FILE *f_frags_list, FILE *f_result,
      FILE *f_frags_count, te_bool get_needed_frags,
      te_string *needed_frags)
{
    char s[DEF_STR_LEN];
    char frag_name[DEF_STR_LEN];
    char frag_path[DEF_STR_LEN];
    char *name_suff;

    uint64_t cum_length = 0;
    uint64_t length;
    uint64_t start_len;
    uint64_t frags_cnt;
    unsigned int parent_id;
    int64_t target_node_id = -1;
    te_bool start_frag;
    te_bool check_after_frag;

    uint64_t x;
    uint64_t y;
    uint64_t z;

    uint64_t i;
    int scanf_rc;
    int needed_frags_cnt = 0;

    unsigned int tin;
    unsigned int test_id;
    int          depth;
    int          seq;

    off_t raw_fp;

    RGT_ERROR_INIT;

    assert(f_raw_gist != NULL && f_frags_list != NULL && f_result != NULL);

    while (!feof(f_frags_list))
    {
        if (fgets(s, sizeof(s), f_frags_list) == NULL)
            break;

        scanf_rc = sscanf(
                     s, "%s %u %d %d %" PRIu64 " %" PRIu64 " %" PRIu64 " %"
                     PRIu64, frag_name, &tin, &depth, &seq, &length,
                     &x, &y, &z);
        if (scanf_rc < 5)
        {
            fprintf(stderr, "sscanf() read too few fragment parameters in "
                    "'%s' (%d)\n", s, scanf_rc);
            exit(EXIT_FAILURE);
        }

        name_suff = NULL;
        check_after_frag = FALSE;
        if ((name_suff = strstr(frag_name, "_end")) != NULL)
        {
            start_frag = FALSE;
            if (scanf_rc >= 7)
            {
                /*
                 * These fields are present only in newer version of
                 * RAW log bundle.
                 */
                frags_cnt = x;
                parent_id = y;
                check_after_frag = TRUE;
            }
        }
        else if ((name_suff = strstr(frag_name, "_start")) != NULL)
        {
            if (scanf_rc < 7)
            {
                fprintf(stderr, "Too few parameters in '%s'\n", s);
                exit(EXIT_FAILURE);
            }
            start_frag = TRUE;
            start_len = x;
            frags_cnt = y;
        }
        else
        {
            fprintf(stderr, "Unknown fragment type '%s'\n", frag_name);
            exit(EXIT_FAILURE);
        }

        if (sscanf(frag_name, "%u_", &test_id) <= 0)
        {
            fprintf(stderr, "sscanf() could not parse node ID in %s\n",
                    frag_name);
            exit(EXIT_FAILURE);
        }

        cum_length += length;

        /* Remove _start or _end suffix from fragment name. */
        *name_suff = '\0';

        if (!start_frag && check_after_frag && target_node_id >= 0 &&
            (unsigned int)target_node_id == parent_id &&
            frags_cnt != 0)
        {
            /*
             * This is a terminating fragment (with control message
             * saying that test/session/package ended) belonging
             * to a child of the target node, and it has some log
             * messages attached after its end (they came after the
             * child node terminated but before the next child node
             * started or parent finished). We should insert them
             * into log here so that RGT will show them for a parent
             * node in a proper place.
             */

            if (get_needed_frags)
            {
                CHECK_TE_RC(te_string_append(needed_frags, " %s_after",
                                             frag_name));
                needed_frags_cnt++;
            }
            else
            {
                TE_SPRINTF(frag_path, "%s/%s_after", split_log_path,
                           frag_name);

                file2file(f_result, f_raw_gist, -1, -1, cum_length);
                cum_length = 0;
                append_frag_to_file(f_result, frag_path);
            }
        }
        else if (start_frag &&
                 ((use_tin && filter_tin == tin) ||
                  (use_test_id && filter_test_id == test_id) ||
                  (!use_tin && !use_test_id &&
                   filter_depth == depth && filter_seq == seq)))
        {
            target_node_id = test_id;

            if (!get_needed_frags)
            {
                /*
                 * In "gist" raw log starting control message goes together
                 * with verdicts and artifacts. Here we leave starting
                 * message but replace verdicts and artifacts with full log
                 * for a target node.
                 */

                cum_length -= (length - start_len);
                file2file(f_result, f_raw_gist, -1, -1, cum_length);
                raw_fp = ftello(f_raw_gist);
                raw_fp += (length - start_len);
                fseeko(f_raw_gist, raw_fp, SEEK_SET);
                cum_length = 0;

                if (f_frags_count != NULL)
                {
                    fprintf(f_frags_count, "%llu",
                            (long long unsigned)frags_cnt);
                    /*
                     * This should never happen, however if multiple
                     * records matched, save number of fragments for
                     * the first of them.
                     */
                    f_frags_count = NULL;
                }
            }

            for (i = 0; i < frags_cnt; i++)
            {
                if (filter_frag_num >= 0)
                    i = filter_frag_num;

                if (get_needed_frags)
                {
                    CHECK_TE_RC(te_string_append(needed_frags,
                                                 " %s_inner_%" PRIu64,
                                                 frag_name, i));
                    needed_frags_cnt++;
                }
                else
                {
                    TE_SPRINTF(frag_path,
                               "%s/%s_inner_%" PRIu64,
                               split_log_path, frag_name, i);

                    append_frag_to_file(f_result, frag_path);
                }

                if (filter_frag_num >= 0)
                    break;
            }
        }
    }


    if (get_needed_frags)
    {
        fseeko(f_frags_list, 0, SEEK_SET);
        return needed_frags_cnt;
    }

    if (cum_length > 0)
        file2file(f_result, f_raw_gist, -1, -1, cum_length);

    RGT_ERROR_SECTION;

    /* TODO: this should be avoided */
    if (RGT_ERROR)
        exit(EXIT_FAILURE);

    return 0;
}

/** Where to find raw log bundle  */
static char *bundle_path = NULL;
/** Where to find raw log fragments and "raw gist" log  */
static char *split_log_path = NULL;
/** Where to save number of log fragments in the requested node */
static char *frags_count_path = NULL;
/** Where to store merged raw log */
static char *output_path = NULL;

/**
 * Parse command line.
 *
 * @param argc    Number of arguments
 * @param argv    Array of command line arguments
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon; /* Context for parsing command-line options */
    int          rc;

    /* Option Table */
    struct poptOption optionsTable[] = {
        { "bundle", 'b', POPT_ARG_STRING, NULL, 'b',
          "Path to raw log bundle (required if bundle was not unpacked).",
          NULL },

        { "split-log", 's', POPT_ARG_STRING, NULL, 's',
          "Path to split raw log.", NULL },

        { "frags-count", 'c', POPT_ARG_STRING, NULL, 'c',
          "Where to save number of fragments in the requested node.",
          NULL },

        { "filter", 'f', POPT_ARG_STRING, NULL, 'f',
          "Either 'TIN' or 'depth_seq'", NULL },

        { "page", 'p', POPT_ARG_STRING, NULL, 'p',
          "Either page number or 'all' to merge all pages at once", NULL },

        { "output", 'o', POPT_ARG_STRING, NULL, 'o',
          "Where to save merged raw log.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            optionsTable, 0);

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 'b')
        {
            bundle_path = poptGetOptArg(optCon);
        }
        else if (rc == 's')
        {
            split_log_path = poptGetOptArg(optCon);
        }
        else if (rc == 'c')
        {
            frags_count_path = poptGetOptArg(optCon);
        }
        else if (rc == 'f')
        {
            char *filter = poptGetOptArg(optCon);

            if (filter == NULL)
                usage(optCon, 1, "--filter value is not specified", NULL);

            if (strchr(filter, '_') != NULL)
            {
                sscanf(filter, "%d_%d", &filter_depth, &filter_seq);
            }
            else if (strstr(filter, "id") == filter)
            {
                sscanf(filter, "id%u", &filter_test_id);
                use_test_id = TRUE;
            }
            else
            {
                sscanf(filter, "%u", &filter_tin);
                use_tin = TRUE;
            }

            free(filter);
        }
        else if (rc == 'p')
        {
            char *page = poptGetOptArg(optCon);

            if (page == NULL)
                usage(optCon, 1, "--page value is not specified", NULL);

            if (strcasecmp(page, "all") == 0)
                filter_frag_num = -1;
            else
                filter_frag_num = atoll(page);

            free(page);
        }
        else if (rc == 'o')
        {
            output_path = poptGetOptArg(optCon);
        }
    }

    if (split_log_path == NULL || output_path == NULL)
        usage(optCon, 1, "Specify all the required parameters", NULL);

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        exit(1);
    }

    if (poptPeekArg(optCon) != NULL)
        usage(optCon, 1, "Too many parameters specified", NULL);

    poptFreeContext(optCon);
}

/**
 * Call te_fopen_fmt(), terminate the program if it failed.
 *
 * @param _f          Where to save FILE pointer on success.
 * @param _mode       File opening mode.
 * @param _path_fmt   File path format string.
 */
#define FOPEN_FMT(_f, _mode, _path_fmt...) \
    do {                                                \
        _f = te_fopen_fmt(_mode, _path_fmt);            \
        if (_f == NULL)                                 \
            exit(EXIT_FAILURE);                         \
    } while (0)

int
main(int argc, char **argv)
{
    FILE *f_raw_gist = NULL;
    FILE *f_frags_list = NULL;
    FILE *f_frags_count = NULL;
    FILE *f_result = NULL;

    te_string cmd = TE_STRING_INIT;
    int res;

    RGT_ERROR_INIT;

    te_log_init("RGT LOG MERGE", te_log_message_file);

    process_cmd_line_opts(argc, argv);

    if (bundle_path != NULL)
    {
        /*
         * Unpack log_gist.raw and frags_list from raw log bundle firstly;
         * these are always needed, from frags_list it will be determined
         * which log fragment files should be unpacked.
         */
        CHECK_TE_RC(
            te_string_append(&cmd,
                             "mkdir -p \"%s/\" && "
                             "pixz -x log_gist.raw frags_list <\"%s\" | "
                             "tar x -C \"%s/\"", split_log_path,
                             bundle_path, split_log_path));
        res = system(cmd.ptr);
        if (res != 0)
        {
            fprintf(stderr, "Failed to extract log_gist.raw and "
                    "frags_list\n");
            exit(EXIT_FAILURE);
        }
    }

    FOPEN_FMT(f_raw_gist, "r", "%s/log_gist.raw", split_log_path);
    FOPEN_FMT(f_frags_list, "r", "%s/frags_list", split_log_path);
    if (frags_count_path != NULL)
        FOPEN_FMT(f_frags_count, "w", "%s", frags_count_path);

    f_result = fopen(output_path, "w");
    if (f_result == NULL)
    {
        fprintf(stderr, "Failed to open %s for writing\n", output_path);
        exit(1);
    }

    if (bundle_path != NULL)
    {
        /*
         * Find out which fragment files are needed; unpack them
         * from raw long bundle.
         */

        te_string_reset(&cmd);
        CHECK_TE_RC(te_string_append(&cmd, "pixz -x "));
        if (merge(split_log_path, f_raw_gist, f_frags_list, f_result,
                  NULL, TRUE, &cmd) > 0)
        {
            CHECK_TE_RC(te_string_append(
                             &cmd, " <\"%s\" | "
                             "tar x -C \"%s/\"", bundle_path,
                             split_log_path));

            res = system(cmd.ptr);
            if (res != 0)
            {
                fprintf(stderr, "Failed to extract required fragments\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    merge(split_log_path, f_raw_gist, f_frags_list, f_result,
          f_frags_count, FALSE, NULL);

    RGT_ERROR_SECTION;

    fclose(f_result);
    fclose(f_frags_list);
    fclose(f_frags_count);
    fclose(f_raw_gist);

    free(split_log_path);
    free(output_path);
    free(bundle_path);
    free(frags_count_path);

    te_string_free(&cmd);

    if (RGT_ERROR)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
