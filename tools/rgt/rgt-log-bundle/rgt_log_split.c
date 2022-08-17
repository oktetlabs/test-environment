/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: implementation of raw log fragmentation.
 *
 * Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <popt.h>

#include <inttypes.h>
#include <byteswap.h>

#include <sys/types.h>
#include <dirent.h>

#if HAVE_PCAP_H
#include <pcap.h>
#else
/** Main PCAP header. See man pcap-savefile. */
struct pcap_file_header {
    uint32_t magic;           /**< Magic number defining the format
                                   of PCAP file */
    uint8_t ignored[20];      /**< Ignored bytes in the main PCAP
                                   header */
};
#endif

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_file.h"
#include "te_str.h"
#include "te_string.h"
#include "te_raw_log.h"
#include "rgt_log_bundle_common.h"
#include "te_sniffers.h"
#include "te_queue.h"

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
    int                    node_id;           /**< Log node ID */
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

    LIST_ENTRY(node_info) links;  /**< Links in a list of currently
                                       opened nodes having no opened
                                       children */

    FILE *f_sniff;                /**< Current file with sniffed network
                                       packets */
    uint64_t cur_sniff_file_num;  /**< Number of the current file with
                                       sniffed packets (if a node is split
                                       into multiple fragments due to a
                                       number of messages, sniffed packets
                                       are split accordingly between
                                       multiple capture files) */
    te_bool sniff_logs;           /**< If @c TRUE, the node has associated
                                       file(s) with sniffed network
                                       packets */
} node_info;

/** Type of the head of a list of log nodes */
typedef LIST_HEAD(node_info_list, node_info) node_info_list;

/** Array of log node descriptions */
static node_info      *nodes_info = NULL;
/** Number of elements in the array of log node descriptions */
static unsigned int    nodes_count = 0;

/** File with sniffed network packets */
typedef struct rgt_pcap_file {
    char path[PATH_MAX];        /**< File path */
    uint32_t file_id;           /**< ID of the PCAP file */
    FILE *f;                    /**< FILE handle of the opened file */

    uint32_t ts_sec;            /**< Seconds in timestamp of the
                                     current packet */
    uint32_t ts_usec;           /**< Microseconds in timestamp of
                                     the current packet */
    uint64_t pkt_offset;        /**< Offset of the current packet */
    te_pcap_pkthdr cur_hdr;     /**< PCAP header of the current packet */

    te_bool no_caps;            /**< Set to @c TRUE if there is no
                                     packets left in this file */
    te_bool other_byte_order;   /**< Set to @c TRUE if byte order
                                     in PCAP headers does not match
                                     host byte order */
    uint64_t start_pos;         /**< Position after the PCAP header of the
                                     second packet (the first non-fake
                                     one) */

    LIST_ENTRY(rgt_pcap_file) links;  /**< List links */
} rgt_pcap_file;

/**
 * Type of the head of a list of PCAP files.
 * They are stored sorted according to timestamps of the current
 * packet in this list.
 */
typedef LIST_HEAD(rgt_pcap_file_list, rgt_pcap_file) rgt_pcap_file_list;

/**
 * PCAP magic number when byte order of PCAP file matches host
 * byte order.
 */
#define PCAP_MAGIC_HOST_ORDER 0xa1b2c3d4

/**
 * Magic number when byte order of PCAP file does not match host byte
 * order.
 */
#define PCAP_MAGIC_OTHER_ORDER 0xd4c3b2a1

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
            nodes_info[i].node_id = i;
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

            nodes_info[i].f_sniff = NULL;
            nodes_info[i].cur_sniff_file_num = 0;
            nodes_info[i].sniff_logs = FALSE;
        }
    }

    return &nodes_info[node_id];
}

