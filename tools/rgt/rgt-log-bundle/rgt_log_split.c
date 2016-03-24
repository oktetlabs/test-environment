/** @file
 * @brief Test Environment: implementation of raw log fragmentation.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
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
#include "te_string.h"
#include "te_raw_log.h"
#include "rgt_log_bundle_common.h"

DEFINE_LGR_ENTITY("RGT LOG SPLIT");

/** Raw log fragment type  */
typedef enum fragment_type {
    FRAG_START,   /**< Starting fragment of log/package/session/test */
    FRAG_INNER,   /**< Inner fragment consisting regular log messages */
    FRAG_END,     /**< Terminating fragment of log/package/session/test */
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
 * Processing information for a given log node
 * (log root node/package/session/test)
 */
typedef struct node_info {
    int                    parent;            /**< Parent ID */
    te_bool                opened;            /**< TRUE if we have not yet
                                                   encountered terminating
                                                   fragment */
    long unsigned int      open_chld_cnt;     /**< Number of child nodes
                                                   for which terminating
                                                   fragment has not been
                                                   encountered yet*/
    uint64_t               inner_frags_cnt;   /**< Number of inner
                                                   fragments related to
                                                   this node */
    uint64_t               cur_file_num;      /**< Current inner fragment
                                                   file number */
    off_t                  cur_file_size;     /**< Current inner fragment
                                                   file size */

    unsigned int           tin;               /**< node TIN */
    off_t                  start_len;         /**< Length of start control
                                                   message in starting
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
 * @return Pointer to node_info structure
 */
static node_info *
get_node_info(int node_id)
{
    if (node_id < 0)
    {
        fprintf(stderr, "Incorrect node_id in %s\n", __FUNCTION__);
        exit(1);
    }

    while (nodes_count <= (unsigned)node_id)
    {
        unsigned int   i = nodes_count;
        void          *p;

        nodes_count = (nodes_count + 1) * 2;
        p = realloc(nodes_info, nodes_count * sizeof(node_info));
        if (p == NULL)
        {
            fprintf(stderr, "%s\n", "Not enough memory");
            exit(1);
        }
        nodes_info = (node_info *)p;

        for ( ; i < nodes_count; i++)
        {
            nodes_info[i].parent = -1;
            nodes_info[i].opened = FALSE;
            nodes_info[i].open_chld_cnt = 0;
            nodes_info[i].inner_frags_cnt = 0;
            nodes_info[i].cur_file_num = 0;
            nodes_info[i].cur_file_size = 0;
        }
    }

    return &nodes_info[node_id];
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
 * @return 0 on success, -1 on failure
 */
static int
append_to_frag(int node_id, fragment_type frag_type,
               FILE *f_raw_log,
               int64_t offset, int64_t length,
               FILE *f_recover,
               const char *output_path)
{
    FILE *f_frag;

    te_string frag_name = TE_STRING_INIT;
    te_string frag_path = TE_STRING_INIT;

    te_string_append(&frag_name, "%d_frag_",
                     node_id);
    switch (frag_type)
    {
        case FRAG_START:
            te_string_append(&frag_name, "%s", "start");
            break;

        case FRAG_INNER:
        {
            node_info *node_descr;
            node_descr = get_node_info(node_id);
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

            te_string_append(&frag_name, "inner_%" PRIu64,
                             node_descr->cur_file_num);
            break;
        }

        case FRAG_END:
            te_string_append(&frag_name, "%s", "end");
            break;
    }

    te_string_append(&frag_path, "%s/%s", output_path, frag_name.ptr);

    f_frag = fopen(frag_path.ptr, "a");
    if (f_frag == NULL)
    {
        fprintf(stderr, "Failed to open %s\n", frag_path.ptr);
        exit(1);
    }

    if (cur_block_offset < 0)
    {
        cur_block_offset = offset;
        cur_block_length = length;
        cur_block_frag_offset = ftello(f_frag);
        snprintf(cur_block_frag_name, sizeof(cur_block_frag_name),
                 frag_name.ptr);
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
                    ERROR("%s\n", "fprintf() failed");
                    exit(1);
                }

                cur_block_offset = offset;
                cur_block_length = length;
                cur_block_frag_offset = ftello(f_frag);
                snprintf(cur_block_frag_name, sizeof(cur_block_frag_name),
                         frag_name.ptr);
            }
        }
    }
    last_msg_offset = offset;

    file2file(f_frag, f_raw_log, -1, offset, length);
    fclose(f_frag);
    te_string_free(&frag_name);
    te_string_free(&frag_path);

    return 0;
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
 * @return 0 on success, -1 on failure
 */
static int
split_raw_log(FILE *f_raw_log, FILE *f_index, FILE *f_recover,
              const char *output_path)
{
    int           parent_id;
    int           node_id;
    int64_t       offset;
    int64_t       length;
    unsigned int  tin_or_verdict;
    unsigned int  timestamp[2];
    long int      line = 0;
    off_t         raw_fp;

    fragment_type frag_type;

    char  str[DEF_STR_LEN];
    char  msg_type[DEF_STR_LEN];
    int   rc;

    while (!feof(f_index))
    {
        fgets(str, sizeof(str), f_index);
        rc = sscanf(str, "%u.%u %" PRId64 " %d %d %s %u %" PRId64,
                    &timestamp[0], &timestamp[1],
                    &offset, &parent_id, &node_id,
                    msg_type, &tin_or_verdict, &length);
        if (rc < 7)
        {
            fprintf(stderr, "Wrong record in raw log index at line %ld",
                    line);
            exit(1);
        }
        if (rc < 8)
        {
            raw_fp = ftello(f_raw_log);
            fseeko(f_raw_log, 0LL, SEEK_END);
            length = ftello(f_raw_log);
            fseeko(f_raw_log, raw_fp, SEEK_SET);
            length -= offset;
        }

        if (strcmp(msg_type, "REGULAR") == 0)
            frag_type = FRAG_INNER;
        else if (strcmp(msg_type, "END") == 0)
        {
            node_info *node_descr;
            node_info *parent_node_descr;

            frag_type = FRAG_END;

            node_descr = get_node_info(node_id);
            node_descr->opened = FALSE;

            parent_node_descr = get_node_info(node_descr->parent);
            if (parent_node_descr != node_descr)
                parent_node_descr->open_chld_cnt--;
        }
        else
        {
            node_info *node_descr;
            node_info *parent_node_descr;

            frag_type = FRAG_START;

            node_descr = get_node_info(node_id);
            node_descr->opened = TRUE;

            node_descr->tin = tin_or_verdict;
            node_descr->start_len = length;

            node_descr->parent = parent_id;
            parent_node_descr = get_node_info(parent_id);
            if (parent_node_descr != node_descr)
                parent_node_descr->open_chld_cnt++;
        }

        if (frag_type == FRAG_INNER)
        {
            if (parent_id != TE_LOG_ID_UNDEFINED)
            {
                append_to_frag(parent_id, frag_type, f_raw_log,
                               offset, length,
                               f_recover, output_path);

                if (tin_or_verdict)
                    append_to_frag(parent_id, FRAG_START, f_raw_log,
                                   offset, length,
                                   f_recover, output_path);
            }
            else
            {
                long unsigned int j;
                te_bool           matching_frag_found = FALSE;

                for (j = 0; j < nodes_count; j++)
                {
                    if (nodes_info[j].opened &&
                        nodes_info[j].open_chld_cnt == 0)
                    {
                        append_to_frag(j, frag_type, f_raw_log,
                                       offset, length,
                                       f_recover, output_path);
                        matching_frag_found = TRUE;
                    }
                }

                if (!matching_frag_found)
                {
                    fprintf(stderr, "%s\n", "Matching fragment not found "
                            "for message with undefined log id");
                    exit(1);
                }
            }
        }
        else
        {
            append_to_frag(node_id, frag_type, f_raw_log,
                           offset, length, f_recover, output_path);
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
            ERROR("%s\n", "fprintf() failed");
            exit(1);
        }
    }

    return 0;
}

/**
 * Get file length.
 *
 * @param f     File pointer
 *
 * @return Length of file
 */
static off_t
file_length(FILE *f)
{
    off_t cur_pos;
    off_t length;

    cur_pos = ftello(f);
    fseeko(f, 0LLU, SEEK_END);
    length = ftello(f);
    fseeko(f, cur_pos, SEEK_SET);

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
 * @return 0 on success, -1 on failure
 */
static int
print_frags_list(const char *output_path, FILE *f_raw_gist,
                 FILE *f_frags_list, int node_id,
                 unsigned int depth, unsigned int seq)
{
    unsigned int i;
    unsigned int child_seq = 0;

    FILE      *f_frag;
    te_string  path = TE_STRING_INIT;
    off_t      frag_len;

    te_string_append(&path, "%s/%d_frag_start", output_path, node_id);
    f_frag = fopen(path.ptr, "r");
    if (f_frag == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", path.ptr);
        exit(1);
    }
    te_string_free(&path);
    frag_len = file_length(f_frag);
    file2file(f_raw_gist, f_frag, -1, -1, frag_len);
    fclose(f_frag);

    if (fprintf(f_frags_list,
                "%d_frag_start %u %u %u %llu %llu %" PRIu64 "\n",
                node_id, nodes_info[node_id].tin, depth, seq,
                (long long unsigned int)frag_len,
                (long long unsigned int)nodes_info[node_id].start_len,
                nodes_info[node_id].inner_frags_cnt) < 0)
    {
        ERROR("%s\n", "fprintf() failed");
        exit(1);
    }

    for (i = node_id + 1; i < nodes_count; i++)
    {
        if (nodes_info[i].parent == node_id)
        {
            print_frags_list(output_path, f_raw_gist, f_frags_list, i,
                             depth + 1, child_seq);
            child_seq++;
        }
    }

    te_string_append(&path, "%s/%d_frag_end", output_path, node_id);
    f_frag = fopen(path.ptr, "r");
    if (f_frag == NULL && errno != ENOENT)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", path.ptr);
        exit(1);
    }
    te_string_free(&path);
    if (f_frag != NULL)
    {
        frag_len = file_length(f_frag);
        file2file(f_raw_gist, f_frag, -1, -1, frag_len);
        fclose(f_frag);

        if (fprintf(f_frags_list, "%d_frag_end %u %u %u %llu\n",
                    node_id, nodes_info[node_id].tin, depth, seq,
                    (long long unsigned int)frag_len) < 0)
        {
            ERROR("%s\n", "fprintf() failed");
            exit(1);
        }
    }

    return 0;
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
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon; /* Context for parsing command-line options */
    int          rc;

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

int
main(int argc, char **argv)
{
    FILE *f_raw_log = NULL;
    FILE *f_index = NULL;
    FILE *f_recover = NULL;
    FILE *f_frags_list = NULL;
    FILE *f_raw_gist = NULL;

    te_string path_aux = TE_STRING_INIT;

    process_cmd_line_opts(argc, argv);

    f_raw_log = fopen(raw_log_path, "r");
    if (f_raw_log == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", raw_log_path);
        exit(1);
    }
    f_index = fopen(index_path, "r");
    if (f_index == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading\n", index_path);
        exit(1);
    }

    te_string_append(&path_aux, "%s/recover_list",
                     output_path);
    f_recover = fopen(path_aux.ptr, "w");
    if (f_recover == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for writing\n", path_aux.ptr);
        exit(1);
    }
    te_string_free(&path_aux);

    split_raw_log(f_raw_log, f_index, f_recover, output_path);

    te_string_append(&path_aux, "%s/frags_list", output_path);
    f_frags_list = fopen(path_aux.ptr, "w");
    if (f_frags_list == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for writing\n", path_aux.ptr);
        exit(1);
    }
    te_string_free(&path_aux);

    te_string_append(&path_aux, "%s/log_gist.raw", output_path);
    f_raw_gist = fopen(path_aux.ptr, "w");
    if (f_raw_gist == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for writing\n", path_aux.ptr);
        exit(1);
    }
    te_string_free(&path_aux);

    print_frags_list(output_path, f_raw_gist, f_frags_list, 0, 1, 0);

    fclose(f_raw_gist);
    fclose(f_frags_list);
    fclose(f_raw_log);
    fclose(f_index);
    fclose(f_recover);
    free(nodes_info);

    free(output_path);
    free(index_path);
    free(raw_log_path);
    return 0;
}
