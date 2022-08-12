/** @file
 * @brief Format string parsing
 *
 * @defgroup te_tools_te_format Format string parsing
 * @ingroup te_tools
 * @{
 *
 * Some TE-specific features, such as memory dump, file content logging,
 * and additional length modifiers are supported.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Soloducha <Ivan.Soloducha@oktetlabs.ru>
 *
 */

#ifndef  __TE_TOOLS_FORMAT_H__
#define  __TE_TOOLS_FORMAT_H__

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"


/** Parameters for te_log_vprintf_old() function */
struct te_log_out_params {
    FILE       *fp;     /**< Output file; if NULL, no file output */
    uint8_t    *buf;    /**< Output buffer; if NULL, no buffer output */
    size_t      buflen; /**< Buffer length; for NULL buffer must be 0 */
    size_t      offset; /**< Offset where output should begin */
};

/**
  * Preprocess and output message to log with special features parsing
  *
  * @param param        Output parameters
  * @param fmt          Format string
  * @param ap           Arguments for the format string
  *
  * @return Status code
  */
extern te_errno te_log_vprintf_old(struct te_log_out_params *param,
                                   const char *fmt, va_list ap);

#endif /* !__TE_TOOLS_FORMAT_H__ */
/**@} <!-- END te_tools_te_format --> */
