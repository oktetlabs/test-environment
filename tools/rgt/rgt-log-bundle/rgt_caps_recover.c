/** @file
 * @brief Test Environment: RAW log bundle processing
 *
 * A tool for reconstructing original sniffer capture files from
 * RAW log bundle.
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
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
#include "te_string.h"
#include "rgt_log_bundle_common.h"

/** Prefix for temporary files */
#define TMP_PREFIX "_tmp_rec_"

/** Where to find unpacked RAW log bundle */
static char *split_log_path = NULL;
/** Where to store recovered sniffer capture files */
static char *caps_path = NULL;

/** Array storing index of PCAP file heads */
static rgt_cap_idx_rec *caps_idx = NULL;
/** Number of elements in the index of heads */
static unsigned int caps_num = 0;
/** File storing PCAP file heads */
static FILE *f_sniff_heads = NULL;

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
    poptContext optCon;
    int rc;
    char *opt_str;

    RGT_ERROR_INIT;

    struct poptOption optionsTable[] = {
        { "split-log", 's', POPT_ARG_STRING, NULL, 's',
          "Path to unpacked raw log.", NULL },

        { "caps", 'c', POPT_ARG_STRING, NULL, 'c',
          "Output directory.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    CHECK_NOT_NULL(optCon = poptGetContext(NULL, argc,
                                           (const char **)argv,
                                           optionsTable, 0));

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 's')
        {
            CHECK_NOT_NULL(opt_str = poptGetOptArg(optCon));
            CHECK_NOT_NULL(split_log_path = strdup(opt_str));
        }
        else if (rc == 'c')
        {
            CHECK_NOT_NULL(opt_str = poptGetOptArg(optCon));
            CHECK_NOT_NULL(caps_path = strdup(opt_str));
        }
        else
        {
            ERROR("Unknown option '-%c'", rc);
            RGT_ERROR_JUMP;
        }
    }

    if (split_log_path == NULL || caps_path == NULL)
    {
        ERROR("Specify all the required parameters");
        RGT_ERROR_JUMP;
    }

    if (rc < -1)
    {
        ERROR("%s: %s",
              poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        poptFreeContext(optCon);
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

/**
 * Process all sniffer fragments related to the single log node.
 * Place contents of every fragment at an approriate offset in
 * the recovered PCAP file to which it belongs.
 *
 * @param base_frag_name      Base fragment name.
 * @param frags_cnt           Number of sniffer fragments to process.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
process_sniff_frags(const char *base_frag_name, uint64_t frags_cnt)
{
    uint64_t i;
    uint32_t file_id;
    uint64_t pkt_offset;
    uint32_t len;
    int rc;
    FILE *f_pcap = NULL;
    FILE *f_frag = NULL;

    RGT_ERROR_INIT;

    for (i = 0; i < frags_cnt; i++)
    {
        CHECK_FOPEN_FMT(f_frag, "r", "%s/%s_sniff_%" PRIu64,
                        split_log_path, base_frag_name, i);

        while (TRUE)
        {
            CHECK_RC(rc = rgt_read_cap_prefix(f_frag, &file_id,
                                              &pkt_offset, &len));
            if (rc == 0)
                break;

            CHECK_FOPEN_FMT(f_pcap, "r+", "%s/%s%u.pcap",
                            caps_path, TMP_PREFIX, file_id);

            CHECK_RC(file2file(f_pcap, f_frag, (off_t)pkt_offset,
                               -1, len));

            CHECK_FCLOSE(f_pcap);
        }

        CHECK_FCLOSE(f_frag);
    }

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_pcap);
    CHECK_FCLOSE(f_frag);

    return RGT_ERROR_VAL;
}

/**
 * Restore heads of original PCAP files (main PCAP header + PCAP header
 * and data of the first (fake) packet).
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
recover_caps_files_heads(void)
{
    unsigned int i;
    FILE *f_head;

    RGT_ERROR_INIT;

    for (i = 0; i < caps_num; i++)
    {
        CHECK_FOPEN_FMT(f_head, "w", "%s/%s%u.pcap", caps_path, TMP_PREFIX,
                        i);

        CHECK_RC(file2file(f_head, f_sniff_heads, -1, -1,
                           caps_idx[i].len));

        CHECK_FCLOSE(f_head);
    }

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
}

/**
 * Recover contents of original PCAP files by looping over all
 * the fragment files and inserting PCAP packets stored in them
 * into proper places in proper files as specified in their
 * prefixes.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
recover_caps_files_contents(void)
{
    FILE *f_frags_list;
    char rec_str[DEF_STR_LEN];
    rgt_frag_rec rec;

    RGT_ERROR_INIT;

    CHECK_FOPEN_FMT(f_frags_list, "r", "%s/frags_list", split_log_path);

    while (!feof(f_frags_list))
    {
        if (fgets(rec_str, sizeof(rec_str), f_frags_list) == NULL)
            break;

        CHECK_RC(rgt_parse_frag_rec(rec_str, &rec));

        if (rec.start_frag && rec.sniff_logs)
            CHECK_RC(process_sniff_frags(rec.frag_name, rec.frags_cnt));
    }

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_frags_list);

    return RGT_ERROR_VAL;
}

/**
 * Rename recovered PCAP files to restore their original names.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
restore_caps_files_names(void)
{
    te_string fsrc = TE_STRING_INIT_STATIC(PATH_MAX);
    te_string fdst = TE_STRING_INIT_STATIC(PATH_MAX);

    FILE *f_names;
    uint32_t file_id;
    char orig_name[DEF_STR_LEN];
    int j;

    RGT_ERROR_INIT;

    CHECK_FOPEN_FMT(f_names, "r", "%s/sniff_fnames", split_log_path);
    file_id = 0;
    while (!feof(f_names))
    {
        if (fgets(orig_name, sizeof(orig_name), f_names) == NULL)
            break;

        j = strlen(orig_name) - 1;
        if (j <= 0)
        {
            ERROR("Empty original file name was encountered");
            RGT_ERROR_JUMP;
        }
        if (orig_name[j] == '\n')
            orig_name[j] = '\0';

        te_string_reset(&fsrc);
        CHECK_TE_RC(te_string_append(&fsrc, "%s/%s%u.pcap",
                                     caps_path, TMP_PREFIX,
                                     (unsigned)file_id));

        te_string_reset(&fdst);
        CHECK_TE_RC(te_string_append(&fdst, "%s/%s", caps_path, orig_name));

        CHECK_OS_RC(rename(fsrc.ptr, fdst.ptr));

        file_id++;
    }

    if (file_id < caps_num)
    {
        ERROR("Not all the capture files got their original names");
        RGT_ERROR_JUMP;
    }

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_names);

    return RGT_ERROR_VAL;
}

int
main(int argc, char **argv)
{
    RGT_ERROR_INIT;

    te_log_init("RGT CAPS RECOVER", te_log_message_file);

    CHECK_RC(process_cmd_line_opts(argc, argv));

    CHECK_RC(rgt_load_caps_idx(split_log_path, &caps_idx, &caps_num,
                               &f_sniff_heads));
    if (caps_num == 0)
        RGT_CLEANUP_JUMP;

    CHECK_RC(recover_caps_files_heads());

    CHECK_RC(recover_caps_files_contents());

    CHECK_RC(restore_caps_files_names());

    RGT_ERROR_SECTION;

    free(split_log_path);
    free(caps_path);
    free(caps_idx);
    CHECK_FCLOSE(f_sniff_heads);

    if (RGT_ERROR)
        exit(EXIT_FAILURE);

    return 0;
}
