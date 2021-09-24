/** @file
 * @brief Test Environment: implementation of raw log fragmentation.
 *
 * Copyright (C) 2016-2021 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <popt.h>

#include <inttypes.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_file.h"
#include "te_str.h"
#include "te_string.h"
#include "te_raw_log.h"
#include "rgt_log_bundle_common.h"

/** Raw log fragment type  */
typedef enum fragment_type {
    FRAG_START,   /**< Starting fragment of log/package/session/test */
    FRAG_INNER,   /**< Inner fragment consisting regular log messages */
    FRAG_END,     /**< Terminating fragment of log/package/session/test */
    FRAG_AFTER,   /**< Fragment for regular log messages which came
                       after the end of the current node but before
                       beginning of the next one */
} fragment_type;

/**
 * We put regular messages belonging to log node N into files
 * N_frag_inner_0
 * N_frag_inner_1
 * ...
 * N_frag_inner_m
 *
 * We append regular log messages to N_frag_inner_0 until its
 * size exceeds MAX_FRAG_SIZE, and after that we start filling
 * N_frag_inner_1, and so on until there is no regular
 * messages left. When there are multiple N_frag_inner_* files,
 * HTML log will be multipaged and each page will be generated from
 * one of these files.
 */
#define MAX_FRAG_SIZE 1000000

/**
 * Maximum number of verdicts per test (only up to this
 * number of verdicts will be included in log_gist.raw).
 */
#define MAX_VERDICTS_NUM 100

/**
 * Processing information for a given log node
 * (log root node/package/session/test)
 */
typedef struct node_info {
    int                    parent;            /**< Parent ID */
    te_bool                opened;            /**< TRUE if we have not yet
                                                   encountered terminating
                                                   fragment */
    int                    opened_children;   /**< Number of opened child
                                                   nodes */
    int                    last_closed_child; /**< ID of the last closed
                                                   child */
    uint64_t               inner_frags_cnt;   /**< Number of inner
                                                   fragments related to
                                                   this node */
    te_bool                after_frag;        /**< If @c TRUE, a fragment
                                                   for messages after end
                                                   is present */
    uint64_t               cur_file_num;      /**< Current inner fragment
                                                   file number */
    off_t                  cur_file_size;     /**< Current inner fragment
                                                   file size */

    unsigned int           tin;               /**< node TIN */
    off_t                  start_len;         /**< Length of start control
                                                   message in starting
                                                   fragment */
    unsigned int           verdicts_num;      /**< Number of verdicts
                                                   included in the start
                                                   fragment */
} node_info;

/** Array of log node descriptions */
static node_info      *nodes_info = NULL;
/** Number of elements in the array of log node descriptions */
static unsigned int    nodes_count = 0;

/**
 * Get processing information stored for a given log node id.
 *
 * @param node_id     Log node id
 *
 * @return Pointer to node_info structure or @c NULL on failure
 */
static node_info *
get_node_info(int node_id)
{
    if (node_id < 0)
    {
        ERROR("%s(): incorrect node_id %d", __FUNCTION__, node_id);
        return NULL;
    }

    while (nodes_count <= (unsigned)node_id)
    {
        unsigned int   i = nodes_count;
        void          *p;

        nodes_count = (nodes_count + 1) * 2;
        p = realloc(nodes_info, nodes_count * sizeof(node_info));
        if (p == NULL)
        {
            ERROR("%s(): not enough memory", __FUNCTION__);
            return NULL;
        }
        nodes_info = (node_info *)p;

        for ( ; i < nodes_count; i++)
        {
            nodes_info[i].parent = -1;
            /* Root node with ID=0 is opened by default */
            nodes_info[i].opened = (i == 0 ? TRUE : FALSE);
            nodes_info[i].opened_children = 0;
            nodes_info[i].last_closed_child = -1;
            nodes_info[i].inner_frags_cnt = 0;
            nodes_info[i].after_frag = FALSE;
            nodes_info[i].cur_file_num = 0;
            nodes_info[i].cur_file_size = 0;
            nodes_info[i].verdicts_num = 0;
        }
    }

    return &nodes_info[node_id];
}

