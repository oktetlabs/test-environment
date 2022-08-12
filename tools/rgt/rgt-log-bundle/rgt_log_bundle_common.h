/** @file
 * @brief Test Environment: splitting raw log.
 *
 * Commong functions for splitting raw log into fragments and
 * merging fragments back into raw log.
 *
 *
 * Copyright (C) 2016-2022 OKTET Labs. All rights reserved.
 *
 *
 */

#ifndef __TE_RGT_LOG_BUNDLE_COMMON_H__
#define __TE_RGT_LOG_BUNDLE_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <popt.h>

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_file.h"
#include "logger_api.h"

/**
 * Generic string length used for strings containing
 * raw log fragment file names and strings from index files
*/
#define DEF_STR_LEN 512

/**
 * Entry in index of capture files heads.
 *
 * Capture file head is the main PCAP header + PCAP header and data
 * related to the first captured packet. That packet is a fake one
 * containing information about sniffer (TA name, interface name, etc).
 *
 * All such heads are stored in a single file in RAW log bundle.
 * A separate index file tells at which position a head for a particular
 * capture file is stored and how many bytes it contains.
 */
typedef struct rgt_cap_idx_rec {
    uint64_t pos;   /**< Position in the file of capture heads */
    uint32_t len;   /**< Length of the capture file head */
} rgt_cap_idx_rec;

/**
 * Record in a RGT log bundle file describing how RAW log is
 * split into fragments.
 */
typedef struct rgt_frag_rec {
    char frag_name[DEF_STR_LEN];    /**< Fragment name */

    te_bool start_frag;             /**< @c TRUE if this is a start
                                         fragment; @c FALSE if it is
                                         an end fragment */

    unsigned int parent_id;         /**< Parent ID */
    unsigned int test_id;           /**< ID of the test/session/package
                                         to which this record belongs */
    unsigned int tin;               /**< Test Iteration Number */
    unsigned int depth;             /**< Depth number */
    unsigned int seq;               /**< Sequence number at a given
                                         depth */
    uint64_t length;                /**< Number of bytes in the fragment
                                         file */
    uint64_t start_len;             /**< Length of start control message
                                         in the starting fragment
                                         (it contains information such
                                          as test name and parameters) */
    uint64_t frags_cnt;             /**< Number of "inner" fragments into
                                         which this test/session/package was
                                         split (excluding starting and
                                         terminating ones) */
    te_bool sniff_logs;             /**< @c TRUE if sniffer logs are
                                         present for this log item */
} rgt_frag_rec;

/**
 * Process capture files index in a RAW log bundle.
 *
 * @param split_log_path        Path to the unpacked bundle
 * @param idx_out               Where to save pointer to allocated
 *                              array of rgt_cap_idx_rec structures
 * @param idx_len_out           Where to save number of elements
 *                              in the array
 * @param f_heads_out           Where to save FILE pointer for an
 *                              opened file where pcap files
 *                              "heads" can be found
 *
 * @return @c 0 on success, @c -1 on failure
 */
extern int rgt_load_caps_idx(const char *split_log_path,
                             rgt_cap_idx_rec **idx_out,
                             unsigned int *idx_len_out,
                             FILE **f_heads_out);

/**
 * Parse fragment record in a RAW log bundle.
 *
 * @param s           String containing the record
 * @param rec         Where to save parsed data
 *
 * @return @c 0 on success, @c -1 on failure
 */
extern int rgt_parse_frag_rec(const char *s, rgt_frag_rec *rec);

/*
 * When a log node (test/session/package) has some sniffer packets
 * associated with it, to every "inner" fragment of this log node
 * corresponds a separate fragment file containing related sniffed
 * packets, possibly from different sniffers. Every packet has
 * a prefix telling to which original sniffer capture file it
 * belongs and at which offset it was there.
 *
 * Format of the prefix:
 *
 * | Field            | Size    |
 * ______________________________
 * | File ID          | 4 bytes |
 * | Packet offset    | 8 bytes |
 * | Packet length    | 4 bytes |
 */

/**
 * Read the prefix of sniffed packet in a fragment file.
 *
 * @param f           From which file to read
 * @param file_id     Where to save sniffer file ID
 * @param pkt_offset  Where to save packet offset
 * @param len         Where to save packet length
 *
 * @return @c 1 if a prefix was read successfully,
 *         @c 0 if EOF was reached,
 *         @c -1 on error.
 */
extern int rgt_read_cap_prefix(FILE *f, uint32_t *file_id,
                               uint64_t *pkt_offset, uint32_t *len);

/**
 * Copy data from one file to another.
 *
 * @param out_f       Destination file pointer
 * @param in_ f       Source file pointer
 * @param out_offset  At which offset to write data in
 *                    destination file (if < 0, then at
 *                    current positon)
 * @param in_offset   At which offset to read data from
 *                    source file (if < 0, then at
 *                    current positon)
 * @param length      Length of data to be copied
 *
 * @return 0 on success, -1 on failure
 */
