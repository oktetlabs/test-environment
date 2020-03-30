/** @file
 * @brief RPC Test Suite
 *
 * Common definitions for rpc/file test suite.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Eugene Suslov <Eugene.Suslov@oktetlabs.ru>
 */

#ifndef __FILE_SUITE_H__
#define __FILE_SUITE_H__

#include "te_defs.h"
#include "tapi_test.h"
#include "tapi_file.h"
#include "tapi_mem.h"
#include "tapi_rpc_unistd.h"

#define AGT_A    "Agt_A"

#define TMP_DIR    "/tmp"

#define BUFSIZE 64
#define RW_MAX_RETRY 3

/**
 * Write the whole buffer.
 *
 * @param pco_        PCO where to send from
 * @param fd_         File descriptor at PCO
 * @param buf_        Pointer to a buffer
 * @param buflen_     Buffer size
 */
#define WRITE_WHOLE_BUF(pco_, fd_, buf_, buflen_)                       \
    do {                                                                \
        size_t total_ = 0;  /* Total bytes has been written */          \
        int current_;  /* Written by a single write() */                \
        int retry_ = 0;                                                 \
                                                                        \
        while(total_ < buflen_)                                         \
        {                                                               \
            RPC_AWAIT_ERROR(pco_);                                      \
            current_ = rpc_write(pco_, fd_, (void *)buf_ + total_,      \
                                 buflen_);                              \
            if (current_ <= 0 && RPC_ERRNO(pco_) != 0)                  \
            {                                                           \
                TEST_VERDICT("rpc_write() unexpectedly returned %d,"    \
                             "errno=%r", current_, RPC_ERRNO(pco_));    \
            }                                                           \
            else if (current_ == 0)                                     \
            {                                                           \
                if (retry_++ == RW_MAX_RETRY)                           \
                {                                                       \
                    TEST_VERDICT("rpc_write() maximum re-try reached"); \
                }                                                       \
                else                                                    \
                {                                                       \
                    VSLEEP(1, "rpc_write() returned 0. Retry writing"); \
                    continue;                                           \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                retry_ = 0;                                             \
            }                                                           \
            total_ += current_;                                         \
        }                                                               \
    } while(0)

/**
 * Read the whole buffer.
 *
 * @param pco_        PCO where to send from
 * @param fd_         File descriptor at PCO
 * @param buf_        Pointer to a buffer
 * @param buflen_     Buffer size
 */
#define READ_WHOLE_BUF(pco_, fd_, buf_, buflen_)                        \
    do {                                                                \
        size_t total_ = 0;  /* Total bytes has been read */             \
        int current_;  /* Read by a single read() */                    \
        int retry_ = 0;                                                 \
                                                                        \
        while(total_ < buflen_)                                         \
        {                                                               \
            RPC_AWAIT_ERROR(pco_);                                      \
            current_ = rpc_read(pco_, fd_, (void *)buf_ + total_,       \
                                buflen_);                               \
                                                                        \
            if (current_ <= 0 && RPC_ERRNO(pco_) != 0)                  \
            {                                                           \
                TEST_VERDICT("rpc_read() unexpectedly returned %d,"     \
                             "errno=%r", current_, RPC_ERRNO(pco_));    \
            }                                                           \
            else if (current_ == 0)                                     \
            {                                                           \
                if (retry_++ == RW_MAX_RETRY)                           \
                {                                                       \
                    TEST_VERDICT("rpc_read() maximum re-try reached");  \
                }                                                       \
                else                                                    \
                {                                                       \
                    VSLEEP(1, "rpc_read() returned 0. Retry reading");  \
                    continue;                                           \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                retry_ = 0;                                             \
            }                                                           \
            total_ += current_;                                         \
        }                                                               \
    } while(0)

#endif /* __FILE_SUITE_H__ */
