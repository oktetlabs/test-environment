/** @file
 * @brief Unix Test Agent serial console common tools
 *
 * @defgroup te_tools_te_serial_common Serial console common tools
 * @ingroup te_tools
 * @{
 *
 * Definition of unix TA serial console configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_SERIAL_COMMON_H__
#define __TE_SERIAL_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Max length of the parser name */
#define TE_SERIAL_MAX_NAME  63
/* Max pattern length */
#define TE_SERIAL_MAX_PATT  255
/* Base size of the buffer for lists */
#define CONSOLE_LIST_SIZE    512
/* Default conserver port */
#define TE_SERIAL_PORT      3109
/* Default user name */
#define TE_SERIAL_USER      "tester"
/* Default log level */
#define TE_SERIAL_LLEVEL    "WARN"

#define TE_SERIAL_MALLOC(ptr, size)       \
    if ((ptr = malloc(size)) == NULL)   \
        assert(0);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_SERIAL_COMMON_H__ */
/**@} <!-- END te_tools_te_serial_common --> */