extern int file2file(FILE *out_f, FILE *in_f,
                     off_t out_offset,
                     off_t in_offset, off_t length);

static inline void
usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptPrintUsage(optCon, stderr, 0);
    if (error)
    {
        fprintf(stderr, "%s", error);
        if (addl != NULL)
            fprintf(stderr, ": %s", addl);
        fprintf(stderr, "\n");
    }

    poptFreeContext(optCon);

    exit(exitcode);
}

/** Value of error variable when no errors occurred */
#define RGT_ERROR_OK 0
/** Value of error variable when errors occurred */
#define RGT_ERROR_FAIL -1

/** Declare and initialize error variable */
#define RGT_ERROR_INIT \
    int rgt_err_var = RGT_ERROR_OK; \
    te_bool rgt_err_section = FALSE

/**
 * Macro for starting a section to which error-checking macros
 * jump in case of failure. It disables further jumping,
 * so those marcos are safe to use after it too, they only set
 * error code then.
 * It should be at the end of the function, meant for releasing
 * resources and determining what to return.
 */
#define RGT_ERROR_SECTION \
    error:                      \
        rgt_err_section = TRUE

/** Set error variable to error value */
#define RGT_ERROR_SET rgt_err_var = RGT_ERROR_FAIL

/**
 * Jump to error/cleanup section without setting error variable
 * (can be used when you simply need to terminate function
 *  execution, do cleanup and return success)
 */
#define RGT_CLEANUP_JUMP \
    do {                        \
        if (!rgt_err_section)   \
            goto error;         \
    } while (0)

/** Set error variable to error value and jump to error section */
#define RGT_ERROR_JUMP \
    do {                        \
        RGT_ERROR_SET;          \
        RGT_CLEANUP_JUMP;       \
    } while (0)

/** Get value of error variable */
#define RGT_ERROR_VAL (rgt_err_var)

/** Check whether some error occurred */
#define RGT_ERROR (RGT_ERROR_VAL != RGT_ERROR_OK)

/**
 * Print error message and go to error section in a function parsing
 * command line options.
 *
 * @param _opt_con    poptContext used when parsing options.
 * @param _fmt...     Error message format and arguments.
 */
#define USAGE_ERROR_JUMP(_opt_con, _fmt...) \
    do {                                        \
        poptPrintUsage(optCon, stderr, 0);      \
        ERROR(_fmt);                            \
        RGT_ERROR_JUMP;                         \
    } while (0)

/**
 * Check value of an expression evaluating to te_errno; if
 * it is not zero, print error and jump to error section.
 *
 * @param _expr     Expression to check.
 */
