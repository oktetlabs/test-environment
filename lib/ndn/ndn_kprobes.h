/** @file
 * @brief KPROBES NDN
 *
 * Definitions of ASN.1 types for NDN for Kprobes.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_NDN_KPROBES_H__
#define __TE_NDN_KPROBES_H__

#if defined(__linux__)

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern asn_type * ndn_kprobes_packet;

#define KPROBES_MAX_FUNC_NAME   64

typedef enum {
    NDN_KPROBES_ACTION_SKIP,
    NDN_KPROBES_ACTION_FAIL,
    NDN_KPROBES_ACTION_BLOCK,
    NDN_KPROBES_ACTION_UNBLOCK,
} ndn_kprobes_action_t;

/* Possible failure results */
/** Driver is not loaded */
#define KPROBES_FAULTS_DRV_LOAD_FAIL     1
/** Interface is not created */
#define KPROBES_FAULTS_IF_CREATE_FAIL    2
/** No failures */
#define KPROBES_FAULTS_NO_FAIL           4

/**
 * Kprobes info structure
 */
typedef struct ndn_kprobes_info_s {
    char function_name[KPROBES_MAX_FUNC_NAME]; /**< Intercepted function
                                                    name */
    ndn_kprobes_action_t action;                /**< Action to be done with
                                                    call of function_name
                                                    function */
    int intercept_count;
    int retval;                               /**< Value to which will be
                                                    replaced returned value
                                                    of funcion_name function;
                                                    unused if action is not
                                                    equals to "fail" */
    int block_timeout;
    int scenario_index;
    int scenario_item_index;
} ndn_kprobes_info_t;

/**
 * see ndn_kprobes.c
 */
extern int ndn_kprobes_parse_info(const char *kprobes_info_str,
                                  ndn_kprobes_info_t **kprobes_info,
                                  int *number_of_structures);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* defined (__linux__) */

#endif /* __TE_NDN_KPROBES_H__ */