/**
 * Update count of opened children for a parent node;
 * add it to or remove it from the list of opened nodes
 * without opened children if necessary.
 *
 * @param parent_id       Parent node ID.
 * @param child_id        Child node ID.
 * @param child_opened    @c TRUE if a child is opened,
 *                        @c FALSE if it is closed.
 * @param leaf_nodes      List of all the opened nodes having no
 *                        opened children, which this function
 *                        updates.
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
update_children_state(int parent_id, int child_id,
                      te_bool child_opened, node_info_list *leaf_nodes)
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
        if (parent_descr->opened_children == 0)
            LIST_REMOVE(parent_descr, links);

        parent_descr->opened_children++;
    }
    else
    {
        parent_descr->opened_children--;
        parent_descr->last_closed_child = child_id;

        if (parent_descr->opened_children == 0)
            LIST_INSERT_HEAD(leaf_nodes, parent_descr, links);
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
 * Get length of the data of the current PCAP packet from
 * its header.
 *
 * @param pfile     PCAP file structure.
 *
 * @return Data length.
 */
static uint32_t
get_pcap_data_len(rgt_pcap_file *pfile)
{
    uint32_t len;

    len = pfile->cur_hdr.caplen;
    if (pfile->other_byte_order)
        len = bswap_32(len);

    return len;
}

/**
 * Get PCAP header and data for the current packet. Then
 * try to read the next PCAP header to update current
 * timestamp.
 *
 * @note It is assumed that PCAP header for the current
 *       packet was already read - we need to know timestamp
 *       before we can decide from which PCAP file to get
 *       the next packet.
 *
 * @param pfile           PCAP file structure.
 * @param hdr             Where to save current PCAP header.
 * @param data            Where to save pointer to the packet data.
 * @param data_len        Where to save length of the packet data.
 *
 * @return @c 0 if there are no packets left in this file,
 *         @c 1 if a packet was read successfully,
 *         @c -1 in case of error.
 */
static int
get_next_pcap(rgt_pcap_file *pfile, te_pcap_pkthdr *hdr,
              void **data, uint32_t *data_len)
{
    uint32_t len;
    void *buf = NULL;
    size_t sz;

    RGT_ERROR_INIT;

    if (pfile == NULL)
        return 0;

    if (hdr != NULL)
        memcpy(hdr, &pfile->cur_hdr, sizeof(*hdr));

    if (pfile->no_caps)
        return 0;

    if (pfile->f == NULL)
    {
        CHECK_FOPEN(pfile->f, pfile->path, "r");
        CHECK_OS_RC(fseeko(pfile->f, pfile->start_pos, SEEK_SET));
    }

    pfile->pkt_offset = ftello(pfile->f) - sizeof(te_pcap_pkthdr);

    len = get_pcap_data_len(pfile);
    CHECK_OS_NOT_NULL(buf = calloc(1, len));

    CHECK_FREAD(buf, 1, len, pfile->f);

    sz = fread(&pfile->cur_hdr, 1, sizeof(te_pcap_pkthdr), pfile->f);
    if (sz == 0)
    {
        pfile->no_caps = TRUE;
        CHECK_FCLOSE(pfile->f);
    }
    else if (sz < sizeof(te_pcap_pkthdr))
    {
        ERROR("%s(): failed to read full PCAP header from %s",
              __FUNCTION__, pfile->path);
        RGT_ERROR_JUMP;
    }
    else
    {
        pfile->ts_sec = pfile->cur_hdr.ts.tv_sec;
        pfile->ts_usec = pfile->cur_hdr.ts.tv_usec;
        if (pfile->other_byte_order)
        {
            pfile->ts_sec = bswap_32(pfile->ts_sec);
            pfile->ts_usec = bswap_32(pfile->ts_usec);
        }
    }

    *data = buf;
    *data_len = len;

    RGT_ERROR_SECTION;

    if (RGT_ERROR)
    {
        free(buf);
        return -1;
    }

    return 1;
}