/**
 * Update count of opened children for a parent node.
 *
 * @param parent_id       Parent node ID.
 * @param child_id        Child node ID.
 * @param child_opened    @c TRUE if a child is opened,
 *                        @c FALSE if it is closed.
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
update_children_state(int parent_id, int child_id,
                      te_bool child_opened)
{
    node_info *parent_descr;

    if (parent_id < 0)
        return 0;

    /* This may happen for root node */
    if (parent_id == child_id)
        return 0;

    parent_descr = get_node_info(parent_id);
    if (parent_descr == NULL)
    {
        ERROR("%s(): failed to get parent node", __FUNCTION__);
        return -1;
    }

    if (child_opened)
    {
        parent_descr->opened_children++;
    }
    else
    {
        parent_descr->opened_children--;
        parent_descr->last_closed_child = child_id;
    }

    if (parent_descr->opened_children < 0)
    {
        ERROR("%s(): more children of node %d were closed than were "
              "opened", __FUNCTION__, parent_id);
        return -1;
    }

    return 0;
}

/**
 * In this array current sequential number for each depth
 * is stored.
 */
static unsigned int *depth_seq = NULL;
/**
 * Number of elements for which memory is allocated in
 * depth_seq array.
 */
static unsigned int  depth_levels = 0;

/**
 * Check that depth_seq array is big enough to contain sequential number
 * for a given depth; if it is not, reallocate it.
 *
 * @param depth     Required depth level
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
depth_levels_up_to_depth(unsigned int depth)
{
    unsigned int i = depth_levels;

    while (depth_levels <= depth)
    {
        void *p;

        depth_levels = (depth_levels + 1) * 2;
        p = realloc(depth_seq, depth_levels * sizeof(unsigned int));
        if (p == NULL)
        {
            ERROR("%s(): not enough memory for depth_seq array",
                  __FUNCTION__);
            return -1;
        }
        depth_seq = (unsigned int *)p;
    }

    for ( ; i < depth_levels; i++)
        depth_seq[i] = 0;

    return 0;
}

/*
 * These variables are used to create recover_list file
 * with help of which original raw log may be recovered from
 * raw log fragments.
 */

/** Last frament file name to which new data was added */
static char cur_block_frag_name[DEF_STR_LEN] = "";

/*
 * We wait while a block of consecutive log messages written
 * to the same fragment file in the same order continues,
 * and when another such block starts, we write information
 * about the finished block in recover_list file.
 */

/**
 * Offset in raw log of the current block of consecutive log messages
 * written to the same log fragment file.
 */
static off_t cur_block_offset = -1;
/**
 * Length of the current block of consecutive chuncks
 * written to the same log fragment file.
 */
static off_t cur_block_length = 0;
/**
 * Offset in log fragment file of the current block of consecutive
 * log messages written to the same log fragment file.
 */
static off_t cur_block_frag_offset = -1;
/** Offset of the last processed message in raw log */
static off_t last_msg_offset = -1;

