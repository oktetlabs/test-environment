/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: splitting raw log.
 *
 * Commong functions for splitting raw log into fragments and
 * merging fragments back into raw log.
 *
 *
 * Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <popt.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "te_errno.h"
#include "te_string.h"

#include "rgt_log_bundle_common.h"

/* See the description in rgt_log_bundle_common.h */
int
file2file(FILE *out_f, FILE *in_f,
          off_t out_offset,
          off_t in_offset, off_t length)
{
#define BUF_SIZE  4096
    char   buf[BUF_SIZE];
    size_t bytes_read;

    RGT_ERROR_INIT;

    if (out_offset >= 0)
        CHECK_OS_RC(fseeko(out_f, out_offset, SEEK_SET));
    if (in_offset >= 0)
        CHECK_OS_RC(fseeko(in_f, in_offset, SEEK_SET));

    while (length > 0)
    {
        bytes_read = (length > BUF_SIZE ? BUF_SIZE : length);
        CHECK_FREAD(buf, 1, bytes_read, in_f);

        CHECK_FWRITE(buf, 1, bytes_read, out_f);
        length -= bytes_read;

        if (feof(in_f))
            break;
    }

    if (length > 0)
    {
        ERROR("Failed to copy last %llu bytes to file",
              (long long unsigned int)length);
        RGT_ERROR_JUMP;
    }

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
#undef BUF_SIZE
}

/* See the description in rgt_log_bundle_common.h */
int
rgt_load_caps_idx(const char *split_log_path,
                  rgt_cap_idx_rec **idx_out,
                  unsigned int *idx_len_out,
                  FILE **f_heads_out)
{
    rgt_cap_idx_rec *idx = NULL;
    uint64_t len = 0;
    uint64_t cnt = 0;
    FILE *f = NULL;

    te_string idx_path = TE_STRING_INIT;
    struct stat st;
    int rc;

    RGT_ERROR_INIT;

    CHECK_TE_RC(te_string_append(&idx_path, "%s/sniff_heads_idx",
                                 split_log_path));
    rc = stat(te_string_value(&idx_path), &st);
    if (rc < 0)
    {
        /* Bundle may simply not include capture files. */
        if (errno == ENOENT)
            RGT_CLEANUP_JUMP;

        ERROR("stat(%s) fails with error %d (%s)",
              te_string_value(&idx_path),
              errno, strerror(errno));
        RGT_ERROR_JUMP;
    }

    CHECK_FOPEN(f, te_string_value(&idx_path), "r");

    CHECK_OS_RC(fseek(f, 0L, SEEK_END));
    CHECK_OS_RC(len = ftello(f));
    CHECK_OS_RC(fseek(f, 0L, SEEK_SET));

    if (len == 0)
        RGT_CLEANUP_JUMP;

    if (len % sizeof(rgt_cap_idx_rec) != 0)
    {
        ERROR("Length of the sniff_heads_idx file is not multiple of "
              "expected record length");
        RGT_ERROR_JUMP;
    }

    cnt = len / sizeof(rgt_cap_idx_rec);

    if (cnt > (uint64_t)UINT_MAX)
    {
        ERROR("Length of the sniff_heads_idx file is too big");
        RGT_ERROR_JUMP;
    }

    CHECK_OS_NOT_NULL(idx = calloc(1, len));

    if (fread(idx, len, 1, f) != 1)
    {
        ERROR("Failed to read sniff_heads_idx file");
        RGT_ERROR_JUMP;
    }

    CHECK_FOPEN_FMT(*f_heads_out, "r", "%s/sniff_heads", split_log_path);

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f);
    te_string_free(&idx_path);

    if (RGT_ERROR)
    {
        CHECK_FCLOSE(*f_heads_out);
        free(idx);
    }
    else
    {
        *idx_out = idx;
        *idx_len_out = cnt;
    }

    return RGT_ERROR_VAL;
}

/* See the description in rgt_log_bundle_common.h */
int
rgt_parse_frag_rec(const char *s, rgt_frag_rec *rec)
{
    int scanf_rc;

    uint64_t f1;
    uint64_t f2;
    uint64_t f3;
    int f4;

    char *name_suff;

    memset(rec, 0, sizeof(*rec));

    scanf_rc = sscanf(
                 s, "%s %u %d %d %" PRIu64 " %" PRIu64 " %" PRIu64 " %"
                 PRIu64 " %d", rec->frag_name, &rec->tin, &rec->depth,
                 &rec->seq, &rec->length, &f1, &f2, &f3, &f4);
    if (scanf_rc < 5)
    {
        ERROR("sscanf() read too few fragment parameters in '%s' (%d)",
              s, scanf_rc);
        return -1;
    }

    name_suff = NULL;
    if ((name_suff = strstr(rec->frag_name, "_end")) != NULL)
    {
        rec->start_frag = FALSE;
        if (scanf_rc >= 7)
        {
            /*
             * These fields are present only in newer version of
             * RAW log bundle.
             */
            rec->frags_cnt = f1;
            rec->parent_id = f2;
        }
    }
    else if ((name_suff = strstr(rec->frag_name, "_start")) != NULL)
    {
        if (scanf_rc < 7)
        {
            ERROR("Too few parameters in '%s'", s);
            return -1;
        }
        rec->start_frag = TRUE;
        rec->start_len = f1;
        rec->frags_cnt = f2;
        rec->parent_id = f3;

        if (scanf_rc >= 9)
            rec->sniff_logs = (f4 != 0);
    }
    else
    {
        ERROR("Unknown fragment type '%s'", rec->frag_name);
        return -1;
    }

    if (sscanf(rec->frag_name, "%u_", &rec->test_id) <= 0)
    {
        ERROR("sscanf() could not parse node ID in %s", rec->frag_name);
        return -1;
    }

    /* Remove _start or _end suffix from fragment name. */
    *name_suff = '\0';
    return 0;
}

/* See the description in rgt_log_bundle_common.h */
int
rgt_read_cap_prefix(FILE *f, uint32_t *file_id,
                    uint64_t *pkt_offset, uint32_t *len)
{
    size_t rc;

    RGT_ERROR_INIT;

    rc = fread(file_id, sizeof(*file_id), 1, f);
    if (rc != 1)
    {
        if (feof(f))
        {
            return 0;
        }
        else
        {
            ERROR("%s(): failed to read the first byte of sniffer prefix",
                  __FUNCTION__);
            return -1;
        }
    }

    CHECK_FREAD(pkt_offset, sizeof(*pkt_offset), 1, f);
    CHECK_FREAD(len, sizeof(*len), 1, f);

    RGT_ERROR_SECTION;

    if (RGT_ERROR)
        return -1;

    return 1;
}
