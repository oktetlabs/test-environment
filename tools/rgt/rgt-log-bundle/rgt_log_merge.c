/** @file
 * @brief Test Environment: implementation of merging fragments into raw log.
 *
 * Copyright (C) 2016-2022 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
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

/** Index of capture file "heads" */
static rgt_cap_idx_rec *caps_idx = NULL;
/** Array of FILE pointers for opened capture files */
static FILE **caps_files = NULL;
/** Number of capture files */
static unsigned int caps_num = 0;
/** Opened file with PCAP files "heads" */
static FILE *f_sniff_heads = NULL;

/**
 * Load index of PCAP files "heads", open file storing those "heads".
 *
 * @param split_log_path      Path to unpacked RAW log bundle.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
load_caps_idx(const char *split_log_path)
{
    RGT_ERROR_INIT;

    CHECK_RC(rgt_load_caps_idx(split_log_path, &caps_idx, &caps_num,
                               &f_sniff_heads));
    CHECK_OS_NOT_NULL(caps_files = calloc(caps_num, sizeof(FILE *)));

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
}

/**
 * Process a fragment file containing sniffed network packets,
 * copying these packets to corresponding sniffer files
 * (there may be packets from different sniffers in such a fragment
 * file).
 *
 * @param fpath             Path to the fragment file.
 * @param sniff_path        Where to save reconstructed sniffer capture
 *                          files.
 * @param split_log_path    Path to unpacked RAW log bundle.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
process_sniff_frag(const char *fpath, const char *sniff_path,
                   const char *split_log_path)
{
    FILE *f;
    uint32_t file_id;
    uint64_t pkt_offset;
    uint32_t len;
    size_t rc;
    void *data = NULL;

    RGT_ERROR_INIT;

    if (caps_idx == NULL)
        CHECK_RC(load_caps_idx(split_log_path));

    CHECK_FOPEN(f, fpath, "r");

    while (TRUE)
    {
        CHECK_RC(rc = rgt_read_cap_prefix(f, &file_id, &pkt_offset, &len));
        if (rc == 0)
            break;

        if (caps_files[file_id] == NULL)
        {
            CHECK_FOPEN_FMT(caps_files[file_id], "w", "%s/%u.pcap",
                            sniff_path, (unsigned int)file_id);

            CHECK_RC(file2file(caps_files[file_id], f_sniff_heads, -1,
                               caps_idx[file_id].pos,
                               caps_idx[file_id].len));
        }

        CHECK_OS_NOT_NULL(data = calloc(1, len));
        CHECK_FREAD(data, len, 1, f);
        CHECK_FWRITE(data, len, 1, caps_files[file_id]);
        free(data);
        data = NULL;
    }

    RGT_ERROR_SECTION;

    free(data);
    CHECK_FCLOSE(f);

    return RGT_ERROR_VAL;
}

/**
 * Open log fragment file and copy all its contents into
 * resulting file.
 *
 * @param f           File to where to copy contents.
 * @param frag_path   Path to the file with log fragment to append.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
append_frag_to_file(FILE *f, const char *frag_path)
{
    FILE *f_frag = NULL;
    off_t frag_len;

    RGT_ERROR_INIT;

    CHECK_FOPEN(f_frag, frag_path, "r");

    CHECK_OS_RC(fseeko(f_frag, 0LLU, SEEK_END));
    CHECK_OS_RC(frag_len = ftello(f_frag));
    CHECK_OS_RC(fseeko(f_frag, 0LLU, SEEK_SET));

    CHECK_RC(file2file(f, f_frag, -1, -1, frag_len));

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_frag);

    return RGT_ERROR_VAL;
}

/**
 * Merge inner log fragments into "raw gist log" consisting
 * of starting and terminating fragments only.
 *
 * @param split_log_path      Where log fragments are to be found
 * @param sniff_path          Where to find sniffer capture files
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
 *         @c TRUE; @c 0 otherwise. On failure this function returns @c -1.
 */