/**
 * Get a "head" of PCAP file (main PCAP header + the first (fake) packet).
 * Save the head in the PCAP heads file, and its length and position
 * in the PCAP heads index file. Then read PCAP header of the
 * second packet (if it is present) to get timestamp of the first real
 * packet. Fill fields of rgt_pcap_file structure.
 *
 * @param pfile             Pointer to rgt_pcap_file structure.
 * @param f_caps_heads      File where PCAP "heads" are stored.
 * @param f_caps_idx        File where index of PCAP "heads" is stored.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
get_pcap_head(rgt_pcap_file *pfile, FILE *f_caps_heads, FILE *f_caps_idx)
{
    struct pcap_file_header head;
    te_pcap_pkthdr phdr;
    rgt_cap_idx_rec idx_rec;
    te_bool main_head_only = FALSE;

    FILE *f = NULL;
    size_t sz;
    void *data = NULL;
    uint32_t data_len = 0;

    RGT_ERROR_INIT;

    CHECK_FOPEN(f, pfile->path, "r");
    CHECK_FREAD(&head, 1, sizeof(head), f);

    if (head.magic == PCAP_MAGIC_HOST_ORDER)
    {
        pfile->other_byte_order = FALSE;
    }
    else if (head.magic == PCAP_MAGIC_OTHER_ORDER)
    {
        pfile->other_byte_order = TRUE;
    }
    else
    {
        ERROR("Unexpected magic number 0x%x in file %s",
              head.magic, pfile->path);
        RGT_ERROR_JUMP;
    }

    sz = fread(&pfile->cur_hdr, 1, sizeof(pfile->cur_hdr), f);
    if (sz == 0)
    {
        pfile->no_caps = TRUE;
        main_head_only = TRUE;
    }
    else if (sz < sizeof(pfile->cur_hdr))
    {
        ERROR("%s(): PCAP header of the first packet was read only "
              "partially from %s", __FUNCTION__, pfile->path);
        RGT_ERROR_JUMP;
    }
    else
    {
        pfile->f = f;
        f = NULL;

        CHECK_RC(get_next_pcap(pfile, &phdr, &data, &data_len));

        /* Position after the PCAP header of the second packet */
        if (!pfile->no_caps)
            pfile->start_pos = ftello(pfile->f);
    }

    idx_rec.pos = ftello(f_caps_heads);
    idx_rec.len = sizeof(head);

    CHECK_FWRITE(&head, sizeof(head), 1, f_caps_heads);

    if (!main_head_only)
    {
        CHECK_FWRITE(&phdr, sizeof(phdr), 1, f_caps_heads);
        CHECK_FWRITE(data, 1, data_len, f_caps_heads);
        idx_rec.len += sizeof(phdr) + data_len;
    }

    CHECK_FWRITE(&idx_rec, sizeof(idx_rec), 1, f_caps_idx);

    RGT_ERROR_SECTION;

    /*
     * File is closed here to avoid opening too many PCAP
     * files at once. Later it is reopened when packets from
     * it are needed, and closed after all the packets were read.
     */

    CHECK_FCLOSE(pfile->f);
    CHECK_FCLOSE(f);

    free(data);

    return RGT_ERROR_VAL;
}

/**
 * Compare two PCAP file structures by timestamps of the current
 * packets. A file with no packets left is considered to be "bigger"
 * than a file still having some packet(s) to be read.
 *
 * @return Comparison result.
 *
 * @retval @c -1    The first file is "smaller" than the second one.
 * @retval @c 0     The first file is "equal" to the second one.
 * @retval @c 1     The first file is "bigger" than the second one.
 */
static int
pcap_files_cmp(const void *pfile1, const void *pfile2)
{
    rgt_pcap_file *p = (rgt_pcap_file *)pfile1;
    rgt_pcap_file *q = (rgt_pcap_file *)pfile2;

    if (p->no_caps && q->no_caps)
        return 0;
    else if (q->no_caps && !p->no_caps)
        return -1;
    else if (p->no_caps && !q->no_caps)
        return 1;

    if (p->ts_sec < q->ts_sec)
        return -1;
    else if (p->ts_sec > q->ts_sec)
        return 1;
    else if (p->ts_usec < q->ts_usec)
        return -1;
    else if (p->ts_usec > q->ts_usec)
        return 1;

    return 0;
}

