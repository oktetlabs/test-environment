/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API for TA events
 *
 * @defgroup tapi_ta_events TA events support
 * @ingroup te_ts_tapi
 * @{
 *
 * Test API for TA events
 *
 * Copyright (C) 2024-2024 ARK NETWORKS LLC. All rights reserved.
 */

#ifndef __TAPI_TA_EVENTS_H__
#define __TAPI_TA_EVENTS_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Event callback to process TA events. */
typedef bool (*tapi_ta_events_cb)(const char *ta, char *name, char *value);

/** TA events handle */
typedef unsigned tapi_ta_events_handle;

/**
 * Register new handler, that will be executed to process TA events.
 *
 * @param ta         Agent from which we want to receive TA events
 * @param events     Comma separated list of events to receive
 * @param callback   Function to process TA event
 * @param handle     TA events handle
 *
 * @return           Status code
 */
extern te_errno tapi_ta_events_subscribe(const char *ta, const char *events,
                                         tapi_ta_events_cb      callback,
                                         tapi_ta_events_handle *handle);

/**
 * Remove handler from list of registered hooks.
 *
 * @param handle    TA events handle
 *
 * @return          Status code
 */
extern te_errno tapi_ta_events_unsubscribe(tapi_ta_events_handle handle);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_TA_EVENTS_H__ */

/**@} <!-- END tapi_ta_events --> */
