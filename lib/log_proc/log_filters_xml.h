/** @file
 * @brief Log processing
 *
 * XML parsing for log filters.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_LOG_FILTERS_XML_H__
#define __TE_LOG_FILTERS_XML_H__

#include <libxml/tree.h>

#include "log_flow_filters.h"

#ifdef _cplusplus
extern "C" {
#endif

/**
 * Load a branch filter from an XML file.
 *
 * @param filter        branch filter
 * @param filter_node   XML root node for the filter
 *
 * @returns Status code
 */
extern te_errno log_branch_filter_load_xml(log_branch_filter *filter,
                                           xmlNodePtr filter_node);

/**
 * Load a duration filter from an XML file.
 *
 * @param filter        branch filter
 * @param filter_node   XML root node for the filter
 *
 * @returns Status code
 */
extern te_errno log_duration_filter_load_xml(log_duration_filter *filter,
                                             xmlNodePtr filter_node);

/**
 * Load a message filter from an XML file.
 *
 * @param filter        branch filter
 * @param filter_node   XML root node for the filter
 *
 * @returns Status code
 */
extern te_errno log_msg_filter_load_xml(log_msg_filter *filter,
                                        xmlNodePtr filter_node);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOG_FILTERS_XML_H__ */