/**
 * If the list of rgt_pcap_file structures contains more than one element,
 * check whether the first element of the list precedes the second
 * one according to pcap_files_cmp() function. If not, move it
 * to the proper place in the list so that it is sorted in ascending
 * order.
 * Then retrieve timestamp of the current packet of the first element in
 * the list if it has any packets left.
 *
 * @param caps_list           Head of the list of PCAP files.
 * @param ts                  Where to save timestamp of the current packet
 *                            of the first file in the list.
 *
 * @return @c 1 if there is still a PCAP packet to be processed,
 *         @c 0 otherwise.
 */
static int
update_pcap_files_list(rgt_pcap_file_list *caps_list, te_ts_t *ts)
{
    rgt_pcap_file *p = NULL;
    rgt_pcap_file *q = NULL;
    rgt_pcap_file *last = NULL;

    if (LIST_EMPTY(caps_list))
        return 0;

    p = LIST_FIRST(caps_list);

    if (p->no_caps)
    {
        LIST_REMOVE(p, links);
        p = LIST_FIRST(caps_list);
    }

    if (p == NULL || p->no_caps)
        return 0;

    q = LIST_NEXT(p, links);
    if (q != NULL)
    {
        if (pcap_files_cmp(p, q) > 0)
        {
            do {
                last = q;
                q = LIST_NEXT(q, links);
            } while (q != NULL && pcap_files_cmp(p, q) > 0);

            LIST_REMOVE(p, links);
            if (q != NULL)
                LIST_INSERT_BEFORE(q, p, links);
            else
                LIST_INSERT_AFTER(last, p, links);

            p = LIST_FIRST(caps_list);
        }
    }

    ts->tv_sec = p->ts_sec;
    ts->tv_usec = p->ts_usec;

    return 1;
}