/**
 * Append a new log message to appropriate log fragment file.
 *
 * @param node_id         Log node id to which the message belongs
 * @param frag_type       Fragment type (starting, inner or terminating)
 * @param f_raw_log       Raw log file
 * @param offset          Offset of the message in the raw log
 * @param length          Length of the message
 * @param f_recover       File storing information about how to recover
 *                        original raw log from fragments
 * @param output_path     Where to store log fragment files
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
append_to_frag(int node_id, fragment_type frag_type,
               FILE *f_raw_log,
               int64_t offset, int64_t length,
               FILE *f_recover,
               const char *output_path)
{
    FILE *f_frag;
    node_info *node_descr;

    RGT_ERROR_INIT;

    te_string frag_name = TE_STRING_INIT;

    CHECK_TE_RC(te_string_append(&frag_name, "%d_frag_",
                                 node_id));
    switch (frag_type)
    {
        case FRAG_START:
            CHECK_TE_RC(te_string_append(&frag_name, "%s", "start"));
            break;

        case FRAG_INNER:
        {
            CHECK_NOT_NULL(node_descr = get_node_info(node_id));

            if (node_descr->inner_frags_cnt == 0)
                node_descr->inner_frags_cnt = 1;
            if (node_descr->cur_file_size > MAX_FRAG_SIZE)
            {
                node_descr->inner_frags_cnt++;
                node_descr->cur_file_num++;
                node_descr->cur_file_size = length;
            }
            else
                node_descr->cur_file_size += length;

            CHECK_TE_RC(te_string_append(&frag_name, "inner_%" PRIu64,
                                         node_descr->cur_file_num));
            break;
        }

        case FRAG_END:
            CHECK_TE_RC(te_string_append(&frag_name, "%s", "end"));
            break;

        case FRAG_AFTER:
            CHECK_NOT_NULL(node_descr = get_node_info(node_id));

            node_descr->after_frag = TRUE;

            CHECK_TE_RC(te_string_append(&frag_name, "%s", "after"));
            break;
    }

    CHECK_FOPEN_FMT(f_frag, "a", "%s/%s", output_path, frag_name.ptr);

    if (cur_block_offset < 0)
    {
        cur_block_offset = offset;
        cur_block_length = length;
        cur_block_frag_offset = ftello(f_frag);
        CHECK_TE_RC(te_snprintf(cur_block_frag_name,
                                sizeof(cur_block_frag_name),
                                "%s", frag_name.ptr));
    }
    else
    {
        if (last_msg_offset != offset)
        {
            if (cur_block_offset + cur_block_length == offset &&
                strcmp(frag_name.ptr, cur_block_frag_name) == 0)
            {
                cur_block_length += length;
            }
            else
            {
                if (fprintf(f_recover, "%llu %llu %s %llu\n",
                            (long long unsigned int)cur_block_offset,
                            (long long unsigned int)cur_block_length,
                            cur_block_frag_name,
                            (long long unsigned int)
                                  cur_block_frag_offset) < 0)
                {
                    ERROR("%s(): fprintf() failed", __FUNCTION__);
                    RGT_ERROR_JUMP;
                }

                cur_block_offset = offset;
                cur_block_length = length;
                cur_block_frag_offset = ftello(f_frag);
                CHECK_TE_RC(te_snprintf(cur_block_frag_name,
                                        sizeof(cur_block_frag_name),
                                        "%s", frag_name.ptr));
            }
        }
    }
    last_msg_offset = offset;

    CHECK_RC(file2file(f_frag, f_raw_log, -1, offset, length));

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_frag);
    te_string_free(&frag_name);

    return RGT_ERROR_VAL;
}

/**
 * Split raw log into fragments.
 *
 * @param f_raw_log         Raw log file
 * @param f_index           File with information about raw log messages
 *                          (their length, offset, etc)
 * @param f_recover         File where to store information allowing to
 *                          restore original raw log from its fragments
 * @param output_path       Where to store raw log fragments
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
split_raw_log(FILE *f_raw_log, FILE *f_index, FILE *f_recover,
              const char *output_path)
{
    int           parent_id;
    int           node_id;
    int64_t       offset;
    int64_t       length;
    unsigned int  tin_or_start_frag;
    unsigned int  timestamp[2];
    long int      line = 0;
    off_t         raw_fp;

    fragment_type frag_type;

    char  str[DEF_STR_LEN];
    char  msg_type[DEF_STR_LEN];
    char  node_type[DEF_STR_LEN];
    int   rc;

    RGT_ERROR_INIT;

    /* Make sure root node is opened */
    CHECK_NOT_NULL(get_node_info(0));

    while (!feof(f_index))
    {
        if (fgets(str, sizeof(str), f_index) == NULL)
        {
            if (feof(f_index))
                break;

            ERROR("fgets() returned NULL unexpectedly at line %ld\n",
                  __FUNCTION__, line);
            RGT_ERROR_JUMP;
        }

        rc = sscanf(str, "%u.%u %" PRId64 " %d %d %s %u %s %" PRId64,
                    &timestamp[0], &timestamp[1],
                    &offset, &parent_id, &node_id,
                    msg_type, &tin_or_start_frag, node_type, &length);
        if (rc < 8)
        {
            ERROR("Wrong record in raw log index at line %ld", line);
            RGT_ERROR_JUMP;
        }
        if (rc < 9)
        {
            CHECK_OS_RC(raw_fp = ftello(f_raw_log));
            CHECK_OS_RC(fseeko(f_raw_log, 0LL, SEEK_END));
            CHECK_OS_RC(length = ftello(f_raw_log));
            CHECK_OS_RC(fseeko(f_raw_log, raw_fp, SEEK_SET));
            length -= offset;
        }

        if (strcmp(msg_type, "REGULAR") == 0)
            frag_type = FRAG_INNER;
        else if (strcmp(msg_type, "END") == 0)
        {
            node_info *node_descr;

            frag_type = FRAG_END;

            CHECK_NOT_NULL(node_descr = get_node_info(node_id));
            node_descr->opened = FALSE;

            CHECK_RC(update_children_state(parent_id, node_id, FALSE));
        }
        else
        {
            node_info *node_descr;

            frag_type = FRAG_START;

            CHECK_NOT_NULL(node_descr = get_node_info(node_id));

            node_descr->opened = TRUE;
            CHECK_RC(update_children_state(parent_id, node_id, TRUE));

            node_descr->tin = tin_or_start_frag;
            node_descr->start_len = length;

            node_descr->parent = parent_id;
        }

        if (frag_type == FRAG_INNER)
        {
            if (parent_id != TE_LOG_ID_UNDEFINED)
            {
                CHECK_RC(append_to_frag(parent_id, frag_type, f_raw_log,
                                        offset, length,
                                        f_recover, output_path));
            }
            else
            {
                long unsigned int j;
                te_bool           matching_frag_found = FALSE;

                /*
                 * Logs from TE components (such as Configurator) do not
                 * have ID of any specific test and should be attached to
                 * all currently opened nodes (tests, sessions, packages)
                 * not having opened children (to avoid attaching log both
                 * to a test and to a session including it). Multiple such
                 * nodes may be opened simultaneously if multiple tests are
                 * run in parallel.
                 */
                for (j = 0; j < nodes_count; j++)
                {
                    if (nodes_info[j].opened &&
                        nodes_info[j].opened_children == 0)
                    {
                        if (nodes_info[j].last_closed_child >= 0)
                        {
                            CHECK_RC(append_to_frag(
                                           nodes_info[j].last_closed_child,
                                           FRAG_AFTER, f_raw_log,
                                           offset, length,
                                           f_recover, output_path));
                        }
                        else
                        {
                            CHECK_RC(append_to_frag(
                                               j, frag_type, f_raw_log,
                                               offset, length,
                                               f_recover, output_path));
                        }
                        matching_frag_found = TRUE;
                    }
                }

                if (!matching_frag_found)
                {
                    ERROR("Failed to find fragment for a message "
                          "with offset %" PRIu64, offset);
                    RGT_ERROR_JUMP;
                }
            }

            if (tin_or_start_frag)
            {
                node_info *node_descr;

                CHECK_NOT_NULL(node_descr = get_node_info(parent_id));

                if (node_descr->verdicts_num < MAX_VERDICTS_NUM)
                {
                    CHECK_RC(append_to_frag(parent_id, FRAG_START,
                                            f_raw_log, offset, length,
                                            f_recover, output_path));

                    node_descr->verdicts_num++;
                }
            }
        }
        else
        {
            CHECK_RC(append_to_frag(node_id, frag_type, f_raw_log,
                                    offset, length, f_recover,
                                    output_path));
        }

        line++;
    }

    if (cur_block_offset >= 0)
    {
        if (fprintf(f_recover, "%llu %llu %s %llu\n",
                    (long long unsigned int)cur_block_offset,
                    (long long unsigned int)cur_block_length,
                    cur_block_frag_name,
                    (long long unsigned int)cur_block_frag_offset) < 0)
        {
            ERROR("%s(): fprintf() failed", __FUNCTION__);
            RGT_ERROR_JUMP;
        }
    }

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
}