static int
merge(const char *split_log_path, const char *sniff_path,
      FILE *f_raw_gist, FILE *f_frags_list, FILE *f_result,
      FILE *f_frags_count, te_bool get_needed_frags,
      te_string *needed_frags)
{
    rgt_frag_rec rec;
    char s[DEF_STR_LEN];
    char frag_path[DEF_STR_LEN];

    uint64_t cum_length = 0;
    int64_t target_node_id = -1;
    te_bool sniff_logs_needed = FALSE;

    uint64_t i;
    int needed_frags_cnt = 0;

    off_t raw_fp;

    RGT_ERROR_INIT;

    assert(f_raw_gist != NULL && f_frags_list != NULL && f_result != NULL);

    while (!feof(f_frags_list))
    {
        if (fgets(s, sizeof(s), f_frags_list) == NULL)
            break;

        CHECK_RC(rgt_parse_frag_rec(s, &rec));

        cum_length += rec.length;

        if (!rec.start_frag && target_node_id >= 0 &&
            (unsigned int)target_node_id == rec.parent_id &&
            rec.frags_cnt != 0)
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
                                             rec.frag_name));
                needed_frags_cnt++;
            }
            else
            {
                TE_SPRINTF(frag_path, "%s/%s_after", split_log_path,
                           rec.frag_name);

                CHECK_RC(file2file(f_result, f_raw_gist, -1, -1,
                                   cum_length));
                cum_length = 0;
                CHECK_RC(append_frag_to_file(f_result, frag_path));
            }
        }
        else if (rec.start_frag &&
                 ((use_tin && filter_tin == rec.tin) ||
                  (use_test_id && filter_test_id == rec.test_id) ||
                  (!use_tin && !use_test_id &&
                   filter_depth == rec.depth && filter_seq == rec.seq)))
        {
            target_node_id = rec.test_id;

            if (!get_needed_frags)
            {
                /*
                 * In "gist" raw log starting control message goes together
                 * with verdicts and artifacts. Here we leave starting
                 * message but replace verdicts and artifacts with full log
                 * for a target node.
                 */

                cum_length -= (rec.length - rec.start_len);
                CHECK_RC(file2file(f_result, f_raw_gist, -1, -1,
                                   cum_length));
                CHECK_OS_RC(raw_fp = ftello(f_raw_gist));
                raw_fp += (rec.length - rec.start_len);
                CHECK_OS_RC(fseeko(f_raw_gist, raw_fp, SEEK_SET));
                cum_length = 0;

                if (f_frags_count != NULL)
                {
                    CHECK_RC(fprintf(f_frags_count, "%llu",
                                     (long long unsigned)(rec.frags_cnt)));
                    /*
                     * This should never happen, however if multiple
                     * records matched, save number of fragments for
                     * the first of them.
                     */
                    f_frags_count = NULL;
                }
            }

            for (i = 0; i < rec.frags_cnt; i++)
            {
                if (filter_frag_num >= 0)
                    i = filter_frag_num;

                if (get_needed_frags)
                {
                    CHECK_TE_RC(te_string_append(needed_frags,
                                                 " %s_inner_%" PRIu64,
                                                 rec.frag_name, i));
                    if (rec.sniff_logs && sniff_path != NULL)
                    {
                        CHECK_TE_RC(te_string_append(
                                               needed_frags,
                                               " %s_sniff_%" PRIu64,
                                               rec.frag_name, i));
                        sniff_logs_needed = TRUE;
                    }

                    needed_frags_cnt++;
                }
                else
                {
                    TE_SPRINTF(frag_path,
                               "%s/%s_inner_%" PRIu64,
                               split_log_path, rec.frag_name, i);

                    CHECK_RC(append_frag_to_file(f_result, frag_path));

                    if (rec.sniff_logs && sniff_path != NULL)
                    {
                        TE_SPRINTF(frag_path,
                                   "%s/%s_sniff_%" PRIu64,
                                   split_log_path, rec.frag_name, i);

                        CHECK_RC(process_sniff_frag(frag_path, sniff_path,
                                                    split_log_path));
                    }
                }

                if (filter_frag_num >= 0)
                    break;
            }
        }
    }

    if (get_needed_frags)
    {
        if (sniff_logs_needed)
        {
            CHECK_TE_RC(te_string_append(
                               needed_frags,
                               " sniff_heads sniff_heads_idx"));
        }

        CHECK_OS_RC(fseeko(f_frags_list, 0, SEEK_SET));
        return needed_frags_cnt;
    }

    if (cum_length > 0)
        CHECK_RC(file2file(f_result, f_raw_gist, -1, -1, cum_length));

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
}