/**
 * Process all the PCAP files in sniffer capture directory.
 * Fill PCAP heads, heads index and file names files.
 * Create an array of rgt_pcap_file structures corresponding to the
 * PCAP files, and a list of them sorted according to timestamps
 * of the second (first non-fake) packets.
 *
 * @param sniff_dir           Path to the directory with sniffer capture
 *                            files.
 * @param dst_path            Path to the directory where RAW log bundle
 *                            is constructed.
 * @param caps_out            Where to save pointer to the array of
 *                            rgt_pcap_file structures.
 * @param caps_num_out        Where to save number of elements in the
 *                            array.
 * @param caps_list           Head of the list of rgt_pcap_file's to fill.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
process_pcap_files(const char *sniff_dir, const char *dst_path,
                   rgt_pcap_file **caps_out, int *caps_num_out,
                   rgt_pcap_file_list *caps_list)
{
    DIR *d;
    struct dirent *ent;
    int len;

    te_string fpath = TE_STRING_INIT_STATIC(PATH_MAX);
    rgt_pcap_file *caps = NULL;

    void *p;
    int caps_num = 0;
    int caps_max = 10;
    int i;

    FILE *f_caps_heads = NULL;
    FILE *f_caps_idx = NULL;
    FILE *f_caps_names = NULL;

    RGT_ERROR_INIT;

    CHECK_OS_NOT_NULL(caps = calloc(caps_max, sizeof(*caps)));

    CHECK_FOPEN_FMT(f_caps_heads, "w", "%s/sniff_heads", dst_path);
    CHECK_FOPEN_FMT(f_caps_idx, "w", "%s/sniff_heads_idx", dst_path);
    CHECK_FOPEN_FMT(f_caps_names, "w", "%s/sniff_fnames", dst_path);

    CHECK_OS_NOT_NULL(d = opendir(sniff_dir));

    while ((ent = readdir(d)) != NULL)
    {
        if (ent->d_type != DT_REG)
            continue;

        len = strlen(ent->d_name);
        if (len >= 5 && strcmp(&ent->d_name[len - 5], ".pcap") == 0)
        {
            te_string_reset(&fpath);
            CHECK_TE_RC(te_string_append(&fpath, "%s/%s",
                                         sniff_dir, ent->d_name));

            if (caps_max == caps_num)
            {
                caps_max *= 2;
                CHECK_OS_NOT_NULL(p = realloc(caps,
                                              caps_max * sizeof(*caps)));
                caps = (rgt_pcap_file *)p;
            }

            memset(&caps[caps_num], 0, sizeof(rgt_pcap_file));
            strcpy(caps[caps_num].path, fpath.ptr);
            caps[caps_num].file_id = caps_num;

            get_pcap_head(&caps[caps_num], f_caps_heads, f_caps_idx);

            CHECK_OS_RC(fprintf(f_caps_names, "%s\n", ent->d_name));

            caps_num++;
        }
    }

    if (caps_num == 0)
    {
        free(caps);
        caps = NULL;
    }
    else
    {
        qsort(caps, caps_num, sizeof(*caps), &pcap_files_cmp);

        for (i = caps_num - 1; i >= 0; i--)
        {
            if (caps[i].no_caps)
                continue;

            LIST_INSERT_HEAD(caps_list, &caps[i], links);
        }
    }

    RGT_ERROR_SECTION;

    CHECK_OS_RC(closedir(d));
    CHECK_FCLOSE(f_caps_heads);
    CHECK_FCLOSE(f_caps_idx);
    CHECK_FCLOSE(f_caps_names);

    if (RGT_ERROR)
    {
        free(caps);
    }
    else
    {
        *caps_out = caps;
        *caps_num_out = caps_num;
    }

    return RGT_ERROR_VAL;
}

/**
 * Create missing sniffer fragment files. If some sniffed packets are
 * present for a given log node, there must be a sniffer fragment file
 * for every "inner" (not start or end) fragment of a given log node,
 * even if some of the sniffer fragment files are empty. This is done
 * to simplify extraction of all the related files for a requested log
 * node.
 *
 * @param output_path       Directory where RAW log bundle is constructed.
 * @param node              Pointer to log node structure.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
create_missing_sniff_frags(const char *output_path, node_info *node)
{
    uint64_t i;
    FILE *f;

    RGT_ERROR_INIT;

    if (!node->sniff_logs)
        return 0;

    for (i = node->cur_sniff_file_num; i <= node->cur_file_num; i++)
    {
        CHECK_FOPEN_FMT(f, "a",
                        "%s/%d_frag_sniff_%" PRIu64,
                        output_path, node->node_id, i);
        CHECK_FCLOSE(f);
    }

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
}

/**
 * Append a new sniffed packet to the current sniffed packets fragment
 * file for a given node.
 *
 * @param output_path       Where RAW log bundle is constructed.
 * @param node              Target log node.
 * @param hdr               PCAP header.
 * @param data              Packet data.
 * @param data_len          Length of the packet data.
 * @param file_id           Sniffer file ID.
 * @param pkt_offset        Offset of the packet in the sniffer file.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
append_pcap_to_node(const char *output_path, node_info *node,
                    te_pcap_pkthdr *hdr, void *data, uint32_t data_len,
                    uint32_t file_id, uint64_t pkt_offset)
{
    FILE *f_aux;
    uint32_t len;

    RGT_ERROR_INIT;

    node->sniff_logs = TRUE;

    if (node->f_sniff != NULL)
    {
        if (node->cur_file_num != node->cur_sniff_file_num)
            CHECK_FCLOSE(node->f_sniff);
    }

    if (node->f_sniff == NULL)
    {
        CHECK_RC(create_missing_sniff_frags(output_path, node));
        CHECK_FOPEN_FMT(node->f_sniff, "w",
                        "%s/%d_frag_sniff_%" PRIu64,
                        output_path, node->node_id,
                        node->cur_file_num);
        node->cur_sniff_file_num = node->cur_file_num;
    }

    if (node->inner_frags_cnt == 0)
    {
        /*
         * If log node has associated sniffer packets, it
         * should have at least one "inner" fragment, since
         * there is one-to-one correspondence between
         * "inner" fragments and sniffer fragments.
         */

        node->inner_frags_cnt = 1;
        CHECK_FOPEN_FMT(f_aux, "a", "%s/%d_frag_inner_0",
                        output_path, node->node_id);
        CHECK_FCLOSE(f_aux);
    }

    len = data_len + sizeof(*hdr);

    CHECK_FWRITE(&file_id, sizeof(file_id), 1, node->f_sniff);
    CHECK_FWRITE(&pkt_offset, sizeof(pkt_offset), 1, node->f_sniff);
    CHECK_FWRITE(&len, sizeof(len), 1, node->f_sniff);

    CHECK_FWRITE(hdr, 1, sizeof(*hdr), node->f_sniff);
    CHECK_FWRITE(data, 1, data_len, node->f_sniff);

    RGT_ERROR_SECTION;

    return RGT_ERROR_VAL;
}