#define CHECK_TE_RC(_expr) \
    do {                                                  \
        te_errno _rc;                                     \
                                                          \
        _rc = (_expr);                                    \
        if (_rc != 0)                                     \
        {                                                 \
            ERROR("%s():%s:%d: %s failed, rc=%r",         \
                  __FUNCTION__, __FILE__, __LINE__,       \
                  #_expr, _rc);                           \
            RGT_ERROR_JUMP;                               \
        }                                                 \
    } while (0)

/**
 * Check value of an expression which can return negative value and
 * set errno in case of error (in which case this macro will
 * print error message and jump to error section).
 *
 * @param _expr     Expression to check.
 */
#define CHECK_OS_RC(_expr) \
    do {                                                                  \
        long long int _rc;                                                \
                                                                          \
        _rc = (_expr);                                                    \
        if (_rc < 0)                                                      \
        {                                                                 \
            ERROR("%s():%s:%d: %s is negative (%lld), errno=%d ('%s')",   \
                  __FUNCTION__, __FILE__, __LINE__, #_expr, _rc, errno,   \
                  strerror(errno));                                       \
            RGT_ERROR_JUMP;                                               \
        }                                                                 \
    } while (0)

/**
 * Check value of an expression which can return negative value.
 * If the value is negative, this macro will print error message
 * and jump to error section.
 *
 * @param _expr     Expression to check.
 */
#define CHECK_RC(_expr) \
    do {                                                                  \
        long long int _rc;                                                \
                                                                          \
        _rc = (_expr);                                                    \
        if (_rc < 0)                                                      \
        {                                                                 \
            ERROR("%s():%s:%d: %s is negative (%lld)",                    \
                  __FUNCTION__, __FILE__, __LINE__, #_expr, _rc);         \
            RGT_ERROR_JUMP;                                               \
        }                                                                 \
    } while (0)

/**
 * Check value of an expression which can return @c NULL and
 * set errno in case of error (in which case this macro will
 * print error message and jump to error section).
 *
 * @param _expr     Expression to check.
 */
#define CHECK_OS_NOT_NULL(_expr) \
    do {                                                      \
        void *_p;                                             \
                                                              \
        _p = (_expr);                                         \
        if (_p == NULL)                                       \
        {                                                     \
            ERROR("%s():%s:%d: %s is NULL, errno=%d ('%s')",  \
                  __FUNCTION__, __FILE__, __LINE__, #_expr,   \
                  errno, strerror(errno));                    \
            RGT_ERROR_JUMP;                                   \
        }                                                     \
    } while (0)

/**
 * Check value of an expression which can return @c NULL
 * (in which case this macro will print error message and
 * jump to error section).
 *
 * @param _expr     Expression to check.
 */
#define CHECK_NOT_NULL(_expr) \
    do {                                                      \
        void *_p;                                             \
                                                              \
        _p = (_expr);                                         \
        if (_p == NULL)                                       \
        {                                                     \
            ERROR("%s():%s:%d: %s is NULL",                   \
                  __FUNCTION__, __FILE__, __LINE__, #_expr);  \
            RGT_ERROR_JUMP;                                   \
        }                                                     \
    } while (0)

/**
 * Call fopen(). If it returns @c NULL, print error message and
 * jump to error section.
 *
 * @param _f        Where to save opened FILE pointer.
 * @param _path     Path to the file.
 * @param _mode     Opening mode.
 */
#define CHECK_FOPEN(_f, _path, _mode) \
    do {                                                                \
        _f = fopen((_path), (_mode));                                   \
        if (_f == NULL)                                                 \
        {                                                               \
            ERROR("%s():%s:%d: failed to open '%s', errno=%d ('%s')",   \
                  __FUNCTION__, __FILE__, __LINE__, (_path), errno,     \
                  strerror(errno));                                     \
            RGT_ERROR_JUMP;                                             \
        }                                                               \
    } while (0)

/**
 * Call te_fopen_fmt(), jump to error section if it failed.
 *
 * @param _f          Where to save FILE pointer on success.
 * @param _mode       File opening mode.
 * @param _path_fmt   File path format string and arguments.
 */
#define CHECK_FOPEN_FMT(_f, _mode, _path_fmt...) \
    do {                                                \
        _f = te_fopen_fmt((_mode), _path_fmt);          \
        if (_f == NULL)                                 \
        {                                               \
            ERROR("%s():%s:%d: te_fopen_fmt() failed",  \
                  __FUNCTION__, __FILE__, __LINE__);    \
            ERROR("Failed to open " _path_fmt);         \
            RGT_ERROR_JUMP;                             \
        }                                               \
    } while (0)

/**
 * Call fread() and check that it read all the expected bytes.
 * If it did not, print error message and jump to error section.
 *
 * @param _ptr      Where to save read data.
 * @param _size     Size of the data element.
 * @param _nmemb    Number of data elements.
 * @param _f        File from which to read.
 */
#define CHECK_FREAD(_ptr, _size, _nmemb, _f) \
    do {                                                      \
        size_t _rc;                                           \
                                                              \
        _rc = fread((_ptr), (_size), (_nmemb), (_f));         \
        if (_rc != (_nmemb))                                  \
        {                                                     \
            ERROR("%s():%s:%d: failed to read %s from file, " \
                  "%" TE_PRINTF_SIZE_T "u is returned "       \
                  "instead of %" TE_PRINTF_SIZE_T "u",        \
                  __FUNCTION__, __FILE__, __LINE__, #_ptr,    \
                  _rc, (size_t)(_nmemb));                     \
            RGT_ERROR_JUMP;                                   \
        }                                                     \
    } while (0)

/**
 * Call fwrite() and check that it wrote all the expected bytes.
 * If it did not, print error message and jump to error section.
 *
 * @param _ptr      Data to write.
 * @param _size     Size of the data element.
 * @param _nmemb    Number of data elements.
 * @param _f        File to which to write.
 */
#define CHECK_FWRITE(_ptr, _size, _nmemb, _f) \
    do {                                                        \
        size_t _rc;                                             \
                                                                \
        _rc = fwrite((_ptr), (_size), (_nmemb), (_f));          \
        if (_rc != (_nmemb))                                    \
        {                                                       \
            ERROR("%s():%s:%d: failed to write %s to file, "    \
                  "%" TE_PRINTF_SIZE_T "u is returned "         \
                  "instead of %" TE_PRINTF_SIZE_T "u",          \
                  __FUNCTION__, __FILE__, __LINE__, #_ptr,      \
                  _rc, (size_t)(_nmemb));                       \
            RGT_ERROR_JUMP;                                     \
        }                                                       \
    } while (0)

/**
 * If FILE pointer is not @c NULL, call fclose() on it and set
 * it to @c NULL. In case of failure, print error message and
 * jump to error section.
 *
 * @param _f        File pointer to close.
 */
#define CHECK_FCLOSE(_f) \
    do {                                                    \
        if (_f != NULL)                                     \
        {                                                   \
            int rc;                                         \
                                                            \
            rc = fclose(_f);                                \
            if (rc != 0)                                    \
            {                                               \
                ERROR("%s():%s:%d: failed to close file",   \
                      __FUNCTION__, __FILE__, __LINE__);    \
                RGT_ERROR_JUMP;                             \
            }                                               \
            _f = NULL;                                      \
        }                                                   \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_LOG_BUNDLE_COMMON_H__ */
