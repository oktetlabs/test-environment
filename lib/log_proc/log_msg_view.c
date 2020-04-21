/** @file
 * @brief Log processing
 *
 * This module provides a view structure for log messages and
 * and a set of utility functions.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log processing"

#include "te_config.h"

#include <assert.h>
#include <arpa/inet.h>

#include "log_msg_view.h"
#include "te_alloc.h"
#include "te_raw_log.h"
#include "logger_api.h"
#include "logger_int.h"

#define UINT8_NTOH(val) (val)
#define UINT16_NTOH(val) ntohs(val)
#define UINT32_NTOH(val) ntohl(val)

/* See description in raw_log_filter.h */
te_errno
te_raw_log_parse(const void *buf, size_t buf_len, log_msg_view *view)
{
    const char *buf_ptr = buf;
    size_t      buf_off = 0;

#define OFFSET(BYTES) \
    do {                  \
        buf_ptr += BYTES; \
        buf_off += BYTES; \
    } while(0)

#define READINT(BITS, TARGET) \
    do {                                                        \
        if ((buf_off + BITS / 8 - 1) >= buf_len)                \
        {                                                       \
            ERROR("te_raw_log_parse: attempt to read %d bytes " \
                  "from offset %d, buffer length is %llu",      \
                  BITS / 8, buf_off, buf_len);                  \
            return TE_EINVAL;                                   \
        }                                                       \
                                                                \
        TARGET = *(const uint ## BITS ## _t *)buf_ptr;          \
        TARGET = UINT ## BITS ## _NTOH(TARGET);                 \
        OFFSET(BITS / 8);                                       \
    } while (0)

    view->length = buf_len;

    READINT(8, view->version);
    assert(view->version == 1);

    READINT(32, view->ts_sec);
    READINT(32, view->ts_usec);
    READINT(16, view->level);
    READINT(32, view->log_id);

    READINT(16, view->entity_len);
    view->entity = buf_ptr;
    OFFSET(view->entity_len);

    READINT(16, view->user_len);
    view->user = buf_ptr;
    OFFSET(view->user_len);

    READINT(16, view->fmt_len);
    view->fmt = buf_ptr;
    OFFSET(view->fmt_len);

    view->args = buf_ptr;

#undef OFFSET
#undef READINT
    return 0;
}