/**
 * Append a PCAP packet to current sniffer fragment files of all the
 * open log nodes not having any open children.
 *
 * @param output_path       Where RAW log bundle is constructed.
 * @param leaf_nodes        List of target log nodes.
 * @param pfile             Pointer to PCAP file structure from
 *                          which to get the next packet.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
append_pcap_to_open_nodes(const char *output_path,
                          node_info_list *leaf_nodes,
                          rgt_pcap_file *pfile)
{
    node_info *node;
    void *data = NULL;
    uint32_t data_len;
    te_pcap_pkthdr phdr;

    RGT_ERROR_INIT;

    CHECK_RC(get_next_pcap(pfile, &phdr, &data, &data_len));

    LIST_FOREACH(node, leaf_nodes, links)
    {
        CHECK_RC(append_pcap_to_node(output_path, node, &phdr,
                                     data, data_len,
                                     pfile->file_id, pfile->pkt_offset));
    }

    RGT_ERROR_SECTION;

    free(data);

    return RGT_ERROR_VAL;
}

/**
 * Append all the PCAP packets up to a given timestamp to current sniffer
 * fragment files of all the open log nodes not having any open children.
 *
 * @param output_path       Where RAW log bundle is constructed.
 * @param leaf_nodes        List of target log nodes.
 * @param caps_list         List of PCAP files sorted according to
 *                          timestamps of the current packets.
 * @param ts_sec            Timestamp, seconds.
 * @param ts_usec           Timestamp, microseconds.
 * @param include_end       If @c TRUE, include packets having exactly
 *                          the target timestamp; otherwise exclude them.
 *
 * @return @c 0 on success, @c -1 on failure.
 */
