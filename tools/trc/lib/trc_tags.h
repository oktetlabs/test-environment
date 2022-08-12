/** @file
 * @brief Testing Results Comparator: common
 *
 * Helper functions to work with TRC tags.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TRC_TAGS_H__
#define __TE_TRC_TAGS_H__

#include "te_errno.h"
#include "tq_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse string with TRC tags and add them into the list.
 *
 * @param tags          List to add tags
 * @param tags_str      String with tags
 *
 * @return Status code.
 */
extern te_errno trc_tags_str_to_list(tqh_strings *tags, char *tags_str);

/**
 * Parse JSON string with TRC tags and add them into the list.
 *
 * @param json_buf      JSON string with TRC tags.
 * @param parsed_tags   List to add tags.
 *
 * @return Status code.
 */
extern te_errno trc_tags_json_to_list(tqh_strings *parsed_tags, char *json_buf);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_TAGS_H__ */