/**
 * Get file length.
 *
 * @param f     File pointer
 *
 * @return Length of file on success, @c -1 on failure
 */
static off_t
file_length(FILE *f)
{
    off_t cur_pos;
    off_t length;

    RGT_ERROR_INIT;

    CHECK_OS_RC(cur_pos = ftello(f));
    CHECK_OS_RC(fseeko(f, 0LLU, SEEK_END));
    CHECK_OS_RC(length = ftello(f));
    CHECK_OS_RC(fseeko(f, cur_pos, SEEK_SET));

    RGT_ERROR_SECTION;

    if (RGT_ERROR)
        return -1;

    return length;
}

/**
 * Print list of all the fragments (in correct order) to specified file;
 * append starting and terminating fragments to "raw gist" log.
 *
 * @param output_path         Where to store log fragments
 * @param f_raw_gist          Where to save "raw gist log"
 * @param f_frags_list        Where to save list of log fragments
 * @param node_id             Id of log node whose children to enumerate
 * @param depth               Depth of current log node
 * @param seq                 Sequence number of current log node
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
print_frags_list(const char *output_path, FILE *f_raw_gist,
                 FILE *f_frags_list, int node_id,
                 unsigned int depth, unsigned int seq)
{
    unsigned int i;

    FILE      *f_frag;
    te_string  path = TE_STRING_INIT;
    off_t      frag_len;

    RGT_ERROR_INIT;

    CHECK_RC(depth_levels_up_to_depth(depth));

    CHECK_FOPEN_FMT(f_frag, "r", "%s/%d_frag_start",
                    output_path, node_id);

    CHECK_RC(frag_len = file_length(f_frag));
    CHECK_RC(file2file(f_raw_gist, f_frag, -1, -1, frag_len));

    CHECK_FCLOSE(f_frag);

    if (fprintf(f_frags_list,
                "%d_frag_start %u %u %u %llu %llu %" PRIu64 " %d\n",
                node_id, nodes_info[node_id].tin, depth, seq,
                (long long unsigned int)frag_len,
                (long long unsigned int)nodes_info[node_id].start_len,
                nodes_info[node_id].inner_frags_cnt,
                nodes_info[node_id].parent) < 0)
    {
        ERROR("fprintf() failed");
        RGT_ERROR_JUMP;
    }

    for (i = node_id + 1; i < nodes_count; i++)
    {
        if (nodes_info[i].parent == node_id)
        {
            CHECK_RC(print_frags_list(output_path, f_raw_gist,
                                      f_frags_list, i,
                                      depth + 1, depth_seq[depth]));
            depth_seq[depth]++;
        }
    }

    CHECK_TE_RC(te_string_append(&path, "%s/%d_frag_end",
                                 output_path, node_id));
    f_frag = fopen(path.ptr, "r");
    if (f_frag == NULL && errno != ENOENT)
    {
        ERROR("Failed to open '%s' for reading, errno = %d ('%s')",
              path.ptr, errno, strerror(errno));
        RGT_ERROR_JUMP;
    }

    if (f_frag != NULL)
    {
        CHECK_RC(frag_len = file_length(f_frag));
        CHECK_RC(file2file(f_raw_gist, f_frag, -1, -1, frag_len));
        CHECK_RC(fclose(f_frag));

        if (fprintf(f_frags_list, "%d_frag_end %u %u %u %llu %d %d\n",
                    node_id, nodes_info[node_id].tin, depth, seq,
                    (long long unsigned int)frag_len,
                    (nodes_info[node_id].after_frag ? 1 : 0),
                    nodes_info[node_id].parent) < 0)
        {
            ERROR("fprintf() failed");
            RGT_ERROR_JUMP;
        }
    }

    RGT_ERROR_SECTION;

    te_string_free(&path);

    return RGT_ERROR_VAL;
}

/** Path to raw log to be split */
static char *raw_log_path = NULL;
/**
 * Path to file where information about log messaged in raw
 * log is stored
*/
static char *index_path = NULL;
/** Where to store log fragments */
static char *output_path = NULL;

