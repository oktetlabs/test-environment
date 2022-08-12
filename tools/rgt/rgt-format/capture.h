/** @file
 * @brief Test Environment: Definitions for packet capture processing callbacks.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Roman Kolobov  <Roman.Kolobov@oktetlabs.ru>
 *
 */

#ifndef __TE_RGT_CAPTURE_H__
#define __TE_RGT_CAPTURE_H__

#include "xml2gen.h"
#include "rgt_tmpls_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flag turning on detailed packet dumps in log.
 */
extern int detailed_packets;

/**
 * Type of callback used for output of RGT templates.
 */
typedef void (*capture_tmpls_output_t)(rgt_gen_ctx_t *ctx,
                                       rgt_depth_ctx_t *depth_ctx,
                                       rgt_tmpl_t *tmpl,
                                       const rgt_attrs_t *attrs);

/**
 * Callback used for output of RGT templates (@c NULL by default,
 * if set to something else, used instead of rgt_tmpls_output()).
 */
extern capture_tmpls_output_t capture_tmpls_out_cb;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_CAPTURE_H__ */
