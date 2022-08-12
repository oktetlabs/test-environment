/** @file
 * @brief Raw log filter
 *
 * YAML parsing for log filters.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_LOG_FILTERS_YAML_H__
#define __TE_LOG_FILTERS_YAML_H__

#include "log_msg_filter.h"
#include <yaml.h>

#ifdef _cplusplus
extern "C" {
#endif

/**
 * Load a message filter from a YAML node.
 *
 * @param filter        branch filter
 * @param doc           YAML document
 * @param node          root YAML node for this filter
 *
 * @returns Status code
 */
extern te_errno log_msg_filter_load_yaml(log_msg_filter *filter,
                                         yaml_document_t *doc,
                                         yaml_node_t *node);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOG_FILTERS_YAML_H__ */