static int
append_pcap_until_ts(const char *output_path,
                     node_info_list *leaf_nodes,
                     rgt_pcap_file_list *caps_list,
                     uint32_t ts_sec, uint32_t ts_usec,
                     te_bool include_end)
{
    te_ts_t ts;
    int rc;

    while (TRUE)
    {
        rc = update_pcap_files_list(caps_list, &ts);
        if (rc < 1)
            return 0;

        if (ts.tv_sec > ts_sec)
            return 0;

        if (ts.tv_sec == ts_sec)
        {
            if (ts.tv_usec > ts_usec)
                return 0;
            else if (!include_end && ts.tv_usec == ts_usec)
                return 0;
        }

        if (append_pcap_to_open_nodes(output_path, leaf_nodes,
                                      LIST_FIRST(caps_list)) < 0)
        {
            return -1;
        }
    }

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
 * @param sniff_dir         Where to search for sniffer files
 *                          (may be @c NULL)
 * @param output_path       Where to store raw log fragments
 *
 * @return @c 0 on success, @c -1 on failure
 */
static int
split_raw_log(FILE *f_raw_log, FILE *f_index, FILE *f_recover,
              const char *sniff_dir, const char *output_path)
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

    node_info_list leaf_nodes = LIST_HEAD_INITIALIZER(node_info_list);
    node_info *node_descr = NULL;

    rgt_pcap_file *caps = NULL;
    rgt_pcap_file_list caps_list = LIST_HEAD_INITIALIZER(rgt_pcap_file_list);
    int caps_num = 0;

    RGT_ERROR_INIT;

    if (sniff_dir != NULL)
    {
        CHECK_RC(process_pcap_files(sniff_dir, output_path,
                                    &caps, &caps_num,
                                    &caps_list));
    }

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
        {
            frag_type = FRAG_INNER;

            CHECK_RC(append_pcap_until_ts(
                                  output_path, &leaf_nodes,
                                  &caps_list,
                                  timestamp[0], timestamp[1], FALSE));
        }
        else if (strcmp(msg_type, "END") == 0)
        {
            frag_type = FRAG_END;

            CHECK_RC(append_pcap_until_ts(
                                output_path, &leaf_nodes, &caps_list,
                                timestamp[0], timestamp[1], TRUE));

            CHECK_NOT_NULL(node_descr = get_node_info(node_id));
            node_descr->opened = FALSE;

            CHECK_RC(update_children_state(parent_id, node_id, FALSE,
                                           &leaf_nodes));

            LIST_REMOVE(node_descr, links);

            if (node_descr->f_sniff != NULL)
                CHECK_FCLOSE(node_descr->f_sniff);

            CHECK_RC(create_missing_sniff_frags(output_path, node_descr));
        }
        else
        {
            frag_type = FRAG_START;

            CHECK_RC(append_pcap_until_ts(
                                  output_path, &leaf_nodes, &caps_list,
                                  timestamp[0], timestamp[1], FALSE));

            CHECK_NOT_NULL(node_descr = get_node_info(node_id));

            node_descr->opened = TRUE;
            CHECK_RC(update_children_state(parent_id, node_id, TRUE,
                                           &leaf_nodes));

            node_descr->tin = tin_or_start_frag;
            node_descr->start_len = length;

            node_descr->parent = parent_id;

            LIST_INSERT_HEAD(&leaf_nodes, node_descr, links);
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
                te_bool matching_frag_found = FALSE;

                /*
                 * Logs from TE components (such as Configurator) do not
                 * have ID of any specific test and should be attached to
                 * all currently opened nodes (tests, sessions, packages)
                 * not having opened children (to avoid attaching log both
                 * to a test and to a session including it). Multiple such
                 * nodes may be opened simultaneously if multiple tests are
                 * run in parallel.
                 */
                LIST_FOREACH(node_descr, &leaf_nodes, links)
                {
                    if (node_descr->last_closed_child >= 0)
                    {
                        CHECK_RC(append_to_frag(
                                       node_descr->last_closed_child,
                                       FRAG_AFTER, f_raw_log,
                                       offset, length,
                                       f_recover, output_path));
                    }
                    else
                    {
                        CHECK_RC(append_to_frag(
                                       node_descr->node_id, frag_type,
                                       f_raw_log, offset, length,
                                       f_recover, output_path));
                    }
                    matching_frag_found = TRUE;
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

    CHECK_RC(append_pcap_until_ts(
                         output_path, &leaf_nodes, &caps_list,
                         UINT_MAX, UINT_MAX, TRUE));

    RGT_ERROR_SECTION;

    free(caps);

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
                "%d_frag_start %u %u %u %llu %llu %" PRIu64 " %d %d\n",
                node_id, nodes_info[node_id].tin, depth, seq,
                (long long unsigned int)frag_len,
                (long long unsigned int)nodes_info[node_id].start_len,
                nodes_info[node_id].inner_frags_cnt,
                nodes_info[node_id].parent,
                (nodes_info[node_id].sniff_logs ? 1 : 0)) < 0)
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
/** Where to find sniffer capture files */
static char *caps_path = NULL;

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

        { "sniff-log-dir", 's', POPT_ARG_STRING, NULL, 's',
          "Path to sniffer capture files directory.", NULL },

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
        else if (rc == 's')
            caps_path = poptGetOptArg(optCon);
        else if (rc == 'o')
            output_path = poptGetOptArg(optCon);
    }

    if (raw_log_path == NULL || index_path == NULL || output_path == NULL)
    {
        poptPrintUsage(optCon, stderr, 0);
        ERROR("Specify all the required parameters: --raw-log, "
              "--log-index, --output-dir");
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

    CHECK_RC(split_raw_log(f_raw_log, f_index, f_recover, caps_path,
                           output_path));

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
    free(caps_path);

    if (RGT_ERROR)
        return EXIT_FAILURE;

    return 0;
}