/**
 * Parse command line.
 *
 * @param argc    Number of arguments
 * @param argv    Array of command line arguments
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon; /* Context for parsing command-line options */
    int          rc;

    RGT_ERROR_INIT;

    /* Option Table */
    struct poptOption optionsTable[] = {
        { "raw-log", 'r', POPT_ARG_STRING, NULL, 'r',
          "Path to raw log.", NULL },

        { "log-index", 'i', POPT_ARG_STRING, NULL, 'i',
          "Path to raw log index file.", NULL },

        { "output-dir", 'o', POPT_ARG_STRING, NULL, 'o',
          "Output directory.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            optionsTable, 0);

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 'r')
            raw_log_path = poptGetOptArg(optCon);
        else if (rc == 'i')
            index_path = poptGetOptArg(optCon);
        else if (rc == 'o')
            output_path = poptGetOptArg(optCon);
    }

    if (raw_log_path == NULL || index_path == NULL || output_path == NULL)
    {
        poptPrintUsage(optCon, stderr, 0);
        ERROR("Specify all the required parameters");
        RGT_ERROR_JUMP;
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s\n",
              poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        RGT_ERROR_JUMP;
    }

    if (poptPeekArg(optCon) != NULL)
    {
        poptPrintUsage(optCon, stderr, 0);
        ERROR("Too many parameters specified");
        RGT_ERROR_JUMP;
    }

    RGT_ERROR_SECTION;

    poptFreeContext(optCon);
    return RGT_ERROR_VAL;
}