/** Where to find raw log bundle  */
static char *bundle_path = NULL;
/** Where to find raw log fragments and "raw gist" log  */
static char *split_log_path = NULL;
/** Path to sniffer capture files directory */
static char *sniff_path = NULL;
/** Where to save number of log fragments in the requested node */
static char *frags_count_path = NULL;
/** Where to store merged raw log */
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
        { "bundle", 'b', POPT_ARG_STRING, NULL, 'b',
          "Path to raw log bundle (required if bundle was not unpacked).",
          NULL },

        { "split-log", 's', POPT_ARG_STRING, NULL, 's',
          "Path to split raw log.", NULL },

        { "sniff-log-dir", 'd', POPT_ARG_STRING, NULL, 'd',
          "Where to find sniffer capture files.", NULL },

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
    CHECK_NOT_NULL(optCon = poptGetContext(NULL, argc,
                                           (const char **)argv,
                                           optionsTable, 0));

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
            {
                ERROR("--filter value is not specified");
                RGT_ERROR_JUMP;
            }

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
            {
                ERROR("--page value is not specified");
                RGT_ERROR_JUMP;
            }

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
        else if (rc == 'd')
        {
            sniff_path = poptGetOptArg(optCon);
        }
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
    {
        if (RGT_ERROR)
            poptPrintUsage(optCon, stderr, 0);

        poptFreeContext(optCon);
    }

    return RGT_ERROR_VAL;
}

int
main(int argc, char **argv)
{
    FILE *f_raw_gist = NULL;
    FILE *f_frags_list = NULL;
    FILE *f_frags_count = NULL;
    FILE *f_result = NULL;

    unsigned int i;

    te_string cmd = TE_STRING_INIT;
    int res;

    RGT_ERROR_INIT;

    te_log_init("RGT LOG MERGE", te_log_message_file);

    CHECK_RC(process_cmd_line_opts(argc, argv));

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
            ERROR("Failed to extract log_gist.raw and frags_list");
            RGT_ERROR_JUMP;
        }
    }

    CHECK_FOPEN_FMT(f_raw_gist, "r", "%s/log_gist.raw", split_log_path);
    CHECK_FOPEN_FMT(f_frags_list, "r", "%s/frags_list", split_log_path);
    if (frags_count_path != NULL)
        CHECK_FOPEN_FMT(f_frags_count, "w", "%s", frags_count_path);

    CHECK_FOPEN(f_result, output_path, "w");

    if (bundle_path != NULL)
    {
        /*
         * Find out which fragment files are needed; unpack them
         * from raw long bundle.
         */

        te_string_reset(&cmd);
        CHECK_TE_RC(te_string_append(&cmd, "pixz -x "));
        CHECK_RC(res = merge(split_log_path, sniff_path, f_raw_gist,
                             f_frags_list, f_result, NULL, TRUE, &cmd));

        if (res > 0)
        {
            CHECK_TE_RC(te_string_append(
                             &cmd, " <\"%s\" | "
                             "tar x -C \"%s/\"", bundle_path,
                             split_log_path));

            res = system(cmd.ptr);
            if (res != 0)
            {
                ERROR("Failed to extract required fragments");
                RGT_ERROR_JUMP;
            }
        }
    }

    CHECK_RC(merge(split_log_path, sniff_path, f_raw_gist, f_frags_list,
                   f_result, f_frags_count, FALSE, NULL));

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_result);
    CHECK_FCLOSE(f_frags_list);
    CHECK_FCLOSE(f_frags_count);
    CHECK_FCLOSE(f_raw_gist);
    CHECK_FCLOSE(f_sniff_heads);

    if (caps_files != NULL)
    {
        for (i = 0; i < caps_num; i++)
            CHECK_FCLOSE(caps_files[i]);
    }

    free(split_log_path);
    free(sniff_path);
    free(output_path);
    free(bundle_path);
    free(frags_count_path);

    te_string_free(&cmd);

    free(caps_idx);
    free(caps_files);

    if (RGT_ERROR)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
