/** @file
 * @brief API to modify TRC tags from prologues
 *
 * @defgroup ts_tapi_tags TRC tags modification
 * @ingroup te_ts_tapi
 * @{
 *
 * Declaration of API to modify TRC tags from prologues.
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_TAGS_H__
#define __TE_TAPI_TAGS_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TE_CFG_TRC_TAGS_FMT "/local:/trc_tags:%s"

/**
 * Modify the set of defined TRC tags for tests by adding a new tag to the
 * configurator database. This function should be used to pass the TRC tag from
 * the root prologue only to the tester.
 *
 * @param tag       Tag name.
 * @param value     Tag value for TRC tags expressions (can be @c NULL).
 *
 * @return          Status code.
 */
extern te_errno tapi_tags_add_tag(const char *tag, const char *value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAGS_H__ */

/**@} <!-- END ts_tapi_tags --> */