int
main(int argc, char **argv)
{
    FILE *f_raw_log = NULL;
    FILE *f_index = NULL;
    FILE *f_recover = NULL;
    FILE *f_frags_list = NULL;
    FILE *f_raw_gist = NULL;

    RGT_ERROR_INIT;

    te_log_init("RGT LOG SPLIT", te_log_message_file);

    CHECK_RC(process_cmd_line_opts(argc, argv));

    CHECK_FOPEN(f_raw_log, raw_log_path, "r");
    CHECK_FOPEN(f_index, index_path, "r");

    CHECK_FOPEN_FMT(f_recover, "w", "%s/recover_list", output_path);

    CHECK_RC(split_raw_log(f_raw_log, f_index, f_recover, output_path));

    CHECK_FOPEN_FMT(f_frags_list, "w", "%s/frags_list", output_path);

    CHECK_FOPEN_FMT(f_raw_gist, "w", "%s/log_gist.raw", output_path);

    CHECK_RC(print_frags_list(output_path, f_raw_gist, f_frags_list,
                              0, 1, 0));

    RGT_ERROR_SECTION;

    CHECK_FCLOSE(f_raw_gist);
    CHECK_FCLOSE(f_frags_list);
    CHECK_FCLOSE(f_raw_log);
    CHECK_FCLOSE(f_index);
    CHECK_FCLOSE(f_recover);

    free(nodes_info);

    free(output_path);
    free(index_path);
    free(raw_log_path);

    if (RGT_ERROR)
        return EXIT_FAILURE;

    return 0;
}
